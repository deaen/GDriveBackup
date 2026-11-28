#pragma once
using namespace geode::prelude;

#include "GDrivePopup.hpp"
#include "GDriveSigninPopup.hpp"

using folderdata_map = std::map<std::string, std::string>;         // var = value
using sizedata_map = std::map<std::string, folderdata_map>;        // username = [var, value]
using slotdata_map = std::unordered_map<std::string, std::string>; // var = value
using metadata_map = std::unordered_map<int, slotdata_map>;        // slot = [var, value]

class GDriveManager : public cocos2d::CCObject
{
  public:
    enum class Status
    {
        Idle,
        Waiting,
        Working
    };

    struct fileRevision
    {
        std::string id;
        std::string time;
        size_t size;
        bool keepForever;
    };

    GDriveManager(const GDriveManager &) = delete;
    GDriveManager &operator=(const GDriveManager &) = delete;
    static GDriveManager *getInstance();

    void signin();
    void verify();
    void signout(bool openAgain);
    arc::Future<std::string> getEmail();
    async::TaskHolder<std::string> *getEmailListner();

    void setCurrentPopup(GDrivePopup *popup);
    GDrivePopup *getCurrentPopup();
    void setCurrentSigninPopup(GDriveSigninPopup *signinPopup);
    GDriveSigninPopup *getCurrentSigninPopup();

    void addToQueue(GDriveSlotBox *box, int slot = -999);
    void removeFromQueue(int slot);
    void removeBoxPointer(int slot);
    void checkForAutoBackup();
    Status checkStatus(GDriveSlotBox *box);

    void loadData(int slot);
    arc::Future<bool> downloadSave(int slot, web::WebRequest responseReq, GDriveLoadLayer *loadLayer, GDriveConsts::slotType saveFileType = GDriveConsts::slotType::Old, ByteVector saveFile = {});
    size_t getSaveProgress();
    size_t getSaveTotal();

    arc::Future<bool> getMetadata();
    async::TaskHolder<bool> *getMetadataListner();
    bool getMetadataStatus();
    metadata_map *getMetadataMap();
    arc::Future<std::optional<sizedata_map>> getSizeInfo();

    arc::Future<std::optional<std::pair<std::string, GDriveConsts::slotInformation>>> getFileID(int slot, bool autoCreateFolder, std::string error = "", std::string defparentID = "", bool visibleError = true);
    arc::Future<bool> setDescription(std::string description, int slot);
    arc::Future<bool> deleteFile(int slot);

    void showError(std::string_view title = "GDriveBackup", std::string_view error = "", bool invasive = true, bool includeDefaultBody = true);

  private:
    GDriveManager();

    void updateQueue();

    void saveData(int slot);
    arc::Future<bool> uploadSave(int slot, web::WebRequest responseReq);
    bool loadGameManagerData(std::string_view GMString);
    bool loadLocalLevelManagerData(std::string_view LLMString);
    size_t getLoadProgress();
    size_t getLoadTotal();

    void setGettingMetadataStatus(bool status);
    void clearMetadata();

    arc::Future<std::optional<std::string>> findFolder(std::string name, bool findByaccountID, std::string accountID = "", std::string parentID = "");
    arc::Future<std::optional<std::string>> createFolder(std::string name, std::string accountID = "", std::string parentID = "");
    arc::Future<bool> renameFolder(std::string fileID, std::string name, std::string accountID);
    arc::Future<std::optional<std::string>> getMainFolderID(bool autoCreate = true);
    arc::Future<std::optional<std::string>> getUserFolderID(bool autoCreate = true);

    arc::Future<std::vector<fileRevision>> getRevisionList(int slot);

    arc::Future<std::string> getRefreshToken();
    arc::Future<std::string> getAccessToken();

    std::string m_androidID;

    GDrivePopup *m_currentPopup = nullptr;
    GDriveSigninPopup *m_currentSigninPopup = nullptr;

    async::TaskHolder<bool> m_saveListener;
    async::TaskHolder<bool> m_loadListener;
    async::TaskHolder<bool> m_metadataListener;
    async::TaskHolder<std::string> m_emailListener;

    std::map<int, GDriveSlotBox *> m_saveQueue;

    size_t m_saveProgress = 0;
    size_t m_saveTotal = 0;

    size_t m_loadProgress = 0;
    size_t m_loadTotal = 0;
    static constexpr size_t m_chunkSize = 256 * 1024 * 256;

    metadata_map m_metadataMap;
    bool m_gettingMetadata = false;
};