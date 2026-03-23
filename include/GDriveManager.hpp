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

    void saveData(const int slot);
    void loadData(const int slot);

    arc::Future<std::string> getFolderID(int slot, bool autoCreate = true);
    arc::Future<> setMetadata(const int slot);
    arc::Future<bool> saveString(const std::string name, const std::string data, const int slot,
                                 web::WebRequest reesponseReq);

    void setCurrentPopup(GDrivePopup *popup);
    void setCurrentSigninPopup(GDriveSigninPopup *signinPopup);

    GDrivePopup *getCurrentPopup();
    GDriveSigninPopup *getCurrentSigninPopup();
    std::string getRefreshToken();
    arc::Future<std::string> getAccessToken();
    arc::Future<std::string> getEmail();

    enum QueueType
    {
        Save,
        Metadata
    };

    void addToQueue(QueueType queue, GDriveSlotBox *box);
    void removeFromQueue(QueueType queue, const int slot);

    enum Status
    {
        Idle,
        Waiting,
        Working
    };

    Status checkStatus(QueueType queue, GDriveSlotBox *box);
    void removeBoxPointer(QueueType queue, const int slot);

    size_t getSaveProgress();
    size_t getsaveTotal();

  private:
    GDriveManager();

    GDrivePopup *m_currentPopup = nullptr;
    GDriveSigninPopup *m_currentSigninPopup = nullptr;
    std::string m_uuid;
    std::time_t m_timestamp = 0;

    void showError(const std::string &title = "GDriveBackup", const std::string &error = "", bool invasive = true);

    void updateQueue(QueueType queue);

    async::TaskHolder<bool> m_saveListener;
    async::TaskHolder<bool> m_metadataListener;

    std::map<int, GDriveSlotBox *> m_saveQueue;
    std::map<int, GDriveSlotBox *> m_metadataQueue;

    size_t m_SaveProgress = 0;
    size_t m_saveTotal = 0;
};