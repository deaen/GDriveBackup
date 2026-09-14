#pragma once
using namespace geode::prelude;

#include "GDrivePopup.hpp"
#include "GDriveSigninPopup.hpp"

using folderdata_map = std::map<std::string, std::string>; // var = value
using sizedata_map = std::map<std::string, folderdata_map>; // username = [var, value]
using slotdata_map = std::unordered_map<std::string, std::string>; // var = value
using metadata_map = std::unordered_map<int, slotdata_map>;        // slot = [var, value]

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

    arc::Future<std::optional<std::string>> findFolder(const std::string name, const bool findByAccountiD, const std::string accountiD = "", const std::string parentID = "");
    arc::Future<std::optional<std::string>> createFolder(const std::string name, const std::string accountiD = "", const std::string parentID = "");
    arc::Future<bool> renameFolder(const std::string fileID, const std::string name, const std::string accountiD);
    arc::Future<std::optional<std::string>> getMainFolderID(const bool autoCreate = true);
    arc::Future<std::optional<std::string>> getUserFolderID(const bool autoCreate = true);
    arc::Future<std::optional<std::string>> getFileID(const int slot, const bool autoCreateFolder, const std::string error = "", const std::string defparentID = "", bool visibleError = true);
    arc::Future<bool> getMetadata2(); // rename this to get metadata later and the other one to like loadMetadataOrMaybeYouWontEvenNeedItWhoKnows
    arc::Future<bool> getMetadata(const int slot);
    arc::Future<bool> setDescription(const std::string description, const int slot);
    arc::Future<bool> deleteFile(const int slot);
    arc::Future<bool> saveString(const std::string data, const int slot, web::WebRequest responseReq);
    arc::Future<bool> loadString(const int slot, web::WebRequest responseReq, GDriveLoadLayer *loadLayer);
    arc::Future<std::optional<sizedata_map>> getSizeInfo();

    struct fileRevision
    {
        std::string id;
        std::string time;
        size_t size;
        bool keepForever;
    };
    arc::Future<std::vector<fileRevision>> getRevisionList(const int slot);

    void setCurrentPopup(GDrivePopup *popup);
    void setCurrentSigninPopup(GDriveSigninPopup *signinPopup);

    GDrivePopup *getCurrentPopup();
    GDriveSigninPopup *getCurrentSigninPopup();

    arc::Future<std::string> getRefreshToken();
    arc::Future<std::string> getAccessToken();
    arc::Future<std::string> getEmail();

    void showError(const std::string_view title = "GDriveBackup", const std::string_view error = "", bool invasive = true);

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

    bool getMetadataStatus();
    metadata_map *getMetadataMap();
    void clearMetadata();
    std::string m_androidID;

  private:
    GDriveManager();

    GDrivePopup *m_currentPopup = nullptr;
    GDriveSigninPopup *m_currentSigninPopup = nullptr;
    // std::string m_uuid;
    // time_t m_timestamp = 0;

    void updateQueue(QueueType queueType);
    void setgettingMetadataStatus(bool status);

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

    metadata_map m_metadataMap;
    bool m_gettingMetadata = false;
};