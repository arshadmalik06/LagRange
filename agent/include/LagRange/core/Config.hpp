#pragma once
#include <string>

namespace lagrange {
namespace core {

class Config {
private:
    std::string agentId;
    std::string backendHost;
    std::string backendPort;
    std::string backendPath;
    int collectionIntervalMs;

public:
    // Constructor initializes the default values
    Config(); 
    // Getters so other classes can read the config safely
    std::string getAgentId() const;
    std::string getBackendHost() const;
    std::string getBackendPort() const;
    std::string getBackendPath() const;
    int getCollectionIntervalMs() const;
};
}
}