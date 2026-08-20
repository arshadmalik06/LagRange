#include "lagrange/core/Config.hpp"

namespace lagrange {
namespace core {
    
Config::Config() {
    // Unique identifier for this specific machine
    agentId = "dev-windows-agent-001";
    
    // Spring Boot Backend Connection Details
    backendHost = "127.0.0.1";
    backendPort = "8080";
    backendPath = "/api/v1/telemetry";

    // How often the infinite loop should collect metrics (eg 5 seconds)
    collectionIntervalMs = 5000;
}
std::string Config::getAgentId() const { return agentId; }
std::string Config::getBackendHost() const { return backendHost; }
std::string Config::getBackendPort() const { return backendPort; }
std::string Config::getBackendPath() const { return backendPath; }
int Config::getCollectionIntervalMs() const { return collectionIntervalMs; }
}
} 