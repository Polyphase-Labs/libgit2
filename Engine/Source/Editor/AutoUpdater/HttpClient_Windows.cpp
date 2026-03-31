#if EDITOR && PLATFORM_WINDOWS

#include "HttpClient.h"
#include "Log.h"

#include <Windows.h>
#include <winhttp.h>
#include <fstream>
#include <vector>

#pragma comment(lib, "winhttp.lib")

bool HttpClient::sInitialized = false;
bool HttpClient::sAvailable = true; // WinHTTP is always available on Windows

bool HttpClient::InitializePlatform()
{
    sInitialized = true;
    sAvailable = true;
    return true;
}

void HttpClient::ShutdownPlatform()
{
    sInitialized = false;
}

bool HttpClient::IsAvailable()
{
    return true;
}

std::string HttpClient::GetMissingDependencyMessage()
{
    return ""; // No dependencies on Windows
}

// Helper to convert UTF-8 to wide string
static std::wstring Utf8ToWide(const std::string& str)
{
    if (str.empty()) return std::wstring();
    int size = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), nullptr, 0);
    std::wstring result(size, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), &result[0], size);
    return result;
}

// Helper to convert wide string to UTF-8
static std::string WideToUtf8(const std::wstring& wstr)
{
    if (wstr.empty()) return std::string();
    int size = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), nullptr, 0, nullptr, nullptr);
    std::string result(size, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), &result[0], size, nullptr, nullptr);
    return result;
}

HttpResponse HttpClient::Get(const std::string& url, int timeoutMs)
{
    HttpResponse response;

    // Crack the URL
    URL_COMPONENTS urlComp = {};
    urlComp.dwStructSize = sizeof(urlComp);

    wchar_t hostName[256] = {};
    wchar_t urlPath[2048] = {};
    urlComp.lpszHostName = hostName;
    urlComp.dwHostNameLength = 256;
    urlComp.lpszUrlPath = urlPath;
    urlComp.dwUrlPathLength = 2048;

    std::wstring wideUrl = Utf8ToWide(url);
    if (!WinHttpCrackUrl(wideUrl.c_str(), (DWORD)wideUrl.length(), 0, &urlComp))
    {
        response.mError = "Failed to parse URL";
        return response;
    }

    // Open session
    HINTERNET hSession = WinHttpOpen(
        L"OctaveEngine/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0);

    if (!hSession)
    {
        response.mError = "Failed to open WinHTTP session";
        return response;
    }

    // Set timeouts
    WinHttpSetTimeouts(hSession, timeoutMs, timeoutMs, timeoutMs, timeoutMs);

    // Connect
    HINTERNET hConnect = WinHttpConnect(hSession, hostName, urlComp.nPort, 0);
    if (!hConnect)
    {
        WinHttpCloseHandle(hSession);
        response.mError = "Failed to connect to server";
        return response;
    }

    // Open request
    DWORD flags = (urlComp.nScheme == INTERNET_SCHEME_HTTPS) ? WINHTTP_FLAG_SECURE : 0;
    HINTERNET hRequest = WinHttpOpenRequest(
        hConnect,
        L"GET",
        urlPath,
        nullptr,
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        flags);

    if (!hRequest)
    {
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        response.mError = "Failed to open request";
        return response;
    }

    // Add headers for GitHub API
    WinHttpAddRequestHeaders(hRequest,
        L"Accept: application/vnd.github.v3+json\r\n"
        L"User-Agent: OctaveEngine/1.0\r\n",
        (DWORD)-1, WINHTTP_ADDREQ_FLAG_ADD);

    // Send request
    if (!WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0))
    {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        response.mError = "Failed to send request";
        return response;
    }

    // Receive response
    if (!WinHttpReceiveResponse(hRequest, nullptr))
    {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        response.mError = "Failed to receive response";
        return response;
    }

    // Get status code
    DWORD statusCode = 0;
    DWORD statusCodeSize = sizeof(statusCode);
    WinHttpQueryHeaders(hRequest,
        WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
        WINHTTP_HEADER_NAME_BY_INDEX,
        &statusCode,
        &statusCodeSize,
        WINHTTP_NO_HEADER_INDEX);
    response.mStatusCode = (int)statusCode;

    // Read response body
    std::string body;
    DWORD bytesAvailable = 0;
    while (WinHttpQueryDataAvailable(hRequest, &bytesAvailable) && bytesAvailable > 0)
    {
        std::vector<char> buffer(bytesAvailable);
        DWORD bytesRead = 0;
        if (WinHttpReadData(hRequest, buffer.data(), bytesAvailable, &bytesRead))
        {
            body.append(buffer.data(), bytesRead);
        }
    }
    response.mBody = std::move(body);

    // Cleanup
    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);

    return response;
}

bool HttpClient::DownloadFile(
    const std::string& url,
    const std::string& destPath,
    DownloadProgressCallback progressCallback,
    std::atomic<bool>& cancelFlag)
{
    // Crack the URL
    URL_COMPONENTS urlComp = {};
    urlComp.dwStructSize = sizeof(urlComp);

    wchar_t hostName[256] = {};
    wchar_t urlPath[2048] = {};
    urlComp.lpszHostName = hostName;
    urlComp.dwHostNameLength = 256;
    urlComp.lpszUrlPath = urlPath;
    urlComp.dwUrlPathLength = 2048;

    std::wstring wideUrl = Utf8ToWide(url);
    if (!WinHttpCrackUrl(wideUrl.c_str(), (DWORD)wideUrl.length(), 0, &urlComp))
    {
        LogError("HttpClient: Failed to parse download URL");
        return false;
    }

    // Open session
    HINTERNET hSession = WinHttpOpen(
        L"OctaveEngine/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0);

    if (!hSession)
    {
        LogError("HttpClient: Failed to open WinHTTP session");
        return false;
    }

    // Connect
    HINTERNET hConnect = WinHttpConnect(hSession, hostName, urlComp.nPort, 0);
    if (!hConnect)
    {
        WinHttpCloseHandle(hSession);
        LogError("HttpClient: Failed to connect for download");
        return false;
    }

    // Open request
    DWORD flags = (urlComp.nScheme == INTERNET_SCHEME_HTTPS) ? WINHTTP_FLAG_SECURE : 0;
    HINTERNET hRequest = WinHttpOpenRequest(
        hConnect,
        L"GET",
        urlPath,
        nullptr,
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        flags);

    if (!hRequest)
    {
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        LogError("HttpClient: Failed to open download request");
        return false;
    }

    // Add user agent
    WinHttpAddRequestHeaders(hRequest,
        L"User-Agent: OctaveEngine/1.0\r\n",
        (DWORD)-1, WINHTTP_ADDREQ_FLAG_ADD);

    // Send request
    if (!WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0))
    {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        LogError("HttpClient: Failed to send download request");
        return false;
    }

    // Receive response
    if (!WinHttpReceiveResponse(hRequest, nullptr))
    {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        LogError("HttpClient: Failed to receive download response");
        return false;
    }

    // Check status code
    DWORD statusCode = 0;
    DWORD statusCodeSize = sizeof(statusCode);
    WinHttpQueryHeaders(hRequest,
        WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
        WINHTTP_HEADER_NAME_BY_INDEX,
        &statusCode,
        &statusCodeSize,
        WINHTTP_NO_HEADER_INDEX);

    if (statusCode != 200)
    {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        LogError("HttpClient: Download returned status %d", (int)statusCode);
        return false;
    }

    // Get content length
    DWORD contentLength = 0;
    DWORD contentLengthSize = sizeof(contentLength);
    WinHttpQueryHeaders(hRequest,
        WINHTTP_QUERY_CONTENT_LENGTH | WINHTTP_QUERY_FLAG_NUMBER,
        WINHTTP_HEADER_NAME_BY_INDEX,
        &contentLength,
        &contentLengthSize,
        WINHTTP_NO_HEADER_INDEX);

    // Open output file
    std::ofstream outFile(destPath, std::ios::binary);
    if (!outFile.is_open())
    {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        LogError("HttpClient: Failed to open output file: %s", destPath.c_str());
        return false;
    }

    // Download loop
    size_t totalDownloaded = 0;
    std::vector<char> buffer(65536); // 64KB buffer
    DWORD bytesAvailable = 0;
    bool success = true;

    while (WinHttpQueryDataAvailable(hRequest, &bytesAvailable) && bytesAvailable > 0)
    {
        if (cancelFlag.load())
        {
            success = false;
            break;
        }

        DWORD toRead = (DWORD)std::min((size_t)bytesAvailable, buffer.size());
        DWORD bytesRead = 0;

        if (WinHttpReadData(hRequest, buffer.data(), toRead, &bytesRead))
        {
            outFile.write(buffer.data(), bytesRead);
            totalDownloaded += bytesRead;

            if (progressCallback)
            {
                progressCallback(totalDownloaded, (size_t)contentLength);
            }
        }
        else
        {
            success = false;
            break;
        }
    }

    outFile.close();

    // Cleanup handles
    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);

    // Delete incomplete file on failure
    if (!success || cancelFlag.load())
    {
        DeleteFileA(destPath.c_str());
        return false;
    }

    return true;
}

#endif // EDITOR && PLATFORM_WINDOWS
