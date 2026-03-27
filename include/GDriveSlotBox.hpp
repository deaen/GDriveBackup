#pragma once
using namespace geode::prelude;

class GDriveSlotBox : public CCNode
{
  public:
    static GDriveSlotBox *create(int slot, float width = 136.6f, float height = 100.f);
    void onExitTransitionDidStart() override;

    int getSlot();
    void setStatusMessage(std::string_view message);
    void setStatusPercentage(const size_t progress);
    void showPercentage(const size_t total);
    void setStatusVisiblity(bool visible);

    void updateInfo();

  private:
    bool init(int slot, float width, float height);

    CCMenu *m_menu;
    CCLabelBMFont* m_slotTitle;
    CCNode *m_infoRow;
    NineSlice *m_separator;
    CCMenuItemSpriteExtra *m_saveButton;
    ButtonSprite* m_saveButtonSprite;
    ButtonSprite* m_loadButtonSprite;
    CCMenuItemSpriteExtra *m_loadButton;

    CCLabelBMFont *m_timeLabel;
    CCLabelBMFont *m_sizeLabel;

    CCLabelBMFont *m_statusMessage;
    CCLabelBMFont *m_statusPercentage;
    LoadingSpinner *m_statusSpinner;
    CCMenuItemSpriteExtra *m_statusCancel;

    int m_slot = 0;
    size_t m_total= 0;
    size_t m_progress= 0;

    void onSave(CCObject *sender);
    void onLoad(CCObject *sender);
    void onCancel(CCObject *onCancel);
};
