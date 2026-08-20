#pragma once
#include <atomic>
#include <memory>
#include "lagrange/core/Config.hpp"
#include "lagrange/core/NetworkClient.hpp"
#include "collectors/SystemCollector.hpp"

namespace lagrange {
namespace core {

class Agent {
private:
    Config config;
    NetworkClient networkClient;
    
    // Smart pointer to automatically manage memory for OS specific collector
    std::unique_ptr<SystemCollector> collector;
    
    // Thread safe boolean to control the infinite loop
    std::atomic<bool> isRunning;

public:
    Agent();
    void start();
    void stop();
};
} 
}