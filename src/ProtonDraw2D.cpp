#include "ProtonDraw2D.hpp"

#include "ProtonLog.hpp"

#include <algorithm>
#include <format>

namespace Proton {
    void Draw2D::BeginFrame() {
        m_RectCommands.clear();
        m_PolygonCommands.clear();
    }

    void Draw2D::DrawRect(
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
        RectCommand command{};
        command.Id = id;
        command.X = x;
        command.Y = y;
        command.Width = width;
        command.Height = height;
        command.R = r;
        command.G = g;
        command.B = b;
        command.A = a;
        command.Layer = layer;

        m_RectCommands.push_back(command);

        if (!id.empty() && !m_LoggedRectIds.contains(id)) {
            m_LoggedRectIds.insert(id);

            Log::Info(std::format(
                "Draw requested | Rect id=\"{}\", layer={}, x={:.1f}, y={:.1f}, w={:.1f}, h={:.1f}, color=({:.2f}, {:.2f}, {:.2f}, {:.2f})",
                id,
                layer,
                x,
                y,
                width,
                height,
                r,
                g,
                b,
                a
            ));
        }
    }

    void Draw2D::DrawCircle(
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
        DrawPolygon(
            id,
            centerX,
            centerY,
            radius,
            64,
            0.0f,
            r,
            g,
            b,
            a,
            layer
        );
    }

    void Draw2D::DrawPolygon(
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
        points = std::clamp(points, 3, 512);

        PolygonCommand command{};
        command.Id = id;
        command.CenterX = centerX;
        command.CenterY = centerY;
        command.Radius = radius;
        command.Points = points;
        command.RotationDegrees = rotationDegrees;
        command.R = r;
        command.G = g;
        command.B = b;
        command.A = a;
        command.Layer = layer;

        m_PolygonCommands.push_back(command);

        if (!id.empty() && !m_LoggedPolygonIds.contains(id)) {
            m_LoggedPolygonIds.insert(id);

            Log::Info(std::format(
                "Draw requested | Polygon id=\"{}\", layer={}, center=({:.1f}, {:.1f}), radius={:.1f}, points={}, rotation={:.1f}, color=({:.2f}, {:.2f}, {:.2f}, {:.2f})",
                id,
                layer,
                centerX,
                centerY,
                radius,
                points,
                rotationDegrees,
                r,
                g,
                b,
                a
            ));
        }
    }

    const std::vector<RectCommand> &Draw2D::GetRectCommands() const {
        return m_RectCommands;
    }

    const std::vector<PolygonCommand> &Draw2D::GetPolygonCommands() const {
        return m_PolygonCommands;
    }

    std::size_t Draw2D::GetRectCount() const {
        return m_RectCommands.size();
    }

    std::size_t Draw2D::GetPolygonCount() const {
        return m_PolygonCommands.size();
    }
}
