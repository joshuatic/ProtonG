#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace Proton
{
    struct ImageData
    {
        int Width = 0;
        int Height = 0;
        int Channels = 0;
        int DesiredChannels = 4;

        std::vector<std::uint8_t> Pixels;

        bool IsValid() const
        {
            return Width > 0 &&
                   Height > 0 &&
                   !Pixels.empty();
        }
    };

    class ImageLoader
    {
    public:
        static ImageData Load(const std::string& path);
    };
}