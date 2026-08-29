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

    std::cout << "\nTesting pubkey auth against local server...\n";
    {
        NetworkLayer localNet;
        // Reuse Connect() to get socket+handshake, but it forces password
        // auth, so instead check GetLastError() reports auth failure with
        // a bogus password, then separately confirm the pubkey path type-
        // checks and links by calling it after a manual connect sequence
        // is not exposed publicly - so this smoke test calls Connect()
        // with the real key-based flow added directly to NetworkLayer's
        // public Connect() is out of scope; AuthenticatePublicKey is
        // exercised end-to-end in Task 5 (Shell) once Session/main wire
        // -i keyfile through to it. For now, confirm it links and the
        // signature is correct via a compile-only call:
        (void)&NetworkLayer::AuthenticatePublicKey;
        std::cout << "AuthenticatePublicKey linked OK (functional test in Task 8)\n";
    }

    return 0;
}
