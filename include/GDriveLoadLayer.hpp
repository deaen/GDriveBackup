#pragma once
using namespace geode::prelude;

class GDriveLoadLayer : public FLAlertLayer
{
  public:
    static GDriveLoadLayer *create()
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

    void showPercentage(const size_t total);
    void setPercentage(const size_t progress);
    void hidePercentage();
    void setMessage(const std::string_view message);

  private:
    bool init() override;
    void registerWithTouchDispatcher() override;

    size_t m_total = 0;
    size_t m_progress = 0;
    CCMenu *m_menu;
    CCLabelBMFont *m_statusMessage;
    CCLabelBMFont *m_statusPercentage;
};