#pragma once
using namespace geode::prelude;

class GDriveSigninPopup : public Popup
{
  public:
    static GDriveSigninPopup *create();

    void showLoading();
    void showVerify();
    void showSignin();
    void finishUp();
    
    void onVerify(CCObject *sender);
  private:
    bool init() override;
    void onExitTransitionDidStart() override;
    void onClose(CCObject *sender) override;

    void onSignin(CCObject *sender);
    void onTitle(CCObject *sender);

    CCMenu* m_popupColumn;
    CCMenu *m_buttonMenu;
    CCLabelBMFont *m_title;
    CCLabelBMFont *m_description;
    CCMenuItemSpriteExtra *m_signinButton;
    CCMenuItemSpriteExtra *m_verifyButton;
    LoadingSpinner *m_loadingCircle;
};
