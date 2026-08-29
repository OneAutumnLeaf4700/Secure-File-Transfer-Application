#include "Shell.h"
#include <iostream>
#include <sstream>

Shell::Shell(NetworkLayer& networkLayer, Session& session)
    : m_network(networkLayer), m_session(session), m_running(true), m_progressEnabled(true) {
    RegisterCommands();
}

NetworkLayer& Shell::Network() { return m_network; }
Session& Shell::Sess() { return m_session; }
bool& Shell::ProgressEnabled() { return m_progressEnabled; }

std::vector<std::string> Shell::Tokenize(const std::string& line) {
    std::vector<std::string> tokens;
    std::istringstream iss(line);
    std::string tok;
    while (iss >> tok) tokens.push_back(tok);
    return tokens;
}

void Shell::RegisterCommands() {
    m_commands["help"] = [](Shell&, const std::vector<std::string>&) {
        std::cout <<
            "Commands:\n"
            "  ls [path]        list remote directory\n"
            "  pwd               print remote working directory\n"
            "  cd <path>         change remote working directory\n"
            "  help              show this message\n"
            "  exit              close the connection and quit\n";
    };

    m_commands["exit"] = [](Shell& shell, const std::vector<std::string>&) {
        shell.m_running = false;
    };

    m_commands["pwd"] = [](Shell& shell, const std::vector<std::string>&) {
        std::cout << shell.Sess().Cwd() << "\n";
    };

    m_commands["cd"] = [](Shell& shell, const std::vector<std::string>& args) {
        if (args.empty()) {
            std::cout << "cd: missing path\n";
            return;
        }
        if (!shell.Sess().ChangeDirectory(args[0])) {
            std::cout << "cd: " << shell.Network().GetLastError() << "\n";
        }
    };

    m_commands["ls"] = [](Shell& shell, const std::vector<std::string>& args) {
        std::string path = args.empty() ? shell.Sess().Cwd() : shell.Sess().Resolve(args[0]);
        auto entries = shell.Network().ListDirectory(path);
        if (entries.empty() && !shell.Network().GetLastError().empty()) {
            std::cout << "ls: " << shell.Network().GetLastError() << "\n";
            return;
        }
        for (const auto& e : entries) {
            std::cout << e.permissions << " " << e.fileSize << " "
                      << e.lastModified << " " << e.fileName << "\n";
        }
    };
}

void Shell::Dispatch(const std::string& line) {
    auto tokens = Tokenize(line);
    if (tokens.empty()) return;

    std::string cmd = tokens[0];
    std::vector<std::string> args(tokens.begin() + 1, tokens.end());

    auto it = m_commands.find(cmd);
    if (it == m_commands.end()) {
        std::cout << cmd << ": unknown command (try 'help')\n";
        return;
    }
    it->second(*this, args);
}

void Shell::Run() {
    std::string line;
    while (m_running) {
        std::cout << "sfta:" << m_session.Cwd() << "> ";
        if (!std::getline(std::cin, line)) break; // EOF (Ctrl-D)
        Dispatch(line);
    }
}
