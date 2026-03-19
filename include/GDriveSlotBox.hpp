#pragma once
using namespace geode::prelude;

class GDriveSlotBox : public CCNode
{
  public:
    static GDriveSlotBox *create(int slot, float width = 136.6f, float height = 100.f);
    void setVisible(bool visible) override;

    int getSlot();
    void setStatusMessage(std::string_view message);
    void setStatusPercentage(size_t completed);
    void showPercentage(const size_t total);
    void setStatusVisiblity(bool visible);

  private:
    bool init(int slot, float width, float height);

    CCMenu *m_menu;
    CCNode *m_infoRow;
    NineSlice *m_separator;
    CCMenuItemSpriteExtra *m_saveButton;
    CCMenuItemSpriteExtra *m_loadButton;

    CCLabelBMFont *m_timeLabel;
    CCLabelBMFont *m_sizeLabel;

    CCLabelBMFont *m_statusMessage;
    CCLabelBMFont *m_statusPercentage;
    LoadingSpinner *m_statusSpinner;

    int m_slot;
    size_t m_totalSaveSize = 0;

    void onSave(CCObject *sender);
    void onLoad(CCObject *sender);
};
