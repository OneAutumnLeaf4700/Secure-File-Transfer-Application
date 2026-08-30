# Secure File Transfer Application

A command-line SFTP client built on libssh2. Connects to any standard
SSH/SFTP server, then drops into an interactive shell for browsing and
transferring files.

## Building

Requires CMake 3.16+, a C++17 compiler, and libssh2 (with pkg-config
metadata) installed. On Arch Linux:

    sudo pacman -S cmake libssh2

Build:

    cmake -S . -B build
    cmake --build build

The resulting binary is `build/sfta`.

## Usage

    sfta <host> [-p port] [-u user] [-i keyfile]

- `-p port`   SSH port (default 22)
- `-u user`   username (default: your local login name)
- `-i keyfile` path to a private key for public-key auth (expects a
  matching `<keyfile>.pub`). Omit to be prompted for a password instead
  (input is not echoed to the terminal).

Example:

    sfta test.rebex.net -u demo
    Password: ******
    sfta:/> ls
    sfta:/> cd pub
    sfta:/pub> get readme.txt
    sfta:/pub> exit

## Shell commands

| Command | Description |
|---|---|
| `ls [path]` | list a remote directory (defaults to cwd) |
| `pwd` | print the remote working directory |
| `cd <path>` | change the remote working directory |
| `get <remote> [local]` | download a file, with a progress bar (defaults to `./downloads/<name>`) |
| `put <local> [remote]` | upload a file, with a progress bar |
| `mkdir <path>` | create a remote directory |
| `rm <path>` | delete a remote file |
| `rmdir <path>` | remove a remote directory |
| `progress on\|off` | toggle the transfer progress bar |
| `help` | show the command list |
| `exit` | disconnect and quit |

## Scope

This is an SFTP client, not a peer-to-peer tool — it connects to a
standard SSH/SFTP server. Compression and connection-level tuning are
not yet implemented.

## Legacy

An earlier Win32 GUI prototype preceded this CLI and has been removed
from the tree; it's still recoverable from git history if needed.
