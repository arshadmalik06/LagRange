package com.lagrange.services;

import org.springframework.stereotype.Service;

import com.google.protobuf.InvalidProtocolBufferException;
import com.lagrange.telemetry.grpc.AgentMessage;

@Service
public class TelemetryService {

    public void processTelemetry(byte[] payload) {
        try {
            AgentMessage message = AgentMessage.parseFrom(payload);

            System.out.println("\n=== NEW TELEMETRY RECEIVED ===");
            System.out.println("Agent ID : " + message.getAgentId());
            
            switch (message.getPayloadCase()) {
                case SYSTEM_METRICS:
                    System.out.println("Data Type: SYSTEM METRICS");
                    System.out.println("CPU Usage : " + message.getSystemMetrics().getCpuUsagePercent() + " %");
                    System.out.println("RAM Used  : " + message.getSystemMetrics().getRamUsedMb() + " / " + message.getSystemMetrics().getRamTotalMb() + " MB");
                    System.out.println("Disk Read : " + message.getSystemMetrics().getDiskReadKbps() + " KB/s");
                    System.out.println("Disk Write: " + message.getSystemMetrics().getDiskWriteKbps() + " KB/s");
                    System.out.println("Active Prc: " + message.getSystemMetrics().getActiveProcesses());
                    System.out.println("Zombie Prc: " + message.getSystemMetrics().getZombieProcesses());
                    break;
                case NETWORK_STATS:
                    System.out.println("Data Type: NETWORK STATS");
                    System.out.println("TCP Conns : " + message.getNetworkStats().getActiveTcpConnections());
                    System.out.println("UDP Conns : " + message.getNetworkStats().getActiveUdpConnections());
                    System.out.println("Net TX    : " + message.getNetworkStats().getNetworkTxKbps() + " KB/s");
                    System.out.println("Net RX    : " + message.getNetworkStats().getNetworkRxKbps() + " KB/s");
                    break;
                case SECURITY_EVENT:
                    System.out.println("Data Type: SECURITY EVENT");
                    System.out.println("Warning   : " + message.getSecurityEvent().getDescription());
                    break;
                case PAYLOAD_NOT_SET:
                    System.out.println("Warning   : Received empty payload block.");
                    break;
            }
            System.out.println("==============================\n");

        } catch (InvalidProtocolBufferException e) {
            System.err.println(">>> ERROR: Failed to parse telemetry data! " + e.getMessage());
        }
    }
}