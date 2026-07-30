#pragma once

#include <string>
#include <thread>
#include <winhttp.h>
#include "../rbx/globals/options.h"

#pragma comment(lib, "winhttp.lib")

inline std::string EscapeJSON(const std::string& s)
{
    std::string out;
    for (char c : s)
    {
        switch (c)
        {
        case '"': out += "\\\""; break;
        case '\\': out += "\\\\"; break;
        case '\n': out += "\\n"; break;
        case '\r': out += "\\r"; break;
        case '\t': out += "\\t"; break;
        default: out += c;
        }
    }
    return out;
}

inline void SendWebhookAsync(const std::string& message, int type = 0)
{
    if (!Options::Misc::WebhookEnabled || Options::Misc::WebhookURL.empty())
        return;

    std::string url = Options::Misc::WebhookURL;
    std::thread([url, message, type]()
    {
        std::string prefix;
        switch (type)
        {
        case 1: prefix = "[+] "; break;
        case 2: prefix = "[-] "; break;
        default: prefix = "[*] "; break;
        }

        std::string json = "{\"content\":\"" + EscapeJSON(prefix + message) + "\"}";

        URL_COMPONENTSW urlComp = { sizeof(urlComp) };
        urlComp.dwSchemeLength = (DWORD)-1;
        urlComp.dwHostNameLength = (DWORD)-1;
        urlComp.dwUrlPathLength = (DWORD)-1;

        int wlen = MultiByteToWideChar(CP_UTF8, 0, url.c_str(), -1, nullptr, 0);
        std::wstring wurl(wlen, 0);
        MultiByteToWideChar(CP_UTF8, 0, url.c_str(), -1, &wurl[0], wlen);

        WinHttpCrackUrl(wurl.c_str(), 0, 0, &urlComp);

        std::wstring host(urlComp.lpszHostName, urlComp.dwHostNameLength);
        std::wstring path(urlComp.lpszUrlPath, urlComp.dwUrlPathLength);

        HINTERNET hSession = WinHttpOpen(L"Armedium/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, nullptr, nullptr, 0);
        if (hSession)
        {
            HINTERNET hConnect = WinHttpConnect(hSession, host.c_str(), urlComp.nPort, 0);
            if (hConnect)
            {
                DWORD flags = WINHTTP_FLAG_REFRESH;
                if (urlComp.nScheme == INTERNET_SCHEME_HTTPS)
                    flags |= WINHTTP_FLAG_SECURE;

                HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"POST", path.c_str(), nullptr, nullptr, nullptr, flags);
                if (hRequest)
                {
                    LPCWSTR headers = L"Content-Type: application/json\r\n";
                    WinHttpSendRequest(hRequest, headers, -1, (LPVOID)json.c_str(), (DWORD)json.size(), (DWORD)json.size(), 0);
                    WinHttpReceiveResponse(hRequest, nullptr);
                    WinHttpCloseHandle(hRequest);
                }
                WinHttpCloseHandle(hConnect);
            }
            WinHttpCloseHandle(hSession);
        }
    }).detach();
}
