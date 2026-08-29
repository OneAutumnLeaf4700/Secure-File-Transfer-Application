# Secure File Transfer Application — CLI Rebuild Design

## Context

The project as it stands is a Win32 GUI application with a mostly-working
libssh2-based SFTP client (`NetworkLayer.cpp/h`, 466 lines) behind a
1456-line raw Win32 GUI (`Secure File Transfer Application.cpp`). The GUI
was never wired up to the network layer for real use, and the whole thing
only builds on Windows (Winsock, `fopen_s`, `localtime_s`). The resume
description calls it "peer-to-peer," but the actual implementation is a
standard SFTP client that connects to any SSH/SFTP server — it does not
implement peer-to-peer negotiation.

Development now happens on Arch Linux with no Windows toolchain installed,
but with `libssh2` 1.11.1 and `cmake`/`g++` available natively. Goal: get a
genuinely functional tool running soon, dropping the GUI in favor of a CLI,
and being honest about scope (SFTP client, not P2P) rather than chasing the
original resume wording verbatim.

## Decisions

1. **Platform**: cross-platform, Linux-first. Port `NetworkLayer` off
   Winsock/Win32 CRT onto POSIX sockets and portable CRT calls. Windows
   remains buildable later via `#ifdef` guards, but is not the primary
   target during this rebuild.
2. **Scope**: SFTP client only, not true peer-to-peer. Connects to any
   standard SSH/SFTP server (OpenSSH, Rebex test server, etc.) via
   libssh2. True P2P (custom protocol, NAT traversal, dual client/server
   role) is out of scope — it is effectively a different project.
3. **Interaction model**: interactive shell (REPL), similar to the
   standard `sftp` command — connect once, then issue `ls`, `cd`, `get`,
   `put`, etc. within the session.
4. **v1 feature scope**: core connect/auth/browse/transfer with a real
   (not simulated) progress bar. Compression and connection-level tuning
   (zlib compression, window/packet size tuning) are explicitly deferred
   to a fast-follow phase — not part of getting the tool functional.
5. **Legacy code**: existing Win32 GUI source and Visual Studio project
   files (`.vcxproj`, `.sln`, GUI `.cpp/.h`) move to `legacy/` for
   reference. They are not built or maintained going forward.

## Architecture

Three layers:

- **`NetworkLayer`** (ported, not rewritten) — owns the libssh2 session,
  SFTP session, and socket. Public interface stays close to what already
  exists (`Connect`, `Disconnect`, `AuthenticatePassword`,
  `AuthenticatePublicKey`, `ListDirectory`, `UploadFile`, `DownloadFile`,
  `DeleteRemoteFile`, `CreateRemoteDirectory`, `RemoveRemoteDirectory`).
  Internal changes only:
  - `winsock2.h`/`ws2tcpip.h`/`WSAStartup`/`WSACleanup`/`closesocket` →
    `sys/socket.h`/`netdb.h`/`unistd.h`/`close()`, guarded by
    `#ifdef _WIN32` so the file still compiles on Windows later.
  - `std::wstring` file paths → `std::string` (UTF-8), since POSIX paths
    are byte strings, not wide strings. `WStringToString`/
    `StringToWString` helpers are dropped; `RemoteFileInfo::fileName`
    etc. become `std::string`.
  - `fopen_s`/`errno_t` → `fopen` + `errno`; `localtime_s` → `localtime_r`;
    `_wremove` → `remove`.
- **`Session`** (new, thin) — owns one connected `NetworkLayer` instance,
  tracks the current remote working directory, and resolves relative
  paths typed in the shell against it (SFTP has no server-side "cwd," so
  the client has to track this itself, as the real `sftp` tool does).
- **`Shell`** (new) — REPL: reads a line, tokenizes it, dispatches
  through a command table (`ls`, `cd`, `pwd`, `get`, `put`, `mkdir`, `rm`,
  `rmdir`, `progress on|off`, `help`, `exit`), prints results or a
  `<command>: <error>` line without exiting the REPL on failure.

Entry point (`main.cpp`): `sfta <host> [-p port] [-u user] [-i keyfile]`.
If `-i` is given, authenticate via `AuthenticatePublicKey`; otherwise
prompt for a password with terminal echo disabled (`termios`) and use
`AuthenticatePassword`. On successful connect, enter the `Shell` REPL.

## Auth

- Password auth: existing `AuthenticatePassword`, reused as-is.
- Public-key auth: implement the already-declared but unimplemented
  `AuthenticatePublicKey` using `libssh2_userauth_publickey_fromfile`.
- Password prompt disables terminal echo via `termios` (`ECHO` flag) on
  Linux; no plaintext password ever appears on screen or in shell
  history (never accepted as a CLI argument).

## Progress bar

Reuse the existing `ProgressCallback` (`std::function<void(long long, long
long)>`) already threaded through `UploadFile`/`DownloadFile`. The shell's
`get`/`put` commands pass a callback that redraws a `\r`-anchored terminal
progress bar from real bytes-transferred/total-bytes values reported by
libssh2's chunked read/write loop — not a simulated or timer-based bar.

## Error handling

`ConnectionResult` (existing enum) surfaces connect/auth failures.
`NetworkLayer::GetLastError()` (existing) supplies human-readable detail
for SFTP operation failures. The `Shell` catches failures at the command
level and prints `<command>: <error>`, remaining in the REPL rather than
terminating the process — matching how `sftp`/`ssh` behave on a failed
single operation.

## Testing

- Manual integration testing against `test.rebex.net` (documented in
  `TEST_CONNECTION_GUIDE.md`) for connect, `ls`, `get` — this server is
  read-only, so it cannot validate `put`/`mkdir`/`rm`.
- A local OpenSSH server (`sshd` via systemd, or a disposable container)
  for round-tripping `put`, `mkdir`, `rm`, `rmdir`, since Rebex can't
  exercise those.
- No mock/unit-test framework for v1: the network-facing surface is thin
  enough, and libssh2's behavior specific enough, that integration tests
  against real servers are more valuable than mocking the library.

## Repo layout after the move

```
legacy/                 <- existing Win32 GUI + .vcxproj/.sln, untouched history
src/
  main.cpp
  NetworkLayer.cpp/.h    <- ported from legacy, POSIX sockets + UTF-8 strings
  Session.cpp/.h
  Shell.cpp/.h
CMakeLists.txt
README.md               <- rewritten for CLI usage, build, and auth instructions
```

## Explicitly out of scope for this rebuild

- True peer-to-peer negotiation (each instance as both client and
  server).
- Compression (`libssh2` zlib compression) and connection-level tuning
  (window/packet size). Deferred to a fast-follow phase once the core
  tool is proven functional.
- GUI of any kind (Win32 or otherwise).
- Windows build/CI — the ported code is guarded to remain
  Windows-compilable, but building and testing on Windows is not part of
  this effort.
