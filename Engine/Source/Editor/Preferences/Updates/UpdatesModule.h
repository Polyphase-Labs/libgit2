#pragma once

#if EDITOR

#include "../PreferencesModule.h"
#include <string>
#include <cstdint>

class UpdatesModule : public PreferencesModule
{
public:
    DECLARE_PREFERENCES_MODULE(UpdatesModule)

    UpdatesModule();
    virtual ~UpdatesModule();

    virtual const char* GetName() const override { return GetStaticName(); }
    virtual const char* GetParentPath() const override { return GetStaticParentPath(); }
    virtual void Render() override;
    virtual void LoadSettings(const rapidjson::Document& doc) override;
    virtual void SaveSettings(rapidjson::Document& doc) override;

    bool GetCheckOnStartup() const { return mCheckOnStartup; }
    int GetCheckIntervalHours() const { return mCheckIntervalHours; }
    int64_t GetLastCheckTime() const { return mLastCheckTime; }
    const std::string& GetSkippedVersion() const { return mSkippedVersion; }

    void SetCheckOnStartup(bool enabled);
    void SetCheckIntervalHours(int hours);
    void SetLastCheckTime(int64_t time);
    void SetSkippedVersion(const std::string& version);
    void ClearSkippedVersion();

    static UpdatesModule* Get();

private:
    bool mCheckOnStartup = true;
    int mCheckIntervalHours = 24;
    int64_t mLastCheckTime = 0;
    std::string mSkippedVersion;

    static UpdatesModule* sInstance;
};

#endif
