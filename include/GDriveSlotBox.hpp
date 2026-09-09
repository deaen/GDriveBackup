#pragma once
using namespace geode::prelude;

class GDriveSlotBox : public CCNode
{
  public:
    static GDriveSlotBox *create(int slot, bool enableEditMode = false, float width = 136.6f, float height = 100.f);
    void onExitTransitionDidStart() override;

    int getSlot();
    void setStatusMessage(std::string_view message);
    void setStatusPercentage(const size_t progress);
    void showPercentage(const size_t total);
    void setStatusVisiblity(bool visible);

    void updateInfo();
    void setEditMode(bool on);
    void setShouldEditMode(bool should);
    void setWorkingStatus(bool busy);
    void setMetadataStatus(bool gettingMetadata);

    bool getEditMode();
    bool getShouldEditMode();

  private:
    bool init(int slot, bool enableEditMode, float width, float height);

    CCMenu *m_menu;
    TextInput *m_slotTitle;
    CCNode *m_infoRow;
    CCNode *m_timeColumn;
    NineSlice *m_separator;
    CCMenuItemSpriteExtra *m_saveButton;
    ButtonSprite *m_saveButtonSprite;
    ButtonSprite *m_loadButtonSprite;
    CCMenuItemSpriteExtra *m_loadButton;

    CCLabelBMFont *m_dateLabel;
    CCLabelBMFont *m_timeLabel;
    CCLabelBMFont *m_sizeLabel;

    CCLabelBMFont *m_statusMessage;
    CCLabelBMFont *m_statusPercentage;
    LoadingSpinner *m_statusSpinner;
    CCMenuItemSpriteExtra *m_statusCancel;

    // CCMenu *m_editMenu;
    ButtonSprite *m_titleButtonSprite;
    ButtonSprite *m_deleteButtonSprite;
    ButtonSprite *m_revisionButtonSprite;
    CCMenuItemSpriteExtra *m_titleButton;
    CCMenuItemSpriteExtra *m_deleteButton;
    CCMenuItemSpriteExtra *m_revisionButton;
    // LoadingSpinner *m_editSpinner;

    int m_slot = 0;
    size_t m_total = 0;
    size_t m_progress = 0;

    time_t m_savedTimestamp = 0;
    size_t m_savedSize = 0;
    std::string m_savedDescription;

    bool m_busy = false;
    bool m_empty = false;
    bool m_gettingMetadata = false;
    bool m_editMode = false;
    bool m_shouldEditMode = false;

    void updateStatus();

    void onSave(CCObject *sender);
    void onLoad(CCObject *sender);
    void onCancel(CCObject *sender);
    void onDelete(CCObject *sender);
    void onRevision(CCObject *sender);
    void onConfirmTitle(CCObject *sender);

    float getCalculatedScale(float childWidth, float childScale);
};
