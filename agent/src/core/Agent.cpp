#include "lagrange/core/Agent.hpp"
#include "telemetry.pb.h"
#include <iostream>
#include <thread>
#include <chrono>

namespace lagrange {
namespace core {

Agent::Agent() : config(), networkClient(config), isRunning(false) {
    // This dynamically creates either WindowsCollector or LinuxCollector
    // based on the OS
    collector.reset(SystemCollector::create());
}

void Agent::start() {
    isRunning = true;
    std::cout << "[Agent] Starting LagRange Agent (" << config.getAgentId() << ")...\n";
    std::cout << "[Agent] Target Backend: http://" << config.getBackendHost() << ":" << config.getBackendPort() << config.getBackendPath() << "\n\n";

    while (isRunning) {
        // Get current Unix timestamp
        auto now = std::chrono::system_clock::now();
        auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();

        // 1. DISPATCH SYSTEM METRICS
        com::lagrange::telemetry::AgentMessage sysMsg;
        sysMsg.set_agent_id(config.getAgentId());
        sysMsg.set_timestamp_ms(timestamp);
        
        // mutable_system_metrics() tells Protobuf we are using the SystemMetrics 'oneof' block
        collector->collectSystemMetrics(sysMsg.mutable_system_metrics());
        
        std::string sysPayload;
        if (sysMsg.SerializeToString(&sysPayload)) {
            std::cout << "[Agent] Dispatching System Metrics (" << sysPayload.length() << " bytes)... ";
            if (networkClient.sendTelemetry(sysPayload)) {
                std::cout << "SUCCESS\n";
            } else {
                std::cout << "FAILED\n";
            }
        }

        // 2. DISPATCH NETWORK STATS
        com::lagrange::telemetry::AgentMessage netMsg;
        netMsg.set_agent_id(config.getAgentId());
        netMsg.set_timestamp_ms(timestamp);

        collector->collectNetworkStats(netMsg.mutable_network_stats());

        std::string netPayload;
        if (netMsg.SerializeToString(&netPayload)) {
            std::cout << "[Agent] Dispatching Network Stats  (" << netPayload.length() << " bytes)... ";
            if (networkClient.sendTelemetry(netPayload)) {
                std::cout << "SUCCESS\n";
            } else {
                std::cout << "FAILED\n";
            }
        }
        std::cout << "--------------------------------------------------\n";

        // 3. SLEEP UNTIL NEXT CYCLE
        std::this_thread::sleep_for(std::chrono::milliseconds(config.getCollectionIntervalMs()));
    }
}

void Agent::stop() {
    isRunning = false;
    std::cout << "[Agent] Stopping..." << std::endl;
}
}
} 