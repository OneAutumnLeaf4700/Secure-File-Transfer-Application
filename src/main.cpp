#include "NetworkLayer.h"
#include <iostream>

int main() {
    NetworkLayer net;
    net.SetStatusCallback([](const std::string& status) {
        std::cout << "[status] " << status << "\n";
    });

    std::cout << "Connecting to test.rebex.net...\n";
    ConnectionResult result = net.Connect("test.rebex.net", 22, "demo", "password");

    if (result != ConnectionResult::Success) {
        std::cout << "Connect failed: " << net.GetLastError() << "\n";
        return 1;
    }

    std::cout << "Connected. Listing /:\n";
    for (const auto& entry : net.ListDirectory("/")) {
        std::cout << "  " << entry.permissions << " " << entry.fileSize
                  << " " << entry.fileName << "\n";
    }

    net.Disconnect();
    std::cout << "Disconnected.\n";
    return 0;
}
