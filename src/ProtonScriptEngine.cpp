#include "ProtonScriptEngine.hpp"

#include "ProtonLog.hpp"

#include <Luau/Compiler.h>

#include <lua.h>
#include <lualib.h>

#include <filesystem>
#include <fstream>
#include <format>
#include <sstream>
#include <string>
#include <utility>
#include <algorithm>
#include <vector>
// Extra safety header
#include <cstdint>

namespace Proton {
    static ScriptEngine *GetScriptEngine(lua_State *state) {
        lua_getglobal(state, "__proton_script_engine");

        ScriptEngine *engine =
                static_cast<ScriptEngine *>(lua_touserdata(state, -1));

        lua_pop(state, 1);
        return engine;
    }

    static int ProtonPrint(lua_State *state) {
        const int argCount = lua_gettop(state);

        std::string output;

        for (int i = 1; i <= argCount; i++) {
            size_t length = 0;
            const char *text = lua_tolstring(state, i, &length);

            if (text != nullptr) {
                output.append(text, length);
            } else {
                output += luaL_typename(state, i);
            }

            if (i < argCount) {
                output += " ";
            }
        }

        Log::Info(std::format("[Luau] {}", output));
        return 0;
    }

    static int ProtonSetClearColor(lua_State *state) {
        const float r = static_cast<float>(luaL_checknumber(state, 1));
        const float g = static_cast<float>(luaL_checknumber(state, 2));
        const float b = static_cast<float>(luaL_checknumber(state, 3));
        const float a = static_cast<float>(luaL_optnumber(state, 4, 1.0));

        Log::Info(std::format(
            "[Luau] proton.setClearColor({}, {}, {}, {})",
            r,
            g,
            b,
            a
        ));

        ScriptEngine *engine = GetScriptEngine(state);

        if (engine != nullptr) {
            engine->InvokeClearColor(r, g, b, a);
        }

        return 0;
    }

    static int ProtonRenderImage(lua_State *state) {
        const char *path = luaL_checkstring(state, 1);

        Log::Info(std::format(
            "[Luau] proton.renderImage({})",
            path
        ));

        ScriptEngine *engine = GetScriptEngine(state);

        if (engine != nullptr) {
            engine->InvokeRenderImage(path);
        }

        return 0;
    }

    static int ProtonSetImageScaleMode(lua_State *state) {
        const char *modeText = luaL_checkstring(state, 1);

        int scaleMode = 1;

        if (std::string(modeText) == "stretch") {
            scaleMode = 0;
        } else if (std::string(modeText) == "fit") {
            scaleMode = 1;
        } else if (std::string(modeText) == "fill") {
            scaleMode = 2;
        } else if (std::string(modeText) == "native") {
            scaleMode = 3;
        } else {
            Log::Error(std::format(
                "[Luau] Unknown image scale mode: {}",
                modeText
            ));

            return 0;
        }

        Log::Info(std::format(
            "[Luau] proton.setImageScaleMode({})",
            modeText
        ));

        ScriptEngine *engine = GetScriptEngine(state);

        if (engine != nullptr) {
            engine->InvokeImageScaleMode(scaleMode);
        }

        return 0;
    }

    static int ProtonPlayAudio(lua_State *state) {
        const char *path = luaL_checkstring(state, 1);

        Log::Info(std::format(
            "[Luau] proton.playAudio({})",
            path
        ));

        ScriptEngine *engine = GetScriptEngine(state);

        if (engine != nullptr) {
            engine->InvokePlayAudio(path);
        }

        return 0;
    }

    static int ProtonGetAudioTime(lua_State *state) {
        ScriptEngine *engine = GetScriptEngine(state);

        double time = 0.0;

        if (engine != nullptr) {
            time = engine->InvokeAudioTime();
        }

        lua_pushnumber(state, time);
        return 1;
    }

    static int ProtonMediaPlayImageSequence(lua_State *state) {
        const char *framePattern = luaL_checkstring(state, 1);
        const char *audioPath = luaL_checkstring(state, 2);

        const int firstFrame =
                static_cast<int>(luaL_checkinteger(state, 3));

        const int lastFrame =
                static_cast<int>(luaL_checkinteger(state, 4));

        const double fps =
                luaL_checknumber(state, 5);

        Log::Info(std::format(
            "[Luau] proton.media.playImageSequence({}, {}, {}, {}, {:.3f})",
            framePattern,
            audioPath,
            firstFrame,
            lastFrame,
            fps
        ));

        ScriptEngine *engine = GetScriptEngine(state);

        if (engine != nullptr) {
            engine->InvokeMediaPlayImageSequence(
                framePattern,
                audioPath,
                firstFrame,
                lastFrame,
                fps
            );
        }

        return 0;
    }

    static int ProtonMediaStop(lua_State *state) {
        Log::Info("[Luau] proton.media.stop()");

        ScriptEngine *engine = GetScriptEngine(state);

        if (engine != nullptr) {
            engine->InvokeMediaStop();
        }

        return 0;
    }

    static int ProtonDrawRect(lua_State *state) {
        const char *id = luaL_checkstring(state, 1);

        const float x = static_cast<float>(luaL_checknumber(state, 2));
        const float y = static_cast<float>(luaL_checknumber(state, 3));
        const float width = static_cast<float>(luaL_checknumber(state, 4));
        const float height = static_cast<float>(luaL_checknumber(state, 5));

        const float r = static_cast<float>(luaL_optnumber(state, 6, 1.0));
        const float g = static_cast<float>(luaL_optnumber(state, 7, 1.0));
        const float b = static_cast<float>(luaL_optnumber(state, 8, 1.0));
        const float a = static_cast<float>(luaL_optnumber(state, 9, 1.0));

        const int layer = static_cast<int>(luaL_optinteger(state, 10, 0));

        ScriptEngine *engine = GetScriptEngine(state);

        if (engine != nullptr) {
            engine->InvokeDrawRect(
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

        return 0;
    }

    static int ProtonDrawClear(lua_State *state) {
        Log::Info("[Luau] proton.draw.clear()");

        ScriptEngine *engine = GetScriptEngine(state);

        if (engine != nullptr) {
            engine->InvokeDrawClear();
        }

        return 0;
    }

    static int ProtonInputIsKeyDown(lua_State *state) {
        const char *keyName = luaL_checkstring(state, 1);

        ScriptEngine *engine = GetScriptEngine(state);

        const bool result =
                engine != nullptr && engine->InvokeInputKeyDown(keyName);

        lua_pushboolean(state, result);
        return 1;
    }

    static int ProtonInputIsKeyPressed(lua_State *state) {
        const char *keyName = luaL_checkstring(state, 1);

        ScriptEngine *engine = GetScriptEngine(state);

        const bool result =
                engine != nullptr && engine->InvokeInputKeyPressed(keyName);

        lua_pushboolean(state, result);
        return 1;
    }

    static int ProtonInputIsKeyReleased(lua_State *state) {
        const char *keyName = luaL_checkstring(state, 1);

        ScriptEngine *engine = GetScriptEngine(state);

        const bool result =
                engine != nullptr && engine->InvokeInputKeyReleased(keyName);

        lua_pushboolean(state, result);
        return 1;
    }

    static int ProtonInputIsMouseButtonDown(lua_State *state) {
        const char *buttonName = luaL_checkstring(state, 1);

        ScriptEngine *engine = GetScriptEngine(state);

        const bool result =
                engine != nullptr && engine->InvokeInputMouseButtonDown(buttonName);

        lua_pushboolean(state, result);
        return 1;
    }

    static int ProtonInputIsMouseButtonPressed(lua_State *state) {
        const char *buttonName = luaL_checkstring(state, 1);

        ScriptEngine *engine = GetScriptEngine(state);

        const bool result =
                engine != nullptr && engine->InvokeInputMouseButtonPressed(buttonName);

        lua_pushboolean(state, result);
        return 1;
    }

    static int ProtonInputIsMouseButtonReleased(lua_State *state) {
        const char *buttonName = luaL_checkstring(state, 1);

        ScriptEngine *engine = GetScriptEngine(state);

        const bool result =
                engine != nullptr && engine->InvokeInputMouseButtonReleased(buttonName);

        lua_pushboolean(state, result);
        return 1;
    }

    static int ProtonInputGetMouseX(lua_State *state) {
        ScriptEngine *engine = GetScriptEngine(state);

        const double result =
                engine != nullptr ? engine->InvokeInputMouseX() : 0.0;

        lua_pushnumber(state, result);
        return 1;
    }

    static int ProtonInputGetMouseY(lua_State *state) {
        ScriptEngine *engine = GetScriptEngine(state);

        const double result =
                engine != nullptr ? engine->InvokeInputMouseY() : 0.0;

        lua_pushnumber(state, result);
        return 1;
    }

    static int ProtonWindowGetWidth(lua_State *state) {
        ScriptEngine *engine = GetScriptEngine(state);

        const double width =
                engine != nullptr ? engine->InvokeWindowWidth() : 0.0;

        lua_pushnumber(state, width);
        return 1;
    }

    static int ProtonWindowGetHeight(lua_State *state) {
        ScriptEngine *engine = GetScriptEngine(state);

        const double height =
                engine != nullptr ? engine->InvokeWindowHeight() : 0.0;

        lua_pushnumber(state, height);
        return 1;
    }

    static int ProtonDrawCircle(lua_State *state) {
        const char *id = luaL_checkstring(state, 1);

        const float centerX = static_cast<float>(luaL_checknumber(state, 2));
        const float centerY = static_cast<float>(luaL_checknumber(state, 3));
        const float radius = static_cast<float>(luaL_checknumber(state, 4));

        const float r = static_cast<float>(luaL_optnumber(state, 5, 1.0));
        const float g = static_cast<float>(luaL_optnumber(state, 6, 1.0));
        const float b = static_cast<float>(luaL_optnumber(state, 7, 1.0));
        const float a = static_cast<float>(luaL_optnumber(state, 8, 1.0));

        const int layer = static_cast<int>(luaL_optinteger(state, 9, 0));

        ScriptEngine *engine = GetScriptEngine(state);

        if (engine != nullptr) {
            engine->InvokeDrawCircle(
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

        return 0;
    }

    static int ProtonDrawPolygon(lua_State *state) {
        const char *id = luaL_checkstring(state, 1);

        const float centerX = static_cast<float>(luaL_checknumber(state, 2));
        const float centerY = static_cast<float>(luaL_checknumber(state, 3));
        const float radius = static_cast<float>(luaL_checknumber(state, 4));

        const int points = static_cast<int>(luaL_checkinteger(state, 5));

        const float rotationDegrees =
                static_cast<float>(luaL_optnumber(state, 6, 0.0));

        const float r = static_cast<float>(luaL_optnumber(state, 7, 1.0));
        const float g = static_cast<float>(luaL_optnumber(state, 8, 1.0));
        const float b = static_cast<float>(luaL_optnumber(state, 9, 1.0));
        const float a = static_cast<float>(luaL_optnumber(state, 10, 1.0));

        const int layer = static_cast<int>(luaL_optinteger(state, 11, 0));

        ScriptEngine *engine = GetScriptEngine(state);

        if (engine != nullptr) {
            engine->InvokeDrawPolygon(
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

        return 0;
    }

    static int ProtonWindowSetTitle(lua_State *state) {
        const char *title = luaL_checkstring(state, 1);

        ScriptEngine *engine = GetScriptEngine(state);

        if (engine != nullptr) {
            engine->InvokeWindowTitle(title);
        }

        return 0;
    }

    static int ProtonWindowSetDebugMode(lua_State *state) {
        const bool enabled = lua_toboolean(state, 1) != 0;

        ScriptEngine *engine = GetScriptEngine(state);

        if (engine != nullptr) {
            engine->InvokeWindowDebugMode(enabled);
        }

        return 0;
    }

    static int ProtonCollisionRectsOverlap(lua_State *state) {
        const float ax = static_cast<float>(luaL_checknumber(state, 1));
        const float ay = static_cast<float>(luaL_checknumber(state, 2));
        const float aw = static_cast<float>(luaL_checknumber(state, 3));
        const float ah = static_cast<float>(luaL_checknumber(state, 4));

        const float bx = static_cast<float>(luaL_checknumber(state, 5));
        const float by = static_cast<float>(luaL_checknumber(state, 6));
        const float bw = static_cast<float>(luaL_checknumber(state, 7));
        const float bh = static_cast<float>(luaL_checknumber(state, 8));

        const bool overlaps =
                ax < bx + bw &&
                ax + aw > bx &&
                ay < by + bh &&
                ay + ah > by;

        lua_pushboolean(state, overlaps);
        return 1;
    }

    static int ProtonSpriteLoad(lua_State *state) {
        const char *path = luaL_checkstring(state, 1);

        ScriptEngine *engine = GetScriptEngine(state);

        std::uint32_t handle = 0;

        if (engine != nullptr) {
            handle = engine->InvokeSpriteLoad(path);
        }

        lua_pushinteger(state, static_cast<lua_Integer>(handle));
        return 1;
    }

    static int ProtonSpriteDraw(lua_State *state) {
        const char *id = luaL_checkstring(state, 1);

        const std::uint32_t handle =
                static_cast<std::uint32_t>(luaL_checkinteger(state, 2));

        const float x = static_cast<float>(luaL_checknumber(state, 3));
        const float y = static_cast<float>(luaL_checknumber(state, 4));
        const float width = static_cast<float>(luaL_checknumber(state, 5));
        const float height = static_cast<float>(luaL_checknumber(state, 6));

        const float rotationDegrees =
                static_cast<float>(luaL_optnumber(state, 7, 0.0));

        const float r = static_cast<float>(luaL_optnumber(state, 8, 1.0));
        const float g = static_cast<float>(luaL_optnumber(state, 9, 1.0));
        const float b = static_cast<float>(luaL_optnumber(state, 10, 1.0));
        const float a = static_cast<float>(luaL_optnumber(state, 11, 1.0));

        const int layer = static_cast<int>(luaL_optinteger(state, 12, 0));

        ScriptEngine *engine = GetScriptEngine(state);

        if (engine != nullptr) {
            engine->InvokeSpriteDraw(
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

        return 0;
    }

    static int ProtonObjectCreate(lua_State *state) {
        const char *name = luaL_checkstring(state, 1);

        const float x = static_cast<float>(luaL_optnumber(state, 2, 0.0));
        const float y = static_cast<float>(luaL_optnumber(state, 3, 0.0));
        const float width = static_cast<float>(luaL_optnumber(state, 4, 0.0));
        const float height = static_cast<float>(luaL_optnumber(state, 5, 0.0));

        ScriptEngine *engine = GetScriptEngine(state);

        std::uint32_t handle = 0;

        if (engine != nullptr) {
            handle = engine->InvokeObjectCreate(
                name,
                x,
                y,
                width,
                height
            );
        }

        lua_pushinteger(state, static_cast<lua_Integer>(handle));
        return 1;
    }

    static int ProtonObjectSetPosition(lua_State *state) {
        const std::uint32_t handle =
                static_cast<std::uint32_t>(luaL_checkinteger(state, 1));

        const float x = static_cast<float>(luaL_checknumber(state, 2));
        const float y = static_cast<float>(luaL_checknumber(state, 3));

        ScriptEngine *engine = GetScriptEngine(state);

        if (engine != nullptr) {
            engine->InvokeObjectSetPosition(handle, x, y);
        }

        return 0;
    }

    static int ProtonObjectSetSize(lua_State *state) {
        const std::uint32_t handle =
                static_cast<std::uint32_t>(luaL_checkinteger(state, 1));

        const float width = static_cast<float>(luaL_checknumber(state, 2));
        const float height = static_cast<float>(luaL_checknumber(state, 3));

        ScriptEngine *engine = GetScriptEngine(state);

        if (engine != nullptr) {
            engine->InvokeObjectSetSize(handle, width, height);
        }

        return 0;
    }

    static int ProtonObjectGetX(lua_State *state) {
        const std::uint32_t handle =
                static_cast<std::uint32_t>(luaL_checkinteger(state, 1));

        ScriptEngine *engine = GetScriptEngine(state);
        const float value = engine != nullptr ? engine->InvokeObjectGetX(handle) : 0.0f;

        lua_pushnumber(state, value);
        return 1;
    }

    static int ProtonObjectGetY(lua_State *state) {
        const std::uint32_t handle =
                static_cast<std::uint32_t>(luaL_checkinteger(state, 1));

        ScriptEngine *engine = GetScriptEngine(state);
        const float value = engine != nullptr ? engine->InvokeObjectGetY(handle) : 0.0f;

        lua_pushnumber(state, value);
        return 1;
    }

    static int ProtonObjectGetWidth(lua_State *state) {
        const std::uint32_t handle =
                static_cast<std::uint32_t>(luaL_checkinteger(state, 1));

        ScriptEngine *engine = GetScriptEngine(state);
        const float value = engine != nullptr ? engine->InvokeObjectGetWidth(handle) : 0.0f;

        lua_pushnumber(state, value);
        return 1;
    }

    static int ProtonObjectGetHeight(lua_State *state) {
        const std::uint32_t handle =
                static_cast<std::uint32_t>(luaL_checkinteger(state, 1));

        ScriptEngine *engine = GetScriptEngine(state);
        const float value = engine != nullptr ? engine->InvokeObjectGetHeight(handle) : 0.0f;

        lua_pushnumber(state, value);
        return 1;
    }


    static int ProtonSpriteDrawOn(lua_State *state) {
        const char *id = luaL_checkstring(state, 1);

        const std::uint32_t handle =
                static_cast<std::uint32_t>(luaL_checkinteger(state, 2));

        const float targetX = static_cast<float>(luaL_checknumber(state, 3));
        const float targetY = static_cast<float>(luaL_checknumber(state, 4));
        const float targetWidth = static_cast<float>(luaL_checknumber(state, 5));
        const float targetHeight = static_cast<float>(luaL_checknumber(state, 6));

        const float offsetX = static_cast<float>(luaL_optnumber(state, 7, 0.0));
        const float offsetY = static_cast<float>(luaL_optnumber(state, 8, 0.0));

        const float width =
                static_cast<float>(luaL_optnumber(state, 9, targetWidth));

        const float height =
                static_cast<float>(luaL_optnumber(state, 10, targetHeight));

        const float rotationDegrees =
                static_cast<float>(luaL_optnumber(state, 11, 0.0));

        const float r = static_cast<float>(luaL_optnumber(state, 12, 1.0));
        const float g = static_cast<float>(luaL_optnumber(state, 13, 1.0));
        const float b = static_cast<float>(luaL_optnumber(state, 14, 1.0));
        const float a = static_cast<float>(luaL_optnumber(state, 15, 1.0));

        const int layer = static_cast<int>(luaL_optinteger(state, 16, 0));

        ScriptEngine *engine = GetScriptEngine(state);

        if (engine != nullptr) {
            engine->InvokeSpriteDraw(
                id,
                handle,
                targetX + offsetX,
                targetY + offsetY,
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

        return 0;
    }

    static int ProtonSpriteDrawObject(lua_State *state) {
        const char *id = luaL_checkstring(state, 1);

        const std::uint32_t spriteHandle =
                static_cast<std::uint32_t>(luaL_checkinteger(state, 2));

        const std::uint32_t objectHandle =
                static_cast<std::uint32_t>(luaL_checkinteger(state, 3));

        const float rotationDegrees =
                static_cast<float>(luaL_optnumber(state, 4, 0.0));

        const float r = static_cast<float>(luaL_optnumber(state, 5, 1.0));
        const float g = static_cast<float>(luaL_optnumber(state, 6, 1.0));
        const float b = static_cast<float>(luaL_optnumber(state, 7, 1.0));
        const float a = static_cast<float>(luaL_optnumber(state, 8, 1.0));

        const int layer = static_cast<int>(luaL_optinteger(state, 9, 0));

        ScriptEngine *engine = GetScriptEngine(state);

        if (engine != nullptr) {
            const float x = engine->InvokeObjectGetX(objectHandle);
            const float y = engine->InvokeObjectGetY(objectHandle);
            const float width = engine->InvokeObjectGetWidth(objectHandle);
            const float height = engine->InvokeObjectGetHeight(objectHandle);

            engine->InvokeSpriteDraw(
                id,
                spriteHandle,
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

        return 0;
    }

    static int ProtonSpriteDrawOnObject(lua_State *state) {
        const char *id = luaL_checkstring(state, 1);

        const std::uint32_t spriteHandle =
                static_cast<std::uint32_t>(luaL_checkinteger(state, 2));

        const std::uint32_t objectHandle =
                static_cast<std::uint32_t>(luaL_checkinteger(state, 3));

        const float offsetX = static_cast<float>(luaL_optnumber(state, 4, 0.0));
        const float offsetY = static_cast<float>(luaL_optnumber(state, 5, 0.0));

        ScriptEngine *engine = GetScriptEngine(state);

        float objectX = 0.0f;
        float objectY = 0.0f;
        float objectWidth = 0.0f;
        float objectHeight = 0.0f;

        if (engine != nullptr) {
            objectX = engine->InvokeObjectGetX(objectHandle);
            objectY = engine->InvokeObjectGetY(objectHandle);
            objectWidth = engine->InvokeObjectGetWidth(objectHandle);
            objectHeight = engine->InvokeObjectGetHeight(objectHandle);
        }

        const float width =
                static_cast<float>(luaL_optnumber(state, 6, objectWidth));

        const float height =
                static_cast<float>(luaL_optnumber(state, 7, objectHeight));

        const float rotationDegrees =
                static_cast<float>(luaL_optnumber(state, 8, 0.0));

        const float r = static_cast<float>(luaL_optnumber(state, 9, 1.0));
        const float g = static_cast<float>(luaL_optnumber(state, 10, 1.0));
        const float b = static_cast<float>(luaL_optnumber(state, 11, 1.0));
        const float a = static_cast<float>(luaL_optnumber(state, 12, 1.0));

        const int layer = static_cast<int>(luaL_optinteger(state, 13, 0));

        if (engine != nullptr) {
            engine->InvokeSpriteDraw(
                id,
                spriteHandle,
                objectX + offsetX,
                objectY + offsetY,
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

        return 0;
    }

    static std::string ReadTextFile(const std::string &path) {
        const std::filesystem::path absolutePath =
                std::filesystem::absolute(path);

        Log::Info(std::format(
            "Reading Luau script from: {}",
            absolutePath.string()
        ));

        std::ifstream file(absolutePath, std::ios::in | std::ios::binary);

        if (!file) {
            Log::Error(std::format(
                "Failed to open Luau script at full path: {}",
                absolutePath.string()
            ));

            return {};
        }

        std::ostringstream stream;
        stream << file.rdbuf();
        return stream.str();
    }

    ScriptEngine::ScriptEngine() {
        m_State = luaL_newstate();

        if (m_State == nullptr) {
            Log::Error("Failed to create Luau state.");
            return;
        }

        luaL_openlibs(m_State);

        lua_pushlightuserdata(m_State, this);
        lua_setglobal(m_State, "__proton_script_engine");

        lua_pushcfunction(m_State, ProtonPrint, "print");
        lua_setglobal(m_State, "print");

        lua_newtable(m_State);

        lua_pushcfunction(m_State, ProtonSetClearColor, "setClearColor");
        lua_setfield(m_State, -2, "setClearColor");

        lua_pushcfunction(m_State, ProtonRenderImage, "renderImage");
        lua_setfield(m_State, -2, "renderImage");

        lua_pushcfunction(m_State, ProtonSetImageScaleMode, "setImageScaleMode");
        lua_setfield(m_State, -2, "setImageScaleMode");

        lua_pushcfunction(m_State, ProtonPlayAudio, "playAudio");
        lua_setfield(m_State, -2, "playAudio");

        lua_pushcfunction(m_State, ProtonGetAudioTime, "getAudioTime");
        lua_setfield(m_State, -2, "getAudioTime");

        lua_newtable(m_State);

        lua_pushcfunction(m_State, ProtonMediaPlayImageSequence, "playImageSequence");
        lua_setfield(m_State, -2, "playImageSequence");

        lua_pushcfunction(m_State, ProtonMediaStop, "stop");
        lua_setfield(m_State, -2, "stop");

        lua_setfield(m_State, -2, "media");

        lua_newtable(m_State);

        lua_pushcfunction(m_State, ProtonDrawRect, "rect");
        lua_setfield(m_State, -2, "rect");

        lua_pushcfunction(m_State, ProtonDrawCircle, "circle");
        lua_setfield(m_State, -2, "circle");

        lua_pushcfunction(m_State, ProtonDrawPolygon, "polygon");
        lua_setfield(m_State, -2, "polygon");

        lua_pushcfunction(m_State, ProtonDrawClear, "clear");
        lua_setfield(m_State, -2, "clear");

        lua_setfield(m_State, -2, "draw");

        lua_newtable(m_State);

        lua_pushcfunction(m_State, ProtonInputIsKeyDown, "isKeyDown");
        lua_setfield(m_State, -2, "isKeyDown");

        lua_pushcfunction(m_State, ProtonInputIsKeyPressed, "isKeyPressed");
        lua_setfield(m_State, -2, "isKeyPressed");

        lua_pushcfunction(m_State, ProtonInputIsKeyReleased, "isKeyReleased");
        lua_setfield(m_State, -2, "isKeyReleased");

        lua_pushcfunction(m_State, ProtonInputIsMouseButtonDown, "isMouseButtonDown");
        lua_setfield(m_State, -2, "isMouseButtonDown");

        lua_pushcfunction(m_State, ProtonInputIsMouseButtonPressed, "isMouseButtonPressed");
        lua_setfield(m_State, -2, "isMouseButtonPressed");

        lua_pushcfunction(m_State, ProtonInputIsMouseButtonReleased, "isMouseButtonReleased");
        lua_setfield(m_State, -2, "isMouseButtonReleased");

        lua_pushcfunction(m_State, ProtonInputGetMouseX, "getMouseX");
        lua_setfield(m_State, -2, "getMouseX");

        lua_pushcfunction(m_State, ProtonInputGetMouseY, "getMouseY");
        lua_setfield(m_State, -2, "getMouseY");

        lua_setfield(m_State, -2, "input");

        lua_newtable(m_State);

        lua_pushcfunction(m_State, ProtonWindowGetWidth, "getWidth");
        lua_setfield(m_State, -2, "getWidth");

        lua_pushcfunction(m_State, ProtonWindowGetHeight, "getHeight");
        lua_setfield(m_State, -2, "getHeight");

        lua_pushcfunction(m_State, ProtonWindowSetTitle, "setTitle");
        lua_setfield(m_State, -2, "setTitle");

        lua_pushcfunction(m_State, ProtonWindowSetDebugMode, "setDebugMode");
        lua_setfield(m_State, -2, "setDebugMode");

        lua_setfield(m_State, -2, "window");

        lua_newtable(m_State);

        lua_pushcfunction(m_State, ProtonCollisionRectsOverlap, "rectsOverlap");
        lua_setfield(m_State, -2, "rectsOverlap");

        lua_setfield(m_State, -2, "collision");

        lua_newtable(m_State);

        lua_pushcfunction(m_State, ProtonSpriteLoad, "load");
        lua_setfield(m_State, -2, "load");

        lua_pushcfunction(m_State, ProtonSpriteDraw, "draw");
        lua_setfield(m_State, -2, "draw");

        lua_pushcfunction(m_State, ProtonSpriteDrawOn, "drawOn");
        lua_setfield(m_State, -2, "drawOn");

        lua_pushcfunction(m_State, ProtonSpriteDrawObject, "drawObject");
        lua_setfield(m_State, -2, "drawObject");

        lua_pushcfunction(m_State, ProtonSpriteDrawOnObject, "drawOnObject");
        lua_setfield(m_State, -2, "drawOnObject");

        lua_setfield(m_State, -2, "sprite");

        lua_newtable(m_State);

        lua_pushcfunction(m_State, ProtonObjectCreate, "create");
        lua_setfield(m_State, -2, "create");

        lua_pushcfunction(m_State, ProtonObjectSetPosition, "setPosition");
        lua_setfield(m_State, -2, "setPosition");

        lua_pushcfunction(m_State, ProtonObjectSetSize, "setSize");
        lua_setfield(m_State, -2, "setSize");

        lua_pushcfunction(m_State, ProtonObjectGetX, "getX");
        lua_setfield(m_State, -2, "getX");

        lua_pushcfunction(m_State, ProtonObjectGetY, "getY");
        lua_setfield(m_State, -2, "getY");

        lua_pushcfunction(m_State, ProtonObjectGetWidth, "getWidth");
        lua_setfield(m_State, -2, "getWidth");

        lua_pushcfunction(m_State, ProtonObjectGetHeight, "getHeight");
        lua_setfield(m_State, -2, "getHeight");

        lua_setfield(m_State, -2, "object");

        lua_setglobal(m_State, "proton");

        Log::Info("Luau script engine initialized.");
    }

    ScriptEngine::~ScriptEngine() {
        if (m_State != nullptr) {
            lua_close(m_State);
            m_State = nullptr;
        }
    }

    void ScriptEngine::SetClearColorCallback(ClearColorCallback callback) {
        m_ClearColorCallback = std::move(callback);
    }

    void ScriptEngine::InvokeClearColor(float r, float g, float b, float a) {
        if (m_ClearColorCallback) {
            m_ClearColorCallback(r, g, b, a);
        }
    }

    void ScriptEngine::SetRenderImageCallback(RenderImageCallback callback) {
        m_RenderImageCallback = std::move(callback);
    }

    void ScriptEngine::InvokeRenderImage(const std::string &path) {
        if (m_RenderImageCallback) {
            m_RenderImageCallback(path);
        }
    }

    void ScriptEngine::SetImageScaleModeCallback(ImageScaleModeCallback callback) {
        m_ImageScaleModeCallback = std::move(callback);
    }

    void ScriptEngine::InvokeImageScaleMode(int scaleMode) {
        if (m_ImageScaleModeCallback) {
            m_ImageScaleModeCallback(scaleMode);
        }
    }

    void ScriptEngine::SetPlayAudioCallback(PlayAudioCallback callback) {
        m_PlayAudioCallback = std::move(callback);
    }

    void ScriptEngine::InvokePlayAudio(const std::string &path) {
        if (m_PlayAudioCallback) {
            m_PlayAudioCallback(path);
        }
    }

    void ScriptEngine::SetAudioTimeCallback(AudioTimeCallback callback) {
        m_AudioTimeCallback = std::move(callback);
    }

    double ScriptEngine::InvokeAudioTime() {
        if (m_AudioTimeCallback) {
            return m_AudioTimeCallback();
        }

        return 0.0;
    }

    void ScriptEngine::SetMediaPlayImageSequenceCallback(
        MediaPlayImageSequenceCallback callback
    ) {
        m_MediaPlayImageSequenceCallback = std::move(callback);
    }

    void ScriptEngine::SetMediaStopCallback(MediaStopCallback callback) {
        m_MediaStopCallback = std::move(callback);
    }

    void ScriptEngine::InvokeMediaPlayImageSequence(
        const std::string &framePattern,
        const std::string &audioPath,
        int firstFrame,
        int lastFrame,
        double fps
    ) {
        if (m_MediaPlayImageSequenceCallback) {
            m_MediaPlayImageSequenceCallback(
                framePattern,
                audioPath,
                firstFrame,
                lastFrame,
                fps
            );
        }
    }

    void ScriptEngine::InvokeMediaStop() {
        if (m_MediaStopCallback) {
            m_MediaStopCallback();
        }
    }

    void ScriptEngine::SetDrawRectCallback(DrawRectCallback callback) {
        m_DrawRectCallback = std::move(callback);
    }

    void ScriptEngine::SetDrawClearCallback(DrawClearCallback callback) {
        m_DrawClearCallback = std::move(callback);
    }

    void ScriptEngine::SetSpriteLoadCallback(SpriteLoadCallback callback) {
        m_SpriteLoadCallback = std::move(callback);
    }

    void ScriptEngine::SetSpriteDrawCallback(SpriteDrawCallback callback) {
        m_SpriteDrawCallback = std::move(callback);
    }

    void ScriptEngine::SetObjectCreateCallback(ObjectCreateCallback callback) {
        m_ObjectCreateCallback = std::move(callback);
    }

    void ScriptEngine::SetObjectSetPositionCallback(ObjectSetPositionCallback callback) {
        m_ObjectSetPositionCallback = std::move(callback);
    }

    void ScriptEngine::SetObjectSetSizeCallback(ObjectSetSizeCallback callback) {
        m_ObjectSetSizeCallback = std::move(callback);
    }

    void ScriptEngine::SetObjectGetXCallback(ObjectFloatQueryCallback callback) {
        m_ObjectGetXCallback = std::move(callback);
    }

    void ScriptEngine::SetObjectGetYCallback(ObjectFloatQueryCallback callback) {
        m_ObjectGetYCallback = std::move(callback);
    }

    void ScriptEngine::SetObjectGetWidthCallback(ObjectFloatQueryCallback callback) {
        m_ObjectGetWidthCallback = std::move(callback);
    }

    void ScriptEngine::SetObjectGetHeightCallback(ObjectFloatQueryCallback callback) {
        m_ObjectGetHeightCallback = std::move(callback);
    }

    std::uint32_t ScriptEngine::InvokeObjectCreate(
        const std::string &name,
        float x,
        float y,
        float width,
        float height
    ) {
        if (m_ObjectCreateCallback) {
            return m_ObjectCreateCallback(name, x, y, width, height);
        }

        return 0;
    }

    void ScriptEngine::InvokeObjectSetPosition(std::uint32_t handle, float x, float y) {
        if (m_ObjectSetPositionCallback) {
            m_ObjectSetPositionCallback(handle, x, y);
        }
    }

    void ScriptEngine::InvokeObjectSetSize(std::uint32_t handle, float width, float height) {
        if (m_ObjectSetSizeCallback) {
            m_ObjectSetSizeCallback(handle, width, height);
        }
    }

    float ScriptEngine::InvokeObjectGetX(std::uint32_t handle) {
        return m_ObjectGetXCallback ? m_ObjectGetXCallback(handle) : 0.0f;
    }

    float ScriptEngine::InvokeObjectGetY(std::uint32_t handle) {
        return m_ObjectGetYCallback ? m_ObjectGetYCallback(handle) : 0.0f;
    }

    float ScriptEngine::InvokeObjectGetWidth(std::uint32_t handle) {
        return m_ObjectGetWidthCallback ? m_ObjectGetWidthCallback(handle) : 0.0f;
    }

    float ScriptEngine::InvokeObjectGetHeight(std::uint32_t handle) {
        return m_ObjectGetHeightCallback ? m_ObjectGetHeightCallback(handle) : 0.0f;
    }

    void ScriptEngine::InvokeSpriteDraw(
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
        if (m_SpriteDrawCallback) {
            m_SpriteDrawCallback(
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
    }

    std::uint32_t ScriptEngine::InvokeSpriteLoad(const std::string &path) {
        if (m_SpriteLoadCallback) {
            return m_SpriteLoadCallback(path);
        }

        return 0;
    }

    void ScriptEngine::InvokeDrawRect(
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
        if (m_DrawRectCallback) {
            m_DrawRectCallback(
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
    }

    void ScriptEngine::InvokeDrawClear() {
        if (m_DrawClearCallback) {
            m_DrawClearCallback();
        }
    }

    void ScriptEngine::SetInputKeyDownCallback(InputKeyCallback callback) {
        m_InputKeyDownCallback = std::move(callback);
    }

    void ScriptEngine::SetInputKeyPressedCallback(InputKeyCallback callback) {
        m_InputKeyPressedCallback = std::move(callback);
    }

    void ScriptEngine::SetInputKeyReleasedCallback(InputKeyCallback callback) {
        m_InputKeyReleasedCallback = std::move(callback);
    }

    void ScriptEngine::SetInputMouseButtonDownCallback(InputMouseButtonCallback callback) {
        m_InputMouseButtonDownCallback = std::move(callback);
    }

    void ScriptEngine::SetInputMouseButtonPressedCallback(InputMouseButtonCallback callback) {
        m_InputMouseButtonPressedCallback = std::move(callback);
    }

    void ScriptEngine::SetInputMouseButtonReleasedCallback(InputMouseButtonCallback callback) {
        m_InputMouseButtonReleasedCallback = std::move(callback);
    }

    void ScriptEngine::SetInputMouseXCallback(InputMousePositionCallback callback) {
        m_InputMouseXCallback = std::move(callback);
    }

    void ScriptEngine::SetInputMouseYCallback(InputMousePositionCallback callback) {
        m_InputMouseYCallback = std::move(callback);
    }

    bool ScriptEngine::InvokeInputKeyDown(const std::string &keyName) {
        return m_InputKeyDownCallback ? m_InputKeyDownCallback(keyName) : false;
    }

    bool ScriptEngine::InvokeInputKeyPressed(const std::string &keyName) {
        return m_InputKeyPressedCallback ? m_InputKeyPressedCallback(keyName) : false;
    }

    bool ScriptEngine::InvokeInputKeyReleased(const std::string &keyName) {
        return m_InputKeyReleasedCallback ? m_InputKeyReleasedCallback(keyName) : false;
    }

    bool ScriptEngine::InvokeInputMouseButtonDown(const std::string &buttonName) {
        return m_InputMouseButtonDownCallback ? m_InputMouseButtonDownCallback(buttonName) : false;
    }

    bool ScriptEngine::InvokeInputMouseButtonPressed(const std::string &buttonName) {
        return m_InputMouseButtonPressedCallback ? m_InputMouseButtonPressedCallback(buttonName) : false;
    }

    bool ScriptEngine::InvokeInputMouseButtonReleased(const std::string &buttonName) {
        return m_InputMouseButtonReleasedCallback ? m_InputMouseButtonReleasedCallback(buttonName) : false;
    }

    double ScriptEngine::InvokeInputMouseX() {
        return m_InputMouseXCallback ? m_InputMouseXCallback() : 0.0;
    }

    double ScriptEngine::InvokeInputMouseY() {
        return m_InputMouseYCallback ? m_InputMouseYCallback() : 0.0;
    }

    void ScriptEngine::SetWindowWidthCallback(WindowDimensionCallback callback) {
        m_WindowWidthCallback = std::move(callback);
    }

    void ScriptEngine::SetWindowHeightCallback(WindowDimensionCallback callback) {
        m_WindowHeightCallback = std::move(callback);
    }

    double ScriptEngine::InvokeWindowWidth() {
        if (m_WindowWidthCallback) {
            return m_WindowWidthCallback();
        }

        return 0.0;
    }

    double ScriptEngine::InvokeWindowHeight() {
        if (m_WindowHeightCallback) {
            return m_WindowHeightCallback();
        }

        return 0.0;
    }

    void ScriptEngine::SetDrawCircleCallback(DrawCircleCallback callback) {
        m_DrawCircleCallback = std::move(callback);
    }

    void ScriptEngine::SetDrawPolygonCallback(DrawPolygonCallback callback) {
        m_DrawPolygonCallback = std::move(callback);
    }

    void ScriptEngine::InvokeDrawCircle(
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
        if (m_DrawCircleCallback) {
            m_DrawCircleCallback(
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
    }

    void ScriptEngine::InvokeDrawPolygon(
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
        if (m_DrawPolygonCallback) {
            m_DrawPolygonCallback(
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
    }

    void ScriptEngine::SetWindowTitleCallback(WindowTitleCallback callback) {
        m_WindowTitleCallback = std::move(callback);
    }

    void ScriptEngine::SetWindowDebugModeCallback(WindowDebugModeCallback callback) {
        m_WindowDebugModeCallback = std::move(callback);
    }

    void ScriptEngine::InvokeWindowTitle(const std::string &title) {
        if (m_WindowTitleCallback) {
            m_WindowTitleCallback(title);
        }
    }

    void ScriptEngine::InvokeWindowDebugMode(bool enabled) {
        if (m_WindowDebugModeCallback) {
            m_WindowDebugModeCallback(enabled);
        }
    }

    bool ScriptEngine::RunFile(const std::string &path) {
        if (m_State == nullptr) {
            Log::Error("Cannot run Luau script because Luau state is null.");
            return false;
        }

        const std::string source = ReadTextFile(path);

        if (source.empty()) {
            Log::Error(std::format("Failed to read Luau script: {}", path));
            return false;
        }

        const std::string bytecode = Luau::compile(source);

        const int loadResult = luau_load(
            m_State,
            path.c_str(),
            bytecode.data(),
            bytecode.size(),
            0
        );

        if (loadResult != 0) {
            const char *error = lua_tostring(m_State, -1);

            Log::Error(std::format(
                "Failed to load Luau script '{}': {}",
                path,
                error != nullptr ? error : "unknown error"
            ));

            lua_pop(m_State, 1);
            return false;
        }

        const int callResult = lua_pcall(m_State, 0, 0, 0);

        if (callResult != 0) {
            const char *error = lua_tostring(m_State, -1);

            Log::Error(std::format(
                "Failed to run Luau script '{}': {}",
                path,
                error != nullptr ? error : "unknown error"
            ));

            lua_pop(m_State, 1);
            return false;
        }

        lua_getglobal(m_State, "update");
        m_HasUpdateFunction = lua_isfunction(m_State, -1);
        lua_pop(m_State, 1);

        if (m_HasUpdateFunction) {
            Log::Info("Luau update(dt) function detected.");
        } else {
            Log::Info("No Luau update(dt) function found.");
        }

        Log::Info(std::format("Executed Luau script: {}", path));
        return true;
    }

    bool ScriptEngine::RunDirectory(const std::string &directoryPath) {
        const std::filesystem::path absoluteDirectory =
                std::filesystem::absolute(directoryPath);

        if (!std::filesystem::exists(absoluteDirectory)) {
            Log::Error(std::format(
                "Luau script directory does not exist: {}",
                absoluteDirectory.string()
            ));

            return false;
        }

        if (!std::filesystem::is_directory(absoluteDirectory)) {
            Log::Error(std::format(
                "Luau script path is not a directory: {}",
                absoluteDirectory.string()
            ));

            return false;
        }

        std::vector<std::filesystem::path> scripts;

        for (const auto &entry: std::filesystem::recursive_directory_iterator(absoluteDirectory)) {
            if (!entry.is_regular_file()) {
                continue;
            }

            const std::filesystem::path filePath = entry.path();

            if (filePath.extension() != ".luau") {
                continue;
            }

            /*
                Type definition files are for the editor/LSP only.
                The runtime should not execute them.
            */
            if (filePath.filename().string().ends_with(".d.luau")) {
                continue;
            }

            scripts.push_back(filePath);
        }

        std::sort(
            scripts.begin(),
            scripts.end(),
            [](const std::filesystem::path &a, const std::filesystem::path &b) {
                return a.string() < b.string();
            }
        );

        if (scripts.empty()) {
            Log::Error(std::format(
                "No Luau scripts found in directory: {}",
                absoluteDirectory.string()
            ));

            return false;
        }

        Log::Info(std::format(
            "Found {} Luau script(s) in: {}",
            scripts.size(),
            absoluteDirectory.string()
        ));

        bool allScriptsSucceeded = true;

        for (const std::filesystem::path &scriptPath: scripts) {
            const bool success = RunFile(scriptPath.string());

            if (!success) {
                allScriptsSucceeded = false;
            }
        }

        return allScriptsSucceeded;
    }

    void ScriptEngine::Update(double deltaTime) {
        if (m_State == nullptr || !m_HasUpdateFunction) {
            return;
        }

        lua_getglobal(m_State, "update");

        if (!lua_isfunction(m_State, -1)) {
            lua_pop(m_State, 1);
            m_HasUpdateFunction = false;
            return;
        }

        lua_pushnumber(m_State, deltaTime);

        const int callResult = lua_pcall(m_State, 1, 0, 0);

        if (callResult != 0) {
            const char *error = lua_tostring(m_State, -1);

            Log::Error(std::format(
                "Failed to run Luau update(dt): {}",
                error != nullptr ? error : "unknown error"
            ));

            lua_pop(m_State, 1);
            m_HasUpdateFunction = false;
        }
    }
}
