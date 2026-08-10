#pragma once
#include "telemetry.pb.h"
class SystemCollector {
public:
    virtual ~SystemCollector() = default;
    virtual void collectSystemMetrics(com::lagrange::telemetry::SystemMetrics* metrics) = 0;
    virtual void collectNetworkStats(com::lagrange::telemetry::NetworkStats* stats) = 0;

    static SystemCollector* create();
};