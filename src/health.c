#include "health.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef _WIN32
#include <curl/curl.h>
#endif

#ifdef _WIN32
#include <windows.h>
#include <winhttp.h>
#pragma comment(lib, "winhttp.lib")
#define SLEEP_MS(x) Sleep(x)
#else
#include <unistd.h>
#define SLEEP_MS(x) usleep((x)*1000)
#endif

static size_t write_dummy(void *ptr, size_t size, size_t nmemb, void *data) {
    (void)ptr; (void)data;
    return size * nmemb;
}

int wait_for_health(const char *url, int timeout_ms) {
#ifdef _WIN32
    // Basic WinHTTP GET to avoid libcurl dependency on Windows
    int elapsed = 0;
    const int interval = 500;

    // Parse URL into components (expecting http://host:port/path)
    URL_COMPONENTS uc;
    memset(&uc, 0, sizeof(uc));
    uc.dwStructSize = sizeof(uc);

    wchar_t wurl[1024];
    int wlen = MultiByteToWideChar(CP_UTF8, 0, url, -1, wurl, (int)(sizeof(wurl)/sizeof(wurl[0])));
    if (wlen <= 0) return -1;

    wchar_t host[256] = {0};
    wchar_t path[512] = {0};
    uc.lpszHostName = host; uc.dwHostNameLength = (DWORD)(sizeof(host)/sizeof(host[0]));
    uc.lpszUrlPath  = path; uc.dwUrlPathLength  = (DWORD)(sizeof(path)/sizeof(path[0]));

    if (!WinHttpCrackUrl(wurl, 0, 0, &uc)) {
        return -1;
    }

    HINTERNET hSession = WinHttpOpen(L"unitml/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) return -1;

    HINTERNET hConnect = WinHttpConnect(hSession, uc.lpszHostName, uc.nPort ? uc.nPort : 80, 0);
    if (!hConnect) { WinHttpCloseHandle(hSession); return -1; }

    while (elapsed < timeout_ms) {
        HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", uc.lpszUrlPath && uc.lpszUrlPath[0] ? uc.lpszUrlPath : L"/", NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, (uc.nScheme == INTERNET_SCHEME_HTTPS) ? WINHTTP_FLAG_SECURE : 0);
        if (!hRequest) break;

        BOOL ok = WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0);
        if (ok) ok = WinHttpReceiveResponse(hRequest, NULL);

        DWORD status = 0; DWORD slen = sizeof(status);
        if (ok && WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &status, &slen, WINHTTP_NO_HEADER_INDEX)) {
            WinHttpCloseHandle(hRequest);
            if (status == 200) { WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); return 0; }
        } else {
            WinHttpCloseHandle(hRequest);
        }

        SLEEP_MS(interval);
        elapsed += interval;
    }

    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    return -1;
#else
    CURL *curl = curl_easy_init();
    if (!curl) return -1;

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 2L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_dummy);
    curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);

    int elapsed = 0;
    const int interval = 500;

    while (elapsed < timeout_ms) {
        CURLcode res = curl_easy_perform(curl);
        if (res == CURLE_OK) {
            long response_code = 0;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
            if (response_code == 200) {
                curl_easy_cleanup(curl);
                return 0;
            }
        }

        SLEEP_MS(interval);
        elapsed += interval;
    }

    curl_easy_cleanup(curl);
    return -1;
#endif
}
