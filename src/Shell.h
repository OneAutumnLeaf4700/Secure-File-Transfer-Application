#pragma once

#include "NetworkLayer.h"
#include "Session.h"
#include <functional>
#include <map>
#include <string>
#include <vector>

class Shell {
public:
    Shell(NetworkLayer& networkLayer, Session& session);

    void Run();

    // Exposed so command handlers (this file and later tasks) can act on shared state.
    NetworkLayer& Network();
    Session& Sess();
    bool& ProgressEnabled();

private:
    using CommandFn = std::function<void(Shell&, const std::vector<std::string>& args)>;

    void RegisterCommands();
    void Dispatch(const std::string& line);
    static std::vector<std::string> Tokenize(const std::string& line);
    static void PrintProgressBar(long long transferred, long long total);

    NetworkLayer& m_network;
    Session& m_session;
    std::map<std::string, CommandFn> m_commands;
    bool m_running;
    bool m_progressEnabled;
};
