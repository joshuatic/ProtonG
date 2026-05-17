#pragma once

#include <cstdint>
#include <string>

namespace Proton
{
    struct SystemStats
    {
        double ProcessCpuPercent = 0.0;

        std::uint64_t WorkingSetBytes = 0;
        std::uint64_t PrivateBytes = 0;

        std::uint64_t TotalPhysicalMemoryBytes = 0;
        std::uint64_t AvailablePhysicalMemoryBytes = 0;

        std::uint32_t ThreadCount = 0;
    };

    class SystemStatsSampler
    {
    public:
        SystemStatsSampler();

        SystemStats Sample();
        static std::string FormatBytes(std::uint64_t bytes);

    private:
        std::uint64_t m_LastProcessTime100ns = 0;
        std::uint64_t m_LastWallTime100ns = 0;
        std::uint32_t m_LogicalProcessorCount = 1;
    };
}