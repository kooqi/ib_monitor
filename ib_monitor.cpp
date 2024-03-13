// Created by Kooqi.LEE on 2024/2/21.

#include <iostream>
#include <fstream>
#include <vector>
#include <unistd.h>
#include <dirent.h>
#include <cstring>
#include <thread>
#include <chrono>
#include <iomanip>
#include <locale>
#include <unordered_map>

struct PortCounters {
    unsigned long long portRcvDataPrev = 0;
    unsigned long long portXmitDataPrev = 0;
};

struct IBInterface {
    std::string name;
    std::vector<std::string> ports;
    std::unordered_map<std::string, PortCounters> counters; 
};

std::vector<IBInterface> findIBInterfaces() {
    std::vector<IBInterface> interfaces;
    std::string path = "/sys/class/infiniband/";
    DIR* dir = opendir(path.c_str());
    if (dir != nullptr) {
        std::cout << "DEBUG: Opened /sys/class/infiniband/ successfully.\n";
        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
                std::string dirPath = path + entry->d_name;
                DIR* testDir = opendir(dirPath.c_str());
                if (testDir != nullptr) {
                    closedir(testDir);
                    IBInterface iface;
                    iface.name = entry->d_name;
                    std::cout << "DEBUG: Processing interface " << iface.name << ".\n";

                    std::string portsPath = path + iface.name + "/ports/";
                    DIR* portsDir = opendir(portsPath.c_str());
                    if (portsDir != nullptr) {
                        struct dirent* portsEntry;
                        while ((portsEntry = readdir(portsDir)) != nullptr) {
                            if (strcmp(portsEntry->d_name, ".") != 0 && strcmp(portsEntry->d_name, "..") != 0) {
                                iface.ports.push_back(portsEntry->d_name);
                                std::cout << "DEBUG: Found port " << portsEntry->d_name << " for interface " << iface.name << ".\n";
                            }
                        }
                        closedir(portsDir);
                    } else {
                        std::cerr << "DEBUG: Failed to open ports directory for " << iface.name << ".\n";
                    }
                    interfaces.push_back(iface);
                }
            }
        }
        closedir(dir);
    } else {
        std::cerr << "ERROR: Failed to open /sys/class/infiniband/ directory.\n";
    }
    return interfaces;
}

void printPortCounters(IBInterface& iface, const std::string& port) {
    std::string counterPath = "/sys/class/infiniband/" + iface.name + "/ports/" + port + "/counters/";
    std::ifstream in;
    std::string portRcvDataStr, portXmitDataStr;
    unsigned long long portRcvData, portXmitData;

    // receive data
    in.open(counterPath + "port_rcv_data");
    if (in.is_open()) {
        std::getline(in, portRcvDataStr);
        portRcvData = std::stoull(portRcvDataStr);
        in.close();
    } else {
        std::cerr << "ERROR: Failed to open port_rcv_data for " << iface.name << ", port " << port << std::endl;
        return;
    }

    // transmit data
    in.open(counterPath + "port_xmit_data");
    if (in.is_open()) {
        std::getline(in, portXmitDataStr);
        portXmitData = std::stoull(portXmitDataStr);
        in.close();
    } else {
        std::cerr << "ERROR: Failed to open port_xmit_data for " << iface.name << ", port " << port << std::endl;
        return;
    }

    // get port counters
    auto& counters = iface.counters[port];

    // ignored first read
    if (counters.portRcvDataPrev == 0 && counters.portXmitDataPrev == 0) {
        counters.portRcvDataPrev = portRcvData;
        counters.portXmitDataPrev = portXmitData;
        return;
    }

    unsigned long long rcvDataDelta = portRcvData - counters.portRcvDataPrev;
    unsigned long long xmitDataDelta = portXmitData - counters.portXmitDataPrev;

    // update counters
    counters.portRcvDataPrev = portRcvData;
    counters.portXmitDataPrev = portXmitData;

    // convert to Mbps
    double rcvMbps = static_cast<double>(rcvDataDelta) * 8 * 4 / 2 / 1e6;
    double xmitMbps = static_cast<double>(xmitDataDelta) * 8 * 4 / 2 / 1e6;

    // setlocale
    std::cout.imbue(std::locale("")); 

    std::cout << std::fixed << std::setprecision(2);
    std::cout << "Interface: " << iface.name << ", Port: " << port
              << ", RcvData: " << rcvMbps << " Mbps"
              << ", XmitData: " << xmitMbps << " Mbps \n"
              << std::endl;

    // reset locale
    std::cout.imbue(std::locale::classic());
}

int main() {
    auto interfaces = findIBInterfaces();

    if (interfaces.empty()) {
        std::cerr << "ERROR: No InfiniBand interfaces found. Exiting." << std::endl;
        return 1;
    }

    while (true) {
        std::cout << "\033[2J\033[H"; // clear screen

        for (auto& iface : interfaces) {
            for (const auto& port : iface.ports) {
                printPortCounters(iface, port);
            }
        }
        std::this_thread::sleep_for(std::chrono::seconds(2)); 
    }

    return 0;
}