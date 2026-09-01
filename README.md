# Secure File Transfer Application

A C++ SFTP client built directly on libssh2. It connects to any standard SSH/SFTP server —
password or public-key auth — and drops into an interactive shell for browsing and transferring
files, with byte-progress bars on `get`/`put` and a hidden-input password prompt.

## Why this exists

This started as a Win32/ImGui GUI prototype. Once the GUI layer was adding more surface area
than value, it was deliberately stripped out (see `git log` — `Remove legacy Win32 GUI prototype
from the tree`) and rebuilt CLI-first on POSIX sockets. The GUI is recoverable from history if a
GUI is ever worth it again, but the point of the rebuild was to get the transfer/session logic
solid — connection handling, auth, directory traversal, byte-accurate transfers — before putting
any UI on top of it.

## What it does

- Connects over SSH/SFTP via libssh2 — password auth or public-key auth (`-i keyfile`, expects a
  matching `<keyfile>.pub`)
- Interactive shell: `ls`, `pwd`, `cd`, `get`, `put`, `mkdir`, `rm`, `rmdir`, `progress on|off`,
  `help`, `exit`
- Real-time byte-progress bar on uploads and downloads
- Hidden (non-echoed) password entry at the terminal

## Architecture

- `NetworkLayer` — owns the libssh2 session/socket lifecycle: connect, auth, SFTP channel setup,
  and the actual read/write loops for transfers
- `Session` — tracks remote working-directory state and resolves relative paths against it, so
  the shell doesn't need to know about the wire protocol
- `Shell` — parses shell commands and dispatches to `Session`/`NetworkLayer`; this is the only
  layer that talks to `stdin`/`stdout`
- `main.cpp` — CLI arg parsing, hidden password prompt, wires the above together and starts the
  shell loop

## Build & run

Requires CMake 3.16+, a C++17 compiler, and libssh2 (with pkg-config metadata).

```bash
sudo pacman -S cmake libssh2   # Arch; substitute your package manager
cmake -S . -B build
cmake --build build
```

```bash
build/sfta <host> [-p port] [-u user] [-i keyfile]
```

```
sfta test.rebex.net -u demo
Password: ******
sfta:/> ls
sfta:/> cd pub
sfta:/pub> get readme.txt
sfta:/pub> exit
```

## Scope

This is an SSH/SFTP client talking to a standard server — not a peer-to-peer tool. Compression
and connection-level tuning are not implemented yet.
