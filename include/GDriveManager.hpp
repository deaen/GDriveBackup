#pragma once
using namespace geode::prelude;
#include "GDrivePopup.hpp"
#include "GDriveSigninPopup.hpp"

class GDriveManager : public cocos2d::CCObject
{
  public:
    GDriveManager(GDriveManager const &) = delete;
    void operator=(GDriveManager const &) = delete;
    static GDriveManager *getInstance();

    void signin();
    void verify();
    void signout(bool openAgain);

    void saveData();
    void getMetadata();
    void loadData();

    void setCurrentPopup(GDrivePopup *popup);
    void setCurrentSigninPopup(GDriveSigninPopup *signinPopup);

    GDrivePopup *getCurrentPopup();
    GDriveSigninPopup *getCurrentSigninPopup();
    std::string getRefreshToken();
    arc::Future<std::string> getAccessToken();
    arc::Future<std::string> getEmail();

  private:
    GDriveManager();

    void showError(const std::string& title = "GDriveBackup", const std::string& error = "", bool invasive = true);
    GDrivePopup *m_currentPopup = nullptr;
    GDriveSigninPopup *m_currentSigninPopup = nullptr;
    std::string m_uuid;
    std::time_t m_timestamp = 0;

    std::string accessToken;
    std::string email;
};