// Secure File Transfer Application.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "Secure File Transfer Application.h"
#include "NetworkLayer.h"
#include <commctrl.h>
#include <commdlg.h>
#include <shellapi.h>
#include <vector>
#include <string>
#include <shlwapi.h>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "shlwapi.lib")

#define MAX_LOADSTRING 100

// File size constants (in bytes)
#define SMALL_FILE_LIMIT    (50LL * 1024 * 1024)    // 50MB
#define MEDIUM_FILE_LIMIT   (500LL * 1024 * 1024)   // 500MB

// File transfer structure
struct FileTransferItem {
    std::wstring fileName;
    std::wstring filePath;
    LONGLONG fileSize;
    bool needsCompression;
};

// Modern light theme colors
#define COLOR_BG             RGB(250, 250, 250)   // Light gray background
#define COLOR_PANEL          RGB(240, 240, 245)   // Panel background
#define COLOR_ACCENT         RGB(0, 120, 215)     // Windows blue accent
#define COLOR_TEXT           RGB(50, 50, 50)      // Dark gray text
#define COLOR_BORDER         RGB(200, 200, 200)   // Light gray border
#define COLOR_DROPZONE       RGB(245, 248, 252)   // Light blue-gray drop zone
#define COLOR_DROPZONE_HOVER RGB(230, 240, 250)   // Hover state for drop zone
#define COLOR_EDIT_BG        RGB(255, 255, 255)   // White edit control background

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

// GUI Control Handles
HWND hMainWindow;
HWND hStatusBar;
HWND hEditServer, hEditPort, hEditUsername, hEditPassword;
HWND hBtnConnect, hBtnDisconnect;
HWND hDropZone;
HWND hListLocal, hListRemote;
HWND hProgress;
HWND hLabelServer, hLabelPort, hLabelUsername, hLabelPassword;
HWND hLabelLocal, hLabelRemote;
HWND hBtnUpload, hBtnDownload;

// Dark theme brushes
HBRUSH hBrushDarkBg, hBrushDarkerBg, hBrushDropZone;
HPEN hPenBorder;
HFONT hFontUI;

// Connection state
bool isConnected = false;
std::string currentRemotePath = "."; // Track current remote directory

// Network layer instance
std::unique_ptr<NetworkLayer> networkLayer;

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK    DropZoneProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
void                CreateMenuBar(HWND hWnd);
void                CreateToolbar(HWND hWnd);
void                CreateControls(HWND hWnd);
void                CreateStatusBar(HWND hWnd);
void                InitializeDarkTheme();
void                CleanupDarkTheme();
void                UpdateConnectionState();
void                SetWindowTheme(HWND hwnd);

// Remote directory functions
void                LoadRemoteDirectory(const std::string& remotePath = ".");
void                AddRemoteFileToList(const RemoteFileInfo& fileInfo);
std::string         GetCurrentRemotePath();

// File handling functions
std::wstring        FormatFileSize(LONGLONG size);
LONGLONG            GetFileSize(const std::wstring& filePath);
void                ProcessDroppedFiles(HWND hWnd, HDROP hDrop);
void                ProcessSelectedFiles(HWND hWnd, WCHAR* fileBuffer, WORD fileOffset);
void                AddFileToList(const FileTransferItem& item);
bool                ShouldCompressFile(LONGLONG fileSize);
INT_PTR CALLBACK    FileProcessingDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

// File transfer functions
void                UploadSelectedFiles();
void                DownloadSelectedFiles();
void                UpdateProgress(long long bytesTransferred, long long totalBytes);
void                ShowContextMenu(HWND hWnd, int x, int y, bool isRemote);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // Initialize Common Controls for modern UI elements
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_BAR_CLASSES | ICC_LISTVIEW_CLASSES | ICC_PROGRESS_CLASS;
    InitCommonControlsEx(&icex);

    // Initialize dark theme resources
    InitializeDarkTheme();

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_SECUREFILETRANSFERAPPLICATION, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        CleanupDarkTheme();
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_SECUREFILETRANSFERAPPLICATION));

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    // Cleanup dark theme resources
    CleanupDarkTheme();
    return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SECUREFILETRANSFERAPPLICATION));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_SECUREFILETRANSFERAPPLICATION);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Store instance handle in our global variable

   // Create main window with light background
   HWND hWnd = CreateWindowW(szWindowClass, L"Secure File Transfer - Modern Light Theme",
      WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, 1200, 800, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   hMainWindow = hWnd;

   // Create all GUI components
   CreateMenuBar(hWnd);
   CreateControls(hWnd);
   CreateStatusBar(hWnd);

   // Update initial connection state
   UpdateConnectionState();

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            int wmEvent = HIWORD(wParam);
            
            // Handle button clicks
            if (wmEvent == BN_CLICKED) {
                switch (wmId) {
                case IDC_BTN_CONNECT:
                    {
                        // Get connection parameters from UI
                        wchar_t server[256], port[16], username[256], password[256];
                        GetWindowText(hEditServer, server, 256);
                        GetWindowText(hEditPort, port, 16);
                        GetWindowText(hEditUsername, username, 256);
                        GetWindowText(hEditPassword, password, 256);
                        
                        // Convert to strings
                        std::string serverStr(server, server + wcslen(server));
                        std::string usernameStr(username, username + wcslen(username));
                        std::string passwordStr(password, password + wcslen(password));
                        int portNum = _wtoi(port);
                        
                        if (serverStr.empty() || usernameStr.empty() || portNum <= 0) {
                            MessageBox(hWnd, L"Please fill in all connection fields.", L"Connection Error", MB_OK | MB_ICONWARNING);
                            break;
                        }
                        
                        // Create network layer if it doesn't exist
                        if (!networkLayer) {
                            networkLayer = std::make_unique<NetworkLayer>();
                            networkLayer->SetStatusCallback([](const std::wstring& status) {
                                SendMessage(hStatusBar, SB_SETTEXT, 0, (LPARAM)status.c_str());
                            });
                        }
                        
                        // Update status
                        SendMessage(hStatusBar, SB_SETTEXT, 0, (LPARAM)L"Connecting...");
                        
                        // Attempt connection
                        ConnectionResult result = networkLayer->Connect(serverStr, portNum, usernameStr, passwordStr);
                        
                        switch (result) {
                        case ConnectionResult::Success:
                            isConnected = true;
                            UpdateConnectionState();
                            
                            // Load remote directory listing
                            LoadRemoteDirectory();
                            
                            MessageBox(hWnd, L"Successfully connected to server!", L"Connection Success", MB_OK | MB_ICONINFORMATION);
                            break;
                        case ConnectionResult::NetworkError:
                            MessageBox(hWnd, L"Network error occurred during connection.", L"Connection Error", MB_OK | MB_ICONERROR);
                            break;
                        case ConnectionResult::AuthenticationFailed:
                            MessageBox(hWnd, L"Authentication failed. Please check your credentials.", L"Authentication Error", MB_OK | MB_ICONERROR);
                            break;
                        case ConnectionResult::HostUnreachable:
                            MessageBox(hWnd, L"Cannot reach the specified host. Please check the server address and port.", L"Connection Error", MB_OK | MB_ICONERROR);
                            break;
                        case ConnectionResult::ConnectionTimeout:
                            MessageBox(hWnd, L"Connection timed out. Please try again.", L"Connection Error", MB_OK | MB_ICONERROR);
                            break;
                        default:
                            MessageBox(hWnd, L"Unknown error occurred during connection.", L"Connection Error", MB_OK | MB_ICONERROR);
                            break;
                        }
                        
                        if (result != ConnectionResult::Success) {
                            SendMessage(hStatusBar, SB_SETTEXT, 0, (LPARAM)L"Connection failed");
                        }
                    }
                    break;
                case IDC_BTN_DISCONNECT:
                    if (networkLayer && networkLayer->IsConnected()) {
                        networkLayer->Disconnect();
                        isConnected = false;
                        UpdateConnectionState();
                        
                        // Clear remote file list
                        ListView_DeleteAllItems(hListRemote);
                        
                        MessageBox(hWnd, L"Disconnected from server.", L"Disconnection", MB_OK | MB_ICONINFORMATION);
                    }
                    break;
                case IDC_DROPZONE:
                    {
                        OPENFILENAME ofn;
                        WCHAR szFile[32768] = { 0 }; // Large buffer for multiple files
                        
                        ZeroMemory(&ofn, sizeof(ofn));
                        ofn.lStructSize = sizeof(ofn);
                        ofn.hwndOwner = hWnd;
                        ofn.lpstrFile = szFile;
                        ofn.nMaxFile = sizeof(szFile) / sizeof(WCHAR);
                        ofn.lpstrFilter = L"All Files\0*.*\0Text Files\0*.txt\0Image Files\0*.jpg;*.jpeg;*.png;*.bmp;*.gif\0Document Files\0*.doc;*.docx;*.pdf;*.rtf\0";
                        ofn.nFilterIndex = 1;
                        ofn.lpstrFileTitle = NULL;
                        ofn.nMaxFileTitle = 0;
                        ofn.lpstrInitialDir = NULL;
                        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_ALLOWMULTISELECT | OFN_EXPLORER;
                        
                        if (GetOpenFileName(&ofn)) {
                            ProcessSelectedFiles(hWnd, szFile, ofn.nFileOffset);
                        }
                    }
                    break;
                case IDC_BTN_UPLOAD:
                    UploadSelectedFiles();
                    break;
                case IDC_BTN_DOWNLOAD:
                    DownloadSelectedFiles();
                    break;
                }
            }
            
            // Parse the menu selections:
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_FILE_EXIT:
                SendMessage(hWnd, WM_CLOSE, 0, 0);
                break;
            case IDM_FILE_CONNECT:
                isConnected = true;
                UpdateConnectionState();
                MessageBox(hWnd, L"Menu Connect clicked! (Functionality not implemented yet)", L"Debug", MB_OK | MB_ICONINFORMATION);
                break;
            case IDM_FILE_DISCONNECT:
                isConnected = false;
                UpdateConnectionState();
                MessageBox(hWnd, L"Menu Disconnect clicked! (Functionality not implemented yet)", L"Debug", MB_OK | MB_ICONINFORMATION);
                break;
            case IDM_EDIT_PREFERENCES:
            case IDM_VIEW_REFRESH:
            case IDM_VIEW_SHOWLOG:
            case IDM_TOOLS_OPTIONS:
            case IDM_HELP_ABOUT:
            case ID_TOOLBAR_UPLOAD:
            case ID_TOOLBAR_DOWNLOAD:
            case ID_TOOLBAR_REFRESH:
                MessageBox(hWnd, L"Feature not implemented.", L"Notice", MB_OK | MB_ICONINFORMATION);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;

    case WM_CREATE:
        {
            SetWindowTheme(hWnd);
        }
        break;

    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            FillRect(hdc, &ps.rcPaint, hBrushDarkBg);
            EndPaint(hWnd, &ps);
        }
        break;
        
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLORBTN:
        {
            HDC hdcStatic = (HDC)wParam;
            SetTextColor(hdcStatic, COLOR_TEXT);
            SetBkColor(hdcStatic, COLOR_BG);
            return (INT_PTR)hBrushDarkBg;
        }
        break;
        
    case WM_CTLCOLOREDIT:
        {
            HDC hdcEdit = (HDC)wParam;
            SetTextColor(hdcEdit, COLOR_TEXT);
            SetBkColor(hdcEdit, COLOR_EDIT_BG);
            return (INT_PTR)hBrushDarkerBg;
        }
        break;
        
    case WM_SIZE:
        {
            // Auto-resize components
            if (hStatusBar) {
                SendMessage(hStatusBar, WM_SIZE, wParam, lParam);
            }
        }
        break;
        
    case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR)lParam;
            
            // Handle ListView notifications
            if (pnmh->idFrom == IDC_LIST_REMOTE && pnmh->code == NM_DBLCLK) {
                // Double-click on remote file list - navigate if it's a directory
                LPNMITEMACTIVATE pnmia = (LPNMITEMACTIVATE)lParam;
                
                if (pnmia->iItem >= 0) {
                    // Get the selected item's text
                    wchar_t itemText[256];
                    ListView_GetItemText(hListRemote, pnmia->iItem, 0, itemText, 256);
                    
                    std::wstring itemName(itemText);
                    
                    // Check if it's the parent directory entry
                    if (itemName == L"[..]") {
                        // Navigate to parent directory
                        std::string newPath;
                        if (currentRemotePath == "." || currentRemotePath.empty()) {
                            newPath = ".";
                        } else {
                            // Find the last slash and remove everything after it
                            size_t lastSlash = currentRemotePath.find_last_of('/');
                            if (lastSlash != std::string::npos && lastSlash > 0) {
                                newPath = currentRemotePath.substr(0, lastSlash);
                            } else {
                                newPath = ".";
                            }
                        }
                        
                        // Update current path and load directory
                        currentRemotePath = newPath;
                        LoadRemoteDirectory(currentRemotePath);
                        
                        // Update the remote files label to show current path
                        std::wstring displayPath = (currentRemotePath == ".") ? L"/" : std::wstring(currentRemotePath.begin(), currentRemotePath.end());
                        std::wstring labelText = L"Remote Files: " + displayPath;
                        SetWindowText(hLabelRemote, labelText.c_str());
                    }
                    // Check if it's a directory (indicated by brackets)
                    else if (itemName.length() > 2 && itemName[0] == L'[' && itemName.back() == L']') {
                        // Extract directory name without brackets
                        std::wstring dirName = itemName.substr(1, itemName.length() - 2);
                        
                        // Convert to std::string for path construction
                        std::string dirNameA(dirName.begin(), dirName.end());
                        
                        // Construct new path
                        std::string newPath;
                        if (currentRemotePath == "." || currentRemotePath.empty()) {
                            newPath = dirNameA;
                        } else {
                            newPath = currentRemotePath + "/" + dirNameA;
                        }
                        
                        // Update current path and load directory
                        currentRemotePath = newPath;
                        LoadRemoteDirectory(currentRemotePath);
                        
                        // Update the remote files label to show current path
                        std::wstring labelText = L"Remote Files: " + std::wstring(currentRemotePath.begin(), currentRemotePath.end());
                        SetWindowText(hLabelRemote, labelText.c_str());
                    }
                }
            }
            else if (pnmh->idFrom == IDC_LIST_REMOTE && pnmh->code == NM_RCLICK) {
                // Right-click on remote file list - show context menu
                // TODO: Implement context menu for remote files
            }
        }
        break;
        
    case WM_DROPFILES:
        {
            HDROP hDrop = (HDROP)wParam;
            ProcessDroppedFiles(hWnd, hDrop);
            DragFinish(hDrop);
        }
        break;

    case WM_DESTROY:
        DragAcceptFiles(hWnd, FALSE);
        PostQuitMessage(0);
        break;
    
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

//
//  FUNCTION: InitializeDarkTheme()
//  PURPOSE: Initialize brushes, pens, and fonts for dark theme
//
void InitializeDarkTheme()
{
    // Create light theme brushes
    hBrushDarkBg = CreateSolidBrush(COLOR_BG);
    hBrushDarkerBg = CreateSolidBrush(COLOR_EDIT_BG);
    hBrushDropZone = CreateSolidBrush(COLOR_DROPZONE);
    
    // Create border pen
    hPenBorder = CreatePen(PS_SOLID, 1, COLOR_BORDER);
    
    // Create UI font (Segoe UI for modern look)
hFontUI = CreateFont(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS,
        ANTIALIASED_QUALITY, VARIABLE_PITCH, L"Segoe UI");
}

//
//  FUNCTION: CleanupDarkTheme()
//  PURPOSE: Clean up theme resources
//
void CleanupDarkTheme()
{
    if (hBrushDarkBg) DeleteObject(hBrushDarkBg);
    if (hBrushDarkerBg) DeleteObject(hBrushDarkerBg);
    if (hBrushDropZone) DeleteObject(hBrushDropZone);
    if (hPenBorder) DeleteObject(hPenBorder);
    if (hFontUI) DeleteObject(hFontUI);
}

//
//  FUNCTION: SetWindowTheme()
//  PURPOSE: Apply dark theme to window
//
void SetWindowTheme(HWND hwnd)
{
    // Set light background
    SetClassLongPtr(hwnd, GCLP_HBRBACKGROUND, (LONG_PTR)CreateSolidBrush(COLOR_BG));
}

//
//  FUNCTION: CreateMenuBar()
//  PURPOSE: Create the modern menu bar
//
void CreateMenuBar(HWND hWnd)
{
    HMENU hMenuBar = CreateMenu();
    
    // File Menu
    HMENU hFileMenu = CreatePopupMenu();
    AppendMenu(hFileMenu, MF_STRING, IDM_FILE_CONNECT, L"&Connect...");
    AppendMenu(hFileMenu, MF_STRING, IDM_FILE_DISCONNECT, L"&Disconnect");
    AppendMenu(hFileMenu, MF_SEPARATOR, 0, NULL);
    AppendMenu(hFileMenu, MF_STRING, IDM_FILE_EXIT, L"E&xit");
    AppendMenu(hMenuBar, MF_POPUP, (UINT_PTR)hFileMenu, L"&File");
    
    // Edit Menu
    HMENU hEditMenu = CreatePopupMenu();
    AppendMenu(hEditMenu, MF_STRING, IDM_EDIT_PREFERENCES, L"&Preferences...");
    AppendMenu(hMenuBar, MF_POPUP, (UINT_PTR)hEditMenu, L"&Edit");
    
    // View Menu
    HMENU hViewMenu = CreatePopupMenu();
    AppendMenu(hViewMenu, MF_STRING, IDM_VIEW_REFRESH, L"&Refresh");
    AppendMenu(hViewMenu, MF_SEPARATOR, 0, NULL);
    AppendMenu(hViewMenu, MF_STRING, IDM_VIEW_SHOWLOG, L"Show &Log");
    AppendMenu(hMenuBar, MF_POPUP, (UINT_PTR)hViewMenu, L"&View");
    
    // Tools Menu
    HMENU hToolsMenu = CreatePopupMenu();
    AppendMenu(hToolsMenu, MF_STRING, IDM_TOOLS_OPTIONS, L"&Options...");
    AppendMenu(hMenuBar, MF_POPUP, (UINT_PTR)hToolsMenu, L"&Tools");
    
    // Help Menu
    HMENU hHelpMenu = CreatePopupMenu();
    AppendMenu(hHelpMenu, MF_STRING, IDM_HELP_ABOUT, L"&About...");
    AppendMenu(hMenuBar, MF_POPUP, (UINT_PTR)hHelpMenu, L"&Help");
    
    SetMenu(hWnd, hMenuBar);
}

//
//  FUNCTION: CreateStatusBar()
//  PURPOSE: Create the status bar
//
void CreateStatusBar(HWND hWnd)
{
    hStatusBar = CreateWindowEx(0, STATUSCLASSNAME, NULL,
        WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP,
        0, 0, 0, 0, hWnd, NULL, hInst, NULL);
    
    // Set status bar font
    SendMessage(hStatusBar, WM_SETFONT, (WPARAM)hFontUI, MAKELPARAM(TRUE, 0));
    
    // Set initial status
    SendMessage(hStatusBar, SB_SETTEXT, 0, (LPARAM)L"Ready - Not Connected");
}

//
//  FUNCTION: CreateControls()
//  PURPOSE: Create all the GUI controls with dark theme
//
void CreateControls(HWND hWnd)
{
    RECT clientRect;
    GetClientRect(hWnd, &clientRect);
    
    int toolbarHeight = 0;
    int statusBarHeight = 25;
    int workAreaHeight = clientRect.bottom - toolbarHeight - statusBarHeight;
    
    // Connection Panel (Top Left)
    int panelY = toolbarHeight + 10;
    
    hLabelServer = CreateWindow(L"STATIC", L"Server:",
        WS_VISIBLE | WS_CHILD | SS_RIGHT,
        20, panelY, 80, 20, hWnd, (HMENU)IDC_LABEL_SERVER, hInst, NULL);
    
    hEditServer = CreateWindow(L"EDIT", L"192.168.1.100",
        WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
        110, panelY, 150, 22, hWnd, (HMENU)IDC_EDIT_SERVER, hInst, NULL);
    
    hLabelPort = CreateWindow(L"STATIC", L"Port:",
        WS_VISIBLE | WS_CHILD | SS_RIGHT,
        280, panelY, 40, 20, hWnd, (HMENU)IDC_LABEL_PORT, hInst, NULL);
    
    hEditPort = CreateWindow(L"EDIT", L"22",
        WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL | ES_NUMBER,
        330, panelY, 60, 22, hWnd, (HMENU)IDC_EDIT_PORT, hInst, NULL);
    
    panelY += 35;
    
    hLabelUsername = CreateWindow(L"STATIC", L"Username:",
        WS_VISIBLE | WS_CHILD | SS_RIGHT,
        20, panelY, 80, 20, hWnd, (HMENU)IDC_LABEL_USERNAME, hInst, NULL);
    
    hEditUsername = CreateWindow(L"EDIT", L"admin",
        WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
        110, panelY, 150, 22, hWnd, (HMENU)IDC_EDIT_USERNAME, hInst, NULL);
    
    hLabelPassword = CreateWindow(L"STATIC", L"Password:",
        WS_VISIBLE | WS_CHILD | SS_RIGHT,
        280, panelY, 60, 20, hWnd, (HMENU)IDC_LABEL_PASSWORD, hInst, NULL);
    
    hEditPassword = CreateWindow(L"EDIT", L"",
        WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL | ES_PASSWORD,
        350, panelY, 120, 22, hWnd, (HMENU)IDC_EDIT_PASSWORD, hInst, NULL);
    
    panelY += 35;
    
    hBtnConnect = CreateWindow(L"BUTTON", L"Connect",
        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        110, panelY, 80, 25, hWnd, (HMENU)IDC_BTN_CONNECT, hInst, NULL);
    
    hBtnDisconnect = CreateWindow(L"BUTTON", L"Disconnect",
        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        200, panelY, 80, 25, hWnd, (HMENU)IDC_BTN_DISCONNECT, hInst, NULL);
    
    // Large Drag & Drop Zone (Center)
    int dropZoneY = panelY + 50;
    int dropZoneHeight = 200;
    
    hDropZone = CreateWindow(L"STATIC", L"Drag & Drop Files Here\r\n\r\nOr click to browse files",
        WS_VISIBLE | WS_CHILD | SS_CENTER | SS_NOTIFY | WS_BORDER,
        50, dropZoneY, clientRect.right - 100, dropZoneHeight,
        hWnd, (HMENU)IDC_DROPZONE, hInst, NULL);
    
    // Enable drag and drop for the main window
    DragAcceptFiles(hWnd, TRUE);
    
    // File Lists (Bottom)
    int listY = dropZoneY + dropZoneHeight + 20;
    int listWidth = (clientRect.right - 80) / 2;
    
    hLabelLocal = CreateWindow(L"STATIC", L"Local Files:",
        WS_VISIBLE | WS_CHILD,
        20, listY, 100, 20, hWnd, (HMENU)IDC_LABEL_LOCAL, hInst, NULL);
    
    hLabelRemote = CreateWindow(L"STATIC", L"Remote Files:",
        WS_VISIBLE | WS_CHILD,
        40 + listWidth, listY, 100, 20, hWnd, (HMENU)IDC_LABEL_REMOTE, hInst, NULL);
    
    listY += 25;
    
    hListLocal = CreateWindow(WC_LISTVIEW, L"",
        WS_VISIBLE | WS_CHILD | WS_BORDER | LVS_REPORT | LVS_SINGLESEL,
        20, listY, listWidth, 150, hWnd, (HMENU)IDC_LIST_LOCAL, hInst, NULL);
    
    hListRemote = CreateWindow(WC_LISTVIEW, L"",
        WS_VISIBLE | WS_CHILD | WS_BORDER | LVS_REPORT | LVS_SINGLESEL,
        40 + listWidth, listY, listWidth, 150, hWnd, (HMENU)IDC_LIST_REMOTE, hInst, NULL);
    
    // Upload/Download buttons (between lists and progress bar)
    int buttonY = listY + 160;
    int buttonCenterX = clientRect.right / 2;
    
    hBtnUpload = CreateWindow(L"BUTTON", L"Upload →",
        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        buttonCenterX - 120, buttonY, 100, 30, hWnd, (HMENU)IDC_BTN_UPLOAD, hInst, NULL);
    
    hBtnDownload = CreateWindow(L"BUTTON", L"← Download",
        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        buttonCenterX + 20, buttonY, 100, 30, hWnd, (HMENU)IDC_BTN_DOWNLOAD, hInst, NULL);
    
    // Progress Bar (Bottom)
    int progressY = buttonY + 40;
    
    hProgress = CreateWindow(PROGRESS_CLASS, NULL,
        WS_VISIBLE | WS_CHILD,
        20, progressY, clientRect.right - 40, 20,
        hWnd, (HMENU)IDC_PROGRESS, hInst, NULL);
    
    // Apply fonts to all controls
    HWND controls[] = {
        hLabelServer, hEditServer, hLabelPort, hEditPort,
        hLabelUsername, hEditUsername, hLabelPassword, hEditPassword,
        hBtnConnect, hBtnDisconnect, hDropZone,
        hLabelLocal, hLabelRemote, hListLocal, hListRemote,
        hBtnUpload, hBtnDownload
    };
    
    for (int i = 0; i < sizeof(controls) / sizeof(HWND); i++) {
        if (controls[i]) {
            SendMessage(controls[i], WM_SETFONT, (WPARAM)hFontUI, MAKELPARAM(TRUE, 0));
        }
    }
    
    // Setup ListView columns
    LVCOLUMN lvc;
    lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
    
    // Local files columns
    lvc.iSubItem = 0;
    lvc.pszText = const_cast<LPWSTR>(L"Name");
    lvc.cx = 200;
    ListView_InsertColumn(hListLocal, 0, &lvc);
    
    lvc.iSubItem = 1;
    lvc.pszText = const_cast<LPWSTR>(L"Size");
    lvc.cx = 100;
    ListView_InsertColumn(hListLocal, 1, &lvc);
    
    lvc.iSubItem = 2;
    lvc.pszText = const_cast<LPWSTR>(L"Type");
    lvc.cx = 100;
    ListView_InsertColumn(hListLocal, 2, &lvc);
    
    // Remote files columns (same as local)
    lvc.iSubItem = 0;
    lvc.pszText = const_cast<LPWSTR>(L"Name");
    lvc.cx = 200;
    ListView_InsertColumn(hListRemote, 0, &lvc);
    
    lvc.iSubItem = 1;
    lvc.pszText = const_cast<LPWSTR>(L"Size");
    lvc.cx = 100;
    ListView_InsertColumn(hListRemote, 1, &lvc);
    
    lvc.iSubItem = 2;
    lvc.pszText = const_cast<LPWSTR>(L"Type");
    lvc.cx = 100;
    ListView_InsertColumn(hListRemote, 2, &lvc);
}

//
//  FUNCTION: UpdateConnectionState()
//  PURPOSE: Update UI based on connection state
//
void UpdateConnectionState()
{
    // Enable/disable controls based on connection state
    EnableWindow(hBtnConnect, !isConnected);
    EnableWindow(hBtnDisconnect, isConnected);
    
    // Update status bar
    if (hStatusBar) {
        const wchar_t* status = isConnected ? L"Connected" : L"Ready - Not Connected";
        SendMessage(hStatusBar, SB_SETTEXT, 0, (LPARAM)status);
    }
}

//
//  FUNCTION: FormatFileSize()
//  PURPOSE: Format file size in human-readable format
//
std::wstring FormatFileSize(LONGLONG size)
{
    const wchar_t* units[] = { L"B", L"KB", L"MB", L"GB", L"TB" };
    int unitIndex = 0;
    double dSize = (double)size;
    
    while (dSize >= 1024.0 && unitIndex < 4) {
        dSize /= 1024.0;
        unitIndex++;
    }
    
    wchar_t buffer[64];
    if (unitIndex == 0) {
        swprintf_s(buffer, L"%.0f %s", dSize, units[unitIndex]);
    } else {
        swprintf_s(buffer, L"%.1f %s", dSize, units[unitIndex]);
    }
    
    return std::wstring(buffer);
}

//
//  FUNCTION: GetFileSize()
//  PURPOSE: Get file size for a given file path
//
LONGLONG GetFileSize(const std::wstring& filePath)
{
    // First try GetFileAttributesEx (faster, no handle needed)
    WIN32_FILE_ATTRIBUTE_DATA fileInfo;
    if (GetFileAttributesEx(filePath.c_str(), GetFileExInfoStandard, &fileInfo)) {
        // Check if it's a directory
        if (fileInfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            return 0; // Directories have no size
        }
        
        LARGE_INTEGER fileSize;
        fileSize.HighPart = fileInfo.nFileSizeHigh;
        fileSize.LowPart = fileInfo.nFileSizeLow;
        return fileSize.QuadPart;
    }
    
    // Fallback: try opening the file directly
    HANDLE hFile = CreateFile(filePath.c_str(), 
                             GENERIC_READ, 
                             FILE_SHARE_READ | FILE_SHARE_WRITE, 
                             NULL, 
                             OPEN_EXISTING, 
                             FILE_ATTRIBUTE_NORMAL, 
                             NULL);
    
    if (hFile != INVALID_HANDLE_VALUE) {
        LARGE_INTEGER fileSize;
        if (GetFileSizeEx(hFile, &fileSize)) {
            CloseHandle(hFile);
            return fileSize.QuadPart;
        }
        CloseHandle(hFile);
    }
    
    return 0; // Could not get file size
}

//
//  FUNCTION: ShouldCompressFile()
//  PURPOSE: Determine if file should be compressed based on size
//
bool ShouldCompressFile(LONGLONG fileSize)
{
    return fileSize > SMALL_FILE_LIMIT;
}

//
//  FUNCTION: ProcessDroppedFiles()
//  PURPOSE: Process files dropped onto the window
//
void ProcessDroppedFiles(HWND hWnd, HDROP hDrop)
{
    UINT fileCount = DragQueryFile(hDrop, 0xFFFFFFFF, NULL, 0);
    if (fileCount == 0) return;
    
    std::vector<FileTransferItem> droppedFiles;
    LONGLONG totalSize = 0;
    
    // Process each dropped file
    for (UINT i = 0; i < fileCount; i++) {
        wchar_t filePath[MAX_PATH];
        UINT pathLength = DragQueryFile(hDrop, i, filePath, MAX_PATH);
        
        if (pathLength > 0) {
            FileTransferItem item;
            item.filePath = filePath;
            
            // Extract filename from path
            wchar_t* fileName = PathFindFileName(filePath);
            item.fileName = fileName;
            
            // Get file size
            item.fileSize = GetFileSize(filePath);
            item.needsCompression = ShouldCompressFile(item.fileSize);
            
            droppedFiles.push_back(item);
            totalSize += item.fileSize;
        }
    }
    
    if (droppedFiles.empty()) {
        MessageBox(hWnd, L"No valid files were dropped.", L"Drag & Drop", MB_OK | MB_ICONWARNING);
        return;
    }
    
    // Update status bar
    wchar_t statusMsg[256];
    swprintf_s(statusMsg, L"Processing %d file(s) - Total size: %s", 
               fileCount, FormatFileSize(totalSize).c_str());
    SendMessage(hStatusBar, SB_SETTEXT, 0, (LPARAM)statusMsg);
    
    // Check if any files need compression
    bool hasLargeFiles = false;
    for (const auto& item : droppedFiles) {
        if (item.needsCompression) {
            hasLargeFiles = true;
            break;
        }
    }
    
    // Show processing dialog if there are large files or multiple files
    if (hasLargeFiles || fileCount > 1) {
        wchar_t message[512];
        if (hasLargeFiles && fileCount > 1) {
            swprintf_s(message, L"You've dropped %d files (Total: %s).\n\n"
                       L"Some files are large and may benefit from compression.\n\n"
                       L"Options:\n"
                       L"• Transfer files as-is\n"
                       L"• Compress large files individually\n"
                       L"• Compress all files into a single archive",
                       fileCount, FormatFileSize(totalSize).c_str());
        } else if (hasLargeFiles) {
            swprintf_s(message, L"Large file detected: %s (%s)\n\n"
                       L"Would you like to compress this file before transfer?",
                       droppedFiles[0].fileName.c_str(), 
                       FormatFileSize(droppedFiles[0].fileSize).c_str());
        } else {
            swprintf_s(message, L"Multiple files dropped (%d files, %s).\n\n"
                       L"Would you like to compress them into a single archive?",
                       fileCount, FormatFileSize(totalSize).c_str());
        }
        
        int result = MessageBox(hWnd, message, L"File Processing Options", 
                               MB_YESNOCANCEL | MB_ICONQUESTION);
        
        switch (result) {
        case IDYES:
            // User wants compression - for now, just add files to list
            for (const auto& item : droppedFiles) {
                AddFileToList(item);
            }
            SendMessage(hStatusBar, SB_SETTEXT, 0, 
                       (LPARAM)L"Files added to transfer queue (compression will be implemented)");
            break;
            
        case IDNO:
            // Transfer as-is
            for (auto& item : droppedFiles) {
                item.needsCompression = false;
                AddFileToList(item);
            }
            SendMessage(hStatusBar, SB_SETTEXT, 0, 
                       (LPARAM)L"Files added to transfer queue (no compression)");
            break;
            
        case IDCANCEL:
            SendMessage(hStatusBar, SB_SETTEXT, 0, (LPARAM)L"File drop cancelled");
            return;
        }
    } else {
        // Single small file - add directly
        AddFileToList(droppedFiles[0]);
        SendMessage(hStatusBar, SB_SETTEXT, 0, (LPARAM)L"File added to transfer queue");
    }
}

//
//  FUNCTION: ProcessSelectedFiles()
//  PURPOSE: Process files selected from file browser dialog
//
void ProcessSelectedFiles(HWND hWnd, WCHAR* fileBuffer, WORD fileOffset)
{
    std::vector<FileTransferItem> selectedFiles;
    LONGLONG totalSize = 0;
    
    if (fileOffset == 0) {
        // Single file selected - the entire buffer is just the file path
        FileTransferItem item;
        item.filePath = std::wstring(fileBuffer);
        
        // Extract filename from path
        wchar_t* fileName = PathFindFileName(fileBuffer);
        item.fileName = std::wstring(fileName);
        
        // Get file size
        item.fileSize = GetFileSize(item.filePath);
        item.needsCompression = ShouldCompressFile(item.fileSize);
        
        selectedFiles.push_back(item);
        totalSize += item.fileSize;
    } else {
        // Multiple files selected - format is: "directory\0file1\0file2\0...\0\0"
        std::wstring directory(fileBuffer);
        WCHAR* fileName = fileBuffer + fileOffset;
        
        while (*fileName) {
            FileTransferItem item;
            
            // Construct full path
            item.filePath = directory + L"\\" + std::wstring(fileName);
            item.fileName = std::wstring(fileName);
            
            // Get file size
            item.fileSize = GetFileSize(item.filePath);
            item.needsCompression = ShouldCompressFile(item.fileSize);
            
            selectedFiles.push_back(item);
            totalSize += item.fileSize;
            
            // Move to next filename
            fileName += wcslen(fileName) + 1;
        }
    }
    
    if (selectedFiles.empty()) {
        SendMessage(hStatusBar, SB_SETTEXT, 0, (LPARAM)L"No valid files were selected.");
        return;
    }
    
    UINT fileCount = (UINT)selectedFiles.size();
    
    // Update status bar
    wchar_t statusMsg[256];
    swprintf_s(statusMsg, L"Selected %d file(s) - Total size: %s", 
               fileCount, FormatFileSize(totalSize).c_str());
    SendMessage(hStatusBar, SB_SETTEXT, 0, (LPARAM)statusMsg);
    
    // Check if any files need compression
    bool hasLargeFiles = false;
    for (const auto& item : selectedFiles) {
        if (item.needsCompression) {
            hasLargeFiles = true;
            break;
        }
    }
    
    // Show processing dialog if there are large files or multiple files
    if (hasLargeFiles || fileCount > 1) {
        wchar_t message[512];
        if (hasLargeFiles && fileCount > 1) {
            swprintf_s(message, L"You've selected %d files (Total: %s).\n\n"
                       L"Some files are large and may benefit from compression.\n\n"
                       L"Options:\n"
                       L"• Transfer files as-is\n"
                       L"• Compress large files individually\n"
                       L"• Compress all files into a single archive",
                       fileCount, FormatFileSize(totalSize).c_str());
        } else if (hasLargeFiles) {
            swprintf_s(message, L"Large file selected: %s (%s)\n\n"
                       L"Would you like to compress this file before transfer?",
                       selectedFiles[0].fileName.c_str(), 
                       FormatFileSize(selectedFiles[0].fileSize).c_str());
        } else {
            swprintf_s(message, L"Multiple files selected (%d files, %s).\n\n"
                       L"Would you like to compress them into a single archive?",
                       fileCount, FormatFileSize(totalSize).c_str());
        }
        
        int result = MessageBox(hWnd, message, L"File Processing Options", 
                               MB_YESNOCANCEL | MB_ICONQUESTION);
        
        switch (result) {
        case IDYES:
            // User wants compression - for now, just add files to list
            for (const auto& item : selectedFiles) {
                AddFileToList(item);
            }
            SendMessage(hStatusBar, SB_SETTEXT, 0, 
                       (LPARAM)L"Files added to transfer queue (compression will be implemented)");
            break;
            
        case IDNO:
            // Transfer as-is
            for (auto& item : selectedFiles) {
                item.needsCompression = false;
                AddFileToList(item);
            }
            SendMessage(hStatusBar, SB_SETTEXT, 0, 
                       (LPARAM)L"Files added to transfer queue (no compression)");
            break;
            
        case IDCANCEL:
            SendMessage(hStatusBar, SB_SETTEXT, 0, (LPARAM)L"File selection cancelled");
            return;
        }
    } else {
        // Single small file - add directly
        AddFileToList(selectedFiles[0]);
        SendMessage(hStatusBar, SB_SETTEXT, 0, (LPARAM)L"File added to transfer queue");
    }
}

//
//  FUNCTION: AddFileToList()
//  PURPOSE: Add file to the local files ListView
//
void AddFileToList(const FileTransferItem& item)
{
    LVITEM lvi = { 0 };
    lvi.mask = LVIF_TEXT;
    lvi.iItem = ListView_GetItemCount(hListLocal);
    lvi.iSubItem = 0;
    lvi.pszText = const_cast<LPWSTR>(item.fileName.c_str());
    
    int index = ListView_InsertItem(hListLocal, &lvi);
    
    if (index >= 0) {
        // Add file size
        std::wstring sizeStr = FormatFileSize(item.fileSize);
        if (item.needsCompression) {
            sizeStr += L" (compress)";
        }
        ListView_SetItemText(hListLocal, index, 1, const_cast<LPWSTR>(sizeStr.c_str()));
        
        // Add file type
        wchar_t* ext = PathFindExtension(item.fileName.c_str());
        std::wstring typeStr = ext && wcslen(ext) > 1 ? ext + 1 : L"File";
        ListView_SetItemText(hListLocal, index, 2, const_cast<LPWSTR>(typeStr.c_str()));
    }
}

//
//  FUNCTION: LoadRemoteDirectory()
//  PURPOSE: Load and display remote directory contents
//
void LoadRemoteDirectory(const std::string& remotePath)
{
    if (!networkLayer || !networkLayer->IsConnected()) {
        SendMessage(hStatusBar, SB_SETTEXT, 0, (LPARAM)L"Not connected to server");
        return;
    }
    
    // Clear existing remote file list
    ListView_DeleteAllItems(hListRemote);
    
    // Update status
    SendMessage(hStatusBar, SB_SETTEXT, 0, (LPARAM)L"Loading remote directory...");
    
    // Add "Parent Directory" entry if we're not in the root directory
    if (remotePath != "." && !remotePath.empty() && remotePath != "/") {
        RemoteFileInfo parentDir;
        parentDir.fileName = L"..";
        parentDir.fileSize = 0;
        parentDir.isDirectory = true;
        parentDir.permissions = L"drwxr-xr-x";
        parentDir.lastModified = L"";
        AddRemoteFileToList(parentDir);
    }
    
    // Get remote file listing
    std::vector<RemoteFileInfo> fileList = networkLayer->ListDirectory(remotePath);
    
    if (fileList.empty() && (remotePath == "." || remotePath.empty() || remotePath == "/")) {
        // Check if there was an error
        std::string lastError = networkLayer->GetLastError();
        if (!lastError.empty()) {
            // Convert error to wide string and show it
            std::wstring errorMsg = L"Error loading remote directory: " + 
                                   std::wstring(lastError.begin(), lastError.end());
            SendMessage(hStatusBar, SB_SETTEXT, 0, (LPARAM)errorMsg.c_str());
            MessageBox(hMainWindow, errorMsg.c_str(), L"Remote Directory Error", MB_OK | MB_ICONERROR);
        } else {
            SendMessage(hStatusBar, SB_SETTEXT, 0, (LPARAM)L"Remote directory is empty");
        }
        return;
    }
    
    // Add each file to the remote files list
    for (const auto& fileInfo : fileList) {
        AddRemoteFileToList(fileInfo);
    }
    
    // Update status with file count
    wchar_t statusMsg[256];
    int totalItems = (int)fileList.size();
    if (remotePath != "." && !remotePath.empty() && remotePath != "/") {
        totalItems += 1; // Count the parent directory entry
    }
    swprintf_s(statusMsg, L"Loaded %d items from remote directory", totalItems);
    SendMessage(hStatusBar, SB_SETTEXT, 0, (LPARAM)statusMsg);
}

//
//  FUNCTION: AddRemoteFileToList()
//  PURPOSE: Add a remote file to the remote files ListView
//
void AddRemoteFileToList(const RemoteFileInfo& fileInfo)
{
    LVITEM lvi = { 0 };
    lvi.mask = LVIF_TEXT;
    lvi.iItem = ListView_GetItemCount(hListRemote);
    lvi.iSubItem = 0;
    
    // Add directory indicator to filename if it's a directory
    std::wstring displayName = fileInfo.fileName;
    if (fileInfo.isDirectory) {
        displayName = L"[" + fileInfo.fileName + L"]"; // Indicate directory with brackets
    }
    
    lvi.pszText = const_cast<LPWSTR>(displayName.c_str());
    
    int index = ListView_InsertItem(hListRemote, &lvi);
    
    if (index >= 0) {
        // Add file size (only for files, not directories)
        std::wstring sizeStr;
        if (fileInfo.isDirectory) {
            sizeStr = L"<DIR>";
        } else {
            sizeStr = FormatFileSize(fileInfo.fileSize);
        }
        ListView_SetItemText(hListRemote, index, 1, const_cast<LPWSTR>(sizeStr.c_str()));
        
        // Add file type or permissions
        std::wstring typeStr;
        if (fileInfo.isDirectory) {
            typeStr = L"Folder";
        } else {
            // Extract file extension for type
            size_t dotPos = fileInfo.fileName.find_last_of(L'.');
            if (dotPos != std::wstring::npos && dotPos < fileInfo.fileName.length() - 1) {
                typeStr = fileInfo.fileName.substr(dotPos + 1);
            } else {
                typeStr = L"File";
            }
        }
        ListView_SetItemText(hListRemote, index, 2, const_cast<LPWSTR>(typeStr.c_str()));
    }
}

//
//  FUNCTION: GetCurrentRemotePath()
//  PURPOSE: Get the current remote directory path
//
std::string GetCurrentRemotePath()
{
    return currentRemotePath;
}

//
//  FUNCTION: UpdateProgress()
//  PURPOSE: Update progress bar with transfer progress
//
void UpdateProgress(long long bytesTransferred, long long totalBytes)
{
    if (totalBytes > 0) {
        int percentage = (int)((bytesTransferred * 100) / totalBytes);
        SendMessage(hProgress, PBM_SETPOS, percentage, 0);
        
        // Update status with progress info
        std::wstring transferredStr = FormatFileSize(bytesTransferred);
        std::wstring totalStr = FormatFileSize(totalBytes);
        
        wchar_t statusMsg[256];
        swprintf_s(statusMsg, L"Transferring: %s / %s (%d%%)", 
                   transferredStr.c_str(), totalStr.c_str(), percentage);
        SendMessage(hStatusBar, SB_SETTEXT, 0, (LPARAM)statusMsg);
    }
}

//
//  FUNCTION: UploadSelectedFiles()
//  PURPOSE: Upload files from local ListView to remote server
//
void UploadSelectedFiles()
{
    if (!networkLayer || !networkLayer->IsConnected()) {
        MessageBox(hMainWindow, L"Not connected to server.", L"Upload Error", MB_OK | MB_ICONWARNING);
        return;
    }
    
    int itemCount = ListView_GetItemCount(hListLocal);
    if (itemCount == 0) {
        MessageBox(hMainWindow, L"No files to upload. Please add files to the local files list first.", L"Upload Error", MB_OK | MB_ICONWARNING);
        return;
    }
    
    // Set progress bar range
    SendMessage(hProgress, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
    SendMessage(hProgress, PBM_SETPOS, 0, 0);
    
    int successCount = 0;
    int failureCount = 0;
    
    // Process each file in the local files list
    for (int i = 0; i < itemCount; i++) {
        wchar_t fileName[256];
        ListView_GetItemText(hListLocal, i, 0, fileName, 256);
        
        // Get the full file path (stored in the item data or construct it)
        // For now, we'll need to find the file path from the original dropped files
        // This is a simplified approach - in a full implementation, we'd store the full path
        
        // Skip files that don't exist or can't be found
        // For demo purposes, let's assume files are in current directory
        std::wstring localPath = std::wstring(fileName);
        std::string remotePath = currentRemotePath + "/" + std::string(fileName, fileName + wcslen(fileName));
        
        // Upload the file with progress callback
        bool success = networkLayer->UploadFile(localPath, remotePath, UpdateProgress);
        
        if (success) {
            successCount++;
        } else {
            failureCount++;
            
            // Show error for this file
            std::string error = networkLayer->GetLastError();
            std::wstring errorMsg = L"Failed to upload " + std::wstring(fileName) + L":\n" + 
                                   std::wstring(error.begin(), error.end());
            MessageBox(hMainWindow, errorMsg.c_str(), L"Upload Error", MB_OK | MB_ICONERROR);
        }
    }
    
    // Reset progress bar
    SendMessage(hProgress, PBM_SETPOS, 0, 0);
    
    // Show summary
    wchar_t summaryMsg[256];
    swprintf_s(summaryMsg, L"Upload complete!\n\nSuccessful: %d\nFailed: %d", successCount, failureCount);
    MessageBox(hMainWindow, summaryMsg, L"Upload Summary", MB_OK | MB_ICONINFORMATION);
    
    // Refresh remote directory to show uploaded files
    if (successCount > 0) {
        LoadRemoteDirectory(currentRemotePath);
    }
}

//
//  FUNCTION: DownloadSelectedFiles()
//  PURPOSE: Download selected files from remote server to local directory
//
void DownloadSelectedFiles()
{
    if (!networkLayer || !networkLayer->IsConnected()) {
        MessageBox(hMainWindow, L"Not connected to server.", L"Download Error", MB_OK | MB_ICONWARNING);
        return;
    }
    
    // Get selected item from remote files list
    int selectedItem = ListView_GetNextItem(hListRemote, -1, LVNI_SELECTED);
    if (selectedItem == -1) {
        MessageBox(hMainWindow, L"Please select a file from the remote files list to download.", L"Download Error", MB_OK | MB_ICONWARNING);
        return;
    }
    
    // Get the selected file name
    wchar_t itemText[256];
    ListView_GetItemText(hListRemote, selectedItem, 0, itemText, 256);
    
    std::wstring itemName(itemText);
    
    // Skip directories
    if (itemName.length() > 2 && itemName[0] == L'[' && itemName.back() == L']') {
        MessageBox(hMainWindow, L"Cannot download directories. Please select a file.", L"Download Error", MB_OK | MB_ICONWARNING);
        return;
    }
    
    // Ask user where to save the file
    OPENFILENAME ofn;
    wchar_t szFile[MAX_PATH] = { 0 };
    wcscpy_s(szFile, itemName.c_str()); // Default filename
    
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hMainWindow;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile) / sizeof(wchar_t);
    ofn.lpstrFilter = L"All Files\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
    
    if (!GetSaveFileName(&ofn)) {
        return; // User cancelled
    }
    
    // Set progress bar range
    SendMessage(hProgress, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
    SendMessage(hProgress, PBM_SETPOS, 0, 0);
    
    // Construct remote file path
    std::string remoteFilePath;
    if (currentRemotePath == ".") {
        remoteFilePath = std::string(itemName.begin(), itemName.end());
    } else {
        remoteFilePath = currentRemotePath + "/" + std::string(itemName.begin(), itemName.end());
    }
    
    std::wstring localFilePath = std::wstring(szFile);
    
    // Download the file with progress callback
    bool success = networkLayer->DownloadFile(remoteFilePath, localFilePath, UpdateProgress);
    
    // Reset progress bar
    SendMessage(hProgress, PBM_SETPOS, 0, 0);
    
    if (success) {
        MessageBox(hMainWindow, L"File downloaded successfully!", L"Download Complete", MB_OK | MB_ICONINFORMATION);
    } else {
        std::string error = networkLayer->GetLastError();
        std::wstring errorMsg = L"Failed to download file:\n" + std::wstring(error.begin(), error.end());
        MessageBox(hMainWindow, errorMsg.c_str(), L"Download Error", MB_OK | MB_ICONERROR);
    }
}
