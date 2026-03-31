#if EDITOR

#include "ReleaseInfo.h"
#include <cstdlib>
#include <cstring>

bool ReleaseInfo::ParseVersion(const std::string& version, int& outMajor, int& outMinor, int& outPatch)
{
    outMajor = 0;
    outMinor = 0;
    outPatch = 0;

    if (version.empty())
    {
        return false;
    }

    // Skip 'v' prefix if present
    const char* str = version.c_str();
    if (*str == 'v' || *str == 'V')
    {
        str++;
    }

    // Parse major
    char* end = nullptr;
    outMajor = (int)strtol(str, &end, 10);
    if (end == str)
    {
        return false; // No digits parsed
    }

    // Check for minor
    if (*end == '.')
    {
        str = end + 1;
        outMinor = (int)strtol(str, &end, 10);

        // Check for patch
        if (*end == '.')
        {
            str = end + 1;
            outPatch = (int)strtol(str, &end, 10);
        }
    }

    return true;
}

bool ReleaseInfo::IsNewerThan(const std::string& currentVersion) const
{
    int relMajor, relMinor, relPatch;
    int curMajor, curMinor, curPatch;

    std::string releaseVer = GetVersion();

    if (!ParseVersion(releaseVer, relMajor, relMinor, relPatch))
    {
        return false;
    }

    if (!ParseVersion(currentVersion, curMajor, curMinor, curPatch))
    {
        return false;
    }

    // Compare major.minor.patch
    if (relMajor != curMajor)
    {
        return relMajor > curMajor;
    }
    if (relMinor != curMinor)
    {
        return relMinor > curMinor;
    }
    return relPatch > curPatch;
}

const ReleaseAsset* ReleaseInfo::GetAssetForPlatform() const
{
    for (const ReleaseAsset& asset : mAssets)
    {
#if PLATFORM_WINDOWS
        // Look for Windows installer (.exe)
        if (asset.mName.find(".exe") != std::string::npos ||
            asset.mName.find("Setup") != std::string::npos ||
            asset.mName.find("Windows") != std::string::npos ||
            asset.mName.find("windows") != std::string::npos)
        {
            return &asset;
        }
#elif PLATFORM_LINUX
        // Prefer .deb package, fallback to tarball
        if (asset.mName.find(".deb") != std::string::npos)
        {
            return &asset;
        }
#endif
    }

#if PLATFORM_LINUX
    // Fallback: look for tarball
    for (const ReleaseAsset& asset : mAssets)
    {
        if (asset.mName.find(".tar.gz") != std::string::npos ||
            asset.mName.find("linux") != std::string::npos ||
            asset.mName.find("Linux") != std::string::npos)
        {
            return &asset;
        }
    }
#endif

    return nullptr;
}

#endif // EDITOR
