#pragma once
using namespace geode::prelude;

class GDriveSlotBox : public CCNode
{
  public:
    static GDriveSlotBox *create(int slot, float width = 136.6f, float height = 100.f);
    void setVisible(bool visible) override;
  private:
    bool init(int slot, float width, float height);
    void onSave(CCObject *sender);
    void onLoad(CCObject *sender);

    int m_slot;
};
