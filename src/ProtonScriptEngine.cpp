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

namespace Proton
{
    static ScriptEngine* GetScriptEngine(lua_State* state)
    {
        lua_getglobal(state, "__proton_script_engine");

        ScriptEngine* engine =
            static_cast<ScriptEngine*>(lua_touserdata(state, -1));

        lua_pop(state, 1);
        return engine;
    }

    static int ProtonPrint(lua_State* state)
    {
        const int argCount = lua_gettop(state);

        std::string output;

        for (int i = 1; i <= argCount; i++)
        {
            size_t length = 0;
            const char* text = lua_tolstring(state, i, &length);

            if (text != nullptr)
            {
                output.append(text, length);
            }
            else
            {
                output += luaL_typename(state, i);
            }

            if (i < argCount)
            {
                output += " ";
            }
        }

        Log::Info(std::format("[Luau] {}", output));
        return 0;
    }

    static int ProtonSetClearColor(lua_State* state)
    {
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

        ScriptEngine* engine = GetScriptEngine(state);

        if (engine != nullptr)
        {
            engine->InvokeClearColor(r, g, b, a);
        }

        return 0;
    }

    static int ProtonRenderImage(lua_State* state)
    {
        const char* path = luaL_checkstring(state, 1);

        Log::Info(std::format(
            "[Luau] proton.renderImage({})",
            path
        ));

        ScriptEngine* engine = GetScriptEngine(state);

        if (engine != nullptr)
        {
            engine->InvokeRenderImage(path);
        }

        return 0;
    }

    static int ProtonSetImageScaleMode(lua_State* state)
    {
        const char* modeText = luaL_checkstring(state, 1);

        int scaleMode = 1;

        if (std::string(modeText) == "stretch")
        {
            scaleMode = 0;
        }
        else if (std::string(modeText) == "fit")
        {
            scaleMode = 1;
        }
        else if (std::string(modeText) == "fill")
        {
            scaleMode = 2;
        }
        else if (std::string(modeText) == "native")
        {
            scaleMode = 3;
        }
        else
        {
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

        ScriptEngine* engine = GetScriptEngine(state);

        if (engine != nullptr)
        {
            engine->InvokeImageScaleMode(scaleMode);
        }

        return 0;
    }

    static int ProtonPlayAudio(lua_State* state)
    {
        const char* path = luaL_checkstring(state, 1);

        Log::Info(std::format(
            "[Luau] proton.playAudio({})",
            path
        ));

        ScriptEngine* engine = GetScriptEngine(state);

        if (engine != nullptr)
        {
            engine->InvokePlayAudio(path);
        }

        return 0;
    }

    static int ProtonGetAudioTime(lua_State* state)
    {
        ScriptEngine* engine = GetScriptEngine(state);

        double time = 0.0;

        if (engine != nullptr)
        {
            time = engine->InvokeAudioTime();
        }

        lua_pushnumber(state, time);
        return 1;
    }

    static int ProtonMediaPlayImageSequence(lua_State* state)
    {
        const char* framePattern = luaL_checkstring(state, 1);
        const char* audioPath = luaL_checkstring(state, 2);

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

        ScriptEngine* engine = GetScriptEngine(state);

        if (engine != nullptr)
        {
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

    static int ProtonMediaStop(lua_State* state)
    {
        Log::Info("[Luau] proton.media.stop()");

        ScriptEngine* engine = GetScriptEngine(state);

        if (engine != nullptr)
        {
            engine->InvokeMediaStop();
        }

        return 0;
    }

    static std::string ReadTextFile(const std::string& path)
    {
        const std::filesystem::path absolutePath =
            std::filesystem::absolute(path);

        Log::Info(std::format(
            "Reading Luau script from: {}",
            absolutePath.string()
        ));

        std::ifstream file(absolutePath, std::ios::in | std::ios::binary);

        if (!file)
        {
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

    ScriptEngine::ScriptEngine()
    {
        m_State = luaL_newstate();

        if (m_State == nullptr)
        {
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

        lua_setglobal(m_State, "proton");

        Log::Info("Luau script engine initialized.");
    }

    ScriptEngine::~ScriptEngine()
    {
        if (m_State != nullptr)
        {
            lua_close(m_State);
            m_State = nullptr;
        }
    }

    void ScriptEngine::SetClearColorCallback(ClearColorCallback callback)
    {
        m_ClearColorCallback = std::move(callback);
    }

    void ScriptEngine::InvokeClearColor(float r, float g, float b, float a)
    {
        if (m_ClearColorCallback)
        {
            m_ClearColorCallback(r, g, b, a);
        }
    }

    void ScriptEngine::SetRenderImageCallback(RenderImageCallback callback)
    {
        m_RenderImageCallback = std::move(callback);
    }

    void ScriptEngine::InvokeRenderImage(const std::string& path)
    {
        if (m_RenderImageCallback)
        {
            m_RenderImageCallback(path);
        }
    }

    void ScriptEngine::SetImageScaleModeCallback(ImageScaleModeCallback callback)
    {
        m_ImageScaleModeCallback = std::move(callback);
    }

    void ScriptEngine::InvokeImageScaleMode(int scaleMode)
    {
        if (m_ImageScaleModeCallback)
        {
            m_ImageScaleModeCallback(scaleMode);
        }
    }

    void ScriptEngine::SetPlayAudioCallback(PlayAudioCallback callback)
    {
        m_PlayAudioCallback = std::move(callback);
    }

    void ScriptEngine::InvokePlayAudio(const std::string& path)
    {
        if (m_PlayAudioCallback)
        {
            m_PlayAudioCallback(path);
        }
    }

    void ScriptEngine::SetAudioTimeCallback(AudioTimeCallback callback)
    {
        m_AudioTimeCallback = std::move(callback);
    }

    double ScriptEngine::InvokeAudioTime()
    {
        if (m_AudioTimeCallback)
        {
            return m_AudioTimeCallback();
        }

        return 0.0;
    }

    void ScriptEngine::SetMediaPlayImageSequenceCallback(
    MediaPlayImageSequenceCallback callback
)
    {
        m_MediaPlayImageSequenceCallback = std::move(callback);
    }

    void ScriptEngine::SetMediaStopCallback(MediaStopCallback callback)
    {
        m_MediaStopCallback = std::move(callback);
    }

    void ScriptEngine::InvokeMediaPlayImageSequence(
        const std::string& framePattern,
        const std::string& audioPath,
        int firstFrame,
        int lastFrame,
        double fps
    )
    {
        if (m_MediaPlayImageSequenceCallback)
        {
            m_MediaPlayImageSequenceCallback(
                framePattern,
                audioPath,
                firstFrame,
                lastFrame,
                fps
            );
        }
    }

    void ScriptEngine::InvokeMediaStop()
    {
        if (m_MediaStopCallback)
        {
            m_MediaStopCallback();
        }
    }

    bool ScriptEngine::RunFile(const std::string& path)
    {
        if (m_State == nullptr)
        {
            Log::Error("Cannot run Luau script because Luau state is null.");
            return false;
        }

        const std::string source = ReadTextFile(path);

        if (source.empty())
        {
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

        if (loadResult != 0)
        {
            const char* error = lua_tostring(m_State, -1);

            Log::Error(std::format(
                "Failed to load Luau script '{}': {}",
                path,
                error != nullptr ? error : "unknown error"
            ));

            lua_pop(m_State, 1);
            return false;
        }

        const int callResult = lua_pcall(m_State, 0, 0, 0);

        if (callResult != 0)
        {
            const char* error = lua_tostring(m_State, -1);

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

        if (m_HasUpdateFunction)
        {
            Log::Info("Luau update(dt) function detected.");
        }
        else
        {
            Log::Info("No Luau update(dt) function found.");
        }

        Log::Info(std::format("Executed Luau script: {}", path));
        return true;
    }

    bool ScriptEngine::RunDirectory(const std::string& directoryPath)
{
    const std::filesystem::path absoluteDirectory =
        std::filesystem::absolute(directoryPath);

    if (!std::filesystem::exists(absoluteDirectory))
    {
        Log::Error(std::format(
            "Luau script directory does not exist: {}",
            absoluteDirectory.string()
        ));

        return false;
    }

    if (!std::filesystem::is_directory(absoluteDirectory))
    {
        Log::Error(std::format(
            "Luau script path is not a directory: {}",
            absoluteDirectory.string()
        ));

        return false;
    }

    std::vector<std::filesystem::path> scripts;

    for (const auto& entry : std::filesystem::recursive_directory_iterator(absoluteDirectory))
    {
        if (!entry.is_regular_file())
        {
            continue;
        }

        const std::filesystem::path filePath = entry.path();

        if (filePath.extension() != ".luau")
        {
            continue;
        }

        /*
            Type definition files are for the editor/LSP only.
            The runtime should not execute them.
        */
        if (filePath.filename().string().ends_with(".d.luau"))
        {
            continue;
        }

        scripts.push_back(filePath);
    }

    std::sort(
        scripts.begin(),
        scripts.end(),
        [](const std::filesystem::path& a, const std::filesystem::path& b)
        {
            return a.string() < b.string();
        }
    );

    if (scripts.empty())
    {
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

    for (const std::filesystem::path& scriptPath : scripts)
    {
        const bool success = RunFile(scriptPath.string());

        if (!success)
        {
            allScriptsSucceeded = false;
        }
    }

    return allScriptsSucceeded;
}

    void ScriptEngine::Update(double deltaTime)
    {
        if (m_State == nullptr || !m_HasUpdateFunction)
        {
            return;
        }

        lua_getglobal(m_State, "update");

        if (!lua_isfunction(m_State, -1))
        {
            lua_pop(m_State, 1);
            m_HasUpdateFunction = false;
            return;
        }

        lua_pushnumber(m_State, deltaTime);

        const int callResult = lua_pcall(m_State, 1, 0, 0);

        if (callResult != 0)
        {
            const char* error = lua_tostring(m_State, -1);

            Log::Error(std::format(
                "Failed to run Luau update(dt): {}",
                error != nullptr ? error : "unknown error"
            ));

            lua_pop(m_State, 1);
            m_HasUpdateFunction = false;
        }
    }
}