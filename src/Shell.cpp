#include "Shell.h"
#include <iostream>
#include <sstream>
#include <iomanip>

Shell::Shell(NetworkLayer& networkLayer, Session& session)
    : m_network(networkLayer), m_session(session), m_running(true), m_progressEnabled(true) {
    RegisterCommands();
}

void Shell::PrintProgressBar(long long transferred, long long total) {
    if (total <= 0) return;
    const int width = 30;
    double fraction = static_cast<double>(transferred) / static_cast<double>(total);
    int filled = static_cast<int>(fraction * width);

    std::cout << "\r[";
    for (int i = 0; i < width; ++i) std::cout << (i < filled ? '#' : '-');
    std::cout << "] " << std::setw(3) << static_cast<int>(fraction * 100) << "% "
              << transferred << "/" << total << " bytes" << std::flush;

    if (transferred >= total) std::cout << "\n";
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
            "  get <remote> [local]  download a file, shows progress\n"
            "  put <local> [remote]  upload a file, shows progress\n"
            "  mkdir <path>      create a remote directory\n"
            "  rm <path>         delete a remote file\n"
            "  rmdir <path>      remove a remote directory\n"
            "  progress on|off       toggle the transfer progress bar\n"
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

    m_commands["progress"] = [](Shell& shell, const std::vector<std::string>& args) {
        if (args.empty() || (args[0] != "on" && args[0] != "off")) {
            std::cout << "progress: usage: progress on|off\n";
            return;
        }
        shell.ProgressEnabled() = (args[0] == "on");
        std::cout << "progress: " << (shell.ProgressEnabled() ? "on" : "off") << "\n";
    };

    m_commands["get"] = [](Shell& shell, const std::vector<std::string>& args) {
        if (args.empty()) {
            std::cout << "get: usage: get <remote> [local]\n";
            return;
        }
        std::string remote = shell.Sess().Resolve(args[0]);
        std::string local = args.size() > 1 ? args[1]
            : remote.substr(remote.find_last_of('/') + 1);

        ProgressCallback cb = nullptr;
        if (shell.ProgressEnabled()) {
            cb = [](long long t, long long total) { PrintProgressBar(t, total); };
        }

        if (!shell.Network().DownloadFile(remote, local, cb)) {
            std::cout << "\nget: " << shell.Network().GetLastError() << "\n";
        }
    };

    m_commands["put"] = [](Shell& shell, const std::vector<std::string>& args) {
        if (args.empty()) {
            std::cout << "put: usage: put <local> [remote]\n";
            return;
        }
        std::string local = args[0];
        std::string localName = local.substr(local.find_last_of('/') + 1);
        std::string remote = args.size() > 1
            ? shell.Sess().Resolve(args[1])
            : shell.Sess().Resolve(localName);

        ProgressCallback cb = nullptr;
        if (shell.ProgressEnabled()) {
            cb = [](long long t, long long total) { PrintProgressBar(t, total); };
        }

        if (!shell.Network().UploadFile(local, remote, cb)) {
            std::cout << "\nput: " << shell.Network().GetLastError() << "\n";
        }
    };

    m_commands["mkdir"] = [](Shell& shell, const std::vector<std::string>& args) {
        if (args.empty()) {
            std::cout << "mkdir: usage: mkdir <path>\n";
            return;
        }
        std::string path = shell.Sess().Resolve(args[0]);
        if (!shell.Network().CreateRemoteDirectory(path)) {
            std::cout << "mkdir: " << shell.Network().GetLastError() << "\n";
        }
    };

    m_commands["rm"] = [](Shell& shell, const std::vector<std::string>& args) {
        if (args.empty()) {
            std::cout << "rm: usage: rm <path>\n";
            return;
        }
        std::string path = shell.Sess().Resolve(args[0]);
        if (!shell.Network().DeleteRemoteFile(path)) {
            std::cout << "rm: " << shell.Network().GetLastError() << "\n";
        }
    };

    m_commands["rmdir"] = [](Shell& shell, const std::vector<std::string>& args) {
        if (args.empty()) {
            std::cout << "rmdir: usage: rmdir <path>\n";
            return;
        }
        std::string path = shell.Sess().Resolve(args[0]);
        if (!shell.Network().RemoveRemoteDirectory(path)) {
            std::cout << "rmdir: " << shell.Network().GetLastError() << "\n";
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
