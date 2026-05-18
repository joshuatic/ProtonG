#pragma once

#include <array>
#include <string>

struct GLFWwindow;

namespace Proton {
    class Input {
    public:
        static constexpr int MaxKeys = 512;
        static constexpr int MaxMouseButtons = 8;

        Input() = default;

        void Attach(GLFWwindow *window);

        /*
            Called once at the start of every frame.

            Pressed/released are one-frame states, so they must be cleared
            every frame before new GLFW input events arrive.
        */
        void BeginFrame();

        /*
            Called when the window focus changes.

            If the window loses focus, Proton G should not keep stale keys
            or mouse buttons stuck as "down".
        */
        void SetWindowFocused(bool focused);

        bool IsWindowFocused() const;

        void HandleKeyEvent(int key, int action);

        void HandleMouseButtonEvent(int button, int action);

        void HandleCursorMove(double x, double y);

        bool IsKeyDown(int key) const;

        bool IsKeyPressed(int key) const;

        bool IsKeyReleased(int key) const;

        bool IsKeyDown(const std::string &keyName) const;

        bool IsKeyPressed(const std::string &keyName) const;

        bool IsKeyReleased(const std::string &keyName) const;

        bool IsMouseButtonDown(int button) const;

        bool IsMouseButtonPressed(int button) const;

        bool IsMouseButtonReleased(int button) const;

        bool IsMouseButtonDown(const std::string &buttonName) const;

        bool IsMouseButtonPressed(const std::string &buttonName) const;

        bool IsMouseButtonReleased(const std::string &buttonName) const;

        void LogHeldInputs(double deltaTime);

        double GetMouseX() const;

        double GetMouseY() const;

        static int ResolveKeyName(const std::string &keyName);

        static int ResolveMouseButtonName(const std::string &buttonName);

    private:
        void ClearAllKeys();

        static bool IsValidKey(int key);

        static bool IsValidMouseButton(int button);

        GLFWwindow *m_Window = nullptr;

        bool m_WindowFocused = true;

        std::array<bool, MaxKeys> m_KeyDown{};
        std::array<bool, MaxKeys> m_KeyPressed{};
        std::array<bool, MaxKeys> m_KeyReleased{};

        std::array<bool, MaxMouseButtons> m_MouseDown{};
        std::array<bool, MaxMouseButtons> m_MousePressed{};
        std::array<bool, MaxMouseButtons> m_MouseReleased{};

        double m_MouseX = 0.0;
        double m_MouseY = 0.0;

        double m_HeldLogTimer = 0.0;
    };
}
