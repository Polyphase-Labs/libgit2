#pragma once

#if EDITOR

#include <string>
#include <vector>
#include <cstdint>

/**
 * @brief Represents a downloadable asset from a GitHub release.
 */
struct ReleaseAsset
{
    std::string mName;              // e.g., "PolyphaseSetup-6.0.0.exe"
    std::string mDownloadUrl;       // Direct download URL
    std::string mContentType;       // e.g., "application/octet-stream"
    size_t mSize = 0;               // File size in bytes
};

/**
 * @brief Represents a GitHub release with version info and changelog.
 */
struct ReleaseInfo
{
    std::string mTagName;           // e.g., "v6.0.0"
    std::string mName;              // Release title, e.g., "Release 6.0.0"
    std::string mBody;              // Changelog/release notes (markdown)
    std::string mHtmlUrl;           // URL to GitHub release page
    std::string mPublishedAt;       // ISO timestamp
    std::vector<ReleaseAsset> mAssets;

    /**
     * @brief Extract the version string from tag name (strips 'v' prefix).
     * @return Version string like "6.0.0"
     */
    std::string GetVersion() const
    {
        if (!mTagName.empty() && mTagName[0] == 'v')
        {
            return mTagName.substr(1);
        }
        return mTagName;
    }

    /**
     * @brief Compare this release version against the current version.
     * @param currentVersion The current version string (e.g., "5.0.0")
     * @return true if this release is newer than currentVersion
     */
    bool IsNewerThan(const std::string& currentVersion) const;

    /**
     * @brief Get the appropriate download asset for the current platform.
     * @return Pointer to the asset, or nullptr if not found
     */
    const ReleaseAsset* GetAssetForPlatform() const;

    /**
     * @brief Parse version string into major, minor, patch components.
     * @param version Version string like "6.0.0" or "6.1"
     * @param outMajor Output major version
     * @param outMinor Output minor version
     * @param outPatch Output patch version
     * @return true if parsing succeeded
     */
    static bool ParseVersion(const std::string& version, int& outMajor, int& outMinor, int& outPatch);
};

#endif // EDITOR
