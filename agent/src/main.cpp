#include "lagrange/core/Agent.hpp"

int main() {
    // Instantiate the orchestrator
    lagrange::core::Agent agent;

    // Start the infinite loop
    agent.start();
    return 0;
}