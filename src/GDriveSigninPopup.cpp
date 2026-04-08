#include "GDriveSigninPopup.hpp"

#include <ctime>

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
    if (!Popup::init(335.f, 160.f, "square01_001.png"))
        return false;
    GDriveManager::getInstance()->setCurrentSigninPopup(this);
    this->setID("gdrive-sign-in-popup"_spr);

    /* Popup Column */
    m_popupColumn = CCMenu::create();
    m_popupColumn->setLayout(ColumnLayout::create()
                                 ->setAxisReverse(true)
                                 ->setAxisAlignment(AxisAlignment::Between)
                                 ->setGap(0)
                                 ->setAutoScale(true));
    m_popupColumn->setContentSize({m_mainLayer->getContentWidth() - 5.f, m_mainLayer->getContentHeight() - 30.f});
    // m_popupColumn->setAnchorPoint({0.5f, 0.5});
    m_popupColumn->setID("popup-column"_spr);

    /* title */
    m_title = CCLabelBMFont::create("GDrive Backup Setup", "goldFont.fnt", m_popupColumn->getContentWidth());
    m_title->setID("title"_spr);
    m_popupColumn->addChild(m_title);

    /*Description label*/
    m_description = CCLabelBMFont::create("Welcome to GDrive Backup!\nPlease click the button below to open the browser and sign in!", "chatFont.fnt", m_popupColumn->getContentWidth() - 30.f, CCTextAlignment::kCCTextAlignmentCenter);
    m_description->setID("description"_spr);
    m_popupColumn->addChild(m_description);

    /*Anchored Buttons Menu */
    m_buttonMenu = CCMenu::create();
    m_buttonMenu->setContentWidth(m_mainLayer->getContentWidth());
    m_buttonMenu->setID("button-menu"_spr);
    m_buttonMenu->setAnchorPoint({0.5f, 0.5f});
    m_buttonMenu->setLayout(RowLayout::create()->setAxisAlignment(AxisAlignment::Center)->setGap(15.f));

    /* button z*/
    m_signinButton = CCMenuItemSpriteExtra::create(ButtonSprite::create("Sign in"), this, menu_selector(GDriveSigninPopup::onSignin));
    m_signinButton->setID("signin-button"_spr);
    m_buttonMenu->addChild(m_signinButton);

    m_verifyButton = CCMenuItemSpriteExtra::create(ButtonSprite::create("Verify"), this, menu_selector(GDriveSigninPopup::onVerify));
    m_verifyButton->setID("verify-button"_spr);
    m_verifyButton->setVisible(false);
    m_buttonMenu->addChild(m_verifyButton);

    m_loadingCircle = LoadingSpinner::create(30.f);
    m_loadingCircle->setID("signin-loading"_spr);
    m_loadingCircle->setVisible(false);
    m_buttonMenu->addChild(m_loadingCircle);

    m_buttonMenu->updateLayout();
    m_popupColumn->addChild(m_buttonMenu);

    m_popupColumn->updateLayout();
    m_mainLayer->addChildAtPosition(m_popupColumn, Anchor::Center);

    auto tempstamp = Mod::get()->getSavedValue<time_t>("temp-timestamp", 0);
    if (tempstamp != 0 && tempstamp > std::time(nullptr))
        showVerify();
    else
    {
        Mod::get()->getSaveContainer().erase("temp-uuid");
        Mod::get()->getSaveContainer().erase("temp-timestamp");
    }

    return true;
}

void GDriveSigninPopup::onExitTransitionDidStart()
{
    Popup::onExitTransitionDidStart();
    if (GDriveManager::getInstance()->getCurrentSigninPopup() == this)
        GDriveManager::getInstance()->setCurrentSigninPopup(nullptr);
}

void GDriveSigninPopup::onClose(CCObject *sender)
{
    Popup::onClose(sender);

    Mod::get()->getSaveContainer().erase("temp-uuid");
    Mod::get()->getSaveContainer().erase("temp-timestamp");
    if (Mod::get()->saveData().isErr())
        log::warn("Could not write save to file");
}

void GDriveSigninPopup::onSignin(CCObject *sender)
{
    showLoading();
    GDriveManager::getInstance()->signin();
}
void GDriveSigninPopup::onTitle(CCObject *sender)
{
}

void GDriveSigninPopup::onVerify(CCObject *sender)
{
    showLoading();
    GDriveManager::getInstance()->verify();
}

void GDriveSigninPopup::showLoading()
{
    m_signinButton->setVisible(false);
    m_verifyButton->setVisible(false);

    m_loadingCircle->setVisible(true);

    m_buttonMenu->updateLayout();
}

void GDriveSigninPopup::showVerify()
{
    m_verifyButton->setVisible(true);
    m_signinButton->setVisible(false);
    m_loadingCircle->setVisible(false);

    m_description->setString("Once you've finished signing in, click the button below to complete setup!");

    m_buttonMenu->updateLayout();
}

void GDriveSigninPopup::showSignin()
{
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

#ifdef GEODE_IS_DESKTOP
#include <camila314.geode-uri/include/GeodeURI.hpp>
$on_mod(Loaded)
{
    URIEvent("gdrivebackup").listen([](std::string_view path) {
                                if (path == "verify")
                                {
                                    if (auto popup = GDriveManager::getInstance()->getCurrentSigninPopup())
                                        popup->onVerify(nullptr);
                                }
                                return true;
                            })
        .leak();
}
#endif