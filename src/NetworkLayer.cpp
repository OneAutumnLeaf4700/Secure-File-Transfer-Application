#include "NetworkLayer.h"
#include <libssh2.h>
#include <libssh2_sftp.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <cstdio>
#include <cstring>
#include <ctime>

NetworkLayer::NetworkLayer() :
    m_session(nullptr),
    m_sftpSession(nullptr),
    m_socket(-1),
    m_connected(false),
    m_libraryInitialized(false) {
}

NetworkLayer::~NetworkLayer() {
    Disconnect();
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

    if (m_libraryInitialized) {
        CleanupLibrary();
        m_libraryInitialized = false;
    }
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

ConnectionResult NetworkLayer::AuthenticatePublicKey(const std::string &username,
                                                     const std::string &publicKeyPath,
                                                     const std::string &privateKeyPath,
                                                     const std::string &passphrase) {
    int rc = libssh2_userauth_publickey_fromfile(
        m_session,
        username.c_str(),
        publicKeyPath.c_str(),
        privateKeyPath.c_str(),
        passphrase.empty() ? nullptr : passphrase.c_str());

    if (rc != 0) {
        return ConnectionResult::AuthenticationFailed;
    }
    return ConnectionResult::Success;
}

bool NetworkLayer::InitializeLibrary() {
    m_libraryInitialized = (libssh2_init(0) == 0);
    return m_libraryInitialized;
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
        close(m_socket);
        m_socket = -1;
    }
}

void NetworkLayer::UpdateStatus(const std::string &status) {
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

std::vector<RemoteFileInfo> NetworkLayer::ListDirectory(const std::string& remotePath) {
    std::vector<RemoteFileInfo> fileList;

    if (!m_connected || !m_sftpSession) {
        m_lastError = "Not connected to server";
        return fileList;
    }

    UpdateStatus("Listing remote directory...");

    LIBSSH2_SFTP_HANDLE* sftpHandle = libssh2_sftp_opendir(m_sftpSession, remotePath.c_str());
    if (!sftpHandle) {
        m_lastError = "Failed to open remote directory: " + remotePath;
        UpdateStatus("Failed to list directory");
        return fileList;
    }

    char buffer[512];
    LIBSSH2_SFTP_ATTRIBUTES attrs;

    while (libssh2_sftp_readdir(sftpHandle, buffer, sizeof(buffer), &attrs) > 0) {
        std::string fileName(buffer);

        if (fileName == "." || fileName == "..") {
            continue;
        }

        RemoteFileInfo fileInfo;
        fileInfo.fileName = fileName;
        fileInfo.fileSize = attrs.filesize;
        fileInfo.isDirectory = (attrs.permissions & LIBSSH2_SFTP_S_IFDIR) != 0;
        fileInfo.permissions = FormatPermissions(attrs.permissions);
        fileInfo.lastModified = FormatTime(attrs.mtime);

        fileList.push_back(fileInfo);
    }

    libssh2_sftp_closedir(sftpHandle);

    UpdateStatus("Directory listing complete");
    return fileList;
}

bool NetworkLayer::CreateRemoteDirectory(const std::string& remotePath) {
    if (!m_connected || !m_sftpSession) {
        m_lastError = "Not connected to server";
        return false;
    }

    int result = libssh2_sftp_mkdir(m_sftpSession, remotePath.c_str(),
                                   LIBSSH2_SFTP_S_IRWXU | LIBSSH2_SFTP_S_IRGRP |
                                   LIBSSH2_SFTP_S_IXGRP | LIBSSH2_SFTP_S_IROTH |
                                   LIBSSH2_SFTP_S_IXOTH);

    if (result != 0) {
        m_lastError = "Failed to create directory: " + remotePath;
        return false;
    }

    return true;
}

bool NetworkLayer::RemoveRemoteDirectory(const std::string& remotePath) {
    if (!m_connected || !m_sftpSession) {
        m_lastError = "Not connected to server";
        return false;
    }

    int result = libssh2_sftp_rmdir(m_sftpSession, remotePath.c_str());

    if (result != 0) {
        m_lastError = "Failed to remove directory: " + remotePath;
        return false;
    }

    return true;
}

bool NetworkLayer::DeleteRemoteFile(const std::string& remoteFilePath) {
    if (!m_connected || !m_sftpSession) {
        m_lastError = "Not connected to server";
        return false;
    }

    int result = libssh2_sftp_unlink(m_sftpSession, remoteFilePath.c_str());

    if (result != 0) {
        m_lastError = "Failed to delete file: " + remoteFilePath;
        return false;
    }

    return true;
}

std::string NetworkLayer::FormatPermissions(unsigned long permissions) {
    std::string result;

    if (permissions & LIBSSH2_SFTP_S_IFDIR) result += "d";
    else if (permissions & LIBSSH2_SFTP_S_IFLNK) result += "l";
    else result += "-";

    result += (permissions & LIBSSH2_SFTP_S_IRUSR) ? "r" : "-";
    result += (permissions & LIBSSH2_SFTP_S_IWUSR) ? "w" : "-";
    result += (permissions & LIBSSH2_SFTP_S_IXUSR) ? "x" : "-";

    result += (permissions & LIBSSH2_SFTP_S_IRGRP) ? "r" : "-";
    result += (permissions & LIBSSH2_SFTP_S_IWGRP) ? "w" : "-";
    result += (permissions & LIBSSH2_SFTP_S_IXGRP) ? "x" : "-";

    result += (permissions & LIBSSH2_SFTP_S_IROTH) ? "r" : "-";
    result += (permissions & LIBSSH2_SFTP_S_IWOTH) ? "w" : "-";
    result += (permissions & LIBSSH2_SFTP_S_IXOTH) ? "x" : "-";

    return result;
}

std::string NetworkLayer::FormatTime(unsigned long timestamp) {
    if (timestamp == 0) {
        return "Unknown";
    }

    time_t rawTime = static_cast<time_t>(timestamp);
    struct tm timeInfo;

    if (localtime_r(&rawTime, &timeInfo) == nullptr) {
        return "Invalid";
    }

    char buffer[64];
    if (strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M", &timeInfo) == 0) {
        return "Error";
    }

    return std::string(buffer);
}

bool NetworkLayer::UploadFile(const std::string& localFilePath, const std::string& remoteFilePath, ProgressCallback progressCallback) {
    if (!m_connected || !m_sftpSession) {
        m_lastError = "Not connected to server";
        return false;
    }

    FILE* localFile = fopen(localFilePath.c_str(), "rb");
    if (!localFile) {
        m_lastError = "Failed to open local file: " + localFilePath;
        return false;
    }

    fseek(localFile, 0, SEEK_END);
    long long fileSize = ftell(localFile);
    fseek(localFile, 0, SEEK_SET);

    LIBSSH2_SFTP_HANDLE* sftpHandle = libssh2_sftp_open(m_sftpSession, remoteFilePath.c_str(),
                                                         LIBSSH2_FXF_WRITE | LIBSSH2_FXF_CREAT | LIBSSH2_FXF_TRUNC,
                                                         LIBSSH2_SFTP_S_IRUSR | LIBSSH2_SFTP_S_IWUSR |
                                                         LIBSSH2_SFTP_S_IRGRP | LIBSSH2_SFTP_S_IROTH);
    if (!sftpHandle) {
        fclose(localFile);
        m_lastError = "Failed to create remote file: " + remoteFilePath;
        return false;
    }

    UpdateStatus("Uploading file...");

    char buffer[BUFFER_SIZE];
    long long totalTransferred = 0;
    bool success = true;

    while (!feof(localFile)) {
        size_t bytesRead = fread(buffer, 1, sizeof(buffer), localFile);
        if (bytesRead == 0) break;

        size_t bytesWritten = 0;
        while (bytesWritten < bytesRead) {
            ssize_t result = libssh2_sftp_write(sftpHandle, buffer + bytesWritten, bytesRead - bytesWritten);
            if (result < 0) {
                success = false;
                m_lastError = "Failed to write to remote file";
                break;
            }
            bytesWritten += result;
        }

        if (!success) break;

        totalTransferred += bytesRead;

        if (progressCallback) {
            progressCallback(totalTransferred, fileSize);
        }
    }

    fclose(localFile);
    libssh2_sftp_close(sftpHandle);

    UpdateStatus(success ? "File upload completed" : "File upload failed");

    return success;
}

bool NetworkLayer::DownloadFile(const std::string& remoteFilePath, const std::string& localFilePath, ProgressCallback progressCallback) {
    if (!m_connected || !m_sftpSession) {
        m_lastError = "Not connected to server";
        return false;
    }

    LIBSSH2_SFTP_HANDLE* sftpHandle = libssh2_sftp_open(m_sftpSession, remoteFilePath.c_str(),
                                                         LIBSSH2_FXF_READ, 0);
    if (!sftpHandle) {
        m_lastError = "Failed to open remote file: " + remoteFilePath;
        return false;
    }

    LIBSSH2_SFTP_ATTRIBUTES attrs;
    if (libssh2_sftp_fstat(sftpHandle, &attrs) != 0) {
        libssh2_sftp_close(sftpHandle);
        m_lastError = "Failed to get remote file attributes";
        return false;
    }

    long long fileSize = attrs.filesize;

    FILE* localFile = fopen(localFilePath.c_str(), "wb");
    if (!localFile) {
        libssh2_sftp_close(sftpHandle);
        m_lastError = "Failed to create local file: " + localFilePath;
        return false;
    }

    UpdateStatus("Downloading file...");

    char buffer[BUFFER_SIZE];
    long long totalTransferred = 0;
    bool success = true;

    while (totalTransferred < fileSize) {
        ssize_t bytesRead = libssh2_sftp_read(sftpHandle, buffer, sizeof(buffer));
        if (bytesRead < 0) {
            success = false;
            m_lastError = "Failed to read from remote file";
            break;
        }

        if (bytesRead == 0) break;

        size_t bytesWritten = fwrite(buffer, 1, bytesRead, localFile);
        if (bytesWritten != (size_t)bytesRead) {
            success = false;
            m_lastError = "Failed to write to local file";
            break;
        }

        totalTransferred += bytesRead;

        if (progressCallback) {
            progressCallback(totalTransferred, fileSize);
        }
    }

    fclose(localFile);
    libssh2_sftp_close(sftpHandle);

    if (success) {
        UpdateStatus("File download completed");
    } else {
        UpdateStatus("File download failed");
        remove(localFilePath.c_str());
    }

    return success;
}
