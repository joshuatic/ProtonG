#version 450

layout(location = 0) in vec2 fragUv;
layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform sampler2D currentImage;

layout(push_constant) uniform ImageScaleData
{
    vec2 imageSize;
    vec2 viewportSize;
    int scaleMode;
} scaleData;

/*
    scaleMode:
    0 = stretch
    1 = fit / letterbox
    2 = fill / crop
    3 = native / centered original image size
*/

vec2 applyAspectFit(vec2 uv)
{
    float imageAspect = scaleData.imageSize.x / scaleData.imageSize.y;
    float viewportAspect = scaleData.viewportSize.x / scaleData.viewportSize.y;

    vec2 scaledUv = uv;

    if (viewportAspect > imageAspect)
    {
        float scale = imageAspect / viewportAspect;
        scaledUv.x = (uv.x - 0.5) / scale + 0.5;
    }
    else
    {
        float scale = viewportAspect / imageAspect;
        scaledUv.y = (uv.y - 0.5) / scale + 0.5;
    }

    return scaledUv;
}

vec2 applyAspectFill(vec2 uv)
{
    float imageAspect = scaleData.imageSize.x / scaleData.imageSize.y;
    float viewportAspect = scaleData.viewportSize.x / scaleData.viewportSize.y;

    vec2 scaledUv = uv;

    if (viewportAspect > imageAspect)
    {
        float scale = viewportAspect / imageAspect;
        scaledUv.y = (uv.y - 0.5) * scale + 0.5;
    }
    else
    {
        float scale = imageAspect / viewportAspect;
        scaledUv.x = (uv.x - 0.5) * scale + 0.5;
    }

    return scaledUv;
}

vec2 applyNativeCentered(vec2 uv)
{
    vec2 normalizedImageSize = scaleData.imageSize / scaleData.viewportSize;

    vec2 scaledUv;
    scaledUv.x = (uv.x - 0.5) / normalizedImageSize.x + 0.5;
    scaledUv.y = (uv.y - 0.5) / normalizedImageSize.y + 0.5;

    return scaledUv;
}

void main()
{
    vec2 uv = fragUv;

    if (scaleData.scaleMode == 1)
    {
        uv = applyAspectFit(fragUv);
    }
    else if (scaleData.scaleMode == 2)
    {
        uv = applyAspectFill(fragUv);
    }
    else if (scaleData.scaleMode == 3)
    {
        uv = applyNativeCentered(fragUv);
    }

    if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0)
    {
        outColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }

    uv.y = 1.0 - uv.y;

    outColor = texture(currentImage, uv);
}