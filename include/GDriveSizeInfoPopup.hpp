#pragma once
using namespace geode::prelude;
#include "GDriveManager.hpp"

class GDriveSizeInfoPopup : public Popup
{
  public:
    static GDriveSizeInfoPopup *create(sizedata_map &sizeDataMap);

  private:
    bool init(sizedata_map &sizeDataMap);

    static constexpr float tableWidth = 350.f;
    static constexpr float tableHeight = 195.f;
    enum rowColor
    {
        INVISIBLE,
        LIGHT,
        DARK,
        DARKER,
    };

    CCNode *createTableRow(const std::string_view firstColumnLabel, const std::string_view secondColumnLabel, const std::string_view thirdColumnLabel, const rowColor color, const bool firstColumnGreen = false, const bool firstColumnGolden = false, const bool secondColumnGolden = false, const bool thirdColumnGolden = false, const bool profileButton = false, const std::string accountID = "", const GLfloat rowHeight = 20.f, const bool noGaps = false);
};
