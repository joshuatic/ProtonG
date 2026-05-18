#pragma once

#include <cstddef>
#include <string>
#include <unordered_set>
#include <vector>

namespace Proton {
    struct RectCommand {
        std::string Id;

        float X = 0.0f;
        float Y = 0.0f;
        float Width = 0.0f;
        float Height = 0.0f;

        float R = 1.0f;
        float G = 1.0f;
        float B = 1.0f;
        float A = 1.0f;

        int Layer = 0;
    };

    struct PolygonCommand {
        std::string Id;

        float CenterX = 0.0f;
        float CenterY = 0.0f;
        float Radius = 0.0f;

        int Points = 3;
        float RotationDegrees = 0.0f;

        float R = 1.0f;
        float G = 1.0f;
        float B = 1.0f;
        float A = 1.0f;

        int Layer = 0;
    };

    class Draw2D {
    public:
        Draw2D() = default;

        void BeginFrame();

        void DrawRect(
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
        );

        void DrawCircle(
            const std::string &id,
            float centerX,
            float centerY,
            float radius,
            float r,
            float g,
            float b,
            float a,
            int layer
        );

        void DrawPolygon(
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
        );

        const std::vector<RectCommand> &GetRectCommands() const;

        const std::vector<PolygonCommand> &GetPolygonCommands() const;

        std::size_t GetRectCount() const;

        std::size_t GetPolygonCount() const;

    private:
        std::vector<RectCommand> m_RectCommands;
        std::vector<PolygonCommand> m_PolygonCommands;

        std::unordered_set<std::string> m_LoggedRectIds;
        std::unordered_set<std::string> m_LoggedPolygonIds;
    };
}
