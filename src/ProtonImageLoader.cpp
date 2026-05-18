#include "ProtonImageLoader.hpp"

#include "ProtonLog.hpp"

#include <filesystem>
#include <format>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace Proton {
    ImageData ImageLoader::Load(const std::string &path) {
        const std::filesystem::path absolutePath =
                std::filesystem::absolute(path);

        Log::Info(std::format(
            "Loading image from: {}",
            absolutePath.string()
        ));

        ImageData image;

        int width = 0;
        int height = 0;
        int channels = 0;

        constexpr int desiredChannels = 4;

        stbi_set_flip_vertically_on_load(true);

        stbi_uc *pixels = stbi_load(
            absolutePath.string().c_str(),
            &width,
            &height,
            &channels,
            desiredChannels
        );

        if (pixels == nullptr) {
            Log::Error(std::format(
                "Failed to load image: {}",
                absolutePath.string()
            ));

            return image;
        }

        const std::size_t pixelCount =
                static_cast<std::size_t>(width) *
                static_cast<std::size_t>(height) *
                static_cast<std::size_t>(desiredChannels);

        image.Width = width;
        image.Height = height;
        image.Channels = channels;
        image.DesiredChannels = desiredChannels;
        image.Pixels.assign(pixels, pixels + pixelCount);

        stbi_image_free(pixels);

        Log::Info(std::format(
            "Image loaded. Width: {}, Height: {}, Source Channels: {}, Loaded Channels: {}, Size: {} bytes",
            image.Width,
            image.Height,
            image.Channels,
            image.DesiredChannels,
            image.Pixels.size()
        ));

        return image;
    }
}