#pragma once
using namespace geode::prelude;

class GDriveRevisionPopup : public Popup
{
  public:
    static GDriveRevisionPopup *create(const int slot);

  private:
    bool init(const int slot);
    CCLayerColor *createTableRow(std::string accountID, std::string_view slot, std::string_view size, bool darkColor, bool goldFont = false, bool accountLabel = false);
    static constexpr float tableWidth = 340.f;
    static constexpr float tableHeight = 220.f;
};
