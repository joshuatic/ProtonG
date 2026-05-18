#include "ProtonSpriteManager.hpp"

#include "ProtonLog.hpp"

#include <format>

namespace Proton {
    void SpriteManager::BeginFrame() {
        m_DrawCommands.clear();
    }

    SpriteHandle SpriteManager::LoadSprite(const std::string &path) {
        const auto existingHandle = m_HandlesByPath.find(path);

        if (existingHandle != m_HandlesByPath.end()) {
            Log::Info(std::format(
                "Sprite already loaded | Handle={} | Path={}",
                existingHandle->second,
                path
            ));

            return existingHandle->second;
        }

        ImageData image = ImageLoader::Load(path);

        if (!image.IsValid()) {
            Log::Error(std::format(
                "Failed to load sprite image: {}",
                path
            ));

            return 0;
        }

        const SpriteHandle handle = m_NextHandle;
        m_NextHandle++;

        SpriteResource resource{};
        resource.Handle = handle;
        resource.Path = path;
        resource.Image = std::move(image);
        resource.Loaded = true;

        const int width = resource.Image.Width;
        const int height = resource.Image.Height;
        const std::size_t byteCount = resource.Image.Pixels.size();

        m_SpritesByHandle.emplace(handle, std::move(resource));
        m_HandlesByPath.emplace(path, handle);

        Log::Info(std::format(
            "Sprite image loaded | Handle={} | Path={} | Size={}x{} | Bytes={}",
            handle,
            path,
            width,
            height,
            byteCount
        ));

        return handle;
    }

    void SpriteManager::DrawSprite(
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
    ) {
        if (!HasSprite(handle)) {
            const std::string invalidId = "invalid:" + id;

            if (!m_LoggedDrawIds.contains(invalidId)) {
                m_LoggedDrawIds.insert(invalidId);

                Log::Error(std::format(
                    "Cannot draw sprite. Invalid handle={} | id=\"{}\"",
                    handle,
                    id
                ));
            }

            return;
        }

        SpriteDrawCommand command{};
        command.Id = id;
        command.Handle = handle;
        command.X = x;
        command.Y = y;
        command.Width = width;
        command.Height = height;
        command.RotationDegrees = rotationDegrees;
        command.R = r;
        command.G = g;
        command.B = b;
        command.A = a;
        command.Layer = layer;

        m_DrawCommands.push_back(command);

        if (!id.empty() && !m_LoggedDrawIds.contains(id)) {
            m_LoggedDrawIds.insert(id);

            Log::Info(std::format(
                "Sprite draw requested | id=\"{}\", handle={}, layer={}, x={:.1f}, y={:.1f}, w={:.1f}, h={:.1f}, rotation={:.1f}, color=({:.2f}, {:.2f}, {:.2f}, {:.2f})",
                id,
                handle,
                layer,
                x,
                y,
                width,
                height,
                rotationDegrees,
                r,
                g,
                b,
                a
            ));
        }
    }

    bool SpriteManager::HasSprite(SpriteHandle handle) const {
        return m_SpritesByHandle.contains(handle);
    }

    const SpriteResource *SpriteManager::GetSprite(SpriteHandle handle) const {
        const auto sprite = m_SpritesByHandle.find(handle);

        if (sprite == m_SpritesByHandle.end()) {
            return nullptr;
        }

        return &sprite->second;
    }

    const std::vector<SpriteDrawCommand> &SpriteManager::GetDrawCommands() const {
        return m_DrawCommands;
    }
}
