#version 450

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec4 inColor;

layout(location = 0) out vec4 fragColor;

layout(push_constant) uniform Rect2DViewportData
{
    vec2 viewportSize;
} viewport;

void main()
{
    vec2 normalizedPosition = inPosition / viewport.viewportSize;

    vec2 clipPosition;
    clipPosition.x = normalizedPosition.x * 2.0 - 1.0;
    clipPosition.y = normalizedPosition.y * 2.0 - 1.0;

    gl_Position = vec4(clipPosition, 0.0, 1.0);

    fragColor = inColor;
}