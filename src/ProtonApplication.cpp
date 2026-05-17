#include "ProtonApplication.hpp"

#include "ProtonLog.hpp"

#include <GLFW/glfw3.h>

#include <chrono>
#include <format>
#include <thread>
#include <utility>

namespace Proton
{
    Application::Application(ApplicationConfig config)
        : m_Config(std::move(config))
    {
    }

    Application::~Application()
    {
        if (m_Running || m_Window != nullptr)
        {
            Shutdown();
        }
    }

    static void FramebufferResizeCallback(GLFWwindow* window, int, int)
    {
        auto* app =
            static_cast<Application*>(glfwGetWindowUserPointer(window));

        if (app != nullptr)
        {
            app->OnFramebufferResized();
        }
    }

    void Application::OnFramebufferResized()
    {
        m_Vulkan.NotifyFramebufferResized();
    }

    int Application::Run()
    {
        Startup();

        using Clock = std::chrono::high_resolution_clock;
        auto previousTime = Clock::now();

        while (m_Running)
        {
            const auto currentTime = Clock::now();

            const double deltaTime =
                std::chrono::duration<double>(currentTime - previousTime).count();

            previousTime = currentTime;

            Tick(deltaTime);

            /*
                Vulkan owns presentation.

                GLFW only handles:
                - window creation
                - input/events
                - close events

                Do not call glfwSwapBuffers(), because Proton G is not using
                OpenGL. Vulkan presents through vkQueuePresentKHR().
            */
            m_Vulkan.DrawFrame();

            glfwPollEvents();

            if (glfwWindowShouldClose(m_Window))
            {
                Log::Info("Window close requested.");
                m_Running = false;
            }

            /*
                Temporary CPU throttle.

                Later, actual frame pacing should come from Vulkan present mode,
                fences, and frame timing instead of sleeping manually.
            */
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        Shutdown();
        return 0;
    }

    void Application::Startup()
    {
        Log::Info("Proton G logging online.");
        Log::Info(std::format("Starting {}...", m_Config.Name));
        Log::Info(std::format(
            "Requested window size: {}x{}",
            m_Config.Width,
            m_Config.Height
        ));

        if (!glfwInit())
        {
            Log::Error("Failed to initialize GLFW.");
            m_Running = false;
            return;
        }

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

        m_Window = glfwCreateWindow(
            m_Config.Width,
            m_Config.Height,
            m_Config.Name.c_str(),
            nullptr,
            nullptr
        );

        if (m_Window == nullptr)
        {
            Log::Error("Failed to create GLFW window.");

            glfwTerminate();

            m_Running = false;
            return;
        }

        Log::Info("GLFW window created.");

        glfwSetWindowUserPointer(m_Window, this);
        glfwSetFramebufferSizeCallback(m_Window, FramebufferResizeCallback);

        if (!m_Vulkan.Init(m_Window))
        {
            Log::Error("Failed to initialize Proton G Vulkan backend.");

            glfwDestroyWindow(m_Window);
            m_Window = nullptr;

            glfwTerminate();

            m_Running = false;
            return;
        }

        Log::Info("Proton G Vulkan backend online.");

        /*
            MediaPlayer is the only system that should coordinate synchronized
            image-sequence playback.

            Application owns the services:
            - AudioEngine
            - VulkanContext

            MediaPlayer uses those services:
            - AudioEngine starts/stops audio.
            - MediaPlayer owns the playback clock.
            - ImageLoader is used internally by MediaPlayer.
            - VulkanContext receives the selected frame texture.
        */

        Log::Info("Proton G v0.01 online.");

        m_ScriptEngine.SetClearColorCallback(
            [this](float r, float g, float b, float a)
            {
                m_Vulkan.SetClearColor(r, g, b, a);
            }
        );

        m_ScriptEngine.SetImageScaleModeCallback(
            [this](int scaleMode)
            {
                m_Vulkan.SetImageScaleMode(scaleMode);
            }
        );

        m_MediaPlayer.Attach(&m_Vulkan);

        m_ScriptEngine.SetMediaPlayImageSequenceCallback(
        [this](
            const std::string& framePattern,
            const std::string& audioPath,
            int firstFrame,
            int lastFrame,
            double fps
        )
        {
            ImageSequencePlaybackConfig config{};
            config.FramePattern = framePattern;
            config.AudioPath = audioPath;
            config.FirstFrame = firstFrame;
            config.LastFrame = lastFrame;
            config.Fps = fps;

            if (!m_MediaPlayer.PlayImageSequence(config))
            {
                Log::Error("Failed to start Luau-requested image sequence.");
            }
        }
    );

            m_ScriptEngine.SetMediaStopCallback(
                [this]()
                {
                    m_MediaPlayer.Stop();
                }
            );

        m_ScriptEngine.RunDirectory("sandbox/scripts");

        m_Running = true;
    }

    void Application::Tick(double deltaTime)
    {
        m_ScriptEngine.Update(deltaTime);

        /*
            MediaPlayer owns synchronized media playback.
        */
        m_MediaPlayer.Update(deltaTime);

        m_FrameCounter++;
        m_StatsLogTimer += deltaTime;

        if (m_StatsLogTimer >= 1.0)
        {
            const SystemStats stats = m_StatsSampler.Sample();

            const double fps =
                static_cast<double>(m_FrameCounter) / m_StatsLogTimer;

            const double frameMs = deltaTime * 1000.0;

            const std::uint64_t usedSystemMemory =
                stats.TotalPhysicalMemoryBytes - stats.AvailablePhysicalMemoryBytes;

            Log::Info(std::format(
                "Runtime stats | FPS: {:.1f} | Frame: {:.3f} ms | CPU: {:.2f}% | Proton RAM: {} working / {} private | System RAM: {} used / {} total",
                fps,
                frameMs,
                stats.ProcessCpuPercent,
                SystemStatsSampler::FormatBytes(stats.WorkingSetBytes),
                SystemStatsSampler::FormatBytes(stats.PrivateBytes),
                SystemStatsSampler::FormatBytes(usedSystemMemory),
                SystemStatsSampler::FormatBytes(stats.TotalPhysicalMemoryBytes)
            ));

            if (m_Window != nullptr)
            {
                const std::string title = std::format(
                    "{} | FPS: {:.1f} | Frame: {:.3f} ms",
                    m_Config.Name,
                    fps,
                    frameMs
                );

                glfwSetWindowTitle(m_Window, title.c_str());
            }

            m_FrameCounter = 0;
            m_StatsLogTimer = 0.0;
        }
    }

    void Application::Shutdown()
    {
        Log::Info("Proton G shutting down.");

        /*
            Stop coordinated media first so it can stop audio cleanly before
            the audio backend is destroyed.
        */
        m_MediaPlayer.Stop();

        m_Vulkan.Shutdown();

        if (m_Window != nullptr)
        {
            glfwDestroyWindow(m_Window);
            m_Window = nullptr;
        }

        glfwTerminate();

        m_Running = false;
    }
}