#include "GDrivePopup.hpp"
#include "GDriveManager.hpp"
#include "GDriveSigninPopup.hpp"
#include "GDriveSizeInfoPopup.hpp"
#include <Geode/ui/GeodeUI.hpp>

GDrivePopup *GDrivePopup::create()
{
    if (Mod::get()->getSavedValue<std::array<std::string, 3>>("refresh_token")[0].empty())
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
    if (!Popup::init(popupWidth, popupHeight, "GJ_square02.png"))
        return false;

    GDriveManager::getInstance()->setCurrentPopup(this);
    this->setID("gdrive-popup"_spr);

    /* Popup column */
    auto popupColumn = CCMenu::create();
    popupColumn->setContentSize({popupWidth - 70.f, popupHeight});
    popupColumn->setAnchorPoint({0.5f, 0.5f});
    popupColumn->setID("popup-column"_spr);

    /* Title sprite */
    auto titleSprite = CCSprite::create("title.png"_spr);
    titleSprite->setID("title-sprite"_spr);
    titleSprite->setLayoutOptions(AxisLayoutOptions::create()->setPrevGap(7.f));
    popupColumn->addChild(titleSprite);

    /* Name Row */
    auto nameRow = CCMenu::create();
    nameRow->setContentWidth(popupWidth);
    nameRow->setAnchorPoint({0.5f, 0.5f});
    nameRow->setLayoutOptions(AxisLayoutOptions::create()->setPrevGap(11.f));
    nameRow->setScale(0.6f);
    nameRow->setID("name-row"_spr);

    /* Player Name */
    int accID = GJAccountManager::sharedState()->m_accountID;
    auto playerName = CCLabelBMFont::create((accID != 0) ? GameManager::sharedState()->m_playerName.c_str() : "Unregistered", "goldFont.fnt");
    if (!accID)
        playerName->setColor({255, 0, 0});
    playerName->setID("player-name"_spr);
    nameRow->addChild(playerName);

    /* 's saves */
    auto ssavesLabel = CCLabelBMFont::create((accID != 0) ? "'s saves" : "saves", "bigFont.fnt");
    ssavesLabel->setScale(0.8f);
    ssavesLabel->setID("saves-label"_spr);
    nameRow->addChild(ssavesLabel);

    /* Name info button */
    auto infoIconSpr = CCSprite::createWithSpriteFrameName("GJ_infoIcon_001.png");
    infoIconSpr->setScale(0.6f);
    auto nameInfoButton = CCMenuItemSpriteExtra::create(infoIconSpr, this, menu_selector(GDrivePopup::onNameInfo));
    nameInfoButton->setID("name-info-button"_spr);
    nameRow->addChild(nameInfoButton);

    nameRow->setLayout(RowLayout::create()->setAutoScale(false)->setAxisAlignment(AxisAlignment::Center)->setCrossAxisLineAlignment(AxisAlignment::End)->setGap(0.f));
    popupColumn->addChild(nameRow);

    /* Slot Row */
    m_slotRow = CCNode::create();
    m_slotRow->setLayout(RowLayout::create()->setGap(-5.f)->setAxisAlignment(AxisAlignment::Start)->setAutoScale(false));
    m_slotRow->setContentSize({popupColumn->getContentWidth(), 100.f});
    m_slotRow->setAnchorPoint({0.5f, 0.5f});
    m_slotRow->setID("slot-row"_spr);
    popupColumn->addChild(m_slotRow);

    /* Left Arrow Button */
    auto leftArrowSpr = CCSprite::createWithSpriteFrameName("GJ_arrow_03_001.png");
    leftArrowSpr->setScale(0.8f);

    m_leftArrowButton = CCMenuItemSpriteExtra::create(leftArrowSpr, this, menu_selector(GDrivePopup::onSlotPageLeft));
    m_leftArrowButton->setID("left-arrow-button"_spr);
    m_buttonMenu->addChildAtPosition(m_leftArrowButton, Anchor::Left, {(m_leftArrowButton->getContentWidth() / 2) + 7.f, -13.f});

    /* Right Arrow Button */
    auto rightArrowSpr = CCSprite::createWithSpriteFrameName("GJ_arrow_03_001.png");
    rightArrowSpr->setScale(0.8f);
    rightArrowSpr->setFlipX(true);

    m_rightArrowButton = CCMenuItemSpriteExtra::create(rightArrowSpr, this, menu_selector(GDrivePopup::onSlotPageRight));
    m_rightArrowButton->setID("right-arrow-button"_spr);
    m_buttonMenu->addChildAtPosition(m_rightArrowButton, Anchor::Right, {-(m_rightArrowButton->getContentWidth() / 2) - 7.f, -13.f});

    /* Page Button Row */
    m_pageButtonsRow = CCMenu::create();
    m_pageButtonsRow->setContentWidth(popupColumn->getContentWidth());
    m_pageButtonsRow->setAnchorPoint({0.5f, 0.5f});
    m_pageButtonsRow->setID("page-button-row"_spr);
    
    /* Slot & metadata setup */
    async::spawn(GDriveManager::getInstance()->getMetadata(), [this](bool gotData) {
        for (const auto &box : m_slotBoxes)
        {
            if (box)
                box->loadMetadata();
        }

        if (!gotData)
            GDriveManager::getInstance()->showError("Couldn't get metadata,", "Please try again later", false);
    });

    showSlotPage(1);
    
    for (int i = 1; i <= m_maxSlotPage; ++i)
    {
        auto buttonSpr = CCSprite::create("smallDot.png");
        buttonSpr->setScale(0.8f);
        auto button = CCMenuItemSpriteExtra::create(buttonSpr, this, menu_selector(GDrivePopup::onPageButton));
        button->setTag(i);
        button->setID(fmt::format("page-button-{}"_spr, i));
        m_pageButtonsRow->addChild(button);

        if (i == m_currentSlotPage)
            button->setColor({255, 255, 255});
        else
            button->setColor({125, 125, 125});
    }

    m_pageButtonsRow->setLayout(RowLayout::create()->setAxisAlignment(AxisAlignment::Center)->setAutoScale(true));
    popupColumn->addChild(m_pageButtonsRow);

    popupColumn->setLayout(ColumnLayout::create()->setAxisReverse(true)->setGap(15.f)->setPadding({0.f, 0.f, 0.f, 36.f})->setAxisAlignment(AxisAlignment::Center)->setAutoScale(false));
    m_mainLayer->addChildAtPosition(popupColumn, Anchor::Center);

    setupEmail();

    /* Corners */
    auto bottomLeftCorner = CCSprite::createWithSpriteFrameName("rewardCorner_001.png");
    bottomLeftCorner->setAnchorPoint({0, 0});
    bottomLeftCorner->setID("bottom-left-corner"_spr);
    m_mainLayer->addChildAtPosition(bottomLeftCorner, Anchor::BottomLeft);

    auto topLeftCorner = CCSprite::createWithSpriteFrameName("rewardCorner_001.png");
    topLeftCorner->setAnchorPoint({0, 1});
    topLeftCorner->setFlipY(true);
    topLeftCorner->setID("top-left-corner"_spr);
    m_mainLayer->addChildAtPosition(topLeftCorner, Anchor::TopLeft);

    // auto bottomRightCorner = CCSprite::createWithSpriteFrameName("rewardCorner_001.png");
    // bottomRightCorner->setAnchorPoint({1.f, 0});
    // bottomRightCorner->setFlipX(true);
    // bottomRightCorner->setID("bottom-right-corner"_spr);
    // m_mainLayer->addChildAtPosition(bottomRightCorner, Anchor::BottomRight);

    auto topRightCorner = CCSprite::createWithSpriteFrameName("rewardCorner_001.png");
    topRightCorner->setAnchorPoint({1.f, 1.f});
    topRightCorner->setFlipX(true);
    topRightCorner->setFlipY(true);
    topRightCorner->setID("top-right-corner"_spr);
    m_mainLayer->addChildAtPosition(topRightCorner, Anchor::TopRight);

    /* Bottom Right Menu */
    auto bottomRightMenu = CCMenu::create();
    bottomRightMenu->setID("bottom-right-menu"_spr);
    bottomRightMenu->setScale(0.6f);
    bottomRightMenu->setAnchorPoint({1.f, 0});
    m_mainLayer->addChildAtPosition(bottomRightMenu, Anchor::BottomRight, {-7.f, 7.f});

    /* Edit Button */
    auto editButtonSpr = CCSprite::createWithSpriteFrameName("GJ_editModeBtn_001.png");
    editButtonSpr->setScale(1.05f);
    auto editButton = CCMenuItemSpriteExtra::create(editButtonSpr, this, menu_selector(GDrivePopup::onToggleEditMode));
    editButton->setID("edit-button"_spr);
    bottomRightMenu->addChild(editButton);

    /* Edit Button */
    auto sizeButtonSpr = CircleButtonSprite::createWithSprite("size.png"_spr, 0.77f, CircleBaseColor::Green, CircleBaseSize::MediumAlt);
    editButtonSpr->setScale(1.05f);
    auto sizeButton = CCMenuItemSpriteExtra::create(sizeButtonSpr, this, menu_selector(GDrivePopup::onSizeInfo));
    sizeButton->setID("size-button"_spr);
    bottomRightMenu->addChild(sizeButton);

    /* Mod Settings Button */
    auto modSettingsButton = CCMenuItemSpriteExtra::create(CCSprite::createWithSpriteFrameName("GJ_optionsBtn_001.png"), this, menu_selector(GDrivePopup::onSettings));
    modSettingsButton->setID("mod-settings-button"_spr);
    bottomRightMenu->addChild(modSettingsButton);

    bottomRightMenu->setLayout(RowLayout::create()->setAxisAlignment(AxisAlignment::End)->setGap(3.f));
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

    m_hideEmailButton->setPosition({(m_emailLabel->getPositionX() + (m_emailLabel->getScaledContentWidth() / 2.f) + (m_hideEmailButton->getScaledContentWidth() / 2.f)), (m_emailLabel->getPositionY() + (m_emailLabel->getScaledContentHeight() / 4.f))});
    m_emailVisible = !m_emailVisible;
}

void GDrivePopup::onNameInfo(CCObject *sender)
{
    int accID = GJAccountManager::sharedState()->m_accountID;
    FLAlertLayer::create(
        "Account info",
        (accID != 0)
            ? fmt::format("Each of your GD accounts has <cj>different</c> save slots!\nFor example, if you sign into your alt GD account and save your data, it <cg>won't</c> overwrite your main account's save! Pretty Cool!\naccount ID: <cy>{}</c>", accID)
            : fmt::format("You are currently on an <cr>unregistered</c> account. Saving data while signed out will <cr>overwrite</c> any other data you might have saved while signed out on other sessions/devices.\naccount ID: <cy>{}</c>", accID),
        "Okay")->show();
}

void GDrivePopup::onSizeInfo(CCObject *sender)
{
    GDriveLoadLayer *layer = GDriveLoadLayer::create();
    layer->setMessage("Getting size information...");
    layer->show();
    async::spawn(GDriveManager::getInstance()->getSizeInfo(), [layer](std::optional<sizedata_map> sizeDataMap) {
        layer->removeFromParent();
        if (sizeDataMap)
            GDriveSizeInfoPopup::create(*sizeDataMap);
        else
            FLAlertLayer::create("No Data", "Couldn't find any save files!\nMake sure to save atleast once before looking for size info!", "ok")->show();
    });
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

void GDrivePopup::setupEmail()
{
    /* Email Loading */
    auto spinner = LoadingSpinner::create(15.f);
    spinner->setID("email-spinner"_spr);
    m_mainLayer->addChildAtPosition(spinner, Anchor::Bottom, {0, 16.f});

    async::spawn(GDriveManager::getInstance()->getEmail(), [this, spinner](std::string email) {
        spinner->setVisible(false);
        if (email.empty())
            return;

        m_email = email;
        auto vec = utils::string::split(email, "@");
        for (const auto &chr : vec[0])
            m_emailCensored += "•";

        m_emailCensored += "@" + vec[1];

        /* email label */
        m_emailLabel = CCLabelBMFont::create(m_emailCensored.c_str(), "bigFont.fnt");
        m_emailLabel->setScale(0.4f);
        m_emailLabel->setAnchorPoint({0.5f, 0.5f});
        m_emailLabel->setID("email-label"_spr);
        m_emailLabel->setPosition({m_buttonMenu->getContentWidth() / 2.f, 16.f});
        m_buttonMenu->addChild(m_emailLabel);

        /* Hide Email Button */
        m_hideEmailButton = CCMenuItemSpriteExtra::create(CCSprite::createWithSpriteFrameName("hideBtn_001.png"), this,
                                                          menu_selector(GDrivePopup::onToggleAccountVisibility));
        m_hideEmailButton->setID("hide-email-button"_spr);
        m_hideEmailButton->setOpacity(150);
        m_hideEmailButton->m_baseScale = 0.5f;
        m_hideEmailButton->setScale(0.5f);
        m_hideEmailButton->setAnchorPoint({0.5f, 0.5f});
        m_hideEmailButton->setPosition({(m_emailLabel->getPositionX() + (m_emailLabel->getScaledContentWidth() / 2.f) + (m_hideEmailButton->getScaledContentWidth() / 2.f)), (m_emailLabel->getPositionY() + (m_emailLabel->getScaledContentHeight() / 4.f))});
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
                child->setColor({255, 255, 255});
            else
                child->setColor({125, 125, 125});
        }
    }
    for (int i = ((pageNumber - 1) * m_slotsPerPage + 1); i <= (std::min(pageNumber * m_slotsPerPage, slotCount)); ++i)
    {
        int vecdex = i - 1;
        if (m_slotBoxes.size() <= vecdex)
            m_slotBoxes.resize(i);

        if (!m_slotBoxes[vecdex])
        {
            m_slotBoxes[vecdex] = GDriveSlotBox::create(i, m_editMode);
            m_slotRow->addChild(m_slotBoxes[vecdex]);
        }
        else
            m_slotBoxes[vecdex]->setVisible(true);
    }

    m_slotRow->updateLayout();
    m_currentSlotPage = pageNumber;
}

void GDrivePopup::onToggleEditMode(CCObject *sender)
{
    m_editMode = !m_editMode;
    for (auto box : m_slotBoxes)
    {
        if (box)
        {
            box->setEditMode(m_editMode);
            box->setShouldEditMode(m_editMode);
        }
    }
}

GDriveLoadLayer *GDrivePopup::showLoadLayer()
{
    this->setKeypadEnabled(false);

    m_loadingLayer = GDriveLoadLayer::create();
    if (auto scene = CCDirector::get()->getRunningScene())
        scene->addChild(m_loadingLayer, 9999);

    return m_loadingLayer;
}

void GDrivePopup::hideLoadLayer()
{
    this->setKeypadEnabled(true);

    if (m_loadingLayer)
        m_loadingLayer->removeFromParent();
}

$execute
{
    listenForSettingChanges<int>("slot-count", [](int count) {
        if (auto popup = GDriveManager::getInstance()->getCurrentPopup())
            popup->removeFromParent();
    });
}