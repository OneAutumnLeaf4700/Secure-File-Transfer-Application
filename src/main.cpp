#include "NetworkLayer.h"
#include "Session.h"
#include "Shell.h"
#include <iostream>
#include <string>
#include <termios.h>
#include <unistd.h>

namespace {

std::string PromptHiddenPassword() {
    termios oldt{};
    tcgetattr(STDIN_FILENO, &oldt);
    termios newt = oldt;
    newt.c_lflag &= ~ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    std::cout << "Password: ";
    std::string password;
    std::getline(std::cin, password);

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    std::cout << "\n";
    return password;
}

struct Args {
    std::string host;
    int port = 22;
    std::string user;
    std::string keyfile;
    bool valid = false;
};

Args ParseArgs(int argc, char** argv) {
    Args args;
    if (argc < 2) return args;

    args.host = argv[1];

    char currentUser[256] = {};
    if (getlogin_r(currentUser, sizeof(currentUser)) == 0) {
        args.user = currentUser;
    }

    for (int i = 2; i < argc; ++i) {
        std::string flag = argv[i];
        if (flag == "-p" && i + 1 < argc) {
            try {
                args.port = std::stoi(argv[++i]);
            } catch (const std::exception&) {
                std::cerr << "Invalid port: " << argv[i] << "\n";
                return args;
            }
        } else if (flag == "-u" && i + 1 < argc) {
            args.user = argv[++i];
        } else if (flag == "-i" && i + 1 < argc) {
            args.keyfile = argv[++i];
        } else {
            std::cerr << "Unknown argument: " << flag << "\n";
            return args;
        }
    }

    args.valid = !args.host.empty() && !args.user.empty();
    return args;
}

}  // namespace

int main(int argc, char** argv) {
    Args args = ParseArgs(argc, argv);
    if (!args.valid) {
        std::cerr << "Usage: sfta <host> [-p port] [-u user] [-i keyfile]\n";
        return 1;
    }

    NetworkLayer net;
    net.SetStatusCallback([](const std::string& status) {
        std::cerr << "[status] " << status << "\n";
    });
    net.SetTrustHostKeyCallback([](const std::string& hostname, const std::string& fingerprint) {
        std::cout << "The authenticity of host '" << hostname << "' can't be established.\n"
                  << "Key fingerprint is " << fingerprint << ".\n"
                  << "Are you sure you want to continue connecting (yes/no)? ";
        std::string answer;
        std::getline(std::cin, answer);
        return answer == "yes" || answer == "y" || answer == "Y";
    });

    ConnectionResult result;
    if (!args.keyfile.empty()) {
        std::string pubKey = args.keyfile + ".pub";
        result = net.Connect(args.host, args.port, [&]() {
            return net.AuthenticatePublicKey(args.user, pubKey, args.keyfile, "");
        });
    } else {
        std::string password = PromptHiddenPassword();
        result = net.Connect(args.host, args.port, [&]() {
            return net.AuthenticatePassword(args.user, password);
        });
    }

    if (result != ConnectionResult::Success) {
        std::cerr << "Connection failed: " << net.GetLastError() << "\n";
        return 1;
    }

    Session session(net);
    session.ChangeDirectory("/");

    Shell shell(net, session);
    shell.Run();

    net.Disconnect();
    return 0;
}
