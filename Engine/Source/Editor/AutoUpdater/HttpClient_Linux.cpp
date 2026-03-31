#if EDITOR && PLATFORM_LINUX

#include "HttpClient.h"
#include "Log.h"

#include <dlfcn.h>
#include <fstream>
#include <cstring>

// libcurl type definitions
typedef void CURL;
typedef int CURLcode;
typedef int CURLoption;
typedef int CURLINFO;

#define CURLE_OK 0
#define CURLOPT_URL 10002
#define CURLOPT_WRITEFUNCTION 20011
#define CURLOPT_WRITEDATA 10001
#define CURLOPT_TIMEOUT_MS 155
#define CURLOPT_USERAGENT 10018
#define CURLOPT_HTTPHEADER 10023
#define CURLOPT_FOLLOWLOCATION 52
#define CURLOPT_NOPROGRESS 43
#define CURLOPT_XFERINFOFUNCTION 20219
#define CURLOPT_XFERINFODATA 10057
#define CURLINFO_RESPONSE_CODE 0x200002

struct curl_slist;

// Function pointer types
typedef CURL* (*curl_easy_init_func)(void);
typedef void (*curl_easy_cleanup_func)(CURL*);
typedef CURLcode (*curl_easy_setopt_func)(CURL*, CURLoption, ...);
typedef CURLcode (*curl_easy_perform_func)(CURL*);
typedef CURLcode (*curl_easy_getinfo_func)(CURL*, CURLINFO, ...);
typedef const char* (*curl_easy_strerror_func)(CURLcode);
typedef curl_slist* (*curl_slist_append_func)(curl_slist*, const char*);
typedef void (*curl_slist_free_all_func)(curl_slist*);
typedef CURLcode (*curl_global_init_func)(long);
typedef void (*curl_global_cleanup_func)(void);

// Global function pointers
static curl_easy_init_func p_curl_easy_init = nullptr;
static curl_easy_cleanup_func p_curl_easy_cleanup = nullptr;
static curl_easy_setopt_func p_curl_easy_setopt = nullptr;
static curl_easy_perform_func p_curl_easy_perform = nullptr;
static curl_easy_getinfo_func p_curl_easy_getinfo = nullptr;
static curl_easy_strerror_func p_curl_easy_strerror = nullptr;
static curl_slist_append_func p_curl_slist_append = nullptr;
static curl_slist_free_all_func p_curl_slist_free_all = nullptr;
static curl_global_init_func p_curl_global_init = nullptr;
static curl_global_cleanup_func p_curl_global_cleanup = nullptr;

static void* sCurlHandle = nullptr;

bool HttpClient::sInitialized = false;
bool HttpClient::sAvailable = false;

bool HttpClient::InitializePlatform()
{
    if (sInitialized)
    {
        return sAvailable;
    }

    sInitialized = true;
    sAvailable = false;

    // Try to load libcurl
    sCurlHandle = dlopen("libcurl.so.4", RTLD_LAZY);
    if (!sCurlHandle)
    {
        // Try alternate names
        sCurlHandle = dlopen("libcurl.so", RTLD_LAZY);
    }
    if (!sCurlHandle)
    {
        sCurlHandle = dlopen("libcurl-gnutls.so.4", RTLD_LAZY);
    }

    if (!sCurlHandle)
    {
        LogWarning("HttpClient: libcurl not found. Auto-updates disabled.");
        LogWarning("HttpClient: Install with: sudo apt install libcurl4");
        return false;
    }

    // Load function pointers
    p_curl_easy_init = (curl_easy_init_func)dlsym(sCurlHandle, "curl_easy_init");
    p_curl_easy_cleanup = (curl_easy_cleanup_func)dlsym(sCurlHandle, "curl_easy_cleanup");
    p_curl_easy_setopt = (curl_easy_setopt_func)dlsym(sCurlHandle, "curl_easy_setopt");
    p_curl_easy_perform = (curl_easy_perform_func)dlsym(sCurlHandle, "curl_easy_perform");
    p_curl_easy_getinfo = (curl_easy_getinfo_func)dlsym(sCurlHandle, "curl_easy_getinfo");
    p_curl_easy_strerror = (curl_easy_strerror_func)dlsym(sCurlHandle, "curl_easy_strerror");
    p_curl_slist_append = (curl_slist_append_func)dlsym(sCurlHandle, "curl_slist_append");
    p_curl_slist_free_all = (curl_slist_free_all_func)dlsym(sCurlHandle, "curl_slist_free_all");
    p_curl_global_init = (curl_global_init_func)dlsym(sCurlHandle, "curl_global_init");
    p_curl_global_cleanup = (curl_global_cleanup_func)dlsym(sCurlHandle, "curl_global_cleanup");

    if (!p_curl_easy_init || !p_curl_easy_cleanup || !p_curl_easy_setopt ||
        !p_curl_easy_perform || !p_curl_easy_getinfo)
    {
        LogError("HttpClient: Failed to load libcurl functions");
        dlclose(sCurlHandle);
        sCurlHandle = nullptr;
        return false;
    }

    // Initialize curl
    if (p_curl_global_init)
    {
        p_curl_global_init(3); // CURL_GLOBAL_ALL
    }

    sAvailable = true;
    LogDebug("HttpClient: libcurl loaded successfully");
    return true;
}

void HttpClient::ShutdownPlatform()
{
    if (sCurlHandle)
    {
        if (p_curl_global_cleanup)
        {
            p_curl_global_cleanup();
        }
        dlclose(sCurlHandle);
        sCurlHandle = nullptr;
    }
    sInitialized = false;
    sAvailable = false;
}

bool HttpClient::IsAvailable()
{
    if (!sInitialized)
    {
        InitializePlatform();
    }
    return sAvailable;
}

std::string HttpClient::GetMissingDependencyMessage()
{
    if (IsAvailable())
    {
        return "";
    }
    return "libcurl is required for auto-updates.\n\nInstall with:\n  sudo apt install libcurl4";
}

// Write callback for curl
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* output)
{
    size_t totalSize = size * nmemb;
    output->append((char*)contents, totalSize);
    return totalSize;
}

// Write callback for file download
struct FileWriteData
{
    std::ofstream* file;
    size_t* downloaded;
    size_t total;
    DownloadProgressCallback* callback;
    std::atomic<bool>* cancelFlag;
};

static size_t FileWriteCallback(void* contents, size_t size, size_t nmemb, FileWriteData* data)
{
    if (data->cancelFlag && data->cancelFlag->load())
    {
        return 0; // Abort
    }

    size_t totalSize = size * nmemb;
    data->file->write((char*)contents, totalSize);
    *(data->downloaded) += totalSize;

    if (data->callback && *data->callback)
    {
        (*data->callback)(*(data->downloaded), data->total);
    }

    return totalSize;
}

HttpResponse HttpClient::Get(const std::string& url, int timeoutMs)
{
    HttpResponse response;

    if (!IsAvailable())
    {
        response.mError = GetMissingDependencyMessage();
        return response;
    }

    CURL* curl = p_curl_easy_init();
    if (!curl)
    {
        response.mError = "Failed to initialize curl";
        return response;
    }

    std::string responseData;

    p_curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    p_curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, (void*)WriteCallback);
    p_curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseData);
    p_curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, (long)timeoutMs);
    p_curl_easy_setopt(curl, CURLOPT_USERAGENT, "OctaveEngine/1.0");
    p_curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    // Add headers for GitHub API
    curl_slist* headers = nullptr;
    if (p_curl_slist_append)
    {
        headers = p_curl_slist_append(headers, "Accept: application/vnd.github.v3+json");
        p_curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    }

    CURLcode res = p_curl_easy_perform(curl);

    if (res == CURLE_OK)
    {
        long httpCode = 0;
        p_curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
        response.mStatusCode = (int)httpCode;
        response.mBody = std::move(responseData);
    }
    else
    {
        if (p_curl_easy_strerror)
        {
            response.mError = p_curl_easy_strerror(res);
        }
        else
        {
            response.mError = "Curl error code: " + std::to_string(res);
        }
    }

    if (headers && p_curl_slist_free_all)
    {
        p_curl_slist_free_all(headers);
    }

    p_curl_easy_cleanup(curl);
    return response;
}

bool HttpClient::DownloadFile(
    const std::string& url,
    const std::string& destPath,
    DownloadProgressCallback progressCallback,
    std::atomic<bool>& cancelFlag)
{
    if (!IsAvailable())
    {
        LogError("HttpClient: %s", GetMissingDependencyMessage().c_str());
        return false;
    }

    CURL* curl = p_curl_easy_init();
    if (!curl)
    {
        LogError("HttpClient: Failed to initialize curl for download");
        return false;
    }

    std::ofstream outFile(destPath, std::ios::binary);
    if (!outFile.is_open())
    {
        p_curl_easy_cleanup(curl);
        LogError("HttpClient: Failed to open output file: %s", destPath.c_str());
        return false;
    }

    size_t downloaded = 0;
    FileWriteData writeData;
    writeData.file = &outFile;
    writeData.downloaded = &downloaded;
    writeData.total = 0;
    writeData.callback = &progressCallback;
    writeData.cancelFlag = &cancelFlag;

    p_curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    p_curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, (void*)FileWriteCallback);
    p_curl_easy_setopt(curl, CURLOPT_WRITEDATA, &writeData);
    p_curl_easy_setopt(curl, CURLOPT_USERAGENT, "OctaveEngine/1.0");
    p_curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    p_curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);

    CURLcode res = p_curl_easy_perform(curl);
    outFile.close();

    bool success = (res == CURLE_OK);

    if (!success)
    {
        if (p_curl_easy_strerror)
        {
            LogError("HttpClient: Download failed: %s", p_curl_easy_strerror(res));
        }
    }

    // Check status code
    if (success)
    {
        long httpCode = 0;
        p_curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
        if (httpCode != 200)
        {
            LogError("HttpClient: Download returned status %ld", httpCode);
            success = false;
        }
    }

    p_curl_easy_cleanup(curl);

    // Delete incomplete file on failure
    if (!success || cancelFlag.load())
    {
        std::remove(destPath.c_str());
        return false;
    }

    return true;
}

#endif // EDITOR && PLATFORM_LINUX
