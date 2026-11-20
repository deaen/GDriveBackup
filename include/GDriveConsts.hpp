#pragma once

namespace GDriveConsts
{
    struct backupSettingValue
    {
        bool save = false;
        bool load = false;
        bool saveToggleEnabled = true;
        bool loadToggleEnabled = true;
        bool operator==(backupSettingValue const& other) const = default;
        backupSettingValue() = default;
        backupSettingValue(bool save, bool load) : save(save), load(load) {}
        backupSettingValue(backupSettingValue const&) = default;
    };
    
    inline constexpr std::string_view GMData = "GMData";
    inline constexpr std::string_view LLMData = "LLMData";
    inline constexpr std::string_view platformerSaves = "platformerSaves";
    inline constexpr std::string_view gameOptions = "gameOptions";
    inline constexpr std::string_view keybinds = "keybinds";
    inline constexpr std::string_view editorBackups = "editorBackups";

}