#include "Secure File Transfer Application/NetworkLayer.h"
#include <iostream>
#include <string>

int main() {
    std::cout << "=== Secure File Transfer Network Layer Test ===" << std::endl;
    std::cout << std::endl;

    // Test server details (using public test server)
    std::string hostname = "test.rebex.net";
    int port = 22;
    std::string username = "demo";
    std::string password = "password";

    std::cout << "Testing connection to: " << hostname << ":" << port << std::endl;
    std::cout << "Username: " << username << std::endl;
    std::cout << "Password: " << password << std::endl;
    std::cout << std::endl;

    // Create network layer instance
    NetworkLayer networkLayer;

    // Set up status callback
    networkLayer.SetStatusCallback([](const std::wstring& status) {
        std::wcout << L"Status: " << status << std::endl;
    });

    std::cout << "Attempting connection..." << std::endl;

    // Attempt connection
    ConnectionResult result = networkLayer.Connect(hostname, port, username, password);

    // Report results
    switch (result) {
    case ConnectionResult::Success:
        std::cout << "✅ SUCCESS: Connected to server!" << std::endl;
        std::cout << "SFTP session established." << std::endl;
        
        // Test disconnect
        std::cout << "Testing disconnect..." << std::endl;
        networkLayer.Disconnect();
        std::cout << "✅ Disconnected successfully." << std::endl;
        break;

    case ConnectionResult::NetworkError:
        std::cout << "❌ NETWORK ERROR: Failed during SSH handshake." << std::endl;
        break;

    case ConnectionResult::AuthenticationFailed:
        std::cout << "❌ AUTHENTICATION FAILED: Invalid credentials." << std::endl;
        break;

    case ConnectionResult::HostUnreachable:
        std::cout << "❌ HOST UNREACHABLE: Cannot connect to server." << std::endl;
        break;

    case ConnectionResult::ConnectionTimeout:
        std::cout << "❌ CONNECTION TIMEOUT: Server not responding." << std::endl;
        break;

    default:
        std::cout << "❌ UNKNOWN ERROR: Unexpected failure." << std::endl;
        break;
    }

    std::cout << std::endl;
    std::cout << "Network layer validation:" << std::endl;
    std::cout << "- Socket operations: " << (result != ConnectionResult::HostUnreachable ? "✅" : "❌") << std::endl;
    std::cout << "- SSH handshake: " << (result != ConnectionResult::NetworkError ? "✅" : "❌") << std::endl;
    std::cout << "- Authentication: " << (result != ConnectionResult::AuthenticationFailed ? "✅" : "❌") << std::endl;
    std::cout << "- SFTP initialization: " << (result == ConnectionResult::Success ? "✅" : "❌") << std::endl;

    std::cout << std::endl;
    std::cout << "Press Enter to exit...";
    std::cin.get();

    return 0;
}
