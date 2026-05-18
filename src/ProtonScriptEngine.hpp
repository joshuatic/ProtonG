#pragma once

#include <functional>
#include <string>
#include <filesystem>
#include <vector>

struct lua_State;

namespace Proton {
    class ScriptEngine {
    public:
        using ClearColorCallback =
        std::function<void(float r, float g, float b, float a)>;

        using RenderImageCallback =
        std::function<void(const std::string &path)>;

        using ImageScaleModeCallback =
        std::function<void(int scaleMode)>;

        using PlayAudioCallback =
        std::function<void(const std::string &path)>;

        using AudioTimeCallback =
        std::function<double()>;

        using MediaPlayImageSequenceCallback =
        std::function<void(
            const std::string &framePattern,
            const std::string &audioPath,
            int firstFrame,
            int lastFrame,
            double fps
        )>;

        using MediaStopCallback =
        std::function<void()>;

        using DrawRectCallback =
        std::function<void(
            const std::string &,
            float,
            float,
            float,
            float,
            float,
            float,
            float,
            float,
            int)>;

        using DrawClearCallback =
        std::function<void()>;

        using InputKeyCallback =
        std::function<bool(const std::string &keyName)>;

        using InputMouseButtonCallback =
        std::function<bool(const std::string &buttonName)>;

        using InputMousePositionCallback =
        std::function<double()>;

        using WindowDimensionCallback =
        std::function<double()>;

        using DrawCircleCallback =
        std::function<void(
            const std::string &,
            float,
            float,
            float,
            float,
            float,
            float,
            float,
            int)>;

        using DrawPolygonCallback =
        std::function<void(
            const std::string &,
            float,
            float,
            float,
            int,
            float,
            float,
            float,
            float,
            float,
            int)>;

        using WindowTitleCallback =
        std::function<void(const std::string &title)>;

        using WindowDebugModeCallback =
        std::function<void(bool enabled)>;

        using SpriteLoadCallback =
        std::function<std::uint32_t(const std::string &path)>;

        using SpriteDrawCallback =
        std::function<void(
            const std::string &id,
            std::uint32_t handle,
            float x,
            float y,
            float width,
            float height,
            float rotationDegrees,
            float r,
            float g,
            float b,
            float a,
            int layer
        )>;

        using ObjectCreateCallback =
        std::function<std::uint32_t(
            const std::string &name,
            float x,
            float y,
            float width,
            float height
        )>;

        using ObjectSetPositionCallback =
        std::function<void(std::uint32_t handle, float x, float y)>;

        using ObjectSetSizeCallback =
        std::function<void(std::uint32_t handle, float width, float height)>;

        using ObjectFloatQueryCallback =
        std::function<float(std::uint32_t handle)>;

        ScriptEngine();

        ~ScriptEngine();

        void SetClearColorCallback(ClearColorCallback callback);

        void InvokeClearColor(float r, float g, float b, float a);

        void SetRenderImageCallback(RenderImageCallback callback);

        void InvokeRenderImage(const std::string &path);

        void SetImageScaleModeCallback(ImageScaleModeCallback callback);

        void InvokeImageScaleMode(int scaleMode);

        void SetPlayAudioCallback(PlayAudioCallback callback);

        void InvokePlayAudio(const std::string &path);

        void SetAudioTimeCallback(AudioTimeCallback callback);

        double InvokeAudioTime();

        void SetMediaPlayImageSequenceCallback(MediaPlayImageSequenceCallback callback);

        void SetMediaStopCallback(MediaStopCallback callback);

        void SetDrawRectCallback(DrawRectCallback callback);

        void SetDrawClearCallback(DrawClearCallback callback);

        void SetInputKeyDownCallback(InputKeyCallback callback);

        void SetInputKeyPressedCallback(InputKeyCallback callback);

        void SetInputKeyReleasedCallback(InputKeyCallback callback);

        void SetInputMouseButtonDownCallback(InputMouseButtonCallback callback);

        void SetInputMouseButtonPressedCallback(InputMouseButtonCallback callback);

        void SetInputMouseButtonReleasedCallback(InputMouseButtonCallback callback);

        void SetInputMouseXCallback(InputMousePositionCallback callback);

        void SetInputMouseYCallback(InputMousePositionCallback callback);

        void SetWindowWidthCallback(WindowDimensionCallback callback);

        void SetWindowHeightCallback(WindowDimensionCallback callback);

        double InvokeWindowWidth();

        double InvokeWindowHeight();

        void InvokeMediaPlayImageSequence(
            const std::string &framePattern,
            const std::string &audioPath,
            int firstFrame,
            int lastFrame,
            double fps
        );

        void InvokeMediaStop();

        void InvokeDrawRect(
            const std::string &id,
            float x,
            float y,
            float width,
            float height,
            float r,
            float g,
            float b,
            float a,
            int layer
        );

        void InvokeDrawClear();

        bool InvokeInputKeyDown(const std::string &keyName);

        bool InvokeInputKeyPressed(const std::string &keyName);

        bool InvokeInputKeyReleased(const std::string &keyName);

        bool InvokeInputMouseButtonDown(const std::string &buttonName);

        bool InvokeInputMouseButtonPressed(const std::string &buttonName);

        bool InvokeInputMouseButtonReleased(const std::string &buttonName);

        double InvokeInputMouseX();

        double InvokeInputMouseY();

        void SetDrawCircleCallback(DrawCircleCallback callback);

        void SetDrawPolygonCallback(DrawPolygonCallback callback);

        void SetSpriteLoadCallback(SpriteLoadCallback callback);

        void SetSpriteDrawCallback(SpriteDrawCallback callback);

        void SetObjectCreateCallback(ObjectCreateCallback callback);

        void SetObjectSetPositionCallback(ObjectSetPositionCallback callback);

        void SetObjectSetSizeCallback(ObjectSetSizeCallback callback);

        void SetObjectGetXCallback(ObjectFloatQueryCallback callback);

        void SetObjectGetYCallback(ObjectFloatQueryCallback callback);

        void SetObjectGetWidthCallback(ObjectFloatQueryCallback callback);

        void SetObjectGetHeightCallback(ObjectFloatQueryCallback callback);

        std::uint32_t InvokeObjectCreate(
            const std::string &name,
            float x,
            float y,
            float width,
            float height
        );

        void InvokeObjectSetPosition(std::uint32_t handle, float x, float y);

        void InvokeObjectSetSize(std::uint32_t handle, float width, float height);

        float InvokeObjectGetX(std::uint32_t handle);

        float InvokeObjectGetY(std::uint32_t handle);

        float InvokeObjectGetWidth(std::uint32_t handle);

        float InvokeObjectGetHeight(std::uint32_t handle);

        std::uint32_t InvokeSpriteLoad(const std::string &path);

        void InvokeSpriteDraw(
            const std::string &id,
            std::uint32_t handle,
            float x,
            float y,
            float width,
            float height,
            float rotationDegrees,
            float r,
            float g,
            float b,
            float a,
            int layer
        );

        void InvokeDrawCircle(
            const std::string &id,
            float centerX,
            float centerY,
            float radius,
            float r,
            float g,
            float b,
            float a,
            int layer
        );

        void InvokeDrawPolygon(
            const std::string &id,
            float centerX,
            float centerY,
            float radius,
            int points,
            float rotationDegrees,
            float r,
            float g,
            float b,
            float a,
            int layer
        );

        void SetWindowTitleCallback(WindowTitleCallback callback);

        void SetWindowDebugModeCallback(WindowDebugModeCallback callback);

        void InvokeWindowTitle(const std::string &title);

        void InvokeWindowDebugMode(bool enabled);

        bool RunFile(const std::string &path);

        bool RunDirectory(const std::string &directoryPath);

        void Update(double deltaTime);

    private:
        lua_State *m_State = nullptr;
        ClearColorCallback m_ClearColorCallback;
        bool m_HasUpdateFunction = false;
        RenderImageCallback m_RenderImageCallback;
        ImageScaleModeCallback m_ImageScaleModeCallback;
        PlayAudioCallback m_PlayAudioCallback;
        AudioTimeCallback m_AudioTimeCallback;
        MediaPlayImageSequenceCallback m_MediaPlayImageSequenceCallback;
        MediaStopCallback m_MediaStopCallback;
        DrawRectCallback m_DrawRectCallback;
        DrawClearCallback m_DrawClearCallback;
        InputKeyCallback m_InputKeyDownCallback;
        InputKeyCallback m_InputKeyPressedCallback;
        InputKeyCallback m_InputKeyReleasedCallback;

        InputMouseButtonCallback m_InputMouseButtonDownCallback;
        InputMouseButtonCallback m_InputMouseButtonPressedCallback;
        InputMouseButtonCallback m_InputMouseButtonReleasedCallback;

        InputMousePositionCallback m_InputMouseXCallback;
        InputMousePositionCallback m_InputMouseYCallback;

        WindowDimensionCallback m_WindowWidthCallback;
        WindowDimensionCallback m_WindowHeightCallback;

        DrawCircleCallback m_DrawCircleCallback;
        DrawPolygonCallback m_DrawPolygonCallback;

        WindowTitleCallback m_WindowTitleCallback;
        WindowDebugModeCallback m_WindowDebugModeCallback;

        SpriteLoadCallback m_SpriteLoadCallback;
        SpriteDrawCallback m_SpriteDrawCallback;

        ObjectCreateCallback m_ObjectCreateCallback;
        ObjectSetPositionCallback m_ObjectSetPositionCallback;
        ObjectSetSizeCallback m_ObjectSetSizeCallback;
        ObjectFloatQueryCallback m_ObjectGetXCallback;
        ObjectFloatQueryCallback m_ObjectGetYCallback;
        ObjectFloatQueryCallback m_ObjectGetWidthCallback;
        ObjectFloatQueryCallback m_ObjectGetHeightCallback;
    };
}
