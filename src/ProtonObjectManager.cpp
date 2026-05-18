#include "ProtonObjectManager.hpp"

#include "ProtonLog.hpp"

#include <format>

namespace Proton {
    ObjectHandle ObjectManager::CreateObject(
        const std::string &name,
        float x,
        float y,
        float width,
        float height
    ) {
        const ObjectHandle handle = m_NextHandle;
        m_NextHandle++;

        RuntimeObject object{};
        object.Handle = handle;
        object.Name = name;
        object.X = x;
        object.Y = y;
        object.Width = width;
        object.Height = height;

        m_Objects.emplace(handle, object);

        Log::Info(std::format(
            "Runtime object created | Handle={} | Name=\"{}\" | x={:.1f}, y={:.1f}, w={:.1f}, h={:.1f}",
            handle,
            name,
            x,
            y,
            width,
            height
        ));

        return handle;
    }

    bool ObjectManager::HasObject(ObjectHandle handle) const {
        return m_Objects.contains(handle);
    }

    RuntimeObject *ObjectManager::GetObject(ObjectHandle handle) {
        const auto object = m_Objects.find(handle);

        if (object == m_Objects.end()) {
            return nullptr;
        }

        return &object->second;
    }

    const RuntimeObject *ObjectManager::GetObject(ObjectHandle handle) const {
        const auto object = m_Objects.find(handle);

        if (object == m_Objects.end()) {
            return nullptr;
        }

        return &object->second;
    }

    void ObjectManager::SetPosition(ObjectHandle handle, float x, float y) {
        RuntimeObject *object = GetObject(handle);

        if (object == nullptr) {
            Log::Error(std::format(
                "Cannot set object position. Invalid object handle={}",
                handle
            ));

            return;
        }

        object->X = x;
        object->Y = y;
    }

    void ObjectManager::SetSize(ObjectHandle handle, float width, float height) {
        RuntimeObject *object = GetObject(handle);

        if (object == nullptr) {
            Log::Error(std::format(
                "Cannot set object size. Invalid object handle={}",
                handle
            ));

            return;
        }

        object->Width = width;
        object->Height = height;
    }

    float ObjectManager::GetX(ObjectHandle handle) const {
        const RuntimeObject *object = GetObject(handle);
        return object != nullptr ? object->X : 0.0f;
    }

    float ObjectManager::GetY(ObjectHandle handle) const {
        const RuntimeObject *object = GetObject(handle);
        return object != nullptr ? object->Y : 0.0f;
    }

    float ObjectManager::GetWidth(ObjectHandle handle) const {
        const RuntimeObject *object = GetObject(handle);
        return object != nullptr ? object->Width : 0.0f;
    }

    float ObjectManager::GetHeight(ObjectHandle handle) const {
        const RuntimeObject *object = GetObject(handle);
        return object != nullptr ? object->Height : 0.0f;
    }
}
