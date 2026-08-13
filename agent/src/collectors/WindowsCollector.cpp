#ifdef _WIN32
#include "SystemCollector.hpp"
#include <windows.h>
#include <pdh.h>
#include <pdhmsg.h>
#include <psapi.h>
#include <iphlpapi.h>
#include <tcpmib.h>   // Added for MIB_TCPTABLE in MinGW
#include <udpmib.h>   // Added for MIB_UDPTABLE in MinGW
#include <cstdlib>    // Added for malloc() and free()

// Ignored by MinGW, but good practice for MSVC
#pragma comment(lib, "pdh.lib")
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "psapi.lib")

class WindowsCollector : public SystemCollector {
private:
    PDH_HQUERY pdhQuery;
    PDH_HCOUNTER cpuTotal, diskRead, diskWrite;
    
    // Track network bytes over time to calculate Kbps
    ULONGLONG lastNetTx = 0, lastNetRx = 0;
    ULONGLONG lastTimeMs = 0;

public:
    WindowsCollector() {
        // We added 'A' to the end of the function names to strictly force standard ANSI strings
        // We also use 0 instead of NULL for the DWORD_PTR argument to satisfy the compiler
        PdhOpenQueryA(NULL, 0, &pdhQuery);
        
        // Map to your CPU and Disk metrics
        PdhAddEnglishCounterA(pdhQuery, "\\Processor(_Total)\\% Processor Time", 0, &cpuTotal);
        PdhAddEnglishCounterA(pdhQuery, "\\PhysicalDisk(_Total)\\Disk Read Bytes/sec", 0, &diskRead);
        PdhAddEnglishCounterA(pdhQuery, "\\PhysicalDisk(_Total)\\Disk Write Bytes/sec", 0, &diskWrite);
        
        PdhCollectQueryData(pdhQuery);
        lastTimeMs = GetTickCount64();
    }

    ~WindowsCollector() {
        PdhCloseQuery(pdhQuery);
    }

    void collectSystemMetrics(com::lagrange::telemetry::SystemMetrics* metrics) override {
        PdhCollectQueryData(pdhQuery);
        PDH_FMT_COUNTERVALUE counterVal;

        // 1. CPU Usage
        if (PdhGetFormattedCounterValue(cpuTotal, PDH_FMT_DOUBLE, NULL, &counterVal) == ERROR_SUCCESS)
            metrics->set_cpu_usage_percent(counterVal.doubleValue);

        // 2. RAM Total & Used
        MEMORYSTATUSEX memInfo;
        memInfo.dwLength = sizeof(MEMORYSTATUSEX);
        if (GlobalMemoryStatusEx(&memInfo)) {
            double totalRam = memInfo.ullTotalPhys / (1024.0 * 1024.0);
            double freeRam = memInfo.ullAvailPhys / (1024.0 * 1024.0);
            metrics->set_ram_total_mb(totalRam);
            metrics->set_ram_used_mb(totalRam - freeRam);
        }

        // 3. Disk I/O (Read/Write Kbps)
        if (PdhGetFormattedCounterValue(diskRead, PDH_FMT_DOUBLE, NULL, &counterVal) == ERROR_SUCCESS)
            metrics->set_disk_read_kbps(counterVal.doubleValue / 1024.0);
            
        if (PdhGetFormattedCounterValue(diskWrite, PDH_FMT_DOUBLE, NULL, &counterVal) == ERROR_SUCCESS)
            metrics->set_disk_write_kbps(counterVal.doubleValue / 1024.0);

        // 4. Active & Zombie Processes
        DWORD aProcesses[1024], cbNeeded;
        if (EnumProcesses(aProcesses, sizeof(aProcesses), &cbNeeded)) {
            metrics->set_active_processes(cbNeeded / sizeof(DWORD));
        }
        metrics->set_zombie_processes(0); // Windows does not have Linux-style zombies
    }

    void collectNetworkStats(com::lagrange::telemetry::NetworkStats* stats) override {
        // 1. Active TCP Connections
        DWORD dwSize = 0;
        GetTcpTable(NULL, &dwSize, FALSE);
        MIB_TCPTABLE* tcpTable = (MIB_TCPTABLE*)malloc(dwSize);
        if (GetTcpTable(tcpTable, &dwSize, FALSE) == NO_ERROR) {
            stats->set_active_tcp_connections(tcpTable->dwNumEntries);
        }
        free(tcpTable);

        // 2. Active UDP Connections
        dwSize = 0;
        GetUdpTable(NULL, &dwSize, FALSE);
        MIB_UDPTABLE* udpTable = (MIB_UDPTABLE*)malloc(dwSize);
        if (GetUdpTable(udpTable, &dwSize, FALSE) == NO_ERROR) {
            stats->set_active_udp_connections(udpTable->dwNumEntries);
        }
        free(udpTable);

        // 3. Network TX/RX Speed (Kbps)
        dwSize = 0;
        GetIfTable(NULL, &dwSize, FALSE);
        MIB_IFTABLE* ifTable = (MIB_IFTABLE*)malloc(dwSize);
        
        ULONGLONG currentTx = 0, currentRx = 0;
        if (GetIfTable(ifTable, &dwSize, FALSE) == NO_ERROR) {
            for (DWORD i = 0; i < ifTable->dwNumEntries; i++) {
                currentRx += ifTable->table[i].dwInOctets;
                currentTx += ifTable->table[i].dwOutOctets;
            }
        }
        free(ifTable);

        ULONGLONG currentTime = GetTickCount64();
        double timeDiffSec = (currentTime - lastTimeMs) / 1000.0;
        if (timeDiffSec > 0 && lastTimeMs > 0) {
            stats->set_network_rx_kbps(((currentRx - lastNetRx) / 1024.0) / timeDiffSec);
            stats->set_network_tx_kbps(((currentTx - lastNetTx) / 1024.0) / timeDiffSec);
        }
        lastNetRx = currentRx;
        lastNetTx = currentTx;
        lastTimeMs = currentTime;
    }
};
SystemCollector* SystemCollector::create() {
    return new WindowsCollector();
}
#endif