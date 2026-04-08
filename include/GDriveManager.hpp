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
    void loadMetadata(const int slot);
    void loadData(const int slot);

    arc::Future<std::string> getFolderID(const int slot, const bool autoCreate = true);
    arc::Future<bool> getMetadata(const int slot);
    arc::Future<bool> setDescription(std::string description, const int slot);
    arc::Future<bool> saveString(const std::string data, const int slot, web::WebRequest responseReq);
    arc::Future<bool> loadString(const int slot, web::WebRequest responseReq, GDriveLoadLayer *loadLayer);
    

    void setCurrentPopup(GDrivePopup *popup);
    void setCurrentSigninPopup(GDriveSigninPopup *signinPopup);

    GDrivePopup *getCurrentPopup();
    GDriveSigninPopup *getCurrentSigninPopup();

    arc::Future<std::string> getRefreshToken();
    arc::Future<std::string> getAccessToken();
    arc::Future<std::string> getEmail();

    enum QueueType
    {
        Save,
        Metadata
    };

    void addToQueue(QueueType queueType, GDriveSlotBox *box);
    void removeFromQueue(QueueType queueType, const int slot);

    enum Status
    {
        Idle,
        Waiting,
        Working
    };

    Status checkStatus(QueueType queueType, GDriveSlotBox *box);
    void removeBoxPointer(QueueType queueType, const int slot);

    size_t getSaveProgress();
    size_t getSaveTotal();

    size_t getLoadProgress();
    size_t getLoadTotal();

    std::string m_androidID;
  private:
    GDriveManager();

    GDrivePopup *m_currentPopup = nullptr;
    GDriveSigninPopup *m_currentSigninPopup = nullptr;
    // std::string m_uuid;
    // time_t m_timestamp = 0;

    void showError(const std::string_view title = "GDriveBackup", const std::string_view error = "", bool invasive = true);

    void updateQueue(QueueType queueType);

    async::TaskHolder<bool> m_saveListener;
    async::TaskHolder<bool> m_loadListener;
    async::TaskHolder<bool> m_metadataListener;

    std::map<int, GDriveSlotBox *> m_saveQueue;
    std::map<int, GDriveSlotBox *> m_metadataQueue;

    size_t m_saveProgress = 0;
    size_t m_saveTotal = 0;

    size_t m_loadProgress = 0;
    size_t m_loadTotal = 0;
    constexpr static size_t m_chunkSize = 256 * 1024 * 256;
};