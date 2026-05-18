#include "ProtonMediaPlayer.hpp"

#include "ProtonImageLoader.hpp"
#include "ProtonLog.hpp"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <cstdint>
#include <filesystem>
#include <format>
#include <memory>
#include <vector>

/*
    IMPORTANT:
    MINIAUDIO_IMPLEMENTATION must exist in exactly one .cpp file.

    If this file owns miniaudio now, remove ProtonAudioEngine.cpp from CMake.
*/
#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>

namespace Proton {
    struct MediaPlayer::Impl {
        ma_decoder Decoder{};
        ma_device Device{};

        bool DecoderInitialized = false;
        bool DeviceInitialized = false;

        std::atomic<std::uint64_t> SubmittedFrames = 0;
        std::atomic<bool> AudioFinished = false;

        ma_uint32 SampleRate = 0;
        ma_uint32 Channels = 0;

        double LeadingSilenceSeconds = 0.0;
    };

    struct MediaPlayerAudioCallbackAccess {
        using Impl = MediaPlayer::Impl;
    };

    static double DetectLeadingSilenceSeconds(const std::string &audioPath) {
        ma_decoder_config decoderConfig =
                ma_decoder_config_init(ma_format_f32, 0, 0);

        ma_decoder decoder{};

        const ma_result decoderResult = ma_decoder_init_file(
            audioPath.c_str(),
            &decoderConfig,
            &decoder
        );

        if (decoderResult != MA_SUCCESS) {
            Log::Error(std::format(
                "Failed to scan audio leading silence. miniaudio result={}",
                static_cast<int>(decoderResult)
            ));

            return 0.0;
        }

        const ma_uint32 channels = decoder.outputChannels;
        const ma_uint32 sampleRate = decoder.outputSampleRate;

        if (channels == 0 || sampleRate == 0) {
            ma_decoder_uninit(&decoder);
            return 0.0;
        }

        constexpr ma_uint64 ChunkFrames = 2048;

        /*
            RMS threshold.

            0.0025 detects MP3 noise.
            0.0200 is a much better first test for "real audible content."
        */
        constexpr double RmsAudibleThreshold = 0.0200;

        std::vector<float> samples(
            static_cast<std::size_t>(ChunkFrames) * channels
        );

        ma_uint64 totalFramesScanned = 0;

        while (true) {
            ma_uint64 framesRead = 0;

            const ma_result readResult = ma_decoder_read_pcm_frames(
                &decoder,
                samples.data(),
                ChunkFrames,
                &framesRead
            );

            if (readResult != MA_SUCCESS || framesRead == 0) {
                break;
            }

            const std::size_t sampleCount =
                    static_cast<std::size_t>(framesRead) * channels;

            double sumSquares = 0.0;

            for (std::size_t i = 0; i < sampleCount; i++) {
                const double sample = static_cast<double>(samples[i]);
                sumSquares += sample * sample;
            }

            const double rms =
                    std::sqrt(sumSquares / static_cast<double>(sampleCount));

            if (rms >= RmsAudibleThreshold) {
                const double seconds =
                        static_cast<double>(totalFramesScanned) /
                        static_cast<double>(sampleRate);

                ma_decoder_uninit(&decoder);
                return seconds;
            }

            totalFramesScanned += framesRead;
        }

        ma_decoder_uninit(&decoder);
        return 0.0;
    }

    static void MediaAudioCallbackImpl(
        ma_device *device,
        void *output,
        const void *,
        ma_uint32 frameCount
    ) {
        auto *impl =
                static_cast<MediaPlayerAudioCallbackAccess::Impl *>(device->pUserData);

        const ma_uint32 channels = device->playback.channels;
        const ma_format format = device->playback.format;

        const std::size_t outputByteCount =
                static_cast<std::size_t>(frameCount) *
                channels *
                ma_get_bytes_per_sample(format);

        if (impl == nullptr || !impl->DecoderInitialized) {
            std::memset(output, 0, outputByteCount);
            return;
        }

        ma_uint64 framesRead = 0;

        const ma_result readResult = ma_decoder_read_pcm_frames(
            &impl->Decoder,
            output,
            frameCount,
            &framesRead
        );

        if (readResult != MA_SUCCESS) {
            std::memset(output, 0, outputByteCount);
            impl->AudioFinished.store(true, std::memory_order_relaxed);
            return;
        }

        if (framesRead < frameCount) {
            const std::size_t bytesPerFrame =
                    static_cast<std::size_t>(channels) *
                    ma_get_bytes_per_sample(format);

            const std::size_t validByteCount =
                    static_cast<std::size_t>(framesRead) *
                    bytesPerFrame;

            std::memset(
                static_cast<std::uint8_t *>(output) + validByteCount,
                0,
                outputByteCount - validByteCount
            );

            impl->AudioFinished.store(true, std::memory_order_relaxed);
        }

        /*
            This is the unified media clock.

            It advances from the audio callback, not from:
            - Luau dt
            - std::chrono wall clock
            - app frame timing
            - guessed offsets
        */
        impl->SubmittedFrames.fetch_add(
            static_cast<std::uint64_t>(framesRead),
            std::memory_order_relaxed
        );
    }

    MediaPlayer::MediaPlayer()
        : m_Impl(std::make_unique<Impl>()) {
    }

    MediaPlayer::~MediaPlayer() {
        Stop();
    }

    void MediaPlayer::Attach(VulkanContext *vulkanContext) {
        m_VulkanContext = vulkanContext;
    }

    bool MediaPlayer::PlayImageSequence(const ImageSequencePlaybackConfig &config) {
        if (m_VulkanContext == nullptr) {
            Log::Error("Cannot play media because VulkanContext is null.");
            return false;
        }

        if (config.FramePattern.empty()) {
            Log::Error("Cannot play media because frame pattern is empty.");
            return false;
        }

        if (config.AudioPath.empty()) {
            Log::Error("Cannot play media because audio path is empty.");
            return false;
        }

        if (config.FirstFrame > config.LastFrame) {
            Log::Error("Cannot play media because firstFrame is greater than lastFrame.");
            return false;
        }

        if (config.Fps <= 0.0) {
            Log::Error("Cannot play media because FPS must be greater than zero.");
            return false;
        }

        const std::filesystem::path absoluteAudioPath =
                std::filesystem::absolute(config.AudioPath);

        if (!std::filesystem::exists(absoluteAudioPath)) {
            Log::Error(std::format(
                "MediaPlayer audio file does not exist: {}",
                absoluteAudioPath.string()
            ));

            m_State = MediaPlaybackState::Failed;
            return false;
        }

        Stop();

        m_Config = config;
        m_State = MediaPlaybackState::Preparing;
        m_Playing = false;
        m_LastRenderedFrame = -1;

        m_Impl->SubmittedFrames.store(0, std::memory_order_relaxed);
        m_Impl->AudioFinished.store(false, std::memory_order_relaxed);
        m_Impl->SampleRate = 0;
        m_Impl->Channels = 0;

        m_Impl->LeadingSilenceSeconds =
                DetectLeadingSilenceSeconds(absoluteAudioPath.string());

        Log::Info(std::format(
            "MediaPlayer audio leading silence detected: {:.3f} seconds.",
            m_Impl->LeadingSilenceSeconds
        ));

        /*
            Show frame 1 as a preview only.

            This is not playback timing.
            The sequence does not advance until audio callback frames start
            being submitted.
        */
        RenderFrameIfNeeded(m_Config.FirstFrame);

        ma_decoder_config decoderConfig =
                ma_decoder_config_init(
                    ma_format_f32,
                    0,
                    0
                );

        const ma_result decoderResult = ma_decoder_init_file(
            absoluteAudioPath.string().c_str(),
            &decoderConfig,
            &m_Impl->Decoder
        );

        if (decoderResult != MA_SUCCESS) {
            Log::Error(std::format(
                "MediaPlayer failed to initialize audio decoder: {} | miniaudio result={}",
                absoluteAudioPath.string(),
                static_cast<int>(decoderResult)
            ));

            m_State = MediaPlaybackState::Failed;
            return false;
        }

        m_Impl->DecoderInitialized = true;
        m_Impl->SampleRate = m_Impl->Decoder.outputSampleRate;
        m_Impl->Channels = m_Impl->Decoder.outputChannels;

        if (m_Impl->SampleRate == 0 || m_Impl->Channels == 0) {
            Log::Error("MediaPlayer audio decoder returned invalid sample rate or channel count.");

            Stop();
            m_State = MediaPlaybackState::Failed;
            return false;
        }

        ma_device_config deviceConfig =
                ma_device_config_init(ma_device_type_playback);

        deviceConfig.playback.format = ma_format_f32;
        deviceConfig.playback.channels = m_Impl->Channels;
        deviceConfig.sampleRate = m_Impl->SampleRate;
        deviceConfig.dataCallback = MediaAudioCallbackImpl;
        deviceConfig.pUserData = m_Impl.get();

        /*
            Low latency profile matters here.

            This keeps the callback buffer smaller and makes the frame clock
            follow submitted audio frames as closely as possible.
        */
        deviceConfig.performanceProfile = ma_performance_profile_low_latency;
        deviceConfig.periodSizeInFrames = 512;
        deviceConfig.periods = 2;

        const ma_result deviceResult = ma_device_init(
            nullptr,
            &deviceConfig,
            &m_Impl->Device
        );

        if (deviceResult != MA_SUCCESS) {
            Log::Error(std::format(
                "MediaPlayer failed to initialize audio device. miniaudio result={}",
                static_cast<int>(deviceResult)
            ));

            Stop();
            m_State = MediaPlaybackState::Failed;
            return false;
        }

        m_Impl->DeviceInitialized = true;

        const ma_result startResult =
                ma_device_start(&m_Impl->Device);

        if (startResult != MA_SUCCESS) {
            Log::Error(std::format(
                "MediaPlayer failed to start audio device. miniaudio result={}",
                static_cast<int>(startResult)
            ));

            Stop();
            m_State = MediaPlaybackState::Failed;
            return false;
        }

        m_State = MediaPlaybackState::Playing;
        m_Playing = true;

        Log::Info(std::format(
            "MediaPlayer started unified image sequence. Frames: {}-{}, FPS: {:.3f}, Audio: {}, SampleRate: {}, Channels: {}",
            m_Config.FirstFrame,
            m_Config.LastFrame,
            m_Config.Fps,
            m_Config.AudioPath,
            m_Impl->SampleRate,
            m_Impl->Channels
        ));

        return true;
    }

    void MediaPlayer::Stop() {
        if (m_Impl == nullptr) {
            m_State = MediaPlaybackState::Idle;
            m_Playing = false;
            m_LastRenderedFrame = -1;
            return;
        }

        if (m_Impl->DeviceInitialized) {
            ma_device_stop(&m_Impl->Device);
            ma_device_uninit(&m_Impl->Device);
            m_Impl->DeviceInitialized = false;
        }

        if (m_Impl->DecoderInitialized) {
            ma_decoder_uninit(&m_Impl->Decoder);
            m_Impl->DecoderInitialized = false;
        }

        m_Impl->SubmittedFrames.store(0, std::memory_order_relaxed);
        m_Impl->AudioFinished.store(false, std::memory_order_relaxed);
        m_Impl->SampleRate = 0;
        m_Impl->Channels = 0;
        m_Impl->LeadingSilenceSeconds = 0.0;

        m_State = MediaPlaybackState::Idle;
        m_Playing = false;
        m_LastRenderedFrame = -1;
    }

    void MediaPlayer::Update(double) {
        if (!m_Playing || m_State != MediaPlaybackState::Playing) {
            return;
        }

        if (m_Impl == nullptr || m_Impl->SampleRate == 0) {
            return;
        }

        const std::uint64_t submittedFrames =
                m_Impl->SubmittedFrames.load(std::memory_order_relaxed);

        /*
            Wait until the audio callback actually submits audio frames.

            This prevents:
            - audio start requested
            - video starts advancing immediately
            - speaker output lags behind
        */
        if (submittedFrames == 0) {
            return;
        }

        const double audioTimeSeconds =
                static_cast<double>(submittedFrames) /
                static_cast<double>(m_Impl->SampleRate);

        static int lastLoggedSecond = -1;

        const int currentSecond =
                static_cast<int>(audioTimeSeconds);

        if (currentSecond != lastLoggedSecond) {
            lastLoggedSecond = currentSecond;

            Log::Info(std::format(
                "Media clock debug | audioTime={:.3f}s | submittedFrames={} | sampleRate={} | frame={}",
                audioTimeSeconds,
                submittedFrames,
                m_Impl->SampleRate,
                CalculateFrameFromAudioTime(audioTimeSeconds)
            ));
        }

        const int frame =
                CalculateFrameFromAudioTime(audioTimeSeconds);

        RenderFrameIfNeeded(frame);

        if (m_Impl->AudioFinished.load(std::memory_order_relaxed) &&
            frame >= m_Config.LastFrame) {
            m_State = MediaPlaybackState::Finished;
            m_Playing = false;

            Log::Info("MediaPlayer finished unified image sequence.");
        }
    }

    bool MediaPlayer::IsPlaying() const {
        return m_Playing;
    }

    MediaPlaybackState MediaPlayer::GetState() const {
        return m_State;
    }

    std::string MediaPlayer::BuildFramePath(int frame) const {
        std::vector<char> buffer(1024);

        int written = std::snprintf(
            buffer.data(),
            buffer.size(),
            m_Config.FramePattern.c_str(),
            frame
        );

        if (written <= 0) {
            return {};
        }

        if (static_cast<std::size_t>(written) >= buffer.size()) {
            buffer.resize(static_cast<std::size_t>(written) + 1);

            written = std::snprintf(
                buffer.data(),
                buffer.size(),
                m_Config.FramePattern.c_str(),
                frame
            );

            if (written <= 0) {
                return {};
            }
        }

        return std::string(buffer.data());
    }

    int MediaPlayer::CalculateFrameFromAudioTime(double audioTimeSeconds) const {
        const int frame =
                m_Config.FirstFrame +
                static_cast<int>(audioTimeSeconds * m_Config.Fps);

        return std::clamp(
            frame,
            m_Config.FirstFrame,
            m_Config.LastFrame
        );
    }

    void MediaPlayer::RenderFrameIfNeeded(int frame) {
        if (frame == m_LastRenderedFrame) {
            return;
        }

        const std::string framePath =
                BuildFramePath(frame);

        if (framePath.empty()) {
            Log::Error("MediaPlayer failed to build frame path.");
            return;
        }

        const ImageData image =
                ImageLoader::Load(framePath);

        if (!image.IsValid()) {
            Log::Error(std::format(
                "MediaPlayer failed to load frame {}: {}",
                frame,
                framePath
            ));

            return;
        }

        if (!m_VulkanContext->SetTextureFromImage(image)) {
            Log::Error(std::format(
                "MediaPlayer failed to upload frame {} to Vulkan.",
                frame
            ));

            return;
        }

        m_LastRenderedFrame = frame;

        Log::Info(std::format(
            "MediaPlayer rendered frame {}: {}",
            frame,
            framePath
        ));
    }
}
