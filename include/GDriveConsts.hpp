#pragma once

namespace GDriveConsts
{
    struct backupSettingValue
    {
        bool save = false;
        bool load = false;

        bool operator==(backupSettingValue const& other) const = default;
        backupSettingValue() = default;
        backupSettingValue(bool save, bool load) : save(save), load(load) {}
        backupSettingValue(backupSettingValue const&) = default;
    };
    inline constexpr std::string_view GMData = "GMData";
    inline constexpr std::string_view LLMData = "LLMData";
}