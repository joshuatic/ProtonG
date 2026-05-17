#pragma once

#include <functional>
#include <string>
#include <vector>

struct lua_State;

namespace Proton
{
    class ScriptEngine
    {
    public:
        using ClearColorCallback =
            std::function<void(float r, float g, float b, float a)>;

        using RenderImageCallback =
            std::function<void(const std::string& path)>;

        using ImageScaleModeCallback =
            std::function<void(int scaleMode)>;

        using PlayAudioCallback =
            std::function<void(const std::string& path)>;

        using AudioTimeCallback =
            std::function<double()>;

        using MediaPlayImageSequenceCallback =
            std::function<void(
                const std::string& framePattern,
                const std::string& audioPath,
                int firstFrame,
                int lastFrame,
                double fps
            )>;

        using MediaStopCallback =
            std::function<void()>;

        ScriptEngine();
        ~ScriptEngine();

        void SetClearColorCallback(ClearColorCallback callback);
        void InvokeClearColor(float r, float g, float b, float a);
        void SetRenderImageCallback(RenderImageCallback callback);
        void InvokeRenderImage(const std::string& path);
        void SetImageScaleModeCallback(ImageScaleModeCallback callback);
        void InvokeImageScaleMode(int scaleMode);
        void SetPlayAudioCallback(PlayAudioCallback callback);
        void InvokePlayAudio(const std::string& path);
        void SetAudioTimeCallback(AudioTimeCallback callback);
        double InvokeAudioTime();
        void SetMediaPlayImageSequenceCallback(MediaPlayImageSequenceCallback callback);
        void SetMediaStopCallback(MediaStopCallback callback);

        void InvokeMediaPlayImageSequence(
            const std::string& framePattern,
            const std::string& audioPath,
            int firstFrame,
            int lastFrame,
            double fps
        );

        void InvokeMediaStop();

        bool RunFile(const std::string& path);
        bool RunDirectory(const std::string& directoryPath);
        void Update(double deltaTime);

    private:
        lua_State* m_State = nullptr;
        ClearColorCallback m_ClearColorCallback;
        bool m_HasUpdateFunction = false;
        RenderImageCallback m_RenderImageCallback;
        ImageScaleModeCallback m_ImageScaleModeCallback;
        PlayAudioCallback m_PlayAudioCallback;
        AudioTimeCallback m_AudioTimeCallback;
        MediaPlayImageSequenceCallback m_MediaPlayImageSequenceCallback;
        MediaStopCallback m_MediaStopCallback;
    };
}