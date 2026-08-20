#pragma once
#include <string>
#include "lagrange/core/Config.hpp"

namespace lagrange {
namespace core {

class NetworkClient {
private:
    Config config;

public:
    // Constructor requires a Config object so it knows where to connect
    NetworkClient(const Config& cfg);
    
    // Sends the binary string. Returns true if successful, false if it failed.
    bool sendTelemetry(const std::string& serializedPayload);
};
} 
}