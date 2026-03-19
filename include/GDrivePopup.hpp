#pragma once
using namespace geode::prelude;
#include "GDriveSlotBox.hpp"

class GDrivePopup : public Popup
{
  public:
    static GDrivePopup *create();

  private:
    bool init() override;
    void onExitTransitionDidStart() override;
    void onToggleAccountVisibility(CCObject *sender);
    void onNameInfo(CCObject *sender);
    void onSettings(CCObject *sender);
    void onSlotPageLeft(CCObject *sender);
    void onSlotPageRight(CCObject *sender);
    void onPageButton(CCObject *sender);
    void onSignOut(CCObject *sender);
    void setupEmail();
    void showSlotPage(int pageNumber);
    

    static constexpr int m_slotsPerPage = 3;
    int m_maxSlotPage = 0;
    int m_slotCount = 0;
    int m_currentSlotPage = 1;

    std::vector<GDriveSlotBox *> m_slotBoxes;
    CCMenu* m_pageButtonsRow;
    CCNode* m_slotRow;
    CCMenuItemSpriteExtra *m_leftArrowButton;
    CCMenuItemSpriteExtra *m_rightArrowButton;

    CCLabelBMFont *m_emailLabel;
    CCMenuItemSpriteExtra *m_emailButton;
    CCMenuItemSpriteExtra *m_hideEmailButton;
    std::string m_email;
    std::string m_emailCensored;
    bool m_emailVisible = true;
};
