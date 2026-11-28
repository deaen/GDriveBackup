#pragma once
#include "GDriveSlotBox.hpp"
using namespace geode::prelude;


class GDriveBackupBox : public GDriveSlotBox{
    public:
      static GDriveBackupBox *create();
      bool init();
};