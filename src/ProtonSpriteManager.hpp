#pragma once

#include "ProtonImageLoader.hpp"

#include <cstdint>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace Proton {
    using SpriteHandle = std::uint32_t;

    struct SpriteResource {
        SpriteHandle Handle = 0;
        std::string Path;

        ImageData Image;
        bool Loaded = false;
    };

    struct SpriteDrawCommand {
        std::string Id;

        SpriteHandle Handle = 0;

        float X = 0.0f;
        float Y = 0.0f;
        float Width = 0.0f;
        float Height = 0.0f;

        float RotationDegrees = 0.0f;

        float R = 1.0f;
        float G = 1.0f;
        float B = 1.0f;
        float A = 1.0f;

        int Layer = 0;
    };

    class SpriteManager {
    public:
        SpriteManager() = default;

        void BeginFrame();

        SpriteHandle LoadSprite(const std::string &path);

        void DrawSprite(
            const std::string &id,
            SpriteHandle handle,
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

        bool HasSprite(SpriteHandle handle) const;

        const SpriteResource *GetSprite(SpriteHandle handle) const;

        const std::vector<SpriteDrawCommand> &GetDrawCommands() const;

    private:
        SpriteHandle m_NextHandle = 1;

        std::unordered_map<SpriteHandle, SpriteResource> m_SpritesByHandle;
        std::unordered_map<std::string, SpriteHandle> m_HandlesByPath;

        std::vector<SpriteDrawCommand> m_DrawCommands;
        std::unordered_set<std::string> m_LoggedDrawIds;
    };
}