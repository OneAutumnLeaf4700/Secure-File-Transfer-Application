#pragma once

#include <string>
#include <functional>
#include <vector>
#include <memory>

// Forward declarations for libssh2 types
struct _LIBSSH2_SESSION;
struct _LIBSSH2_SFTP;
struct _LIBSSH2_SFTP_HANDLE;
typedef struct _LIBSSH2_SESSION LIBSSH2_SESSION;
typedef struct _LIBSSH2_SFTP LIBSSH2_SFTP;
typedef struct _LIBSSH2_SFTP_HANDLE LIBSSH2_SFTP_HANDLE;

// Network connection result codes
enum class ConnectionResult {
    Success,
    NetworkError,
    AuthenticationFailed,
    HostUnreachable,
    ConnectionTimeout,
    UnknownError
};

// File information structure
struct RemoteFileInfo {
    std::wstring fileName;
    long long fileSize;
    bool isDirectory;
    std::wstring permissions;
    std::wstring lastModified;
};

// Transfer progress callback
using ProgressCallback = std::function<void(long long bytesTransferred, long long totalBytes)>;

// Connection status callback
using StatusCallback = std::function<void(const std::wstring& status)>;

// Network layer class for secure file transfers
class NetworkLayer {
public:
    NetworkLayer();
    ~NetworkLayer();

    // Connection management
    ConnectionResult Connect(const std::string& hostname, int port, 
                           const std::string& username, const std::string& password);
    void Disconnect();
    bool IsConnected() const;

    // Authentication methods
    ConnectionResult AuthenticatePassword(const std::string& username, const std::string& password);
    ConnectionResult AuthenticatePublicKey(const std::string& username, 
                                         const std::string& publicKeyPath, 
                                         const std::string& privateKeyPath, 
                                         const std::string& passphrase = "");

    // Directory operations
    std::vector<RemoteFileInfo> ListDirectory(const std::string& remotePath = ".");
    bool CreateRemoteDirectory(const std::string& remotePath);
    bool RemoveRemoteDirectory(const std::string& remotePath);

    // File operations
    bool UploadFile(const std::wstring& localFilePath, const std::string& remoteFilePath, 
                   ProgressCallback progressCallback = nullptr);
    bool DownloadFile(const std::string& remoteFilePath, const std::wstring& localFilePath,
                     ProgressCallback progressCallback = nullptr);
    bool DeleteRemoteFile(const std::string& remoteFilePath);

    // Utility functions
    std::string GetLastError() const;
    void SetStatusCallback(StatusCallback callback);

private:
    // Initialize/cleanup libssh2
    bool InitializeLibrary();
    void CleanupLibrary();

    // Socket operations
    bool ConnectSocket(const std::string& hostname, int port);
    void CloseSocket();

    // Helper functions
    void UpdateStatus(const std::wstring& status);
    std::string WStringToString(const std::wstring& wstr);
    std::wstring StringToWString(const std::string& str);
    std::wstring FormatPermissions(unsigned long permissions);
    std::wstring FormatTime(unsigned long timestamp);

    // Member variables
    LIBSSH2_SESSION* m_session;
    LIBSSH2_SFTP* m_sftpSession;
    int m_socket;
    bool m_connected;
    bool m_libraryInitialized;
    std::string m_lastError;
    StatusCallback m_statusCallback;

    // Constants
    static const int BUFFER_SIZE = 1024 * 16; // 16KB buffer for file transfers
    static const int CONNECTION_TIMEOUT = 30; // 30 seconds timeout
};
