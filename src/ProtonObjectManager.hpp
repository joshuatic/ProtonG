#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>

namespace Proton {
    using ObjectHandle = std::uint32_t;

    struct RuntimeObject {
        ObjectHandle Handle = 0;
        std::string Name;

        float X = 0.0f;
        float Y = 0.0f;
        float Width = 0.0f;
        float Height = 0.0f;
    };

    class ObjectManager {
    public:
        ObjectManager() = default;

        ObjectHandle CreateObject(
            const std::string &name,
            float x,
            float y,
            float width,
            float height
        );

        bool HasObject(ObjectHandle handle) const;

        RuntimeObject *GetObject(ObjectHandle handle);

        const RuntimeObject *GetObject(ObjectHandle handle) const;

        void SetPosition(ObjectHandle handle, float x, float y);

        void SetSize(ObjectHandle handle, float width, float height);

        float GetX(ObjectHandle handle) const;

        float GetY(ObjectHandle handle) const;

        float GetWidth(ObjectHandle handle) const;

        float GetHeight(ObjectHandle handle) const;

    private:
        ObjectHandle m_NextHandle = 1;
        std::unordered_map<ObjectHandle, RuntimeObject> m_Objects;
    };
}
