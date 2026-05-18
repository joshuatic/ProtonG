#pragma once

#include "ProtonSystemStats.hpp"
#include "ProtonVulkanContext.hpp"
#include "ProtonScriptEngine.hpp"
#include "ProtonMediaPlayer.hpp"
#include "ProtonInput.hpp"
#include "ProtonDraw2D.hpp"
#include "ProtonSpriteManager.hpp"
#include "ProtonObjectManager.hpp"

#include <string>

struct GLFWwindow;

namespace Proton {
    struct ApplicationConfig {
        std::string Name = "Proton G";
        int Width = 1280;
        int Height = 720;
    };

    class Application {
    public:
        explicit Application(ApplicationConfig config);

        ~Application();

        int Run();

        void OnFramebufferResized();

        void OnKeyEvent(int key, int action);

        void OnMouseButtonEvent(int button, int action);

        void OnCursorMoved(double x, double y);

        void OnWindowFocusChanged(bool focused);

    private:
        void Startup();

        void Tick(double deltaTime);

        void Shutdown();

        ApplicationConfig m_Config;
        GLFWwindow *m_Window = nullptr;
        VulkanContext m_Vulkan;
        ScriptEngine m_ScriptEngine;
        SystemStatsSampler m_StatsSampler;
        MediaPlayer m_MediaPlayer;
        Input m_Input;
        Draw2D m_Draw2D;
        SpriteManager m_SpriteManager;
        ObjectManager m_ObjectManager;

        bool m_Running = false;
        double m_StatsLogTimer = 0.0;
        std::uint64_t m_FrameCounter = 0;

        std::string m_WindowTitle = "Proton G Sandbox";
        bool m_WindowDebugMode = true;
    };
}
