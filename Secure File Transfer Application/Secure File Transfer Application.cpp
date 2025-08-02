// Secure File Transfer Application.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "Secure File Transfer Application.h"
#include <commctrl.h>
#include <commdlg.h>
#include <shellapi.h>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shell32.lib")

#define MAX_LOADSTRING 100

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
HWND hToolbar;
HWND hStatusBar;
HWND hEditServer, hEditPort, hEditUsername, hEditPassword;
HWND hBtnConnect, hBtnDisconnect;
HWND hDropZone;
HWND hListLocal, hListRemote;
HWND hProgress;
HWND hLabelServer, hLabelPort, hLabelUsername, hLabelPassword;
HWND hLabelLocal, hLabelRemote;

// Dark theme brushes
HBRUSH hBrushDarkBg, hBrushDarkerBg, hBrushDropZone;
HPEN hPenBorder;
HFONT hFontUI;

// Connection state
bool isConnected = false;

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
   CreateToolbar(hWnd);
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
                    isConnected = true;
                    UpdateConnectionState();
                    MessageBox(hWnd, L"Connect button clicked! (Functionality not implemented yet)", L"Debug", MB_OK | MB_ICONINFORMATION);
                    break;
                case IDC_BTN_DISCONNECT:
                    isConnected = false;
                    UpdateConnectionState();
                    MessageBox(hWnd, L"Disconnect button clicked! (Functionality not implemented yet)", L"Debug", MB_OK | MB_ICONINFORMATION);
                    break;
                case IDC_DROPZONE:
                    {
                        OPENFILENAME ofn;
                        WCHAR szFile[260] = { 0 };
                        
                        ZeroMemory(&ofn, sizeof(ofn));
                        ofn.lStructSize = sizeof(ofn);
                        ofn.hwndOwner = hWnd;
                        ofn.lpstrFile = szFile;
                        ofn.nMaxFile = sizeof(szFile);
                        ofn.lpstrFilter = L"All Files\0*.*\0";
                        ofn.nFilterIndex = 1;
                        ofn.lpstrFileTitle = NULL;
                        ofn.nMaxFileTitle = 0;
                        ofn.lpstrInitialDir = NULL;
                        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
                        
                        if (GetOpenFileName(&ofn)) {
                            MessageBox(hWnd, szFile, L"File Selected", MB_OK | MB_ICONINFORMATION);
                        }
                    }
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
            // Auto-resize toolbar and status bar
            if (hToolbar) {
                SendMessage(hToolbar, TB_AUTOSIZE, 0, 0);
            }
            if (hStatusBar) {
                SendMessage(hStatusBar, WM_SIZE, wParam, lParam);
            }
        }
        break;

    case WM_DESTROY:
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
//  FUNCTION: CreateToolbar()
//  PURPOSE: Create the modern toolbar
//
void CreateToolbar(HWND hWnd)
{
    hToolbar = CreateWindowEx(0, TOOLBARCLASSNAME, NULL,
        WS_CHILD | WS_VISIBLE | TBSTYLE_FLAT | TBSTYLE_LIST,
        0, 0, 0, 0, hWnd, NULL, hInst, NULL);
    
    // Set toolbar font
    SendMessage(hToolbar, WM_SETFONT, (WPARAM)hFontUI, MAKELPARAM(TRUE, 0));
    
    // Define toolbar buttons
    TBBUTTON tbButtons[] = {
        {0, IDM_FILE_CONNECT, TBSTATE_ENABLED, TBSTYLE_BUTTON, {0}, 0, (INT_PTR)L"Connect"},
        {1, IDM_FILE_DISCONNECT, TBSTATE_ENABLED, TBSTYLE_BUTTON, {0}, 0, (INT_PTR)L"Disconnect"},
        {0, 0, 0, TBSTYLE_SEP, {0}, 0, 0},
        {2, ID_TOOLBAR_UPLOAD, TBSTATE_ENABLED, TBSTYLE_BUTTON, {0}, 0, (INT_PTR)L"Upload"},
        {3, ID_TOOLBAR_DOWNLOAD, TBSTATE_ENABLED, TBSTYLE_BUTTON, {0}, 0, (INT_PTR)L"Download"},
        {0, 0, 0, TBSTYLE_SEP, {0}, 0, 0},
        {4, ID_TOOLBAR_REFRESH, TBSTATE_ENABLED, TBSTYLE_BUTTON, {0}, 0, (INT_PTR)L"Refresh"}
    };
    
    SendMessage(hToolbar, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
    SendMessage(hToolbar, TB_ADDBUTTONS, sizeof(tbButtons) / sizeof(TBBUTTON), (LPARAM)tbButtons);
    SendMessage(hToolbar, TB_AUTOSIZE, 0, 0);
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
    
    int toolbarHeight = 40;
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
    
    // Progress Bar (Bottom)
    int progressY = listY + 160;
    
    hProgress = CreateWindow(PROGRESS_CLASS, NULL,
        WS_VISIBLE | WS_CHILD,
        20, progressY, clientRect.right - 40, 20,
        hWnd, (HMENU)IDC_PROGRESS, hInst, NULL);
    
    // Apply fonts to all controls
    HWND controls[] = {
        hLabelServer, hEditServer, hLabelPort, hEditPort,
        hLabelUsername, hEditUsername, hLabelPassword, hEditPassword,
        hBtnConnect, hBtnDisconnect, hDropZone,
        hLabelLocal, hLabelRemote, hListLocal, hListRemote
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
