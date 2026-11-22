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
    
    enum class slotType
    {
        New,
        Old,
    };
    struct slotInformation
    {
        GDriveConsts::slotType type;
        bool GMData = false;
        bool LLMData = false;
        bool platformerSaves = false;
        bool gameOptions= false;
        bool keybinds = false;
        bool editorBackups = false;
    };

    inline constexpr std::string_view platformerSaves = "platformerSaves";
    inline constexpr std::string_view gameOptions = "gameOptions";
    inline constexpr std::string_view keybinds = "keybinds";
    inline constexpr std::string_view editorBackups = "editorBackups";

    inline constexpr std::string_view auto_backups = "auto_backups";
}