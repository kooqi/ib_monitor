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
    std::unordered_map<std::string, PortCounters> counters; // 映射端口名到其计数器数据
};

std::vector<IBInterface> findIBInterfaces() {
    std::vector<IBInterface> interfaces;
    std::string path = "/sys/class/infiniband/";
    DIR* dir = opendir(path.c_str());
    if (dir != nullptr) {
        std::cout << "DEBUG: Opened /sys/class/infiniband/ successfully.\n";
        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            // 修改逻辑：直接检查是否能够打开目录
            if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
                std::string dirPath = path + entry->d_name;
                DIR* testDir = opendir(dirPath.c_str());
                if (testDir != nullptr) {
                    // 成功打开目录，表明这是一个有效目录
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

    // 读取接收到的数据量
    in.open(counterPath + "port_rcv_data");
    if (in.is_open()) {
        std::getline(in, portRcvDataStr);
        portRcvData = std::stoull(portRcvDataStr);
        in.close();
    } else {
        std::cerr << "ERROR: Failed to open port_rcv_data for " << iface.name << ", port " << port << std::endl;
        return;
    }

    // 读取发送的数据量
    in.open(counterPath + "port_xmit_data");
    if (in.is_open()) {
        std::getline(in, portXmitDataStr);
        portXmitData = std::stoull(portXmitDataStr);
        in.close();
    } else {
        std::cerr << "ERROR: Failed to open port_xmit_data for " << iface.name << ", port " << port << std::endl;
        return;
    }

    // 获取当前端口的计数器
    auto& counters = iface.counters[port];

    // 如果是第一次读取，初始化上一次的计数器值，然后跳过差分计算
    if (counters.portRcvDataPrev == 0 && counters.portXmitDataPrev == 0) {
        counters.portRcvDataPrev = portRcvData;
        counters.portXmitDataPrev = portXmitData;
        // 第一次读取时，没有差分数据可计算，所以跳过此次输出
        return;
    }

    // 计算增量
    unsigned long long rcvDataDelta = portRcvData - counters.portRcvDataPrev;
    unsigned long long xmitDataDelta = portXmitData - counters.portXmitDataPrev;

    // 更新存储的上一次计数器值
    counters.portRcvDataPrev = portRcvData;
    counters.portXmitDataPrev = portXmitData;

    // 转换为 Mbps ，port_xmit_data 和 port_rcv_data 的数值是实际的 1/4, 因此实际的带宽是在其基础之上乘以 4
    double rcvMbps = static_cast<double>(rcvDataDelta) * 8 * 4 / 2 / 1e6;
    double xmitMbps = static_cast<double>(xmitDataDelta) * 8 * 4 / 2 / 1e6;

    // 设置cout使用本地化格式
    std::cout.imbue(std::locale("")); // 使用当前环境的默认区域设置

    std::cout << std::fixed << std::setprecision(2);
    std::cout << "Interface: " << iface.name << ", Port: " << port
              << ", RcvData: " << rcvMbps << " Mbps"
              << ", XmitData: " << xmitMbps << " Mbps \n"
              << std::endl;

    // 清除本地化设置，如果需要的话
    std::cout.imbue(std::locale::classic());
}

int main() {
    auto interfaces = findIBInterfaces();

    if (interfaces.empty()) {
        std::cerr << "ERROR: No InfiniBand interfaces found. Exiting." << std::endl;
        return 1;
    }

    while (true) {
        std::cout << "\033[2J\033[H"; // 清屏并将光标移动到左上角

        for (auto& iface : interfaces) {
            for (const auto& port : iface.ports) {
                printPortCounters(iface, port);
            }
        }
        std::this_thread::sleep_for(std::chrono::seconds(2)); // 间隔
    }

    return 0;
}