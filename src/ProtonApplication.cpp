#include "ProtonApplication.hpp"

#include "ProtonLog.hpp"

#include <GLFW/glfw3.h>

#include <chrono>
#include <format>
#include <thread>
#include <utility>

#include "ProtonInput.hpp"

namespace Proton {
    Application::Application(ApplicationConfig config)
        : m_Config(std::move(config)) {
    }

    Application::~Application() {
        if (m_Running || m_Window != nullptr) {
            Shutdown();
        }
    }

    static void FramebufferResizeCallback(GLFWwindow *window, int, int) {
        auto *app =
                static_cast<Application *>(glfwGetWindowUserPointer(window));

        if (app != nullptr) {
            app->OnFramebufferResized();
        }
    }

    static void KeyCallback(GLFWwindow *window, int key, int, int action, int) {
        auto *app =
                static_cast<Application *>(glfwGetWindowUserPointer(window));

        if (app != nullptr) {
            app->OnKeyEvent(key, action);
        }
    }

    static void MouseButtonCallback(GLFWwindow *window, int button, int action, int) {
        auto *app =
                static_cast<Application *>(glfwGetWindowUserPointer(window));

        if (app != nullptr) {
            app->OnMouseButtonEvent(button, action);
        }
    }

    static void CursorPositionCallback(GLFWwindow *window, double x, double y) {
        auto *app =
                static_cast<Application *>(glfwGetWindowUserPointer(window));

        if (app != nullptr) {
            app->OnCursorMoved(x, y);
        }
    }

    static void WindowFocusCallback(GLFWwindow *window, int focused) {
        auto *app =
                static_cast<Application *>(glfwGetWindowUserPointer(window));

        if (app != nullptr) {
            app->OnWindowFocusChanged(focused == GLFW_TRUE);
        }
    }

    void Application::OnFramebufferResized() {
        m_Vulkan.NotifyFramebufferResized();
    }

    void Application::OnKeyEvent(int key, int action) {
        m_Input.HandleKeyEvent(key, action);
    }

    void Application::OnMouseButtonEvent(int button, int action) {
        m_Input.HandleMouseButtonEvent(button, action);
    }

    void Application::OnCursorMoved(double x, double y) {
        m_Input.HandleCursorMove(x, y);
    }

    void Application::OnWindowFocusChanged(bool focused) {
        m_Input.SetWindowFocused(focused);
    }

    int Application::Run() {
        Startup();

        using Clock = std::chrono::high_resolution_clock;
        auto previousTime = Clock::now();

        while (m_Running) {
            const auto currentTime = Clock::now();

            const double deltaTime =
                    std::chrono::duration<double>(currentTime - previousTime).count();

            previousTime = currentTime;

            /*
                BeginFrame clears one-frame input states.

                glfwPollEvents then fills pressed/released states through callbacks.
                Luau update sees the input for this frame.
            */
            m_Input.BeginFrame();
            glfwPollEvents();

            Tick(deltaTime);

            m_Vulkan.DrawFrame();

            if (glfwWindowShouldClose(m_Window)) {
                Log::Info("Window close requested.");
                m_Running = false;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        Shutdown();
        return 0;
    }

    void Application::Startup() {
        Log::Info("Proton G logging online.");
        Log::Info(std::format("Starting {}...", m_Config.Name));
        Log::Info(std::format(
            "Requested window size: {}x{}",
            m_Config.Width,
            m_Config.Height
        ));

        if (!glfwInit()) {
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

        if (m_Window == nullptr) {
            Log::Error("Failed to create GLFW window.");

            glfwTerminate();

            m_Running = false;
            return;
        }

        Log::Info("GLFW window created.");

        glfwSetWindowUserPointer(m_Window, this);
        glfwSetFramebufferSizeCallback(m_Window, FramebufferResizeCallback);
        glfwSetKeyCallback(m_Window, KeyCallback);
        glfwSetWindowFocusCallback(m_Window, WindowFocusCallback);
        glfwSetMouseButtonCallback(m_Window, MouseButtonCallback);
        glfwSetCursorPosCallback(m_Window, CursorPositionCallback);

        m_Input.Attach(m_Window);

        if (!m_Vulkan.Init(m_Window)) {
            Log::Error("Failed to initialize Proton G Vulkan backend.");

            glfwDestroyWindow(m_Window);
            m_Window = nullptr;

            glfwTerminate();

            m_Running = false;
            return;
        }

        Log::Info("Proton G Vulkan backend online.");

        m_Vulkan.SetDraw2D(&m_Draw2D);
        m_Vulkan.SetSpriteManager(&m_SpriteManager);
        m_MediaPlayer.Attach(&m_Vulkan);

        Log::Info("Proton G v0.02 online.");

        m_ScriptEngine.SetClearColorCallback(
            [this](float r, float g, float b, float a) {
                m_Vulkan.SetClearColor(r, g, b, a);
            }
        );

        m_ScriptEngine.SetImageScaleModeCallback(
            [this](int scaleMode) {
                m_Vulkan.SetImageScaleMode(scaleMode);
            }
        );

        m_ScriptEngine.SetMediaPlayImageSequenceCallback(
            [this](
        const std::string &framePattern,
        const std::string &audioPath,
        int firstFrame,
        int lastFrame,
        double fps
    ) {
                ImageSequencePlaybackConfig config{};
                config.FramePattern = framePattern;
                config.AudioPath = audioPath;
                config.FirstFrame = firstFrame;
                config.LastFrame = lastFrame;
                config.Fps = fps;

                if (!m_MediaPlayer.PlayImageSequence(config)) {
                    Log::Error("Failed to start Luau-requested image sequence.");
                }
            }
        );

        m_ScriptEngine.SetMediaStopCallback(
            [this]() {
                m_MediaPlayer.Stop();
            }
        );

        m_ScriptEngine.SetDrawRectCallback(
            [this](
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
    ) {
                m_Draw2D.DrawRect(
                    id,
                    x,
                    y,
                    width,
                    height,
                    r,
                    g,
                    b,
                    a,
                    layer
                );
            }
        );

        m_ScriptEngine.SetDrawCircleCallback(
            [this](
        const std::string &id,
        float centerX,
        float centerY,
        float radius,
        float r,
        float g,
        float b,
        float a,
        int layer
    ) {
                m_Draw2D.DrawCircle(
                    id,
                    centerX,
                    centerY,
                    radius,
                    r,
                    g,
                    b,
                    a,
                    layer
                );
            }
        );

        m_ScriptEngine.SetDrawPolygonCallback(
            [this](
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
    ) {
                m_Draw2D.DrawPolygon(
                    id,
                    centerX,
                    centerY,
                    radius,
                    points,
                    rotationDegrees,
                    r,
                    g,
                    b,
                    a,
                    layer
                );
            }
        );

        m_ScriptEngine.SetDrawClearCallback(
            [this]() {
                m_Draw2D.BeginFrame();
            }
        );

        m_ScriptEngine.SetInputKeyDownCallback(
            [this](const std::string &keyName) {
                return m_Input.IsKeyDown(keyName);
            }
        );

        m_ScriptEngine.SetInputKeyPressedCallback(
            [this](const std::string &keyName) {
                return m_Input.IsKeyPressed(keyName);
            }
        );

        m_ScriptEngine.SetInputKeyReleasedCallback(
            [this](const std::string &keyName) {
                return m_Input.IsKeyReleased(keyName);
            }
        );

        m_ScriptEngine.SetInputMouseButtonDownCallback(
            [this](const std::string &buttonName) {
                return m_Input.IsMouseButtonDown(buttonName);
            }
        );

        m_ScriptEngine.SetInputMouseButtonPressedCallback(
            [this](const std::string &buttonName) {
                return m_Input.IsMouseButtonPressed(buttonName);
            }
        );

        m_ScriptEngine.SetInputMouseButtonReleasedCallback(
            [this](const std::string &buttonName) {
                return m_Input.IsMouseButtonReleased(buttonName);
            }
        );

        m_ScriptEngine.SetInputMouseXCallback(
            [this]() {
                return m_Input.GetMouseX();
            }
        );

        m_ScriptEngine.SetInputMouseYCallback(
            [this]() {
                return m_Input.GetMouseY();
            }
        );

        m_ScriptEngine.SetWindowWidthCallback(
            [this]() {
                int width = 0;
                int height = 0;

                glfwGetFramebufferSize(m_Window, &width, &height);

                return static_cast<double>(width);
            }
        );

        m_ScriptEngine.SetWindowHeightCallback(
            [this]() {
                int width = 0;
                int height = 0;

                glfwGetFramebufferSize(m_Window, &width, &height);

                return static_cast<double>(height);
            }
        );

        m_ScriptEngine.SetWindowTitleCallback(
            [this](const std::string &title) {
                m_WindowTitle = title;

                if (m_Window != nullptr && !m_WindowDebugMode) {
                    glfwSetWindowTitle(m_Window, m_WindowTitle.c_str());
                }

                Log::Info(std::format(
                    "Window title changed to: {}",
                    m_WindowTitle
                ));
            }
        );

        m_ScriptEngine.SetWindowDebugModeCallback(
            [this](bool enabled) {
                m_WindowDebugMode = enabled;

                if (m_Window != nullptr && !m_WindowDebugMode) {
                    glfwSetWindowTitle(m_Window, m_WindowTitle.c_str());
                }

                Log::Info(std::format(
                    "Window debug title mode changed to: {}",
                    m_WindowDebugMode ? "true" : "false"
                ));
            }
        );

        m_ScriptEngine.SetSpriteLoadCallback(
            [this](const std::string &path) {
                return m_SpriteManager.LoadSprite(path);
            }
        );

        m_ScriptEngine.SetSpriteDrawCallback(
            [this](
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
    ) {
                m_SpriteManager.DrawSprite(
                    id,
                    handle,
                    x,
                    y,
                    width,
                    height,
                    rotationDegrees,
                    r,
                    g,
                    b,
                    a,
                    layer
                );
            }
        );

        m_ScriptEngine.SetObjectCreateCallback(
            [this](
        const std::string &name,
        float x,
        float y,
        float width,
        float height
    ) {
                return m_ObjectManager.CreateObject(
                    name,
                    x,
                    y,
                    width,
                    height
                );
            }
        );

        m_ScriptEngine.SetObjectSetPositionCallback(
            [this](std::uint32_t handle, float x, float y) {
                m_ObjectManager.SetPosition(handle, x, y);
            }
        );

        m_ScriptEngine.SetObjectSetSizeCallback(
            [this](std::uint32_t handle, float width, float height) {
                m_ObjectManager.SetSize(handle, width, height);
            }
        );

        m_ScriptEngine.SetObjectGetXCallback(
            [this](std::uint32_t handle) {
                return m_ObjectManager.GetX(handle);
            }
        );

        m_ScriptEngine.SetObjectGetYCallback(
            [this](std::uint32_t handle) {
                return m_ObjectManager.GetY(handle);
            }
        );

        m_ScriptEngine.SetObjectGetWidthCallback(
            [this](std::uint32_t handle) {
                return m_ObjectManager.GetWidth(handle);
            }
        );

        m_ScriptEngine.SetObjectGetHeightCallback(
            [this](std::uint32_t handle) {
                return m_ObjectManager.GetHeight(handle);
            }
        );

        m_ScriptEngine.RunDirectory("sandbox/scripts");

        m_Running = true;
    }

    void Application::Tick(double deltaTime) {
        // Unless you need to debug the held
        // input, it's best to keep this commented
        // because this can cause a major lag spike
        // like the below:
        //
        // 517.9fps -> 312.7fps (it was more like 15fps)
        //
        // If you ever do need to debug the held input, uncomment
        // the below, but do it at the sacrifice of FPS.
        //
        // m_Input.LogHeldInputs(deltaTime);

        // Before updating Luau, ALWAYS begin drawing the frame
        // I hope this is basic common sense, you need to start
        // the job to get it done ;)
        m_Draw2D.BeginFrame();
        // Also, start the sprite manager
        m_SpriteManager.BeginFrame();

        m_ScriptEngine.Update(deltaTime);

        /*
            MediaPlayer owns synchronized media playback.
        */
        m_MediaPlayer.Update(deltaTime);

        m_FrameCounter++;
        m_StatsLogTimer += deltaTime;

        if (m_StatsLogTimer >= 1.0) {
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

            const double frameTimeMs = deltaTime * 1000.0;

            if (m_Window != nullptr) {
                if (m_WindowDebugMode) {
                    const std::string debugTitle = std::format(
                        "{} | FPS: {:.1f} | Frame: {:.3f} ms",
                        m_WindowTitle,
                        fps,
                        frameTimeMs
                    );

                    glfwSetWindowTitle(m_Window, debugTitle.c_str());
                } else {
                    glfwSetWindowTitle(m_Window, m_WindowTitle.c_str());
                }
            }

            m_FrameCounter = 0;
            m_StatsLogTimer = 0.0;
        }
    }

    void Application::Shutdown() {
        Log::Info("Proton G shutting down.");

        /*
            Stop coordinated media first so it can stop audio cleanly before
            the audio backend is destroyed.
        */
        m_MediaPlayer.Stop();

        m_Vulkan.Shutdown();

        if (m_Window != nullptr) {
            glfwSetFramebufferSizeCallback(m_Window, nullptr);
            glfwSetKeyCallback(m_Window, nullptr);
            glfwSetWindowFocusCallback(m_Window, nullptr);
            glfwSetMouseButtonCallback(m_Window, nullptr);
            glfwSetCursorPosCallback(m_Window, nullptr);
            glfwSetWindowUserPointer(m_Window, nullptr);

            glfwDestroyWindow(m_Window);
            m_Window = nullptr;
        }

        glfwTerminate();

        m_Running = false;
    }
}
