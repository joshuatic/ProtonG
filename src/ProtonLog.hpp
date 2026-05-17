#pragma once

#include <string_view>

namespace Proton
{
    class Log
    {
    public:
        static void Info(std::string_view message);
        static void Warn(std::string_view message);
        static void Error(std::string_view message);

    private:
        static void Write(std::string_view level, std::string_view message);
    };
}