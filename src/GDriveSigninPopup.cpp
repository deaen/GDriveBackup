#include "GDriveSigninPopup.hpp"

#include "GDriveManager.hpp"
#include "GDrivePopup.hpp"

GDriveSigninPopup *GDriveSigninPopup::create()
{
    auto ret = new GDriveSigninPopup();
    if (ret->init())
    {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}

bool GDriveSigninPopup::init()
{
    if (!Popup::init(300.f, 145.f, "square01_001.png"))
        return false;
    GDriveManager::getInstance()->setCurrentSigninPopup(this);
    this->setID("gdrive-sign-in-popup"_spr);
    this->setTitle("GDrive Backup Setup", "goldFont.fnt", 0.8f, 22.f);

    /* Popup Column */
    auto popupColumn = CCMenu::create();
    popupColumn->setLayout(ColumnLayout::create()
                               ->setAxisReverse(true)
                               ->setAxisAlignment(AxisAlignment::Start)
                               ->setGap(6.f)
                               ->setAutoScale(false));
    popupColumn->setContentSize({m_mainLayer->getContentWidth() - 20.f, m_mainLayer->getContentHeight() - 50.f});
    popupColumn->setAnchorPoint({0.5f, 0});
    popupColumn->setID("popup-column"_spr);

    /*Description label*/
    m_description = CCLabelBMFont::create(
        "Welcome to GDrive Backup!\nPlease click the button below to open the browser and sign in!", "chatFont.fnt",
        popupColumn->getContentWidth() - 20.f, CCTextAlignment::kCCTextAlignmentCenter);
    m_description->setID("description"_spr);
    popupColumn->addChild(m_description);
    /*Anchored Buttons Menu */
    m_buttonMenu = CCMenu::create();
    m_buttonMenu->setContentWidth(m_mainLayer->getContentWidth());
    m_buttonMenu->setID("button-menu"_spr);
    m_buttonMenu->setAnchorPoint({0.5f, 0.5f});
    m_buttonMenu->setLayout(RowLayout::create()->setAxisAlignment(AxisAlignment::Center));

    /* Mod Settings Button */
    m_signinButton = CCMenuItemSpriteExtra::create(ButtonSprite::create("Sign in"), this,
                                                   menu_selector(GDriveSigninPopup::onSignin));
    m_signinButton->setID("signin-button"_spr);
    m_buttonMenu->addChild(m_signinButton);

    m_verifyButton =
        CCMenuItemSpriteExtra::create(ButtonSprite::create("Verify"), this, menu_selector(GDriveSigninPopup::onVerify));
    m_verifyButton->setID("verify-button"_spr);
    m_verifyButton->setVisible(false);
    m_buttonMenu->addChild(m_verifyButton);

    m_buttonMenu->updateLayout();
    popupColumn->addChild(m_buttonMenu);
    popupColumn->updateLayout();
    m_mainLayer->addChildAtPosition(popupColumn, Anchor::Bottom, {0, 20.f});

    return true;
}
void GDriveSigninPopup::onExitTransitionDidStart()
{
    Popup::onExitTransitionDidStart();
    if (GDriveManager::getInstance()->getCurrentSigninPopup() == this)
        GDriveManager::getInstance()->setCurrentSigninPopup(nullptr);
}

void GDriveSigninPopup::onSignin(CCObject *sender)
{
    showLoading();
    GDriveManager::getInstance()->signin();
}

void GDriveSigninPopup::onVerify(CCObject *sender)
{
    showLoading();
    GDriveManager::getInstance()->verify();
}

void GDriveSigninPopup::showLoading()
{
    if (!m_loadingCircle)
    {
        m_loadingCircle = LoadingSpinner::create(35.f);
        m_buttonMenu->addChild(m_loadingCircle);
    }
    m_loadingCircle->setVisible(true);
    m_signinButton->setVisible(false);
    m_verifyButton->setVisible(false);

    m_buttonMenu->updateLayout();
}
void GDriveSigninPopup::showVerify()
{
    if (!m_verifyButton)
        return;

    m_verifyButton->setVisible(true);
    m_signinButton->setVisible(false);
    m_loadingCircle->setVisible(false);

    m_description->setString("Once you've finished signing in, click the button below to complete setup!");

    m_buttonMenu->updateLayout();
}
void GDriveSigninPopup::showSignin()
{
    if (!m_signinButton)
        return;

    m_signinButton->setVisible(true);
    m_verifyButton->setVisible(false);
    m_loadingCircle->setVisible(false);

    m_buttonMenu->updateLayout();
}

void GDriveSigninPopup::finishUp()
{
    GDrivePopup::create();
    this->onClose(this);
}
