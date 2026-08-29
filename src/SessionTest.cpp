#include "Session.h"
#include <cassert>
#include <iostream>

int main() {
    NetworkLayer net; // unconnected - Resolve() must not touch the network
    Session session(net);

    assert(session.Cwd() == "/");
    assert(session.Resolve("foo.txt") == "/foo.txt");
    assert(session.Resolve("/abs/path") == "/abs/path");
    assert(session.Resolve(".") == "/");

    session.SetCwdForTest("/home/demo");
    assert(session.Resolve("foo.txt") == "/home/demo/foo.txt");
    assert(session.Resolve("..") == "/home");
    assert(session.Resolve("../sibling") == "/home/sibling");
    assert(session.Resolve("/etc/passwd") == "/etc/passwd");

    session.SetCwdForTest("/");
    assert(session.Resolve("..") == "/");

    std::cout << "All Session tests passed\n";
    return 0;
}
