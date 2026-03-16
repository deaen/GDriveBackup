#include "GDriveSlotBox.hpp"

GDriveSlotBox *GDriveSlotBox::create(int slot, float width, float height)
{
    auto ret = new GDriveSlotBox();
    if (ret->init(slot, width, height))
    {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}

bool GDriveSlotBox::init(int slot, float width, float height)
{
    if (!CCNode::init())
        return false;
    m_slot = slot;
    this->setContentSize({width, height});
    this->setAnchorPoint({0.5f, 0.5f});
    this->setID(fmt::format("slot-box-{}", m_slot));

    /* Box Background */
    auto backgroundSpr = NineSlice::create("square02b_001.png");
    backgroundSpr->setInsets({backgroundSpr->getContentWidth() / 3.f, backgroundSpr->getContentHeight() / 3.f,
                              backgroundSpr->getContentWidth() / 3.f, backgroundSpr->getContentHeight() / 3.f});
    backgroundSpr->setAnchorPoint({0.5, 0.5});
    backgroundSpr->setColor({53, 79, 179});
    backgroundSpr->setContentSize({width - 5.f, height + 10.f});
    backgroundSpr->setID("m_slot-background"_spr);
    this->addChildAtPosition(backgroundSpr, Anchor::Center);

    /* Borders */
    auto topBorder = NineSlice::createWithSpriteFrameName("GJ_commentTop2_001.png");
    topBorder->setInsetLeft(topBorder->getContentWidth() / 2);
    topBorder->setContentWidth(width);
    topBorder->setID("top-border"_spr);
    this->addChildAtPosition(topBorder, Anchor::Top);

    auto bottomBorder = NineSlice::createWithSpriteFrameName("GJ_commentTop2_001.png");
    bottomBorder->setInsetLeft(bottomBorder->getContentWidth() / 2);
    bottomBorder->setContentWidth(width);
    bottomBorder->setScaleY(-1.f);
    bottomBorder->setID("bottom-border"_spr);
    this->addChildAtPosition(bottomBorder, Anchor::Bottom);

    auto rightBorder = NineSlice::createWithSpriteFrameName("GJ_commentSide2_001.png");
    rightBorder->setContentHeight(height - 20.f);
    rightBorder->setAnchorPoint({0., 0.5f});
    rightBorder->setScaleX(-1.f);
    rightBorder->setID("right-border"_spr);
    this->addChildAtPosition(rightBorder, Anchor::Right);

    auto leftBorder = NineSlice::createWithSpriteFrameName("GJ_commentSide2_001.png");
    leftBorder->setContentHeight(height - 20.f);
    leftBorder->setAnchorPoint({0., 0.5f});
    leftBorder->setID("left-border"_spr);
    this->addChildAtPosition(leftBorder, Anchor::Left);

    /* Menu */
    auto menu = CCMenu::create();
    menu->setContentSize({width, height - 5.f});
    menu->setLayout(
        (m_slot != 0) // m_slot 0 = automatic m_slot
            ? ColumnLayout::create()->setAxisReverse(true)->setAutoScale(true)->setAxisAlignment(AxisAlignment::Between)
            : RowLayout::create()->setAutoScale(true)->setAxisAlignment(AxisAlignment::Between));
    menu->setID("menu"_spr);

    /* Title */
    auto m_slotTitle =
        CCLabelBMFont::create((m_slot != 0) ? fmt::format("Slot {}", m_slot).c_str() : "Automatic Backup",
                              "bigFont.fnt", menu->getContentWidth() * 2, CCTextAlignment::kCCTextAlignmentCenter);
    m_slotTitle->setID("m_slot-title"_spr);

    /* Info Row */
    auto infoRow = CCNode::create();
    infoRow->setLayout(RowLayout::create()->setAutoScale(true)->setAxisAlignment(AxisAlignment::Between));
    infoRow->setContentWidth((m_slot != 0) ? menu->getScaledContentWidth() : (width / 2.f));
    infoRow->setAnchorPoint({0.5f, 0.5f});
    infoRow->setID("info-row"_spr);

    /* Time Icon */
    auto timeIcon = CCSprite::createWithSpriteFrameName("GJ_timeIcon_001.png");
    timeIcon->setID("time-icon"_spr);
    infoRow->addChild(timeIcon);

    /*Time Label*/
    auto timeLabel = CCLabelBMFont::create("12/03/2026 7:48PM | ", "chatFont.fnt");
    timeLabel->setID("time-label"_spr);
    infoRow->addChild(timeLabel);

    /* Size Icon */
    auto sizeIcon = CCSprite::create("size.png"_spr);
    sizeIcon->setID("size-icon"_spr);
    infoRow->addChild(sizeIcon);

    /*Size Label*/
    auto sizeLabel = CCLabelBMFont::create("302MB", "chatFont.fnt");
    sizeLabel->setID("size-label"_spr);
    infoRow->addChild(sizeLabel);

    infoRow->updateLayout();

    /* Load Button */
    auto loadButton = CCMenuItemSpriteExtra::create(
        ButtonSprite::create("Load", (m_slot != 0) ? menu->getScaledContentWidth() : (width / 4.f),
                             (m_slot != 0) ? menu->getScaledContentWidth() : (width / 4.f), 1.f, false, "goldFont.fnt",
                             "GJ_button_01.png"),
        this, menu_selector(GDriveSlotBox::onLoad));
    loadButton->setID("Load-button"_spr);

    if (m_slot == 0)
    {
        /* Auto Slot Text Column */
        auto textColumn = CCNode::create();
        textColumn->setLayout(ColumnLayout::create()
                                  ->setAxisReverse(true)
                                  ->setAutoScale(true)
                                  ->setAxisAlignment(AxisAlignment::Between)
                                  ->setCrossAxisLineAlignment(AxisAlignment::Start));
        textColumn->setContentHeight(menu->getContentHeight());
        textColumn->setID("text-column"_spr);

        m_slotTitle->setCString("Automatic Backup");
        // add title and info to column
        textColumn->addChild(m_slotTitle);

        textColumn->addChild(infoRow);

        textColumn->updateLayout();
        menu->addChild(textColumn);

        auto buttonColumn = CCMenu::create();
        buttonColumn->setLayout(ColumnLayout::create()->setAxisReverse(true)->setAutoScale(false)->setAxisAlignment(
            AxisAlignment::Between));
        buttonColumn->setContentSize({width / 4.f, menu->getContentHeight()});
        buttonColumn->setID("button-column"_spr);

        loadButton->setScale(0.9);
        loadButton->m_baseScale = 0.9;
        buttonColumn->addChild(loadButton);

        /*Freq Label*/
        auto freqLabel = CCLabelBMFont::create("Every 6 hours", "goldFont.fnt");
        freqLabel->setScale(0.35f);
        freqLabel->setID("freq-label"_spr);
        buttonColumn->addChild(freqLabel);

        buttonColumn->updateLayout();
        menu->addChild(buttonColumn);

        menu->setContentWidth(width - 45.f);
    }
    else
    {
        // Add children to Menu
        menu->addChild(m_slotTitle);
        menu->addChild(infoRow);

        /* Separator */
        auto separator = NineSlice::createWithSpriteFrameName("floorLine_01_001.png");
        separator->setInsetLeft(separator->getContentWidth() / 3);
        separator->setContentSize({menu->getScaledContentWidth(), 1.f});
        separator->setID("separtor"_spr);
        menu->addChild(separator);

        /* Save Button */
        auto saveButton = CCMenuItemSpriteExtra::create(ButtonSprite::create("Save", menu->getScaledContentWidth(),
                                                                             menu->getScaledContentWidth(), 1.f, false,
                                                                             "goldFont.fnt", "GJ_button_01.png"),
                                                        this, menu_selector(GDriveSlotBox::onSave));
        saveButton->setID("save-button"_spr);
        menu->addChild(saveButton);
        menu->addChild(loadButton);
    }

    menu->updateLayout();
    this->addChildAtPosition(menu, Anchor::Center);
    this->updateLayout();
    this->setVisible(true);
    return true;
}
void GDriveSlotBox::onSave(CCObject *sender)
{
}
void GDriveSlotBox::onLoad(CCObject *sender)
{
}
void GDriveSlotBox::setVisible(bool visible)
{
    // if(visible)log::debug("i am {} and i am now {}", m_slot, visible);
    CCNode::setVisible(visible);
}