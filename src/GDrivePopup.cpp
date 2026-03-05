#include "GDrivePopup.hpp"

GDrivePopup *GDrivePopup::create()
{
    auto ret = new GDrivePopup();
    if (ret->init())
    {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}

bool GDrivePopup::init()
{
    if (!Popup::init(420.f, 220.f))
        return false;
    this->setID("gdrive-popup"_spr);

    // poppup column
    auto popupColumn = CCMenu::create();
    popupColumn->setLayout(geode::ColumnLayout::create()->setAxisReverse(true)->setGap(15.f));
    popupColumn->setContentSize({m_mainLayer->getContentWidth(), m_mainLayer->getContentHeight()});
    popupColumn->setAnchorPoint({0.5f, 0.5f});
    popupColumn->setID("popup-column"_spr);

    // temp login button
    auto loginButton = CCMenuItemSpriteExtra::create(ButtonSprite::create("login", "goldFont.fnt", "GJ_button_01.png"),
                                                     this, menu_selector(GDrivePopup::onLogin));
    loginButton->setID("login-button"_spr);
    popupColumn->addChild(loginButton);

    // auto slot
    popupColumn->addChild(createSlotBox(popupColumn->getContentWidth() - 20.f, 40.f, 0));

    // slot row
    auto slotRow = CCNode::create();
    slotRow->setLayout(geode::RowLayout::create());
    slotRow->setContentWidth(m_mainLayer->getContentWidth() - 20.f);
    slotRow->setAnchorPoint({0.5f, 0.5f});
    slotRow->setID("slot-row"_spr);

    slotRow->addChild(createSlotBox(150.f, 120.f, 1));
    slotRow->addChild(createSlotBox(150.f, 120.f, 2));
    slotRow->addChild(createSlotBox(150.f, 120.f, 3));

    slotRow->updateLayout();
    popupColumn->addChild(slotRow);

    popupColumn->updateLayout();
    m_mainLayer->addChildAtPosition(popupColumn, Anchor::Center, {0, 10.f});

    // account row
    auto accountRow = CCMenu::create();
    accountRow->setLayout(
        geode::RowLayout::create()->setCrossAxisLineAlignment(geode::AxisAlignment::Center)->setAutoScale(false));
    accountRow->setContentWidth(m_mainLayer->getContentWidth());
    accountRow->setAnchorPoint({0.5f, -0.4f});
    accountRow->setID("account-row"_spr);

    // hide email button
    auto hideEmailButton = CCMenuItemSpriteExtra::create(CCSprite::createWithSpriteFrameName("hideBtn_001.png"), this,
                                                         menu_selector(GDrivePopup::onToggleAccountVisibility));
    hideEmailButton->setOpacity(100);
    hideEmailButton->setID("hide-email-button"_spr);
    accountRow->addChild(hideEmailButton);

    // email label
    m_emailLabel->setScale(0.6f);
    m_emailLabel->setID("email-label"_spr);
    accountRow->addChild(m_emailLabel);

    accountRow->updateLayout();
    m_mainLayer->addChildAtPosition(accountRow, Anchor::Bottom);
    return true;
}

cocos2d::CCNode *GDrivePopup::createSlotBox(float width, float height, int slot)
{ // 0 = automatic slot
    auto node = CCNode::create();
    node->setContentSize({width, height});
    node->setAnchorPoint({0.5f, 0.5f});
    node->setID(CCString::createWithFormat("slot-box-%d", slot)->getCString());

    // box bg
    auto backgroundSpr = NineSlice::create("square02b_001.png");

    // borders
    backgroundSpr->setInsets({backgroundSpr->getContentWidth() / 3.f, backgroundSpr->getContentHeight() / 3.f,
                              backgroundSpr->getContentWidth() / 3.f, backgroundSpr->getContentHeight() / 3.f});
    backgroundSpr->setAnchorPoint({0.5, 0.5});
    backgroundSpr->setColor({148, 78, 39});
    backgroundSpr->setContentSize({width - 5.f, height + 10.f});
    backgroundSpr->setID("slot-background"_spr);
    node->addChildAtPosition(backgroundSpr, Anchor::Center);

    auto topBorder = NineSlice::createWithSpriteFrameName("GJ_commentTop_001.png");
    topBorder->setInsetLeft(topBorder->getContentWidth() / 2);
    topBorder->setContentWidth(node->getContentWidth());
    topBorder->setID("top-border"_spr);
    node->addChildAtPosition(topBorder, Anchor::Top);

    auto bottomBorder = NineSlice::createWithSpriteFrameName("GJ_commentTop_001.png");
    bottomBorder->setInsetLeft(bottomBorder->getContentWidth() / 2);
    bottomBorder->setContentWidth(node->getContentWidth());
    bottomBorder->setScaleY(-1.f);
    bottomBorder->setID("bottom-border"_spr);
    node->addChildAtPosition(bottomBorder, Anchor::Bottom);

    auto rightBorder = NineSlice::createWithSpriteFrameName("GJ_commentSide_001.png");
    rightBorder->setContentHeight(node->getContentHeight() - 20.f);
    rightBorder->setAnchorPoint({0., 0.5f});
    rightBorder->setScaleX(-1.f);
    rightBorder->setID("right-border"_spr);
    node->addChildAtPosition(rightBorder, Anchor::Right);

    auto leftBorder = NineSlice::createWithSpriteFrameName("GJ_commentSide_001.png");
    leftBorder->setContentHeight(node->getContentHeight() - 20.f);
    leftBorder->setAnchorPoint({0., 0.5f});
    leftBorder->setID("left-border"_spr);
    node->addChildAtPosition(leftBorder, Anchor::Left);

    // menu
    auto menu = CCMenu::create();
    menu->setContentSize({width - 5.f, height});
    menu->setLayout(
        (slot == 0) ? geode::RowLayout::create()->setAutoScale(false)->setAxisAlignment(geode::AxisAlignment::Between)
                    : geode::ColumnLayout::create()->setAxisReverse(true)->setAutoScale(false));
    menu->setID("menu-column"_spr);

    // title
    auto slotTitle = CCLabelBMFont::create(CCString::createWithFormat("Slot %d", slot)->getCString(), "goldFont.fnt");
    slotTitle->setID("slot-title"_spr);
    slotTitle->setScale(0.84f);

    // info label
    auto infoLabel = CCLabelBMFont::create("Last Saved: 2/03/2026 7:48PM | Size: 297MB", "chatFont.fnt");
    infoLabel->setID("info-label"_spr);
    infoLabel->setScale(0.4f);

    if (slot == 0)
    {
        // auto slot settings
        menu->setContentWidth(width - 40.f);
        auto textColumn = CCNode::create();
        textColumn->setLayout(geode::ColumnLayout::create()
                                  ->setAxisReverse(true)
                                  ->setAutoScale(false)
                                  ->setCrossAxisLineAlignment(geode::AxisAlignment::Start)
                                  ->setGap(2.f));
        textColumn->setContentHeight(menu->getContentHeight());
        textColumn->setID("text-column"_spr);

        slotTitle->setCString("Automatic Backup");
        textColumn->addChild(slotTitle);
        textColumn->addChild(infoLabel);

        textColumn->updateLayout();
        menu->addChild(textColumn);
    }
    else
    {
        menu->addChild(slotTitle);
        menu->addChild(infoLabel);

        // separator
        auto separator = NineSlice::createWithSpriteFrameName("floorLine_01_001.png");
        separator->setInsetLeft(separator->getContentWidth() / 3);
        separator->setContentSize({width - 20.f, 1.f});
        separator->setID("separtor"_spr);
        menu->addChild(separator);

        // save button
        auto saveButton =
            CCMenuItemSpriteExtra::create(ButtonSprite::create("  Save  ", "bigFont.fnt", "GJ_button_01.png", 0.8f),
                                          this, menu_selector(GDrivePopup::onSave));
        saveButton->setTag(slot);
        saveButton->setScale(0.84f);
        saveButton->m_baseScale = 0.84f;
        saveButton->setID("save-button"_spr);
        menu->addChild(saveButton);
    }

    // load button
    auto loadButton =
        CCMenuItemSpriteExtra::create(ButtonSprite::create("  Load  ", "bigFont.fnt", "GJ_button_01.png", 0.8f), this,
                                      menu_selector(GDrivePopup::onLoad));
    loadButton->setTag(slot);
    loadButton->setScale(0.84f);
    loadButton->m_baseScale = 0.84f;
    loadButton->setID("load-button"_spr);
    menu->addChild(loadButton);

    menu->updateLayout();
    node->addChildAtPosition(menu, Anchor::Center);
    return node;
}
void GDrivePopup::onSave(CCObject *sender)
{
}
void GDrivePopup::onLoad(CCObject *sender)
{
    GDriveManager::getInstance()->refresh();
}

void GDrivePopup::onLogin(CCObject *sender)
{
    static_cast<CCMenuItemSpriteExtra *>(sender)->setEnabled(false);
    GDriveManager::getInstance()->login();
}

void GDrivePopup::onToggleAccountVisibility(CCObject *sender)
{
    return;
}
