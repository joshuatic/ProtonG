#include "ProtonLog.hpp"

#include <chrono>
#include <format>
#include <iostream>

namespace Proton
{
    static std::string GetCurrentTimeString()
    {
        const auto now = std::chrono::system_clock::now();
        const auto rawTime = std::chrono::system_clock::to_time_t(now);

        std::tm localTime{};

#ifdef _WIN32
        localtime_s(&localTime, &rawTime);
#else
        localtime_r(&rawTime, &localTime);
#endif

        return std::format(
            "{:02}:{:02}:{:02}",
            localTime.tm_hour,
            localTime.tm_min,
            localTime.tm_sec
        );
    }

    void Log::Info(std::string_view message)
    {
        Write("INFO", message);
    }

    void Log::Warn(std::string_view message)
    {
        Write("WARN", message);
    }

    void Log::Error(std::string_view message)
    {
        Write("ERROR", message);
    }

    void Log::Write(std::string_view level, std::string_view message)
    {
        std::cout
            << "["
            << GetCurrentTimeString()
            << "] ["
            << level
            << "] "
            << message
            << '\n';
    }
}