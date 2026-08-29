#pragma once

#include "NetworkLayer.h"
#include <string>

class Session {
public:
    explicit Session(NetworkLayer& networkLayer);

    std::string Cwd() const;
    std::string Resolve(const std::string& path) const;
    bool ChangeDirectory(const std::string& path);

    // Test-only hook so path-resolution logic is testable without a live connection.
    void SetCwdForTest(const std::string& cwd);

private:
    NetworkLayer& m_network;
    std::string m_cwd;
};
