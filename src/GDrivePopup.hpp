#pragma once
#include "GDriveManager.hpp"

using namespace geode::prelude;

class GDrivePopup : public geode::Popup
{
  public:
    static GDrivePopup *create();

  private:
    bool init();
    CCNode *createSlotBox(float width, float height, int slot);
    void onSave(CCObject *sender);
    void onLoad(CCObject *sender);
    void onLogin(CCObject *sender);
    void onToggleAccountVisibility(CCObject *sender);

    std::string_view m_emailString = "deaen@gmail.com";
    CCLabelBMFont *m_emailLabel = CCLabelBMFont::create(m_emailString.data(), "bigFont.fnt");
};
