#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <memory>
#include "telemetry.pb.h" 
#include "collectors/SystemCollector.hpp" // dynamically links OS-specific code
using namespace com::lagrange::telemetry;
// Utility function to get current timestamp in milliseconds
int64_t getCurrentTimestampMs() {
    auto now = std::chrono::system_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()).count();
}
int main() {
    // Verify that the version of the library that we linked against is
    // compatible with the version of the headers we compiled against.
    GOOGLE_PROTOBUF_VERIFY_VERSION;
    std::cout << "[LagRange] Initializing dynamic telemetry agent..." << std::endl;
    std::cout << "[LagRange] Press Ctrl+C to stop." << std::endl;
    // THE FACTORY PATTERN: 
    // This dynamically instantiates WindowsCollector or LinuxCollector based on the OS.
    std::unique_ptr<SystemCollector> collector(SystemCollector::create());
    // The True Daemon Loop
    while (true) {
        int64_t now = getCurrentTimestampMs();
        // 1. POPULATE & SERIALIZE SYSTEM METRICS
        AgentMessage sysPayload;
        sysPayload.set_agent_id("lagrange-node-prod-01");
        sysPayload.set_api_key("sk_live_vanguard_88f92a");
        sysPayload.set_timestamp_ms(now);

        // Allocate on heap. Protobuf takes ownership and deletes it automatically.
        SystemMetrics* sysMetrics = new SystemMetrics();
        
        // DYNAMIC FETCH: Query the Windows Kernel / Linux ProcFS
        collector->collectSystemMetrics(sysMetrics);
        
        // Attach payload (transfers memory ownership)
        sysPayload.set_allocated_system_metrics(sysMetrics);

        // Serialize System Metrics
        std::string binarySystemData;
        if (sysPayload.SerializeToString(&binarySystemData)) {
            std::cout << "\n--- SYSTEM METRICS DISPATCHED (" << binarySystemData.length() << " bytes) ---" << std::endl;
            std::cout << "CPU Usage   : " << sysPayload.system_metrics().cpu_usage_percent() << " %" << std::endl;
            std::cout << "RAM Used    : " << sysPayload.system_metrics().ram_used_mb() << " / " << sysPayload.system_metrics().ram_total_mb() << " MB" << std::endl;
            std::cout << "Disk Read   : " << sysPayload.system_metrics().disk_read_kbps() << " KB/s" << std::endl;
            std::cout << "Disk Write  : " << sysPayload.system_metrics().disk_write_kbps() << " KB/s" << std::endl;
            std::cout << "Active Procs: " << sysPayload.system_metrics().active_processes() << std::endl;
            std::cout << "Zombie Procs: " << sysPayload.system_metrics().zombie_processes() << std::endl;
        }

        // 2. POPULATE & SERIALIZE NETWORK STATS
        AgentMessage netPayload;
        netPayload.set_agent_id("lagrange-node-prod-01");
        netPayload.set_api_key("sk_live_vanguard_88f92a");
        netPayload.set_timestamp_ms(now);

        NetworkStats* netStats = new NetworkStats();
        
        // DYNAMIC FETCH: Query OS Routing Tables
        collector->collectNetworkStats(netStats);
        netPayload.set_allocated_network_stats(netStats);

        // Serialize Network Stats
        std::string binaryNetworkData;
        if (netPayload.SerializeToString(&binaryNetworkData)) {
            std::cout << "--- NETWORK STATS DISPATCHED (" << binaryNetworkData.length() << " bytes) ---" << std::endl;
            std::cout << "TCP Conns   : " << netPayload.network_stats().active_tcp_connections() << std::endl;
            std::cout << "UDP Conns   : " << netPayload.network_stats().active_udp_connections() << std::endl;
            std::cout << "Network TX  : " << netPayload.network_stats().network_tx_kbps() << " KB/s" << std::endl;
            std::cout << "Network RX  : " << netPayload.network_stats().network_rx_kbps() << " KB/s" << std::endl;
        }
        // Wait 2 seconds before the next tick
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }

    // Clean up Protobuf memory allocations (Unreachable due to infinite loop, but required for C++ standards)
    google::protobuf::ShutdownProtobufLibrary();
    return 0;
}