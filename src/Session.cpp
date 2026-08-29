#include "Session.h"
#include <sstream>
#include <vector>

Session::Session(NetworkLayer& networkLayer) : m_network(networkLayer), m_cwd("/") {}

std::string Session::Cwd() const {
    return m_cwd;
}

void Session::SetCwdForTest(const std::string& cwd) {
    m_cwd = cwd;
}

std::string Session::Resolve(const std::string& path) const {
    if (path.empty() || path == ".") {
        return m_cwd;
    }

    std::string base = (path[0] == '/') ? "" : m_cwd;
    std::string combined = base.empty() ? path : base + "/" + path;

    std::vector<std::string> parts;
    std::stringstream ss(combined);
    std::string segment;
    while (std::getline(ss, segment, '/')) {
        if (segment.empty() || segment == ".") continue;
        if (segment == "..") {
            if (!parts.empty()) parts.pop_back();
            continue;
        }
        parts.push_back(segment);
    }

    std::string result = "/";
    for (size_t i = 0; i < parts.size(); ++i) {
        result += parts[i];
        if (i + 1 < parts.size()) result += "/";
    }
    return result;
}

bool Session::ChangeDirectory(const std::string& path) {
    std::string target = Resolve(path);
    auto entries = m_network.ListDirectory(target);
    // ListDirectory returns an empty vector both for "empty dir" and
    // "failed to open" - disambiguate via GetLastError().
    if (entries.empty() && !m_network.GetLastError().empty()) {
        return false;
    }
    m_cwd = target;
    return true;
}
