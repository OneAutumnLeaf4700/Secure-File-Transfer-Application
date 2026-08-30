#pragma once

#include <string>
#include <functional>
#include <vector>

// Forward declarations for libssh2 types
struct _LIBSSH2_SESSION;
struct _LIBSSH2_SFTP;
struct _LIBSSH2_SFTP_HANDLE;
typedef struct _LIBSSH2_SESSION LIBSSH2_SESSION;
typedef struct _LIBSSH2_SFTP LIBSSH2_SFTP;
typedef struct _LIBSSH2_SFTP_HANDLE LIBSSH2_SFTP_HANDLE;

enum class ConnectionResult {
    Success,
    NetworkError,
    AuthenticationFailed,
    HostUnreachable,
    ConnectionTimeout,
    UnknownError
};

struct RemoteFileInfo {
    std::string fileName;
    long long fileSize;
    bool isDirectory;
    std::string permissions;
    std::string lastModified;
};

using ProgressCallback = std::function<void(long long bytesTransferred, long long totalBytes)>;
using StatusCallback = std::function<void(const std::string& status)>;
using TrustHostKeyCallback = std::function<bool(const std::string& hostname, const std::string& fingerprint)>;

class NetworkLayer {
public:
    NetworkLayer();
    ~NetworkLayer();

    ConnectionResult Connect(const std::string& hostname, int port,
                           std::function<ConnectionResult()> authenticate);
    void Disconnect();
    bool IsConnected() const;

    ConnectionResult AuthenticatePassword(const std::string& username, const std::string& password);
    ConnectionResult AuthenticatePublicKey(const std::string& username,
                                         const std::string& publicKeyPath,
                                         const std::string& privateKeyPath,
                                         const std::string& passphrase = "");

    std::vector<RemoteFileInfo> ListDirectory(const std::string& remotePath = ".");
    bool IsRemoteDirectory(const std::string& remotePath);
    bool CreateRemoteDirectory(const std::string& remotePath);
    bool RemoveRemoteDirectory(const std::string& remotePath);

    bool UploadFile(const std::string& localFilePath, const std::string& remoteFilePath,
                   ProgressCallback progressCallback = nullptr);
    bool DownloadFile(const std::string& remoteFilePath, const std::string& localFilePath,
                     ProgressCallback progressCallback = nullptr);
    bool DeleteRemoteFile(const std::string& remoteFilePath);

    std::string GetLastError() const;
    void SetStatusCallback(StatusCallback callback);
    void SetTrustHostKeyCallback(TrustHostKeyCallback callback);

private:
    bool InitializeLibrary();
    void CleanupLibrary();

    bool ConnectSocket(const std::string& hostname, int port);
    void CloseSocket();

    ConnectionResult VerifyHostKey(const std::string& hostname, int port);
    std::string ComputeHostKeyFingerprint() const;

    void UpdateStatus(const std::string& status);
    std::string FormatPermissions(unsigned long permissions);
    std::string FormatTime(unsigned long timestamp);

    LIBSSH2_SESSION* m_session;
    LIBSSH2_SFTP* m_sftpSession;
    int m_socket;
    bool m_connected;
    bool m_libraryInitialized;
    std::string m_lastError;
    StatusCallback m_statusCallback;
    TrustHostKeyCallback m_trustHostKeyCallback;

    static const int BUFFER_SIZE = 1024 * 16;
    static const int CONNECTION_TIMEOUT = 30;
};
