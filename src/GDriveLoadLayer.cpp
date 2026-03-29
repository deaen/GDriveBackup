#include "GDriveLoadLayer.hpp"

bool GDriveLoadLayer::init()
{
    if (!FLAlertLayer::init(240))
        return false;

    this->setID("loading-layer"_spr);
    this->setKeypadEnabled(false);

    /* menu */
    m_menu = CCMenu::create();
    m_menu->setContentSize(this->getContentSize());
    m_menu->setLayout(ColumnLayout::create()->setAxisReverse(true)->setAxisAlignment(AxisAlignment::Center));
    m_menu->setID("menu"_spr);

    /* Status Message */
    m_statusMessage = CCLabelBMFont::create("Preparing...", "goldFont.fnt");
    m_statusMessage->setID("status-message"_spr);
    m_menu->addChild(m_statusMessage);

    /* Status Percentage*/
    m_statusPercentage = CCLabelBMFont::create("0% (0/0MB)", "goldFont.fnt");
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

void GDriveLoadLayer::registerWithTouchDispatcher()
{
    FLAlertLayer::registerWithTouchDispatcher();
    CCTouchDispatcher::get()->addPrioTargetedDelegate(static_cast<CCTouchDelegate *>(this),
                                                      CCTouchDispatcher::get()->getTargetPrio(), true);
}

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
    m_statusPercentage->setCString(fmt::format("{}% ({:.2f}/{:.2f}MB)",
                                               std::floor((static_cast<float>(progress) / m_total) * 100.f),
                                               progress / (1024.f * 1024.f), m_total / (1024.f * 1024.f))
                                       .c_str());
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
