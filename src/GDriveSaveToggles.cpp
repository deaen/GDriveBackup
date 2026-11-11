#include "GDriveSaveToggles.hpp"
using namespace geode::prelude;

$execute
{
    auto ret = Mod::get()->registerCustomSettingType("gdrive-save-toggles", &GDriveSaveToggles::parse);
    if (!ret)
    {
        log::error("Unable to register setting type: {}", ret.unwrapErr());
    }
}