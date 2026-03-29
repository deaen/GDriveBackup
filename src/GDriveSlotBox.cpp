#include "GDriveSlotBox.hpp"

#include "GDriveManager.hpp"

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

void GDriveSlotBox::onExitTransitionDidStart()
{
    GDriveManager::getInstance()->removeBoxPointer(GDriveManager::Save, m_slot);
    GDriveManager::getInstance()->removeBoxPointer(GDriveManager::Metadata, m_slot);
    
    CCNode::onExitTransitionDidStart();
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

    /* m_menu */
    m_menu = CCMenu::create();
    m_menu->setContentSize({width, height - 5.f});
    m_menu->setLayout(
        (m_slot != 0) // m_slot 0 = automatic m_slot
            ? ColumnLayout::create()->setAxisReverse(true)->setAutoScale(true)->setAxisAlignment(AxisAlignment::Between)
            : RowLayout::create()->setAutoScale(true)->setAxisAlignment(AxisAlignment::Between));
    m_menu->setID("menu"_spr);
    float buttonSize = m_menu->getScaledContentWidth() + 5.f;
    /* Title */
    m_slotTitle =
        CCLabelBMFont::create((m_slot != 0) ? fmt::format("Slot {}", m_slot).c_str() : "Automatic Backup",
                              "bigFont.fnt", m_menu->getContentWidth() * 2, CCTextAlignment::kCCTextAlignmentCenter);
    m_slotTitle->setID("slot-title"_spr);

    /* Info Row */
    m_infoRow = CCNode::create();
    m_infoRow->setLayout(RowLayout::create()->setAutoScale(true)->setAxisAlignment(AxisAlignment::Center));
    m_infoRow->setContentWidth((m_slot != 0) ? m_menu->getScaledContentWidth() + 17.f : (width / 2.f));
    m_infoRow->setAnchorPoint({0.5f, 0.5f});
    m_infoRow->setID("info-row"_spr);

    /* Time Icon */
    auto timeIcon = CCSprite::createWithSpriteFrameName("GJ_timeIcon_001.png");
    timeIcon->setID("time-icon"_spr);
    m_infoRow->addChild(timeIcon);

    /*Time Label*/
    m_timeLabel = CCLabelBMFont::create("Never Saved", "chatFont.fnt");
    m_timeLabel->setID("time-label"_spr);
    m_infoRow->addChild(m_timeLabel);

    /* Size Icon */
    auto sizeIcon = CCSprite::create("size.png"_spr);
    sizeIcon->setID("size-icon"_spr);
    m_infoRow->addChild(sizeIcon);

    /*Size Label*/
    m_sizeLabel = CCLabelBMFont::create("N/A", "chatFont.fnt");
    m_sizeLabel->setID("size-label"_spr);
    m_infoRow->addChild(m_sizeLabel);

    m_infoRow->updateLayout();

    /* Load Button */
    m_loadButtonSprite = ButtonSprite::create("Load", (m_slot != 0) ? buttonSize : (width / 4.f),
                                              (m_slot != 0) ? buttonSize : (width / 4.f), 1.f, false, "goldFont.fnt",
                                              "GJ_button_01.png");
    m_loadButton = CCMenuItemSpriteExtra::create(m_loadButtonSprite, this, menu_selector(GDriveSlotBox::onLoad));
    m_loadButton->setID("load-button"_spr);
    m_loadButton->m_scaleMultiplier = 1.1f;

    if (m_slot == 0)
    {
        /* Auto Slot Text Column */
        auto textColumn = CCNode::create();
        textColumn->setLayout(ColumnLayout::create()
                                  ->setAxisReverse(true)
                                  ->setAutoScale(true)
                                  ->setAxisAlignment(AxisAlignment::Between)
                                  ->setCrossAxisLineAlignment(AxisAlignment::Start));
        textColumn->setContentHeight(m_menu->getContentHeight());
        textColumn->setID("text-column"_spr);

        m_slotTitle->setCString("Automatic Backup");
        // add title and info to column
        textColumn->addChild(m_slotTitle);

        textColumn->addChild(m_infoRow);

        textColumn->updateLayout();
        m_menu->addChild(textColumn);

        auto buttonColumn = CCMenu::create();
        buttonColumn->setLayout(ColumnLayout::create()->setAxisReverse(true)->setAutoScale(false)->setAxisAlignment(
            AxisAlignment::Between));
        buttonColumn->setContentSize({width / 4.f, m_menu->getContentHeight()});
        buttonColumn->setID("button-column"_spr);

        m_loadButton->setScale(0.9);
        m_loadButton->m_baseScale = 0.9;
        buttonColumn->addChild(m_loadButton);

        /*Freq Label*/
        auto freqLabel = CCLabelBMFont::create("Every 6 hours", "goldFont.fnt");
        freqLabel->setScale(0.35f);
        freqLabel->setID("freq-label"_spr);
        buttonColumn->addChild(freqLabel);

        buttonColumn->updateLayout();
        m_menu->addChild(buttonColumn);

        m_menu->setContentWidth(width - 45.f);
    }
    else
    {
        // Add children to m_menu
        m_menu->addChild(m_slotTitle);
        m_menu->addChild(m_infoRow);

        /* separator */
        m_separator = NineSlice::createWithSpriteFrameName("floorLine_01_001.png");
        m_separator->setInsetLeft(m_separator->getContentWidth() / 3);
        m_separator->setContentSize({m_menu->getScaledContentWidth(), 1.f});
        m_separator->setID("separtor"_spr);
        m_menu->addChild(m_separator);

        /* Save Button */
        m_saveButtonSprite =
            ButtonSprite::create("Save", buttonSize, buttonSize, 1.f, false, "goldFont.fnt", "GJ_button_01.png");
        m_saveButton = CCMenuItemSpriteExtra::create(m_saveButtonSprite, this, menu_selector(GDriveSlotBox::onSave));
        m_saveButton->setID("save-button"_spr);
        m_saveButton->m_scaleMultiplier = 1.1f;
        m_menu->addChild(m_saveButton);
        m_menu->addChild(m_loadButton);

        /* Status Message */
        m_statusMessage = CCLabelBMFont::create("Waiting...", "goldFont.fnt");
        m_statusMessage->setID("status-message"_spr);
        m_menu->addChild(m_statusMessage);

        /* Status Percentage*/
        m_statusPercentage = CCLabelBMFont::create("0% (0/0MB)", "goldFont.fnt");
        m_statusPercentage->setID("status-percentage"_spr);
        m_statusPercentage->setLayoutOptions(AxisLayoutOptions::create()->setNextGap(10.f)->setRelativeScale(0.6f));
        m_menu->addChild(m_statusPercentage);

        /* Cancel Button*/
        m_statusCancel =
            CCMenuItemSpriteExtra::create(ButtonSprite::create("Cancel", "bigFont.fnt", "GJ_button_06.png"), this,
                                          menu_selector(GDriveSlotBox::onCancel));
        m_statusCancel->setID("cancel_button"_spr);
        m_statusCancel->setLayoutOptions(AxisLayoutOptions::create()->setRelativeScale(0.5f));
        m_menu->addChild(m_statusCancel);

        /* Loading Spinner */
        m_statusSpinner = LoadingSpinner::create(60.f);
        m_statusSpinner->setID("status-spinner"_spr);
        m_menu->addChild(m_statusSpinner);
    }

    setStatusVisiblity(false);
    m_menu->updateLayout();
    this->addChildAtPosition(m_menu, Anchor::Center);
    this->updateLayout();

    auto saveStatus = GDriveManager::getInstance()->checkStatus(GDriveManager::Save, this);
    if (saveStatus == GDriveManager::Working)
    {
        setStatusVisiblity(true);
        showPercentage(GDriveManager::getInstance()->getSaveTotal());
        setStatusPercentage(GDriveManager::getInstance()->getSaveProgress());
        setStatusMessage("Saving...");
    }
    else if (saveStatus == GDriveManager::Waiting)
    {
        setStatusVisiblity(true);
        setStatusMessage("Waiting...");
    }
    else
    {
        GDriveManager::getInstance()->addToQueue(GDriveManager::Metadata, this);
        auto savedTimestamp = Mod::get()->getSavedValue<time_t>(
            fmt::format("{}-{}-timestamp", GJAccountManager::sharedState()->m_accountID, m_slot), -1);
        if (savedTimestamp == -1)
        {
            setStatusVisiblity(true);
            setStatusMessage("Loading...");
        }
        else
        {
            updateInfo();

            // i dont understand actions
            float duration = 1.0f;
            auto action = CCRepeatForever::create(
                CCSequence::createWithTwoActions(CCFadeTo::create(duration, 55), CCFadeTo::create(duration, 255)));
            action->setTag(1);
            m_slotTitle->runAction(action);
        }
    }
    return true;
}

void GDriveSlotBox::onSave(CCObject *sender)
{
    createQuickPopup(
        fmt::format("Save Slot {}", m_slot).c_str(),
        fmt::format("Do you want to <cg>save</c> your data to <cj>slot {}</c>?\n<cy>This will</c> <cr>overwrite</c> "
                    "<cy>any previously saved data to this slot!</c>",
                    m_slot)
            .c_str(),
        "Cancel", "Save",
        [this](auto, bool btn2) {
            if (btn2)
            {
                setStatusVisiblity(true);
                setStatusMessage("Waiting...");
                GDriveManager::getInstance()->addToQueue(GDriveManager::Save, this);
            }
        },
        true, true);
}

void GDriveSlotBox::onLoad(CCObject *sender)
{
    createQuickPopup(
        fmt::format("Load Slot {}", m_slot).c_str(),
        fmt::format("Do you want to <cg>load</c> your <cj>slot {}</c> data?\n<cy>This will</c> <cr>overwrite</c> "
                    "<cy>your data!</c>",
                    m_slot)
            .c_str(),
        "Cancel", "Load",
        [this](auto, bool btn2) {
            if (btn2)
            {
                GDriveManager::getInstance()->loadData(m_slot);
            }
        },
        true, true);
}
void GDriveSlotBox::onCancel(CCObject *onCancel)
{
    createQuickPopup(
        fmt::format("Cancel Save to Slot {}", m_slot).c_str(),
        fmt::format("Do you want to <cr>cancel</c> saving to <cj>slot {}</c>?", m_slot).c_str(), "Continue", "Cancel",
        [this](auto, bool btn2) {
            if (btn2)
            {
                setStatusVisiblity(false);
                GDriveManager::getInstance()->removeFromQueue(GDriveManager::Save, m_slot);
            }
        },
        true, true);
}

void GDriveSlotBox::setStatusVisiblity(bool visible)
{
    if (visible)
    {
        if (m_infoRow)
            m_infoRow->setVisible(false);
        if (m_saveButton)
            m_saveButton->setVisible(false);
        if (m_loadButton)
            m_loadButton->setVisible(false);

        if (m_statusMessage)
            m_statusMessage->setVisible(true);
        if (m_statusSpinner)
            m_statusSpinner->setVisible(true);
        if (m_statusSpinner)
            m_statusSpinner->setContentSize({60.f, 60.f});
        if (m_statusSpinner)
            m_statusSpinner->updateLayout();

        if (m_menu)
            m_menu->updateLayout();
    }
    else
    {
        if (m_infoRow)
            m_infoRow->setVisible(true);
        if (m_saveButton)
            m_saveButton->setVisible(true);
        if (m_loadButton)
            m_loadButton->setVisible(true);

        if (m_statusMessage)
            m_statusMessage->setVisible(false);
        if (m_statusPercentage)
            m_statusPercentage->setVisible(false);
        if (m_statusSpinner)
            m_statusSpinner->setVisible(false);
        if (m_statusCancel)
            m_statusCancel->setVisible(false);

        if (m_menu)
            m_menu->updateLayout();
    }
}
void GDriveSlotBox::updateInfo()
{
    auto savedTimestamp = Mod::get()->getSavedValue<time_t>(
        fmt::format("{}-{}-timestamp", GJAccountManager::sharedState()->m_accountID, m_slot));
    auto savedSize = Mod::get()->getSavedValue<size_t>(
        fmt::format("{}-{}-size", GJAccountManager::sharedState()->m_accountID, m_slot));

    m_slotTitle->stopActionByTag(1);
    m_slotTitle->setOpacity(255);

    if (savedSize == 0)
    {
        m_timeLabel->setCString("Never Saved");
        m_sizeLabel->setCString("N/A");
        m_loadButton->setEnabled(false);
        m_loadButtonSprite->setOpacity(175);
        m_loadButtonSprite->setColor({165, 165, 165});
        m_infoRow->updateLayout();
        return;
    }

    m_timeLabel->setCString(fmt::format("{:%-d/%b/%y %-I:%M %p}", localtime(savedTimestamp)).c_str());
    m_sizeLabel->setCString(fmt::format("{:.2f}MB", savedSize / (1024.f * 1024.f)).c_str());

    m_loadButton->setEnabled(true);
    m_loadButtonSprite->setOpacity(255);
    m_loadButtonSprite->setColor({255, 255, 255});
    m_infoRow->updateLayout();

    m_menu->updateLayout();
}

int GDriveSlotBox::getSlot()
{
    return m_slot;
}
void GDriveSlotBox::setStatusMessage(const std::string_view message)
{
    m_statusMessage->setCString(message.data());
}

void GDriveSlotBox::showPercentage(const size_t total)
{
    m_total = total;
    m_statusPercentage->setCString(fmt::format("0% (0/{:.2f}MB)", m_total / (1024.f * 1024.f)).c_str());
    m_statusPercentage->setVisible(true);
    m_statusCancel->setVisible(true);
    m_statusSpinner->setContentSize({28.f, 28.f});
    m_statusSpinner->updateLayout();
    m_menu->updateLayout();
}

void GDriveSlotBox::setStatusPercentage(size_t progress)
{
    if (progress == m_progress)
        return;
    m_statusPercentage->setCString(fmt::format("{}% ({:.2f}/{:.2f}MB)",
                                               std::floor((static_cast<float>(progress) / m_total) * 100.f),
                                               progress / (1024.f * 1024.f), m_total / (1024.f * 1024.f))
                                       .c_str());
    m_progress = progress;
}
