# Secure File Transfer Application - GUI Overview

This C++ Windows application provides a graphical user interface for secure file transfers. The application is built using the Win32 API and includes the following features:

## GUI Components

### Connection Panel
- **Server IP Input**: Text field for entering the target server's IP address
- **Port Input**: Text field for the connection port (defaults to 22 for SSH/SFTP)
- **Username Input**: Text field for authentication username
- **Password Input**: Masked text field for authentication password
- **Connect Button**: Establishes connection to the remote server
- **Disconnect Button**: Terminates the current connection

### File Management
- **Local Files List**: ListView displaying files on the local machine with columns:
  - Name: Filename
  - Size: File size
  - Type: File type description
- **Remote Files List**: ListView displaying files on the remote server (populated after connection)
  - Same column structure as local files
- **Upload Button**: Transfers selected local files to the remote server
- **Download Button**: Transfers selected remote files to the local machine

### Status and Progress
- **Progress Bar**: Shows transfer progress during file operations
- **Status Label**: Displays current connection and operation status

## Current Implementation

### Features Implemented
- Complete GUI layout with all controls positioned
- Connection state management (connected/disconnected)
- Button state management (enable/disable based on connection status)
- Sample data population for demonstration purposes
- Basic event handling for all buttons

### Sample Data
The application includes sample files for demonstration:

**Local Files (always visible):**
- document.pdf (2.3 MB, PDF File)
- image.jpg (1.8 MB, JPEG Image)
- data.txt (15 KB, Text File)

**Remote Files (visible when connected):**
- server_config.xml (5.2 KB, XML File)
- backup.zip (45.7 MB, ZIP Archive)

### TODO - Future Implementation
- Actual network connection logic (SSH/SFTP/FTP protocols)
- Real file system browsing for local files
- File upload/download functionality
- Progress tracking during transfers
- Error handling and validation
- Encryption/security features
- Logging capabilities
- Configuration management

## Building and Running

This project requires:
- Visual Studio with C++ development tools
- Windows SDK
- Common Controls library (comctl32.lib)

To build:
1. Open `Secure File Transfer Application.sln` in Visual Studio
2. Select Debug or Release configuration
3. Build the solution (F7 or Build > Build Solution)

## Architecture

The application follows a traditional Win32 architecture:
- `WinMain`: Application entry point and message loop
- `WndProc`: Main window procedure handling user input
- `InitInstance`: Window creation and GUI component initialization
- Control handles are stored as global variables for easy access

The GUI is created programmatically using Win32 API calls, providing full control over layout and appearance while maintaining compatibility with all Windows versions.
