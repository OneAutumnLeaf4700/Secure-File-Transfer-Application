#include "NetworkLayer.h"
#include "libssh2.h"
#include "libssh2_sftp.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <codecvt>
#include <locale>

#pragma comment(lib, "Ws2_32.lib")

NetworkLayer::NetworkLayer() :
    m_session(nullptr),
    m_sftpSession(nullptr),
    m_socket(-1),
    m_connected(false),
    m_libraryInitialized(false) {
    WSADATA wsadata;
    if (WSAStartup(MAKEWORD(2, 2), &wsadata) == 0) {
        m_libraryInitialized = true;
    }
}

NetworkLayer::~NetworkLayer() {
    Disconnect();
    if (m_libraryInitialized) {
        WSACleanup();
    }
}

ConnectionResult NetworkLayer::Connect(const std::string &hostname, int port, const std::string &username, const std::string &password) {
    if (!InitializeLibrary()) {
        return ConnectionResult::UnknownError;
    }

    if (!ConnectSocket(hostname, port)) {
        return ConnectionResult::HostUnreachable;
    }

    m_session = libssh2_session_init();
    if (!m_session) {
        return ConnectionResult::UnknownError;
    }

    if (libssh2_session_handshake(m_session, m_socket)) {
        return ConnectionResult::NetworkError;
    }

    ConnectionResult authResult = AuthenticatePassword(username, password);
    if (authResult != ConnectionResult::Success) {
        return authResult;
    }

    m_sftpSession = libssh2_sftp_init(m_session);
    if (!m_sftpSession) {
        return ConnectionResult::UnknownError;
    }

    m_connected = true;
    return ConnectionResult::Success;
}

void NetworkLayer::Disconnect() {
    if (m_sftpSession) {
        libssh2_sftp_shutdown(m_sftpSession);
        m_sftpSession = nullptr;
    }
    
    if (m_session) {
        libssh2_session_disconnect(m_session, "Normal Shutdown");
        libssh2_session_free(m_session);
        m_session = nullptr;
    }

    CloseSocket();
    m_connected = false;
}

bool NetworkLayer::IsConnected() const {
    return m_connected;
}

ConnectionResult NetworkLayer::AuthenticatePassword(const std::string &username, const std::string &password) {
    if (libssh2_userauth_password(m_session, username.c_str(), password.c_str())) {
        return ConnectionResult::AuthenticationFailed;
    }
    return ConnectionResult::Success;
}

bool NetworkLayer::InitializeLibrary() {
    return libssh2_init(0) == 0;
}

void NetworkLayer::CleanupLibrary() {
    libssh2_exit();
}

bool NetworkLayer::ConnectSocket(const std::string &hostname, int port) {
    struct addrinfo hints = {}, *res = nullptr;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_family = AF_INET;

    std::string portStr = std::to_string(port);
    if (getaddrinfo(hostname.c_str(), portStr.c_str(), &hints, &res) != 0) {
        return false;
    }

    m_socket = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (m_socket == -1) {
        freeaddrinfo(res);
        return false;
    }

    if (connect(m_socket, res->ai_addr, res->ai_addrlen) == -1) {
        CloseSocket();
        freeaddrinfo(res);
        return false;
    }

    freeaddrinfo(res);
    return true;
}

void NetworkLayer::CloseSocket() {
    if (m_socket != -1) {
        closesocket(m_socket);
        m_socket = -1;
    }
}

void NetworkLayer::UpdateStatus(const std::wstring &status) {
    if (m_statusCallback) {
        m_statusCallback(status);
    }
}

std::string NetworkLayer::GetLastError() const {
    return m_lastError;
}

void NetworkLayer::SetStatusCallback(StatusCallback callback) {
    m_statusCallback = callback;
}

std::string NetworkLayer::WStringToString(const std::wstring &wstr) {
    return std::string(wstr.begin(), wstr.end());
}

std::wstring NetworkLayer::StringToWString(const std::string &str) {
    return std::wstring(str.begin(), str.end());
}
