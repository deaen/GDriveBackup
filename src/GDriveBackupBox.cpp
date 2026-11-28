#include "GDriveBackupBox.hpp"


GDriveBackupBox *GDriveBackupBox::create()
{
    auto ret = new GDriveBackupBox();
    if (ret->init())
    {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}

bool GDriveBackupBox::init(){
     if (!CCNode::init())
        return false;

    m_slot = slot;
    m_editMode = enableEditMode;
    m_shouldEditMode = enableEditMode;

    this->setContentSize({width, height});
    this->setAnchorPoint({0.5f, 0.5f});
    this->setID(fmt::format("slot-box-{}", getSlot()));

    /* Background */
    auto backgroundSpr = NineSlice::create("square02b_001.png");
    backgroundSpr->setInsets({backgroundSpr->getContentWidth() / 3.f, backgroundSpr->getContentHeight() / 3.f, backgroundSpr->getContentWidth() / 3.f, backgroundSpr->getContentHeight() / 3.f});
    backgroundSpr->setAnchorPoint({0.5, 0.5});
    backgroundSpr->setColor({53, 79, 179});
    backgroundSpr->setContentSize({width - 5.f, height + 10.f});
    backgroundSpr->setID("background"_spr);
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
    rightBorder->setAnchorPoint({0.f, 0.5f});
    rightBorder->setScaleX(-1.f);
    rightBorder->setID("right-border"_spr);
    this->addChildAtPosition(rightBorder, Anchor::Right);

    auto leftBorder = NineSlice::createWithSpriteFrameName("GJ_commentSide2_001.png");
    leftBorder->setContentHeight(height - 20.f);
    leftBorder->setAnchorPoint({0., 0.5f});
    leftBorder->setID("left-border"_spr);
    this->addChildAtPosition(leftBorder, Anchor::Left);

    /* Menu */
    m_menu = CCMenu::create();
    m_menu->setContentSize({width, height - 5.f});
    m_menu->setID("menu"_spr);

    /* Title */
    m_slotTitle = TextInput::create(this->getContentWidth(), "enter title", "bigFont.fnt");
    m_slotTitle->setString(fmt::format("Slot {}", getSlot()));
    m_slotTitle->hideBG();
    m_slotTitle->setEnabled(false);
    m_slotTitle->setCommonFilter(CommonFilter::Any);
    m_slotTitle->setMaxCharCount(30);
    m_slotTitle->setFontScale(.9f);
    m_slotTitle->setAnchorPoint({0.5f, 0.5f});
    m_slotTitle->setID("slot-title"_spr);

    /* Info Row */
    m_infoRow = CCNode::create();
    m_infoRow->setContentWidth(width + 17.f);
    m_infoRow->setAnchorPoint({0.5f, 0.5f});
    m_infoRow->setID("info-row"_spr);

    /* Time Icon */
    auto timeIcon = CCSprite::createWithSpriteFrameName("GJ_timeIcon_001.png");
    timeIcon->setLayoutOptions(SimpleAxisLayoutOptions::create()->setMaxRelativeScale(0.6f));
    timeIcon->setID("time-icon"_spr);
    m_infoRow->addChild(timeIcon);

    /* Time Column */
    m_timeColumn = CCNode::create();
    m_timeColumn->setContentHeight(25.f);
    m_timeColumn->setAnchorPoint({0.5f, 0.5f});
    m_timeColumn->setID("time-column"_spr);

    /*Date Label */
    m_dateLabel = CCLabelBMFont::create("Saved", "chatFont.fnt");
    m_dateLabel->setID("time-label"_spr);
    m_timeColumn->addChild(m_dateLabel);

    /*Time Label */
    m_timeLabel = CCLabelBMFont::create("Never", "chatFont.fnt");
    m_timeLabel->setID("time-label"_spr);
    m_timeColumn->addChild(m_timeLabel);

    m_timeColumn->setLayout(ColumnLayout::create()->setAutoScale(true)->setCrossAxisLineAlignment(AxisAlignment::Start)->setGap(0));
    m_infoRow->addChild(m_timeColumn);

    /* Size Icon */
    auto sizeIcon = CCSprite::create("size.png"_spr);
    sizeIcon->setLayoutOptions(SimpleAxisLayoutOptions::create()->setMaxRelativeScale(0.6f));
    sizeIcon->setID("size-icon"_spr);
    m_infoRow->addChild(sizeIcon);

    /* Size Label */
    m_sizeLabel = CCLabelBMFont::create("N/A", "chatFont.fnt");
    m_sizeLabel->setID("size-label"_spr);
    m_infoRow->addChild(m_sizeLabel);

    m_infoRow->setLayout(RowLayout::create()->setAutoScale(true)->setAxisAlignment(AxisAlignment::Center));

    m_menu->setLayout(ColumnLayout::create()->setAxisReverse(true)->setAutoScale(true)->setAxisAlignment(AxisAlignment::Between));
    /* Add children to m_menu */
    m_menu->addChild(m_slotTitle);
    m_menu->addChild(m_infoRow);

    /* Separator */
    m_separator = NineSlice::createWithSpriteFrameName("floorLine_01_001.png");
    m_separator->setInsetLeft(m_separator->getContentWidth() / 3);
    m_separator->setContentSize({width, 1.f});
    m_separator->setID("separator"_spr);
    m_menu->addChild(m_separator);

    /* Save Button */
    m_saveButtonSprite = ButtonSprite::create("Save", width, width, 1.f, false, "goldFont.fnt", "GJ_button_01.png");
    m_saveButton = CCMenuItemSpriteExtra::create(m_saveButtonSprite, this, menu_selector(GDriveSlotBox::onSave));
    m_saveButton->setID("save-button"_spr);
    m_saveButton->m_scaleMultiplier = 1.1f;
    m_menu->addChild(m_saveButton);

    /* Load Button */
    m_loadButtonSprite = ButtonSprite::create("Load", width, width, 1.f, false, "goldFont.fnt", "GJ_button_01.png");
    m_loadButton = CCMenuItemSpriteExtra::create(m_loadButtonSprite, this, menu_selector(GDriveSlotBox::onLoad));
    m_loadButton->setID("load-button"_spr);
    m_loadButton->m_scaleMultiplier = 1.1f;
    m_menu->addChild(m_loadButton);

    /* Status Message */
    m_statusMessage = CCLabelBMFont::create("Waiting...", "goldFont.fnt");
    m_statusMessage->setID("status-message"_spr);
    m_menu->addChild(m_statusMessage);

    /* Status Percentage */
    m_statusPercentage = CCLabelBMFont::create("0% (0/0MB)", "goldFont.fnt");
    m_statusPercentage->setID("status-percentage"_spr);
    m_statusPercentage->setLayoutOptions(AxisLayoutOptions::create()->setNextGap(10.f)->setRelativeScale(0.6f));
    m_menu->addChild(m_statusPercentage);

    /* Cancel Button */
    m_statusCancel = CCMenuItemSpriteExtra::create(ButtonSprite::create("Cancel", "bigFont.fnt", "GJ_button_06.png"), this, menu_selector(GDriveSlotBox::onCancel));
    m_statusCancel->setID("cancel-button"_spr);
    m_statusCancel->setLayoutOptions(AxisLayoutOptions::create()->setRelativeScale(0.5f));
    m_menu->addChild(m_statusCancel);

    /* Loading Spinner */
    m_statusSpinner = LoadingSpinner::create(60.f);
    m_statusSpinner->setID("status-spinner"_spr);
    m_menu->addChild(m_statusSpinner);

    /* Edit Menu */
    m_editMenu = CCNode::create();
    m_editMenu->setContentSize({width, 95.f});
    m_editMenu->setID("edit-menu"_spr);
    m_menu->addChild(m_editMenu);

    auto firstEditRow = CCMenu::create();
    firstEditRow->setContentWidth(width);
    firstEditRow->setID("first-edit-row"_spr);
    m_editMenu->addChild(firstEditRow);

    auto secondEditRow = CCMenu::create();
    secondEditRow->setContentWidth(width);
    secondEditRow->setID("second-edit-row"_spr);
    m_editMenu->addChild(secondEditRow);

    /* Delete Button */
    m_deleteButton = CCMenuItemSpriteExtra::create(CCSprite::create("editDeleteBtn.png"_spr), this, menu_selector(GDriveSlotBox::onDelete));
    m_deleteButton->setID("delete-button"_spr);
    firstEditRow->addChild(m_deleteButton);

    /* Title Button */
    m_titleButton = CCMenuItemSpriteExtra::create(CCSprite::create("editRenameBtn.png"_spr), this, menu_selector(GDriveSlotBox::onConfirmTitle));
    m_titleButton->setID("title-button"_spr);
    firstEditRow->addChild(m_titleButton);

    /* slot info Button */
    m_slotInfoButton = CCMenuItemSpriteExtra::create(CCSprite::create("editInfoBtn.png"_spr), this, menu_selector(GDriveSlotBox::onSlotInfo));
    m_slotInfoButton->setID("slot-info-button"_spr);
    secondEditRow->addChild(m_slotInfoButton);

    /* history Button */
    m_historyButton = CCMenuItemSpriteExtra::create(CCSprite::create("editHistoryBtn.png"_spr), this, menu_selector(GDriveSlotBox::onHistory));
    m_historyButton->setID("history-button"_spr);
    secondEditRow->addChild(m_historyButton);

    auto editRowLayout = RowLayout::create()->setAxisAlignment(geode::AxisAlignment::Between);
    firstEditRow->setLayout(editRowLayout);
    secondEditRow->setLayout(editRowLayout);
    m_editMenu->setLayout(ColumnLayout::create()->setAxisAlignment(geode::AxisAlignment::Between)->setAutoScale(false)->setAxisReverse(true));
    m_editMenu->setVisible(false);

    // setStatusVisiblity(false);
    this->addChildAtPosition(m_menu, Anchor::Center);

    // updateStatus();

    return true;

    return true;
}
