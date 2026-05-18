#version 450

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec2 inUv;
layout(location = 2) in vec4 inColor;

layout(location = 0) out vec2 fragUv;
layout(location = 1) out vec4 fragColor;

layout(push_constant) uniform Sprite2DViewportData
{
    vec2 viewportSize;
} viewport;

void main()
{
/*
        Proton 2D coordinates:
        - 0,0 is top-left
        - +Y goes down

        This matches your current Rect2D behavior.
    */
    vec2 normalizedPosition = inPosition / viewport.viewportSize;

    vec2 clipPosition;
    clipPosition.x = normalizedPosition.x * 2.0 - 1.0;
    clipPosition.y = normalizedPosition.y * 2.0 - 1.0;

    gl_Position = vec4(clipPosition, 0.0, 1.0);

    fragUv = inUv;
    fragColor = inColor;
}