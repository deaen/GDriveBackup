#include "GDriveSlotBox.hpp"
#include "GDriveManager.hpp"

GDriveSlotBox *GDriveSlotBox::create(int slot, bool enableEditMode)
{
    auto ret = new GDriveSlotBox();
    if (ret->init(slot, enableEditMode))
    {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}

void GDriveSlotBox::onExitTransitionDidStart()
{
    GDriveManager::getInstance()->removeBoxPointer(getSlot());
    CCNode::onExitTransitionDidStart();
}

bool GDriveSlotBox::init(int slot, bool enableEditMode)
{
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
    m_slotTitle = TextInput::create(this->getContentWidth(), "enter slot title", "bigFont.fnt");
    m_slotTitle->setString(fmt::format("Slot {}", getSlot()));
    m_slotTitle->hideBG();
    m_slotTitle->setEnabled(false);
    m_slotTitle->setCommonFilter(CommonFilter::Any);
    m_slotTitle->setMaxCharCount(30);
    m_slotTitle->getInputNode()->setMaxLabelScale(1.f);
    m_slotTitle->setAnchorPoint({0.5f, 0.5f});
    m_slotTitle->setID("slot-title"_spr);

    /* Info Row */
    m_infoRow = CCNode::create();
    m_infoRow->setContentWidth(m_menu->getScaledContentWidth() + 17.f);
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

    /* Edit Menu */
    m_editMenu = CCMenu::create();
    m_editMenu->setLayout(ColumnLayout::create()->setGap(0)->setAxisAlignment(AxisAlignment::End));
    m_editMenu->setID("edit-menu"_spr);
    m_editMenu->setScale(0.45f);
    m_editMenu->setAnchorPoint({.5f, 1.f});
    this->addChildAtPosition(m_editMenu, Anchor::TopLeft, {10.f, 8.f});

    /* Title Button */
    m_titleButton = CCMenuItemSpriteExtra::create(CircleButtonSprite::create(CCLabelBMFont::create("A", "bigFont.fnt"), CircleBaseColor::Green, CircleBaseSize::Small), this, menu_selector(GDriveSlotBox::onConfirmTitle));
    m_titleButton->setID("title-button"_spr);
    m_editMenu->addChild(m_titleButton);

    /* Delete Button */
    m_deleteButton = CCMenuItemSpriteExtra::create(CCSprite::createWithSpriteFrameName("GJ_deleteSongBtn_001.png"), this, menu_selector(GDriveSlotBox::onDelete));
    m_deleteButton->setID("delete-button"_spr);
    m_editMenu->addChild(m_deleteButton);

    /* Loading Spinner */
    m_editSpinner = LoadingSpinner::create(40.f);
    m_editSpinner->setID("edit-spinner"_spr);
    m_editMenu->addChild(m_editSpinner);

    m_titleButton->setVisible(false);
    m_deleteButton->setVisible(false);
    m_editSpinner->setVisible(false);
    m_menu->setLayout(ColumnLayout::create()->setAxisReverse(true)->setAutoScale(true)->setAxisAlignment(AxisAlignment::Between));

    /* Add children to m_menu */
    m_menu->addChild(m_slotTitle);
    m_menu->addChild(m_infoRow);

    /* Separator */
    m_separator = NineSlice::createWithSpriteFrameName("floorLine_01_001.png");
    m_separator->setInsetLeft(m_separator->getContentWidth() / 3);
    m_separator->setContentSize({m_menu->getScaledContentWidth(), 1.f});
    m_separator->setID("separator"_spr);
    m_menu->addChild(m_separator);

    /* Save Button */
    m_saveButtonSprite = ButtonSprite::create("Save", m_menu->getScaledContentWidth(), m_menu->getScaledContentWidth(), 1.f, false, "goldFont.fnt", "GJ_button_01.png");
    m_saveButton = CCMenuItemSpriteExtra::create(m_saveButtonSprite, this, menu_selector(GDriveSlotBox::onSave));
    m_saveButton->setID("save-button"_spr);
    m_saveButton->m_scaleMultiplier = 1.1f;
    m_menu->addChild(m_saveButton);

    /* Load Button */
    m_loadButtonSprite = ButtonSprite::create("Load", m_menu->getScaledContentWidth(), m_menu->getScaledContentWidth(), 1.f, false, "goldFont.fnt", "GJ_button_01.png");
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

    setStatusVisiblity(false);
    this->addChildAtPosition(m_menu, Anchor::Center);

    updateStatus();

    return true;
}

void GDriveSlotBox::updateStatus()
{
    auto saveStatus = GDriveManager::getInstance()->checkStatus(this);
    if (saveStatus == GDriveManager::Working)
    {
        setStatusVisiblity(true);
        showPercentage(GDriveManager::getInstance()->getSaveTotal());
        setStatusPercentage(GDriveManager::getInstance()->getSaveProgress());
        setStatusMessage("Saving...");

        setEditMode(false);
    }
    else if (saveStatus == GDriveManager::Waiting)
    {
        setStatusVisiblity(true);
        setStatusMessage("Waiting...");

        setEditMode(false);
    }
    else
    {
        setMetadataStatus(true);

        auto metadataMap = GDriveManager::getInstance()->getMetadataMap();
        if (metadataMap && !metadataMap->contains(getSlot()) && GDriveManager::getInstance()->getMetadataStatus())
        {
            setStatusVisiblity(true);
            setStatusMessage("Loading...");
        }
        else
            updateInfo();

        if (GDriveManager::getInstance()->getMetadataStatus())
        {
            float duration = 1.0f;
            auto action = CCRepeatForever::create(CCSequence::createWithTwoActions(CCFadeTo::create(duration, 55), CCFadeTo::create(duration, 255)));
            action->setTag(1);
            m_slotTitle->getInputNode()->getTextLabel()->runAction(action);
        }

        if (getEditMode())
        {
            setShouldEditMode(true);
            setEditMode(true);
        }
    }
}

void GDriveSlotBox::onSave(CCObject *sender)
{
    createQuickPopup(
        fmt::format("Save Slot {}", getSlot()).c_str(),
        fmt::format("Do you want to <cg>save</c> your data to <cj>slot {}</c>?\n<cy>This will</c> <cr>overwrite</c> <cy>any previously saved data to this slot!</c>", getSlot()).c_str(),
        "Cancel", "Save",
        [this](auto, bool btn2) {
            if (btn2)
            {
                setWorkingStatus(true);
                setStatusVisiblity(true);
                setStatusMessage("Waiting...");

                if (getEditMode())
                {
                    setShouldEditMode(true);
                    setEditMode(true);
                }

                GDriveManager::getInstance()->addToQueue(this);
            }
        },
        true, true);
}

void GDriveSlotBox::onLoad(CCObject *sender)
{
    createQuickPopup(
        fmt::format("Load Slot {}", getSlot()).c_str(),
        fmt::format("Do you want to <cg>load</c> your <cj>slot {}</c> data?\n<cy>This will merge your data!</c>", getSlot()).c_str(),
        "Cancel", "Load",
        [this](auto, bool btn2) {
            if (btn2)
                GDriveManager::getInstance()->loadData(getSlot());
        },
        true, true);
}

void GDriveSlotBox::onCancel(CCObject *sender)
{
    createQuickPopup(
        fmt::format("Cancel Save to Slot {}", getSlot()).c_str(),
        fmt::format("Do you want to <cr>cancel</c> saving to <cj>slot {}</c>?", getSlot()).c_str(), "Continue", "Cancel",
        [this](auto, bool btn2) {
            if (btn2)
            {
                setStatusVisiblity(false);
                GDriveManager::getInstance()->removeFromQueue(getSlot());
            }
        },
        true, true);
}

void GDriveSlotBox::onDelete(CCObject *sender)
{
    auto deletePopup = createQuickPopup(
        "Delete save file",
        fmt::format("Are you sure you want to <cr>permanently delete</c> <cy>slot {}'s</c> save file? This is irreversible!\n<cg>(Tip: moving the save file to the trash in the google drive website/app is also an option!)</c>", getSlot()),
        "Cancel",
        "Delete",
        [this](auto, bool btn2) {
            if (!btn2)
            {
                return;
            }
            auto doubleUp = createQuickPopup(
                "Double check",
                "Double checking if you really want to <cr>permanently delete</c> this save? There is no going back...",
                "Cancel",
                "Really delete",
                [this](auto, bool btn2) {
                    if (btn2)
                    {
                        GDriveLoadLayer *layer = GDriveLoadLayer::create();
                        layer->setMessage(fmt::format("Deleting Slot {}...", getSlot()));
                        layer->show();
                        async::spawn(GDriveManager::getInstance()->deleteFile(getSlot()), [layer, this](bool ok) {
                            layer->removeFromParent();
                            if (ok)
                            {
                                Notification::create(fmt::format("Save {} successfully deleted", getSlot()), NotificationIcon::Success, 3.f)->show();
                                auto metadataMap = GDriveManager::getInstance()->getMetadataMap();
                                if (metadataMap && metadataMap->contains(getSlot()))
                                    metadataMap->erase(getSlot());
                                updateStatus();
                            }
                        });
                    }
                },
                false,
                true);
            doubleUp->m_button2->updateBGImage("GJ_button_06.png");
            doubleUp->show();
        },
        false,
        true);

    deletePopup->m_button2->updateBGImage("GJ_button_06.png");
    deletePopup->show();
}

void GDriveSlotBox::onConfirmTitle(CCObject *sender)
{
    auto input = m_slotTitle->getString();
    createQuickPopup((input.empty()) ? "Reset slot title" : "Change slot title", (input.empty()) ? fmt::format("<cr>Reset</c> slot {}'s title to default?", getSlot()) : fmt::format("Change slot {}'s title to <cy>{}</c>?\n<cl>(the new title will be uploaded to google drive)</c>", getSlot(), input), "Cancel", "Confirm", [input, this](auto, bool btn2) {
        if (btn2)
        {
            GDriveLoadLayer *layer = GDriveLoadLayer::create();
            layer->setMessage("Setting new title...");
            layer->show();
            async::spawn(GDriveManager::getInstance()->setDescription(input, getSlot()), [layer, this](bool ok) {
                layer->removeFromParent();
                if (ok)
                {
                    Notification::create("New slot title successfully set!", NotificationIcon::Success, 3.f)->show();
                    updateInfo();
                }
            });
        }
        else
            updateInfo();

        m_infoRow->setVisible(true);
        m_menu->updateLayout();
        m_slotTitle->setCallbackEnabled(true);
    });
}

void GDriveSlotBox::setStatusVisiblity(bool visible)
{
    if (visible)
    {
        m_infoRow->setVisible(false);
        m_saveButton->setVisible(false);
        m_loadButton->setVisible(false);

        m_statusMessage->setVisible(true);

        m_statusSpinner->setVisible(true);
        m_statusSpinner->setContentSize({60.f, 60.f});
        m_statusSpinner->updateLayout();

        m_slotTitle->getInputNode()->getTextLabel()->setOpacity(255);
    }
    else
    {
        m_infoRow->setVisible(true);
        m_saveButton->setVisible(true);
        m_loadButton->setVisible(true);

        m_statusMessage->setVisible(false);
        m_statusPercentage->setVisible(false);
        m_statusSpinner->setVisible(false);
        m_statusCancel->setVisible(false);
    }

    m_menu->updateLayout();
}

void GDriveSlotBox::updateInfo()
{
    auto metadataMap = GDriveManager::getInstance()->getMetadataMap();
    if (metadataMap && metadataMap->contains(getSlot()))
    {
        setMetadataStatus(false);
        m_savedTimestamp = utils::numFromString<time_t>(metadataMap->at(getSlot())["timestamp"]).unwrapOrDefault();
        m_savedSize = utils::numFromString<size_t>(metadataMap->at(getSlot())["size"]).unwrapOrDefault();
        m_savedDescription = metadataMap->at(getSlot())["description"];
    }
    else{
        m_savedTimestamp = 0;
        m_savedSize = 0;
        m_savedDescription = "";
    }

    if (m_savedSize == 0)
    {
        m_timeLabel->setCString("Never Saved");
        m_dateLabel->setVisible(false);
        m_sizeLabel->setCString("N/A");
        m_loadButton->setEnabled(false);
        m_loadButtonSprite->setOpacity(175);
        m_loadButtonSprite->setColor(ccGRAY);
        m_empty = true;
    }
    else
    {
        auto time = localtime(m_savedTimestamp);
        m_timeLabel->setCString(fmt::format("{:%-I:%M %p}", time).c_str());
        m_timeLabel->setVisible(true);

        m_dateLabel->setCString(fmt::format("{:%b %-d, %Y}", time).c_str());
        m_dateLabel->setVisible(true);

        m_sizeLabel->setCString(fmt::format("{:.2f}MB", m_savedSize / (1024.f * 1024.f)).c_str());
        m_sizeLabel->setVisible(true);

        m_loadButton->setEnabled(true);
        m_loadButtonSprite->setOpacity(255);
        m_loadButtonSprite->setColor({255, 255, 255});
        if (m_savedDescription.empty())
            m_slotTitle->setString(fmt::format("Slot {}", getSlot()));
        else
            m_slotTitle->setString(m_savedDescription);
        m_empty = false;
    }

    m_slotTitle->getInputNode()->getTextLabel()->stopActionByTag(1);
    m_slotTitle->getInputNode()->getTextLabel()->setOpacity(255);

    m_timeColumn->updateLayout();
    m_infoRow->updateLayout();
    m_menu->updateLayout();
}

void GDriveSlotBox::setEditMode(bool on)
{
    m_editMode = on;

    const bool isBusy = m_busy || m_gettingMetadata;
    const bool showEditControls = m_editMode && !isBusy && !m_empty;

    m_slotTitle->getBGSprite()->setVisible(showEditControls);
    m_slotTitle->setEnabled(showEditControls);

    m_titleButton->setVisible(showEditControls);
    m_deleteButton->setVisible(showEditControls);

    m_editSpinner->setVisible(m_editMode && isBusy && !m_empty);

    if (!m_editMode)
        updateInfo();

    m_editMenu->updateLayout();
}

void GDriveSlotBox::setShouldEditMode(bool should)
{
    m_shouldEditMode = should;
}

void GDriveSlotBox::setWorkingStatus(bool busy)
{
    m_busy = busy;
}

void GDriveSlotBox::setMetadataStatus(bool gettingMetadata)
{
    m_gettingMetadata = gettingMetadata;
}
bool GDriveSlotBox::getEditMode()
{
    return m_editMode;
}

bool GDriveSlotBox::getShouldEditMode()
{
    return m_shouldEditMode;
};

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
    m_statusPercentage->setCString(fmt::format("{}% ({:.2f}/{:.2f}MB)", std::floor((static_cast<float>(progress) / m_total) * 100.f), progress / (1024.f * 1024.f), m_total / (1024.f * 1024.f)).c_str());
    m_progress = progress;
}

float GDriveSlotBox::getCalculatedScale(float childWidth, float childScale)
{
    return (childWidth > this->getContentWidth()) ? (this->getContentWidth()) / childWidth : childScale;
}

void GDriveSlotBox::loadMetadata()
{
    auto saveStatus = GDriveManager::getInstance()->checkStatus(this);
    if (saveStatus == GDriveManager::Status::Idle)
    {
        setStatusVisiblity(false);
        updateInfo();

        if (getShouldEditMode())
        {
            setShouldEditMode(false);
            setEditMode(true);
        }
    }
};
