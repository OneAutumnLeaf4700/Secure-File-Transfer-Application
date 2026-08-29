#include "NetworkLayer.h"
#include "Session.h"
#include "Shell.h"
#include <iostream>

int main() {
    NetworkLayer net;
    net.SetStatusCallback([](const std::string& status) {
        std::cerr << "[status] " << status << "\n";
    });

    std::cout << "Connecting to test.rebex.net...\n";
    ConnectionResult result = net.Connect("test.rebex.net", 22, "demo", "password");
    if (result != ConnectionResult::Success) {
        std::cerr << "Connect failed: " << net.GetLastError() << "\n";
        return 1;
    }

    Session session(net);
    session.ChangeDirectory("/");

    Shell shell(net, session);
    shell.Run();

    net.Disconnect();
    return 0;
}
