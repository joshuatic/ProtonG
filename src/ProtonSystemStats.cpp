#include "ProtonSystemStats.hpp"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <Psapi.h>
#endif

#include <format>

namespace Proton {
#ifdef _WIN32
    static std::uint64_t FileTimeToUInt64(const FILETIME &fileTime) {
        ULARGE_INTEGER value{};
        value.LowPart = fileTime.dwLowDateTime;
        value.HighPart = fileTime.dwHighDateTime;
        return value.QuadPart;
    }

    static std::uint64_t GetWallTime100ns() {
        FILETIME fileTime{};
        GetSystemTimeAsFileTime(&fileTime);
        return FileTimeToUInt64(fileTime);
    }
#endif

    SystemStatsSampler::SystemStatsSampler() {
#ifdef _WIN32
        SYSTEM_INFO systemInfo{};
        GetSystemInfo(&systemInfo);

        m_LogicalProcessorCount =
                systemInfo.dwNumberOfProcessors == 0
                    ? 1
                    : systemInfo.dwNumberOfProcessors;

        FILETIME creationTime{};
        FILETIME exitTime{};
        FILETIME kernelTime{};
        FILETIME userTime{};

        GetProcessTimes(
            GetCurrentProcess(),
            &creationTime,
            &exitTime,
            &kernelTime,
            &userTime
        );

        m_LastProcessTime100ns =
                FileTimeToUInt64(kernelTime) + FileTimeToUInt64(userTime);

        m_LastWallTime100ns = GetWallTime100ns();
#endif
    }

    SystemStats SystemStatsSampler::Sample() {
        SystemStats stats{};

#ifdef _WIN32
        HANDLE process = GetCurrentProcess();

        PROCESS_MEMORY_COUNTERS_EX memoryCounters{};
        memoryCounters.cb = sizeof(memoryCounters);

        if (GetProcessMemoryInfo(
            process,
            reinterpret_cast<PROCESS_MEMORY_COUNTERS *>(&memoryCounters),
            sizeof(memoryCounters))) {
            stats.WorkingSetBytes =
                    static_cast<std::uint64_t>(memoryCounters.WorkingSetSize);

            stats.PrivateBytes =
                    static_cast<std::uint64_t>(memoryCounters.PrivateUsage);
        }

        MEMORYSTATUSEX memoryStatus{};
        memoryStatus.dwLength = sizeof(memoryStatus);

        if (GlobalMemoryStatusEx(&memoryStatus)) {
            stats.TotalPhysicalMemoryBytes = memoryStatus.ullTotalPhys;
            stats.AvailablePhysicalMemoryBytes = memoryStatus.ullAvailPhys;
        }

        FILETIME creationTime{};
        FILETIME exitTime{};
        FILETIME kernelTime{};
        FILETIME userTime{};

        if (GetProcessTimes(
            process,
            &creationTime,
            &exitTime,
            &kernelTime,
            &userTime)) {
            const std::uint64_t processTime100ns =
                    FileTimeToUInt64(kernelTime) + FileTimeToUInt64(userTime);

            const std::uint64_t wallTime100ns = GetWallTime100ns();

            const std::uint64_t processDelta =
                    processTime100ns - m_LastProcessTime100ns;

            const std::uint64_t wallDelta =
                    wallTime100ns - m_LastWallTime100ns;

            if (wallDelta > 0) {
                stats.ProcessCpuPercent =
                        (static_cast<double>(processDelta) /
                         static_cast<double>(wallDelta)) *
                        100.0 /
                        static_cast<double>(m_LogicalProcessorCount);
            }

            m_LastProcessTime100ns = processTime100ns;
            m_LastWallTime100ns = wallTime100ns;
        }

        DWORD handleCount = 0;
        GetProcessHandleCount(process, &handleCount);

        stats.ThreadCount = handleCount;
#endif

        return stats;
    }

    std::string SystemStatsSampler::FormatBytes(std::uint64_t bytes) {
        constexpr double KiB = 1024.0;
        constexpr double MiB = KiB * 1024.0;
        constexpr double GiB = MiB * 1024.0;

        const double value = static_cast<double>(bytes);

        if (value >= GiB) {
            return std::format("{:.2f} GiB", value / GiB);
        }

        if (value >= MiB) {
            return std::format("{:.2f} MiB", value / MiB);
        }

        if (value >= KiB) {
            return std::format("{:.2f} KiB", value / KiB);
        }

        return std::format("{} B", bytes);
    }
}