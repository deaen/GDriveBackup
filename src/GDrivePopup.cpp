#include "GDrivePopup.hpp"

#include <Geode/loader/SettingV3.hpp>
#include <Geode/ui/GeodeUI.hpp>
#include <Geode/utils/cocos.hpp>

#include "GDriveManager.hpp"
#include "GDriveSigninPopup.hpp"

GDrivePopup *GDrivePopup::create()
{
    if (Mod::get()->getSavedValue<std::array<std::string, 3>>("refresh_token")[0] == "")
    {
        GDriveSigninPopup::create()->show();
        return nullptr;
    }
    auto ret = new GDrivePopup();
    if (ret->init())
    {
        ret->autorelease();
        ret->show();
        return ret;
    }

    delete ret;
    return nullptr;
}

bool GDrivePopup::init()
{
    if (!Popup::init(470.f, 235.f, "GJ_square02.png"))
        return false;
    GDriveManager::getInstance()->setCurrentPopup(this);
    this->setID("gdrive-popup"_spr);

    /* Title Sprite */
    auto titleSprite = CCSprite::create("title.png"_spr);
    titleSprite->setID("title-sprite"_spr);
    m_mainLayer->addChildAtPosition(titleSprite, Anchor::Top, {0, -33.f});

    /* Mod Settings Button */
    auto modSettingsSpr = CCSprite::createWithSpriteFrameName("GJ_optionsBtn_001.png");
    modSettingsSpr->setScale(0.8f);
    auto modSettingsButton =
        CCMenuItemSpriteExtra::create(modSettingsSpr, this, menu_selector(GDrivePopup::onSettings));
    modSettingsButton->setID("mod-settings-button"_spr);
    m_buttonMenu->addChildAtPosition(modSettingsButton, Anchor::BottomRight, {-3, 3});

    /* Left Arrow Button */
    auto leftArrowSpr = CCSprite::createWithSpriteFrameName("GJ_arrow_03_001.png");
    leftArrowSpr->setScale(0.8f);

    m_leftArrowButton = CCMenuItemSpriteExtra::create(leftArrowSpr, this, menu_selector(GDrivePopup::onSlotPageLeft));
    m_leftArrowButton->setID("left-arrow-button"_spr);
    m_buttonMenu->addChildAtPosition(m_leftArrowButton, Anchor::Left,
                                     {
                                         (m_leftArrowButton->getContentWidth() / 2) + 7.f,
                                         -13.f,
                                     });
    /* Right Arrow Button */
    auto rightArrowSpr = CCSprite::createWithSpriteFrameName("GJ_arrow_03_001.png");
    rightArrowSpr->setScale(0.8f);
    rightArrowSpr->setFlipX(true);

    m_rightArrowButton =
        CCMenuItemSpriteExtra::create(rightArrowSpr, this, menu_selector(GDrivePopup::onSlotPageRight));
    m_rightArrowButton->setID("right-arrow-button"_spr);
    m_buttonMenu->addChildAtPosition(m_rightArrowButton, Anchor::Right,
                                     {
                                         -(m_rightArrowButton->getContentWidth() / 2) - 7.f,
                                         -13.f,
                                     });

    /* Popup Column */
    auto popupColumn = CCMenu::create();
    popupColumn->setLayout(
        ColumnLayout::create()->setAxisReverse(true)->setGap(17.f)->setAxisAlignment(AxisAlignment::Start));
    popupColumn->setContentSize({400.f, m_mainLayer->getContentHeight() - 60.f});
    popupColumn->setAnchorPoint({0.5f, 0.5f});
    popupColumn->setID("popup-column"_spr);

    /* Auto Slot */
    // popupColumn->addChild(GDriveSlotBox::create(0, popupColumn->getContentWidth(), 40.f));

    /* Slot Row */
    m_slotRow = CCNode::create();
    m_slotRow->setLayout(
        RowLayout::create()->setGap(-5.f)->setAxisAlignment(AxisAlignment::Start)->setAutoScale(false));
    m_slotRow->setContentSize({popupColumn->getContentWidth(), 100.f});
    m_slotRow->setAnchorPoint({0.5f, 0.5f});
    m_slotRow->setID("slot-row"_spr);
    popupColumn->addChild(m_slotRow);

    /* Page Button Row */
    m_pageButtonsRow = CCMenu::create();
    m_pageButtonsRow->setLayout(RowLayout::create()->setAxisAlignment(AxisAlignment::Center)->setAutoScale(true));
    m_pageButtonsRow->setContentWidth(popupColumn->getContentWidth());
    m_pageButtonsRow->setAnchorPoint({0.5f, 0.5f});
    m_pageButtonsRow->setID("page-button-row"_spr);

    showSlotPage(1);

    for (int i = 1; i <= m_maxSlotPage; ++i)
    {
        auto buttonSpr = CCSprite::create("smallDot.png");
        buttonSpr->setScale(0.8f);
        auto button = CCMenuItemSpriteExtra::create(buttonSpr, this, menu_selector(GDrivePopup::onPageButton));
        button->setTag(i);
        button->setID(fmt::format("page-button-{}"_spr, i));
        m_pageButtonsRow->addChild(button);
        // i dont fuckin care anymore
        if (i == m_currentSlotPage)
            button->setColor({255, 255, 255});
        else
            button->setColor({125, 125, 125});
    }
    m_pageButtonsRow->updateLayout();
    popupColumn->addChild(m_pageButtonsRow);

    popupColumn->updateLayout();
    m_mainLayer->addChildAtPosition(popupColumn, Anchor::Center); //, {0, -50.f});

    /* Name Row */
    auto infoRow = CCMenu::create();
    infoRow->setLayout(RowLayout::create()
                           ->setAutoScale(false)
                           ->setAxisAlignment(AxisAlignment::Center)
                           ->setGap(0.f)
                           ->setCrossAxisLineAlignment(AxisAlignment::End));
    infoRow->setContentWidth(m_mainLayer->getContentWidth());
    infoRow->setAnchorPoint({0.5f, 0.5f});
    infoRow->setScale(0.6f);
    infoRow->setID("name-row"_spr);

    /* Player Name */
    int accID = GJAccountManager::sharedState()->m_accountID;
    auto playerName = CCLabelBMFont::create(
        (accID != 0) ? GameManager::sharedState()->m_playerName.c_str() : "Unregistered", "goldFont.fnt");
    if (!accID)
        playerName->setColor({255, 0, 0});
    playerName->setID("player-name"_spr);
    infoRow->addChild(playerName);

    /* 's saves */
    auto ssavesLabel = CCLabelBMFont::create((accID != 0) ? "'s saves" : "saves", "bigFont.fnt");
    ssavesLabel->setScale(0.8f);
    ssavesLabel->setID("saves-label"_spr);
    infoRow->addChild(ssavesLabel);

    /* Name info button */
    auto infoIconSpr = CCSprite::createWithSpriteFrameName("GJ_infoIcon_001.png");
    infoIconSpr->setScale(0.6f);
    auto nameInfoButton = CCMenuItemSpriteExtra::create(infoIconSpr, this, menu_selector(GDrivePopup::onNameInfo));
    nameInfoButton->setID("name-info-button"_spr);
    infoRow->addChild(nameInfoButton);

    infoRow->updateLayout();
    m_mainLayer->addChildAtPosition(infoRow, Anchor::Top, {0, -65.f});

    setupEmail();

    /* Corners */
    auto bottomLeftCorner = CCSprite::createWithSpriteFrameName("rewardCorner_001.png");
    bottomLeftCorner->setAnchorPoint({0, 0});
    bottomLeftCorner->setID("bottom-left-corner");
    m_mainLayer->addChildAtPosition(bottomLeftCorner, Anchor::BottomLeft);

    auto topLeftCorner = CCSprite::createWithSpriteFrameName("rewardCorner_001.png");
    topLeftCorner->setAnchorPoint({0, 1});
    topLeftCorner->setFlipY(true);
    topLeftCorner->setID("top-left-corner");
    m_mainLayer->addChildAtPosition(topLeftCorner, Anchor::TopLeft);

    auto bottomRightCorner = CCSprite::createWithSpriteFrameName("rewardCorner_001.png");
    bottomRightCorner->setAnchorPoint({1, 0});
    bottomRightCorner->setFlipX(true);
    bottomRightCorner->setID("bottom-right-corner");
    m_mainLayer->addChildAtPosition(bottomRightCorner, Anchor::BottomRight);

    auto topRightCorner = CCSprite::createWithSpriteFrameName("rewardCorner_001.png");
    topRightCorner->setAnchorPoint({1, 1});
    topRightCorner->setFlipX(true);
    topRightCorner->setFlipY(true);
    topRightCorner->setID("top-right-corner");
    m_mainLayer->addChildAtPosition(topRightCorner, Anchor::TopRight);

    return true;
}
void GDrivePopup::onExitTransitionDidStart()
{
    Popup::onExitTransitionDidStart();

    if (GDriveManager::getInstance()->getCurrentPopup() == this)
        GDriveManager::getInstance()->setCurrentPopup(nullptr);
}

void GDrivePopup::onToggleAccountVisibility(CCObject *sender)
{
    if (m_emailVisible)
        m_emailLabel->setCString(m_email.c_str());
    else
        m_emailLabel->setCString(m_emailCensored.c_str());

    m_hideEmailButton->setPosition({(m_emailButton->getPositionX() + (m_emailLabel->getScaledContentWidth() / 2.f) +
                                     (m_hideEmailButton->getScaledContentWidth() / 2.f)),
                                    (m_emailButton->getPositionY() + (m_emailLabel->getScaledContentHeight() / 4.f))});
    m_emailVisible = !m_emailVisible;
}
void GDrivePopup::onNameInfo(CCObject *sender)
{
    int accID = GJAccountManager::sharedState()->m_accountID;
    FLAlertLayer::create(
        "Account info",
        (accID != 0)
            ? fmt::format(
                  "Each of your GD accounts has <cj>different</c> save slots!\nFor example, if you sign into your alt "
                  "GD account and save your data, it <cg>won't</c> overwrite your main account's save! Pretty "
                  "Cool!\nfolder ID: <cy>{}</c>",
                  accID)
            : fmt::format(
                  "You are currently on an <cr>unregistered</c> account. Saving data while signed out will overwrite "
                  "any other data you might have saved while signed out other sessions/devices.\nfolder ID: <cy>{}</c>",
                  accID),
        "Okay")
        ->show();
}
void GDrivePopup::onSettings(CCObject *sender)
{
    openSettingsPopup(Mod::get(), true);
}

void GDrivePopup::onSlotPageLeft(CCObject *sender)
{
    showSlotPage(m_currentSlotPage - 1);
}
void GDrivePopup::onSlotPageRight(CCObject *sender)
{
    showSlotPage(m_currentSlotPage + 1);
}
void GDrivePopup::onPageButton(CCObject *sender)
{
    showSlotPage(sender->getTag());
}
void GDrivePopup::onSignOut(CCObject *sender)
{
    createQuickPopup(
        "Google Account", fmt::format("Currently signed in as <cy>{}</c>\n Do you want to sign out?", m_email),
        "Cancel", "Sign Out",
        [](auto, bool btn2) {
            if (btn2)
                GDriveManager::getInstance()->signout(true);
        },
        true, true);
}

void GDrivePopup::setupEmail()
{
    /* Email Loading */
    auto spinner = LoadingSpinner::create(15.f);
    spinner->setID("email-spinner"_spr);
    m_mainLayer->addChildAtPosition(spinner, Anchor::Bottom, {0, 15.f});

    async::spawn(GDriveManager::getInstance()->getEmail(), [this, spinner](std::string email) {
        spinner->setVisible(false);
        if (email == "")
        {
            return;
        }

        m_email = email;
        auto vec = utils::string::split(email, "@");
        for (const auto &chr : vec[0])
        {
            m_emailCensored += "•";
        }
        m_emailCensored += "@" + vec[1];

        /* email button */
        m_emailLabel = CCLabelBMFont::create(m_emailCensored.c_str(), "bigFont.fnt");
        m_emailLabel->setScale(0.4f);
        m_emailButton = CCMenuItemSpriteExtra::create(m_emailLabel, this, menu_selector(GDrivePopup::onSignOut));
        m_emailButton->setAnchorPoint({0.5f, 0.5f});
        m_emailButton->setID("email-button"_spr);
        m_emailButton->setPosition({m_buttonMenu->getContentWidth() / 2.f, 15.f});
        m_buttonMenu->addChild(m_emailButton);

        /* Hide Email Button */
        m_hideEmailButton = CCMenuItemSpriteExtra::create(CCSprite::createWithSpriteFrameName("hideBtn_001.png"), this,
                                                          menu_selector(GDrivePopup::onToggleAccountVisibility));
        m_hideEmailButton->setID("hide-email-button"_spr);
        m_hideEmailButton->setOpacity(150);
        m_hideEmailButton->m_baseScale = 0.5f;
        m_hideEmailButton->setScale(0.5f);
        m_hideEmailButton->setAnchorPoint({0.5f, 0.5f});
        m_hideEmailButton->setPosition(
            {(m_emailButton->getPositionX() + (m_emailLabel->getScaledContentWidth() / 2.f) +
              (m_hideEmailButton->getScaledContentWidth() / 2.f)),
             (m_emailButton->getPositionY() + (m_emailLabel->getScaledContentHeight() / 4.f))});
        m_buttonMenu->addChild(m_hideEmailButton);
    });
}

void GDrivePopup::showSlotPage(int pageNumber)
{
    int slotCount = Mod::get()->getSettingValue<int64_t>("slot-count");
    m_maxSlotPage = (slotCount + m_slotsPerPage - 1) / m_slotsPerPage;
    pageNumber = std::clamp(pageNumber, 1, m_maxSlotPage);
    if (pageNumber == 1)
        m_leftArrowButton->setVisible(false);
    else
        m_leftArrowButton->setVisible(true);
    if (pageNumber == m_maxSlotPage)
        m_rightArrowButton->setVisible(false);
    else
        m_rightArrowButton->setVisible(true);

    for (const auto &box : m_slotBoxes)
    {
        if (box)
            box->setVisible(false);
    }

    auto buttons = m_pageButtonsRow->getChildrenExt<CCMenuItemSpriteExtra *>();
    if (!buttons.empty())
    {
        for (auto *child : buttons)
        {
            if (child->getTag() == pageNumber)
            {
                child->setColor({255, 255, 255});
            }
            else
            {
                child->setColor({125, 125, 125});
            }
        }
    }
    for (int i = ((pageNumber - 1) * m_slotsPerPage + 1); i <= (std::min(pageNumber * m_slotsPerPage, slotCount)); ++i)
    {
        int vecdex = i - 1;
        if (m_slotBoxes.size() <= vecdex)
        {
            m_slotBoxes.resize(i);
        }

        if (!m_slotBoxes[vecdex])
        {
            m_slotBoxes[vecdex] = GDriveSlotBox::create(i);
            m_slotRow->addChild(m_slotBoxes[vecdex]);
        }
        else
        {
            m_slotBoxes[vecdex]->setVisible(true);
        }
    }

    m_slotRow->updateLayout();
    m_currentSlotPage = pageNumber;
}

/* Banished to comment zone #FuckTouchPriority */

// $execute
// {
//     listenForAllSettingChanges([](std::string_view key, std::shared_ptr<SettingV3> setting) {
//         if (auto popup = GDriveManager::getInstance()->getCurrentPopup())
//         {
//             int zOrder = popup->getZOrder();
//             int touchPrio = popup->getTouchPriority();
//             popup->removeFromParent();
//             if (auto newPopup = GDrivePopup::create())
//             {
//                  newPopup->setZOrder(zOrder);
//                  newPopup->setTouchPriority(touchPrio);
//             }
//         }
//     });
// }