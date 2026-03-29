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
    
  private:
    bool init() override;
    void onExitTransitionDidStart() override;

    void onSignin(CCObject *sender);
    void onVerify(CCObject *sender);

    CCMenu *m_buttonMenu;
    CCLabelBMFont *m_description;
    CCMenuItemSpriteExtra *m_signinButton;
    CCMenuItemSpriteExtra *m_verifyButton;
    LoadingSpinner *m_loadingCircle;
};
