#pragma once

using namespace geode::prelude;

class GDriveManager : public cocos2d::CCObject
{
  public:
    GDriveManager(GDriveManager const &) = delete;
    void operator=(GDriveManager const &) = delete;
    static GDriveManager *getInstance()
    {
        static GDriveManager *instance = new GDriveManager();
        return instance;
    }
    void login();
    void refresh();

  private:
    GDriveManager();
};