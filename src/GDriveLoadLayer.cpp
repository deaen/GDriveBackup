#include "GDriveLoadLayer.hpp"
#include "GDriveSaveToggles.hpp"

GDriveLoadLayer *GDriveLoadLayer::create()
{
    auto ret = new GDriveLoadLayer();
    if (ret->init())
    {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}

bool GDriveLoadLayer::init()
{
    if (!FLAlertLayer::init(240))
        return false;

    this->setID("loading-layer"_spr);
    this->setKeypadEnabled(false);

    /* Menu */
    m_menu = CCMenu::create();
    m_menu->setContentSize(this->getContentSize());
    m_menu->setLayout(ColumnLayout::create()->setAxisReverse(true)->setAxisAlignment(AxisAlignment::Center)->setGap(5.f));
    m_menu->setID("menu"_spr);

    /* Status Message */
    m_statusMessage = CCLabelBMFont::create("please wait...", "goldFont.fnt");
    m_statusMessage->setID("status-message"_spr);
    m_menu->addChild(m_statusMessage);

    /* Status Percentage */
    m_statusPercentage = CCLabelBMFont::create("please wait...", "goldFont.fnt");
    m_statusPercentage->setID("status-percentage"_spr);
    m_statusPercentage->setLayoutOptions(AxisLayoutOptions::create()->setPrevGap(10.f)->setRelativeScale(0.6f));
    m_statusPercentage->setVisible(false);
    m_menu->addChild(m_statusPercentage);

    /* Loading Spinner */
    auto statusSpinner = LoadingSpinner::create(60.f);
    statusSpinner->setID("status-spinner"_spr);
    m_menu->addChild(statusSpinner);

    m_menu->updateLayout();
    this->addChild(m_menu);

    return true;
}

void GDriveLoadLayer::showPercentage(const size_t total)
{
    m_total = total;
    m_statusPercentage->setCString(fmt::format("0% (0/{:.2f}MB)", m_total / (1024.f * 1024.f)).c_str());
    m_statusPercentage->setVisible(true);
    m_menu->updateLayout();
}

void GDriveLoadLayer::setPercentage(const size_t progress)
{
    if (progress == m_progress)
        return;
    m_statusPercentage->setCString(fmt::format("{}% ({:.2f}/{:.2f}MB)", std::floor((static_cast<float>(progress) / m_total) * 100.f), progress / (1024.f * 1024.f), m_total / (1024.f * 1024.f)).c_str());
    m_progress = progress;
}

void GDriveLoadLayer::hidePercentage()
{
    m_statusPercentage->setVisible(false);
    m_menu->updateLayout();
}

void GDriveLoadLayer::setMessage(const std::string_view message)
{
    m_statusMessage->setCString(message.data());
}

void GDriveLoadLayer::showFileInfo(GDriveConsts::slotInformation info)
{
    if(info.type == GDriveConsts::slotType::Old) return;

    auto bg = NineSlice::create("GJ_square01.png");
    bg->setID("info-background"_spr);
    bg->setContentSize({400.f, 90.f});
    bg->setAnchorPoint({0.f, 0.f});

    auto titleLabel = Label::create("extras that'll be loaded:", "goldFont.fnt");
    titleLabel->setID("title-label"_spr);
    titleLabel->setScale(0.5f);

    auto infoLabel = Label::createRich(
            std::string((info.platformerSaves && Mod::get()->getSettingValue<GDriveConsts::backupSettingValue>("platformer-data").load) ? ":check: <cg>" : ":cross: <cr>")
            + "Platformer saves</c> \n"
            + ((info.editorBackups && Mod::get()->getSettingValue<GDriveConsts::backupSettingValue>("level-back-data").load) ? ":check: <cg>" : ":cross: <cr>")
            + "Editor auto-backups</c> \n"
            + ((info.keybinds && Mod::get()->getSettingValue<GDriveConsts::backupSettingValue>("keybind-data").load) ? ":check: <cg>" : ":cross: <cr>")
            + "Custom keybinds</c>\n"
            + ((info.gameOptions && Mod::get()->getSettingValue<GDriveConsts::backupSettingValue>("game-options").load) ? ":check: <cg>" : ":cross: <cr>")
            + "Game & editor options  </c>",
            "bigFont.fnt");
    infoLabel->setID("info-label"_spr);
    infoLabel->setScale(0.4f);

    bg->addChild(titleLabel);
    bg->addChild(infoLabel);
    bg->setLayout(ColumnLayout::create()->setAutoScale(false)->setGap(4.f)->setAxisReverse(true)->setAxisAlignment(AxisAlignment::End)->setCrossAxisLineAlignment(AxisAlignment::Start)->setPadding({7.f, 7.f, 0, 0}));

    this->addChildAtPosition(bg, geode::Anchor::BottomLeft, {5.f, 5.f}, false);

    bg->setScale(0.f);
    bg->runAction(CCEaseBackOut::create(CCScaleTo::create(.4, 1.f)));
}
