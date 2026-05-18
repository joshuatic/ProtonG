#include "ProtonInput.hpp"

#include "ProtonLog.hpp"

#include <GLFW/glfw3.h>

#include <algorithm>
#include <cctype>
#include <format>
#include <unordered_map>

namespace Proton {
    static const char *GetActionName(int action) {
        if (action == GLFW_PRESS) {
            return "pressed";
        }

        if (action == GLFW_RELEASE) {
            return "released";
        }

        if (action == GLFW_REPEAT) {
            return "repeated";
        }

        return "unknown";
    }

    void Input::Attach(GLFWwindow *window) {
        m_Window = window;

        ClearAllKeys();

        if (m_Window == nullptr) {
            m_WindowFocused = false;
            return;
        }

        m_WindowFocused =
                glfwGetWindowAttrib(m_Window, GLFW_FOCUSED) == GLFW_TRUE;
    }

    void Input::BeginFrame() {
        m_KeyPressed.fill(false);
        m_KeyReleased.fill(false);

        m_MousePressed.fill(false);
        m_MouseReleased.fill(false);

        if (!m_WindowFocused) {
            ClearAllKeys();
        }
    }

    void Input::SetWindowFocused(bool focused) {
        m_WindowFocused = focused;

        if (!m_WindowFocused) {
            ClearAllKeys();
        }
    }

    bool Input::IsWindowFocused() const {
        return m_WindowFocused;
    }

    void Input::HandleKeyEvent(int key, int action) {
        if (!m_WindowFocused) {
            return;
        }

        if (!IsValidKey(key)) {
            Log::Info(std::format(
                "Input ignored unknown key. Key={}, Action={}",
                key,
                GetActionName(action)
            ));

            return;
        }

        if (action == GLFW_REPEAT) {
            return;
        }

        const char *keyName = glfwGetKeyName(key, 0);

        if (keyName == nullptr) {
            Log::Info(std::format(
                "Input event | Key={} | Action={}",
                key,
                GetActionName(action)
            ));
        } else {
            Log::Info(std::format(
                "Input event | Key={} ({}) | Action={}",
                key,
                keyName,
                GetActionName(action)
            ));
        }

        const std::size_t index = static_cast<std::size_t>(key);

        if (action == GLFW_PRESS) {
            if (!m_KeyDown[index]) {
                m_KeyPressed[index] = true;
            }

            m_KeyDown[index] = true;
            return;
        }

        if (action == GLFW_RELEASE) {
            if (m_KeyDown[index]) {
                m_KeyReleased[index] = true;
            }

            m_KeyDown[index] = false;
        }
    }

    void Input::HandleMouseButtonEvent(int button, int action) {
        if (!m_WindowFocused || !IsValidMouseButton(button)) {
            return;
        }

        const std::size_t index = static_cast<std::size_t>(button);

        if (action == GLFW_PRESS) {
            if (!m_MouseDown[index]) {
                m_MousePressed[index] = true;
            }

            m_MouseDown[index] = true;

            Log::Info(std::format(
                "Mouse event | Button={} | Action=pressed | Position=({:.1f}, {:.1f})",
                button,
                m_MouseX,
                m_MouseY
            ));

            return;
        }

        if (action == GLFW_RELEASE) {
            if (m_MouseDown[index]) {
                m_MouseReleased[index] = true;
            }

            m_MouseDown[index] = false;

            Log::Info(std::format(
                "Mouse event | Button={} | Action=released | Position=({:.1f}, {:.1f})",
                button,
                m_MouseX,
                m_MouseY
            ));
        }
    }

    void Input::HandleCursorMove(double x, double y) {
        if (!m_WindowFocused) {
            return;
        }

        m_MouseX = x;

        /*
            GLFW gives mouse Y from the top of the window.
            Our current Rect2D renderer is behaving like Y is measured from the bottom.

            Flip mouse Y so hit detection and debug cursor match the rendered rectangles.
        */
        int framebufferWidth = 0;
        int framebufferHeight = 0;

        glfwGetFramebufferSize(
            m_Window,
            &framebufferWidth,
            &framebufferHeight
        );

        m_MouseY = static_cast<double>(framebufferHeight) - y;
    }

    bool Input::IsKeyDown(int key) const {
        if (!m_WindowFocused || !IsValidKey(key)) {
            return false;
        }

        return m_KeyDown[static_cast<std::size_t>(key)];
    }

    bool Input::IsKeyPressed(int key) const {
        if (!m_WindowFocused || !IsValidKey(key)) {
            return false;
        }

        return m_KeyPressed[static_cast<std::size_t>(key)];
    }

    bool Input::IsKeyReleased(int key) const {
        if (!m_WindowFocused || !IsValidKey(key)) {
            return false;
        }

        return m_KeyReleased[static_cast<std::size_t>(key)];
    }

    bool Input::IsKeyDown(const std::string &keyName) const {
        return IsKeyDown(ResolveKeyName(keyName));
    }

    bool Input::IsKeyPressed(const std::string &keyName) const {
        return IsKeyPressed(ResolveKeyName(keyName));
    }

    bool Input::IsKeyReleased(const std::string &keyName) const {
        return IsKeyReleased(ResolveKeyName(keyName));
    }

    bool Input::IsMouseButtonDown(int button) const {
        if (!m_WindowFocused || !IsValidMouseButton(button)) {
            return false;
        }

        return m_MouseDown[static_cast<std::size_t>(button)];
    }

    bool Input::IsMouseButtonPressed(int button) const {
        if (!m_WindowFocused || !IsValidMouseButton(button)) {
            return false;
        }

        return m_MousePressed[static_cast<std::size_t>(button)];
    }

    bool Input::IsMouseButtonReleased(int button) const {
        if (!m_WindowFocused || !IsValidMouseButton(button)) {
            return false;
        }

        return m_MouseReleased[static_cast<std::size_t>(button)];
    }

    bool Input::IsMouseButtonDown(const std::string &buttonName) const {
        return IsMouseButtonDown(ResolveMouseButtonName(buttonName));
    }

    bool Input::IsMouseButtonPressed(const std::string &buttonName) const {
        return IsMouseButtonPressed(ResolveMouseButtonName(buttonName));
    }

    bool Input::IsMouseButtonReleased(const std::string &buttonName) const {
        return IsMouseButtonReleased(ResolveMouseButtonName(buttonName));
    }

    double Input::GetMouseX() const {
        return m_MouseX;
    }

    double Input::GetMouseY() const {
        return m_MouseY;
    }

    void Input::LogHeldInputs(double deltaTime) {
        if (!m_WindowFocused) {
            return;
        }

        m_HeldLogTimer += deltaTime;

        if (m_HeldLogTimer < 1.0) {
            return;
        }

        m_HeldLogTimer = 0.0;

        for (int key = 0; key < MaxKeys; key++) {
            if (!m_KeyDown[static_cast<std::size_t>(key)]) {
                continue;
            }

            const char *keyName = glfwGetKeyName(key, 0);

            if (keyName == nullptr) {
                Log::Info(std::format(
                    "Input held | Key={}",
                    key
                ));
            } else {
                Log::Info(std::format(
                    "Input held | Key={} ({})",
                    key,
                    keyName
                ));
            }
        }

        for (int button = 0; button < MaxMouseButtons; button++) {
            if (!m_MouseDown[static_cast<std::size_t>(button)]) {
                continue;
            }

            Log::Info(std::format(
                "Mouse held | Button={} | Position=({:.1f}, {:.1f})",
                button,
                m_MouseX,
                m_MouseY
            ));
        }
    }

    int Input::ResolveKeyName(const std::string &keyName) {
        std::string normalized = keyName;

        std::transform(
            normalized.begin(),
            normalized.end(),
            normalized.begin(),
            [](unsigned char character) {
                return static_cast<char>(std::tolower(character));
            }
        );

        static const std::unordered_map<std::string, int> keyMap =
        {
            {"space", GLFW_KEY_SPACE},
            {"escape", GLFW_KEY_ESCAPE},
            {"esc", GLFW_KEY_ESCAPE},
            {"enter", GLFW_KEY_ENTER},
            {"return", GLFW_KEY_ENTER},
            {"tab", GLFW_KEY_TAB},
            {"backspace", GLFW_KEY_BACKSPACE},

            {"left", GLFW_KEY_LEFT},
            {"right", GLFW_KEY_RIGHT},
            {"up", GLFW_KEY_UP},
            {"down", GLFW_KEY_DOWN},

            {"shift", GLFW_KEY_LEFT_SHIFT},
            {"left_shift", GLFW_KEY_LEFT_SHIFT},
            {"right_shift", GLFW_KEY_RIGHT_SHIFT},

            {"ctrl", GLFW_KEY_LEFT_CONTROL},
            {"control", GLFW_KEY_LEFT_CONTROL},
            {"left_ctrl", GLFW_KEY_LEFT_CONTROL},
            {"right_ctrl", GLFW_KEY_RIGHT_CONTROL},

            {"alt", GLFW_KEY_LEFT_ALT},
            {"left_alt", GLFW_KEY_LEFT_ALT},
            {"right_alt", GLFW_KEY_RIGHT_ALT},

            {"a", GLFW_KEY_A},
            {"b", GLFW_KEY_B},
            {"c", GLFW_KEY_C},
            {"d", GLFW_KEY_D},
            {"e", GLFW_KEY_E},
            {"f", GLFW_KEY_F},
            {"g", GLFW_KEY_G},
            {"h", GLFW_KEY_H},
            {"i", GLFW_KEY_I},
            {"j", GLFW_KEY_J},
            {"k", GLFW_KEY_K},
            {"l", GLFW_KEY_L},
            {"m", GLFW_KEY_M},
            {"n", GLFW_KEY_N},
            {"o", GLFW_KEY_O},
            {"p", GLFW_KEY_P},
            {"q", GLFW_KEY_Q},
            {"r", GLFW_KEY_R},
            {"s", GLFW_KEY_S},
            {"t", GLFW_KEY_T},
            {"u", GLFW_KEY_U},
            {"v", GLFW_KEY_V},
            {"w", GLFW_KEY_W},
            {"x", GLFW_KEY_X},
            {"y", GLFW_KEY_Y},
            {"z", GLFW_KEY_Z},

            {"0", GLFW_KEY_0},
            {"1", GLFW_KEY_1},
            {"2", GLFW_KEY_2},
            {"3", GLFW_KEY_3},
            {"4", GLFW_KEY_4},
            {"5", GLFW_KEY_5},
            {"6", GLFW_KEY_6},
            {"7", GLFW_KEY_7},
            {"8", GLFW_KEY_8},
            {"9", GLFW_KEY_9},

            {"f1", GLFW_KEY_F1},
            {"f2", GLFW_KEY_F2},
            {"f3", GLFW_KEY_F3},
            {"f4", GLFW_KEY_F4},
            {"f5", GLFW_KEY_F5},
            {"f6", GLFW_KEY_F6},
            {"f7", GLFW_KEY_F7},
            {"f8", GLFW_KEY_F8},
            {"f9", GLFW_KEY_F9},
            {"f10", GLFW_KEY_F10},
            {"f11", GLFW_KEY_F11},
            {"f12", GLFW_KEY_F12},
        };

        const auto iterator = keyMap.find(normalized);

        if (iterator == keyMap.end()) {
            return GLFW_KEY_UNKNOWN;
        }

        return iterator->second;
    }

    int Input::ResolveMouseButtonName(const std::string &buttonName) {
        std::string normalized = buttonName;

        std::transform(
            normalized.begin(),
            normalized.end(),
            normalized.begin(),
            [](unsigned char character) {
                return static_cast<char>(std::tolower(character));
            }
        );

        if (normalized == "left" || normalized == "mouse_left" || normalized == "left_click") {
            return GLFW_MOUSE_BUTTON_LEFT;
        }

        if (normalized == "right" || normalized == "mouse_right" || normalized == "right_click") {
            return GLFW_MOUSE_BUTTON_RIGHT;
        }

        if (normalized == "middle" || normalized == "mouse_middle" || normalized == "middle_click") {
            return GLFW_MOUSE_BUTTON_MIDDLE;
        }

        if (normalized == "button4" || normalized == "mouse4") {
            return GLFW_MOUSE_BUTTON_4;
        }

        if (normalized == "button5" || normalized == "mouse5") {
            return GLFW_MOUSE_BUTTON_5;
        }

        return -1;
    }

    void Input::ClearAllKeys() {
        m_KeyDown.fill(false);
        m_KeyPressed.fill(false);
        m_KeyReleased.fill(false);

        m_MouseDown.fill(false);
        m_MousePressed.fill(false);
        m_MouseReleased.fill(false);
    }

    bool Input::IsValidKey(int key) {
        return key >= 0 && key < MaxKeys;
    }

    bool Input::IsValidMouseButton(int button) {
        return button >= 0 && button < MaxMouseButtons;
    }
}
