#pragma once
#include "GDriveConsts.hpp"
using namespace geode::prelude;

class GDriveLoadLayer : public Popup
{
  public:
    static GDriveLoadLayer *create();

    void setPercentage(const size_t progress);
    void setMessage(const std::string_view message);
    void showPercentage(const size_t total);
    void showFileInfo(GDriveConsts::slotInformation info);
    void hidePercentage();

  private:
    bool init() override;

    size_t m_total = 0;
    size_t m_progress = 0;
    CCMenu *m_menu;
    CCLabelBMFont *m_statusMessage;
    CCLabelBMFont *m_statusPercentage;
};