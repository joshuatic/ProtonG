#pragma once

#include "ProtonVulkanContext.hpp"

#include <memory>
#include <string>

namespace Proton
{
    struct MediaPlayerAudioCallbackAccess;

    struct ImageSequencePlaybackConfig
    {
        std::string FramePattern;
        std::string AudioPath;

        int FirstFrame = 1;
        int LastFrame = 1;
        double Fps = 30.0;
    };

    enum class MediaPlaybackState
    {
        Idle,
        Preparing,
        Playing,
        Finished,
        Failed
    };

    class MediaPlayer
    {
    public:
        MediaPlayer();
        ~MediaPlayer();

        MediaPlayer(const MediaPlayer&) = delete;
        MediaPlayer& operator=(const MediaPlayer&) = delete;

        void Attach(VulkanContext* vulkanContext);

        bool PlayImageSequence(const ImageSequencePlaybackConfig& config);
        void Stop();

        void Update(double deltaTime);

        bool IsPlaying() const;
        MediaPlaybackState GetState() const;

    private:
        friend struct MediaPlayerAudioCallbackAccess;

        struct Impl;

        std::string BuildFramePath(int frame) const;
        int CalculateFrameFromAudioTime(double audioTimeSeconds) const;
        void RenderFrameIfNeeded(int frame);

    private:
        VulkanContext* m_VulkanContext = nullptr;

        ImageSequencePlaybackConfig m_Config{};

        MediaPlaybackState m_State = MediaPlaybackState::Idle;

        bool m_Playing = false;
        int m_LastRenderedFrame = -1;

        std::unique_ptr<Impl> m_Impl;
    };
}