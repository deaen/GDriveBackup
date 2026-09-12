#pragma once
using namespace geode::prelude;
#include "GDriveManager.hpp"

class GDriveSizeInfoPopup : public Popup
{
  public:
    static GDriveSizeInfoPopup *create(const sizedata_map &sizeDataMap);

  private:
    bool init(const sizedata_map &sizeDataMap);
    CCLayerColor *createTableRow(std::string buttonLabel, std::string accountID, std::string_view slot, std::string_view size, bool darkColor, bool goldFont = false, bool accountLabel = false);
    float calculatePercentageScale(const CCSize dividend, const CCSize divisor, const float percentage);
    static constexpr float tableWidth = 350.f;
    static constexpr float tableHeight = 220.f;
};
