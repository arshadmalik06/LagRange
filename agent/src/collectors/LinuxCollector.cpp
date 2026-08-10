#ifdef __linux__
#include "SystemCollector.hpp"
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <dirent.h>
#include <unistd.h>
#include <chrono>

class LinuxCollector : public SystemCollector {
private:
    // State variables for calculating rates (CPU %, Network Kbps, Disk Kbps)
    uint64_t last_cpu_total = 0;
    uint64_t last_cpu_idle = 0;
    uint64_t last_disk_read_kb = 0;
    uint64_t last_disk_write_kb = 0;
    uint64_t last_net_rx_bytes = 0;
    uint64_t last_net_tx_bytes = 0;
    uint64_t last_time_ms = 0;
    uint64_t getCurrentTimeMs() {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
    }

public:
    LinuxCollector() {
        last_time_ms = getCurrentTimeMs();
        // Initialize state so our first delta isn't a massive spike
        SystemMetrics dummySys;
        NetworkStats dummyNet;
        collectSystemMetrics(&dummySys);
        collectNetworkStats(&dummyNet);
    }

    void collectSystemMetrics(com::lagrange::telemetry::SystemMetrics* metrics) override {
        uint64_t current_time_ms = getCurrentTimeMs();
        double time_diff_sec = (current_time_ms - last_time_ms) / 1000.0;
        if (time_diff_sec <= 0) time_diff_sec = 1.0;

        // 1. CPU USAGE (/proc/stat)
        std::ifstream stat_file("/proc/stat");
        std::string line;
        if (std::getline(stat_file, line)) {
            std::istringstream iss(line);
            std::string cpu_label;
            uint64_t user, nice, system, idle, iowait, irq, softirq, steal;
            if (iss >> cpu_label >> user >> nice >> system >> idle >> iowait >> irq >> softirq >> steal) {
                uint64_t current_idle = idle + iowait;
                uint64_t current_total = user + nice + system + current_idle + irq + softirq + steal;
                uint64_t total_diff = current_total - last_cpu_total;
                uint64_t idle_diff = current_idle - last_cpu_idle;
                
                if (total_diff > 0) {
                    double cpu_usage = (100.0 * (total_diff - idle_diff)) / total_diff;
                    metrics->set_cpu_usage_percent(cpu_usage);
                }
                last_cpu_total = current_total;
                last_cpu_idle = current_idle;
            }
        }

        // 2. RAM USAGE (/proc/meminfo)
        std::ifstream mem_file("/proc/meminfo");
        double mem_total = 0, mem_free = 0, buffers = 0, cached = 0, sreclaimable = 0;
        while (std::getline(mem_file, line)) {
            std::istringstream iss(line);
            std::string key;
            uint64_t value;
            if (iss >> key >> value) {
                if (key == "MemTotal:") mem_total = value / 1024.0;
                else if (key == "MemFree:") mem_free = value / 1024.0;
                else if (key == "Buffers:") buffers = value / 1024.0;
                else if (key == "Cached:") cached = value / 1024.0;
                else if (key == "SReclaimable:") sreclaimable = value / 1024.0;
            }
        }
        metrics->set_ram_total_mb(mem_total);
        // Linux truly used RAM = Total - Free - Buffers - Cached - SReclaimable
        metrics->set_ram_used_mb(mem_total - mem_free - buffers - cached - sreclaimable);

        // 3. DISK I/O (/proc/vmstat - pgpgin / pgpgout are in KB)
        std::ifstream vmstat_file("/proc/vmstat");
        uint64_t current_disk_read = 0, current_disk_write = 0;
        while (std::getline(vmstat_file, line)) {
            std::istringstream iss(line);
            std::string key;
            uint64_t value;
            if (iss >> key >> value) {
                if (key == "pgpgin") current_disk_read = value;
                else if (key == "pgpgout") current_disk_write = value;
            }
        }
        
        metrics->set_disk_read_kbps(static_cast<double>(current_disk_read - last_disk_read_kb) / time_diff_sec);
        metrics->set_disk_write_kbps(static_cast<double>(current_disk_write - last_disk_write_kb) / time_diff_sec);
        
        last_disk_read_kb = current_disk_read;
        last_disk_write_kb = current_disk_write;

        // 4. ACTIVE & ZOMBIE PROCESSES (/proc/[pid]/stat)
        DIR* proc_dir = opendir("/proc");
        int32_t active_procs = 0;
        int32_t zombie_procs = 0;
        if (proc_dir != nullptr) {
            struct dirent* entry;
            while ((entry = readdir(proc_dir)) != nullptr) {
                // If directory name is a number, it's a process
                if (entry->d_type == DT_DIR && isdigit(entry->d_name[0])) {
                    active_procs++;
                    std::string stat_path = std::string("/proc/") + entry->d_name + "/stat";
                    std::ifstream pstat(stat_path);
                    std::string p_line;
                    if (std::getline(pstat, p_line)) {
                        // Extract the 3rd field (state)
                        size_t first_paren = p_line.find_last_of(')');
                        if (first_paren != std::string::npos && p_line.length() > first_paren + 2) {
                            char state = p_line[first_paren + 2];
                            if (state == 'Z') {
                                zombie_procs++;
                            }
                        }
                    }
                }
            }
            closedir(proc_dir);
        }
        metrics->set_active_processes(active_procs);
        metrics->set_zombie_processes(zombie_procs);

        // 5. CPU TEMPERATURE (Thermal Zone)
        std::ifstream temp_file("/sys/class/thermal/thermal_zone0/temp");
        uint64_t millicelsius;
        if (temp_file >> millicelsius) {
            metrics->set_cpu_temperature_celsius(millicelsius / 1000.0);
        }
        last_time_ms = current_time_ms;
    }

    void collectNetworkStats(com::lagrange::telemetry::NetworkStats* stats) override {
        // 1. TCP & UDP CONNECTIONS (/proc/net/tcp & udp)
        auto count_lines = [](const char* filepath) -> int32_t {
            std::ifstream file(filepath);
            int32_t lines = 0;
            std::string line;
            while (std::getline(file, line)) lines++;
            return (lines > 0) ? lines - 1 : 0; // Subtract header line
        };
        stats->set_active_tcp_connections(count_lines("/proc/net/tcp"));
        stats->set_active_udp_connections(count_lines("/proc/net/udp"));

        // 2. NETWORK TX/RX SPEED (/proc/net/dev)
        std::ifstream net_file("/proc/net/dev");
        std::string line;
        uint64_t current_rx = 0;
        uint64_t current_tx = 0;
        
        // Skip the two header lines
        std::getline(net_file, line);
        std::getline(net_file, line);
        
        while (std::getline(net_file, line)) {
            std::istringstream iss(line);
            std::string iface;
            uint64_t rx_bytes, rx_packets, rx_errs, rx_drop, rx_fifo, rx_frame, rx_comp, rx_mc;
            uint64_t tx_bytes;
            iss >> iface >> rx_bytes >> rx_packets >> rx_errs >> rx_drop >> rx_fifo >> rx_frame >> rx_comp >> rx_mc >> tx_bytes;
            // Ignore loopback interface
            if (iface.find("lo:") == std::string::npos) {
                current_rx += rx_bytes;
                current_tx += tx_bytes;
            }
        }

        uint64_t current_time_ms = getCurrentTimeMs();
        double time_diff_sec = (current_time_ms - last_time_ms) / 1000.0;
        if (time_diff_sec <= 0) time_diff_sec = 1.0; // Fallback to avoid division by zero

        if (last_net_rx_bytes > 0 || last_net_tx_bytes > 0) {
            stats->set_network_rx_kbps(((current_rx - last_net_rx_bytes) / 1024.0) / time_diff_sec);
            stats->set_network_tx_kbps(((current_tx - last_net_tx_bytes) / 1024.0) / time_diff_sec);
        }

        last_net_rx_bytes = current_rx;
        last_net_tx_bytes = current_tx;
    }
};
// THE FACTORY CREATOR (Linux Implementation)
SystemCollector* SystemCollector::create() {
    return new LinuxCollector();
}
#endif