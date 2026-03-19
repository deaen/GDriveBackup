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

    void saveData(GDriveSlotBox *box);

    void loadData(GDriveSlotBox *box);

    /*
    returns the id of the slot's folder in drive
    */
    arc::Future<std::string> getFolderID(int slot, bool autoCreate = true);
    arc::Future<> setMetadata(const int slot);
    arc::Future<bool> saveString(const std::string_view name, const std::string data, const int slot,
                                 web::WebRequest reesponseReq, GDriveSlotBox* box = nullptr);

    void setCurrentPopup(GDrivePopup *popup);
    void setCurrentSigninPopup(GDriveSigninPopup *signinPopup);

    GDrivePopup *getCurrentPopup();
    GDriveSigninPopup *getCurrentSigninPopup();
    std::string getRefreshToken();
    arc::Future<std::string> getAccessToken();
    arc::Future<std::string> getEmail();

    size_t m_SaveProgress = 0;

  private:
    GDriveManager();

    void showError(const std::string &title = "GDriveBackup", const std::string &error = "", bool invasive = true);
    GDrivePopup *m_currentPopup = nullptr;
    GDriveSigninPopup *m_currentSigninPopup = nullptr;
    std::string m_uuid;
    std::time_t m_timestamp = 0;
};