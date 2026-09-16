#include "GDriveManager.hpp"
#include "GDriveEncrypt.hpp"
#include <Geode/binding/LocalLevelManager.hpp>
#include <Geode/utils/base64.hpp>
#include <ctime>

#ifdef GEODE_IS_ANDROID
#include <Geode/cocos/platform/android/jni/JniHelper.h>
#endif

GDriveManager *GDriveManager::getInstance()
{
    static GDriveManager *instance = new GDriveManager();
    return instance;
}

GDriveManager::GDriveManager()
{
    m_saveListener.setName("gdrive-save-listener");
    m_loadListener.setName("gdrive-load-listener");

/* Get the android hardware ID and store in here to avoid all the threading nonsense */
#ifdef GEODE_IS_ANDROID
    JniMethodInfo t;
    JniHelper::getJavaVM()->AttachCurrentThread(&t.env, nullptr);
    if (JniHelper::getStaticMethodInfo(t, "com/customRobTop/BaseRobTopActivity", "getUserID", "()Ljava/lang/String;"))
    {
        jstring str = reinterpret_cast<jstring>(t.env->CallStaticObjectMethod(t.classID, t.methodID));
        m_androidID = JniHelper::jstring2string(str);

        t.env->DeleteLocalRef(t.classID);
        t.env->DeleteLocalRef(str);
    }
    else
    {
        auto vm = cocos2d::JniHelper::getJavaVM();

        JNIEnv *env;
        if (vm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6) == JNI_OK)
        {
            env->ExceptionClear();
        }
    }
#endif
}

void GDriveManager::showError(const std::string_view title, const std::string_view error, const bool invasive)
{
    if (invasive)
    {
        std::string body = "<cg>GDriveBackup</c> has encountered an unexpected <cr>error</c>, please check your  <cl>internet connection</c>.\nIf this keeps happening, please report the issue to the developer.";
        if (error != "")
            body += fmt::format("\n <cr>{}</c>", error);

        FLAlertLayer::create(fmt::format("{} Error", title).c_str(), body, "OK")->show();
    }
    else
        Notification::create(fmt::format("GDrive Backup: {} {}", title, error), NotificationIcon::Error, 5.f)->show();
}

void GDriveManager::signin()
{
    /* Generate new uuid and current timestamp */
    Mod::get()->setSavedValue<std::string>("temp-uuid", utils::random::generateUUID());
    Mod::get()->setSavedValue<time_t>("temp-timestamp", std::time(nullptr));
    if (Mod::get()->saveData().isErr())
        log::warn("Could not write save to file");

    /* Create request */
    auto req = web::WebRequest();

    req.param("uuid", Mod::get()->getSavedValue<std::string>("temp-uuid"));
    req.param("timestamp", Mod::get()->getSavedValue<time_t>("temp-timestamp"));

    /* Get url from api and then open in browser */
    async::spawn(req.get("https://api.deaen.top/gdb/getgoogleauth/"), [this](web::WebResponse res) {
        if (res.ok())
        {
            auto URL = res.json().unwrapOrDefault().get<std::string>("authorizationUrl").unwrapOrDefault();
            if (!URL.empty())
            {
                web::openLinkInBrowser(fmt::format("https://gdb.deaen.top/auth/google.html?uuid={}&timestamp={}&url={}", Mod::get()->getSavedValue<std::string>("temp-uuid"), Mod::get()->getSavedValue<time_t>("temp-timestamp"), utils::base64::encode(URL, base64::Base64Variant::UrlWithPad).c_str()));
                if (m_currentSigninPopup)
                    m_currentSigninPopup->showVerify();
                return;
            }
        }
        log::warn("Sign in Error Code: {}", res.string());
        showError("Sign in", fmt::format("error: {} {}", res.code(), res.json().unwrapOrDefault()["error"].get<std::string>("status").unwrapOrDefault()));
        if (m_currentSigninPopup)
            m_currentSigninPopup->showSignin();
    });
}

void GDriveManager::verify()
{
    /* Create request */
    auto req = web::WebRequest();

    req.param("uuid", Mod::get()->getSavedValue<std::string>("temp-uuid"));
    req.param("timestamp", Mod::get()->getSavedValue<time_t>("temp-timestamp"));

    /* Get token and save it in file */
    async::spawn(req.get("https://api.deaen.top/gdb/getrefreshtoken/"), [this](web::WebResponse res) {
        if (res.ok())
        {
            auto token = res.json().unwrapOrDefault().get<std::string>("refresh_token").unwrapOrDefault();
            if (!token.empty())
            {
                Mod::get()->setSavedValue<EncStr>("refresh_token", GDriveEncrypt::create()->encryptString(token));
                async::spawn(getAccessToken(), [this](std::string token) {
                    if (!token.empty())
                    {
                        if (m_currentSigninPopup)
                            m_currentSigninPopup->finishUp();
                    }
                    else
                    {
                        showError("Verify", "Could not get auth token... please check your internet connection and try again later");
                        if (m_currentSigninPopup)
                            m_currentSigninPopup->showVerify();
                    }
                });
                return;
            }
        }
        log::warn("Verify Error Code: {}", res.code());

        showError("Verify", fmt::format("error: {} {}", res.code(), res.json().unwrapOrDefault()["error"].get<std::string>("status").unwrapOrDefault()));
        if (m_currentSigninPopup)
            m_currentSigninPopup->showVerify();
    });
}

void GDriveManager::signout(const bool openAgain)
{
    m_saveListener.cancel();
    m_loadListener.cancel();
    m_saveQueue.clear();

    if (m_currentPopup)
        m_currentPopup->removeFromParent();
    if (m_currentSigninPopup)
        m_currentSigninPopup->removeFromParent();

    Mod::get()->getSaveContainer().clear();

    if (openAgain)
        GDrivePopup::create();
}

arc::Future<std::optional<std::string>> GDriveManager::findFolder(const std::string name, const bool findByAccountID, const std::string accountID, const std::string parentID)
{
    auto req = web::WebRequest();
    req.header("Authorization", fmt::format("Bearer {}", co_await getAccessToken()));

    std::string query = "trashed = false and mimeType = 'application/vnd.google-apps.folder'";

    if (findByAccountID)
        query += " and appProperties has { key='accountID' " + fmt::format("and value='{}'", accountID) + " }";
    else
        query += fmt::format(" and name = '{}'", name);

    if (!parentID.empty())
        query += fmt::format(" and '{}' in parents", parentID);

    req.param("q", query);
    auto res = co_await req.get("https://www.googleapis.com/drive/v3/files");

    if (res.ok())
    {
        auto id = res.json().unwrapOrDefault()["files"][0].get<std::string>("id").unwrapOrDefault();
        if (!id.empty())
        {
            co_return id;
        }
    }
    else
        log::warn("Folder id find error: {}", res.string());

    co_return std::nullopt;
}

arc::Future<std::optional<std::string>> GDriveManager::createFolder(const std::string name, const std::string accountID, const std::string parentID)
{
    auto req = web::WebRequest();
    req.header("Authorization", fmt::format("Bearer {}", co_await getAccessToken()));

    auto body = matjson::makeObject({
        {"name", name},
        {"mimeType", "application/vnd.google-apps.folder"},
    });
    if (!accountID.empty())
    {
        body.set("appProperties", matjson::makeObject({{"accountID", accountID}}));
    }
    if (!parentID.empty())
    {
        body.set("parents", std::vector<matjson::Value>({parentID}));
    }

    req.bodyJSON(body);
    auto res = co_await req.post("https://www.googleapis.com/drive/v3/files");
    if (res.ok())
    {
        auto id = res.json().unwrapOrDefault().get<std::string>("id").unwrapOrDefault();
        if (!id.empty())
            co_return id;
    }
    else
        log::warn("Folder creation error: {}", res.string());

    co_return std::nullopt;
}

arc::Future<bool> GDriveManager::renameFolder(const std::string fileID, const std::string name, const std::string accountID)
{
    auto req = web::WebRequest();
    req.header("Authorization", fmt::format("Bearer {}", co_await getAccessToken()));

    auto body = matjson::Value();
    body.set("name", name);
    body.set("appProperties", matjson::makeObject({{"accountID", accountID}}));
    req.bodyJSON(body);

    auto res = co_await req.patch(fmt::format("https://www.googleapis.com/drive/v3/files/{}", fileID));
    if (res.ok())
        co_return true;
    else
        log::warn("Folder id rename error: {}", res.string());

    co_return false;
}

arc::Future<std::optional<std::string>> GDriveManager::getMainFolderID(const bool autoCreate)
{
    std::string gdriveFolderName = "GDrive Backup";
    std::string gdriveAccountID = "gdriveMainFolder";

    // find big gdrive
    auto gdriveFolderID = co_await findFolder(gdriveFolderName, true, gdriveAccountID);

    if (!gdriveFolderID)
    {
        // look for old big gdrive
        auto oldGDriveFolderID = co_await findFolder(gdriveFolderName, false);
        if (oldGDriveFolderID)
        {
            if (co_await renameFolder(*oldGDriveFolderID, gdriveFolderName, gdriveAccountID))
                gdriveFolderID = *oldGDriveFolderID;
        }
        // else just create
        else if (autoCreate)
            gdriveFolderID = co_await createFolder(gdriveFolderName);
    }

    co_return gdriveFolderID;
}

arc::Future<std::optional<std::string>> GDriveManager::getUserFolderID(const bool autoCreate)
{
    std::string gdriveFolderName = "GDrive Backup";
    std::string userFolderName = fmt::format("{}'s saves", GJAccountManager::sharedState()->m_username);
    std::string accountID = std::to_string(GJAccountManager::sharedState()->m_accountID);
    std::string gdriveAccountID = "gdriveMainFolder";
    if (userFolderName == "'s saves")
        userFolderName = "Unregistered saves";

    // find big gdrive
    auto gdriveFolderID = co_await getMainFolderID(autoCreate);

    if (gdriveFolderID)
    {
        // look for new name styled save
        auto userFolderID = co_await findFolder(userFolderName, true, accountID, *gdriveFolderID);

        if (userFolderID)
            co_return userFolderID;

        // before creating lets first look for old style name
        auto oldNameduserFolderID = co_await findFolder(accountID, false, "", *gdriveFolderID);

        // if found lets rename it so we dont do this again else jus auto create
        if (oldNameduserFolderID)
        {
            if (co_await renameFolder(*oldNameduserFolderID, userFolderName, accountID))
                co_return oldNameduserFolderID;
        }
        else if (autoCreate)
        {
            userFolderID = co_await createFolder(userFolderName, accountID, *gdriveFolderID);
            if (userFolderID)
                co_return userFolderID;
        }
    }

    co_return std::nullopt;
}

arc::Future<std::optional<std::string>> GDriveManager::getFileID(const int slot, bool autoCreateFolder, std::string error, std::string defparentID, bool visibleError)
{
    std::string token = co_await getAccessToken();
    std::optional<std::string> parentID = (defparentID.empty()) ? co_await getUserFolderID(autoCreateFolder) : defparentID;

    if (!parentID)
    {
        if (visibleError)
            co_await waitForMainThread([this] { showError("Couldn't find user folder", "", false); });
    }
    else
    {
        auto req = web::WebRequest();
        web::WebResponse res;
        req.header("Authorization", fmt::format("Bearer {}", token));

        // look for new-style named save
        req.param("q", fmt::format("name='save-{}' and trashed=false and '{}' in parents", slot, *parentID));

        res = co_await req.get("https://www.googleapis.com/drive/v3/files");
        if (res.ok())
        {
            auto id = res.json().unwrapOrDefault()["files"][0].get<std::string>("id").unwrapOrDefault();
            if (!id.empty())
            {
                co_return id;
            }
            else
            {
                // look for old-styled name
                req.removeParam("q");
                req.param("q", fmt::format("name='save-{}.dat' and trashed=false and '{}' in parents", slot, *parentID));
                res = co_await req.get("https://www.googleapis.com/drive/v3/files");
                if (res.ok())
                {
                    auto id = res.json().unwrapOrDefault()["files"][0].get<std::string>("id").unwrapOrDefault();
                    if (!id.empty())
                    {
                        // rename to new-styled name if found
                        req.removeParam("q");
                        auto body = matjson::Value();
                        body.set("name", fmt::format("save-{}", slot));
                        req.bodyJSON(body);

                        res = co_await req.patch(fmt::format("https://www.googleapis.com/drive/v3/files/{}", id));
                        if (!res.ok())
                            log::warn("File id rename error: {}", res.string());

                        co_return id;
                    }
                }
                else
                {
                    // continue
                    if (visibleError)
                        co_await waitForMainThread([this, slot, errStr = error] {
                            showError(errStr.c_str(), "Valid fileID not found.", false);
                        });
                }
            }
        }
        else
        {
            log::warn("{}", res.string());

            if (res.code() == 401 && res.json().unwrapOrDefault()["error"].get<std::string>("status").unwrapOrDefault() == "UNAUTHENTICATED")
            {
                signout(true);
                if (visibleError)
                    co_await waitForMainThread([this] {
                        showError("Failed to authenticate,", "please sign in again", false);
                    });
            }
            else if (visibleError)
            {
                auto code = res.code();
                auto status = res.json().unwrapOrDefault()["error"].get<std::string>("status").unwrapOrDefault();
                co_await waitForMainThread([this, slot, errStr = error, code, status] {
                    showError(errStr.c_str(), fmt::format("error: {} {}", code, status), false);
                });
            }
        }
    }
    co_return std::nullopt;
}

void GDriveManager::saveData(const int slot)
{
    auto req = web::WebRequest().onProgress([this, slot](web::WebProgress const &p) {
        if (m_saveQueue[slot])
            m_saveQueue[slot]->setStatusPercentage(m_saveProgress + p.uploaded());
    });

    m_saveListener.spawn(saveString(static_cast<std::string>(GameManager::sharedState()->getCompressedSaveString()) + "|" + static_cast<std::string>(LocalLevelManager::sharedState()->getCompressedSaveString()), slot, req), [this, slot](bool ok) {
        if (m_saveQueue[slot])
        {
            m_saveQueue[slot]->setStatusVisiblity(false);
            m_saveQueue[slot]->setWorkingStatus(false);
            m_saveQueue[slot]->updateInfo();

            if (m_saveQueue[slot]->getShouldEditMode())
            {
                m_saveQueue[slot]->setShouldEditMode(false);
                m_saveQueue[slot]->setEditMode(true);
            }
        }

        if (ok)
            Notification::create(fmt::format("GDrive Backup: Slot {} Save Complete!", slot), NotificationIcon::Success, 5.f)->show();

        removeFromQueue(slot);
    });
}

void GDriveManager::loadData(const int slot)
{
    auto loadLayer = GDriveManager::getInstance()->getCurrentPopup()->showLoadLayer();

    auto req = web::WebRequest().onProgress([this, loadLayer](web::WebProgress const &p) {
        if (loadLayer)
            loadLayer->setPercentage(m_loadProgress + p.downloaded());
    });

    m_loadListener.spawn(loadString(slot, req, loadLayer), [slot](bool ok) {
        if (auto popup = GDriveManager::getInstance()->getCurrentPopup())
            popup->hideLoadLayer();

        if (ok)
            Notification::create(fmt::format("GDrive Backup: Slot {} Load Complete!", slot), NotificationIcon::Success, 5.f)->show();
    });
}

arc::Future<bool> GDriveManager::saveString(const std::string data, const int slot, web::WebRequest responseReq)
{
    co_await waitForMainThread([slot, this] {
        if (m_saveQueue[slot])
            m_saveQueue[slot]->setStatusMessage("Preparing...");
    });

    m_saveProgress = 0;
    m_saveTotal = data.size();
    std::string resumableURL;
    std::string token = co_await getAccessToken();
    std::optional<std::string> parentID = co_await getUserFolderID(true);

    if (!parentID)
    {
        co_await waitForMainThread([this] { showError("Couldn't find user folder", "", false); });
        co_return false;
    }

    auto fileID = co_await getFileID(slot, true, fmt::format("Slot {} Save Failed", slot), *parentID, false);
    if (!fileID)
        fileID = "";

    /* Initial request */
    {
        auto req = web::WebRequest();
        req.param("uploadType", "resumable");
        req.header("Authorization", fmt::format("Bearer {}", token));

        req.header("X-Upload-Content-Length", std::to_string(data.size()));
        req.header("Content-Type", "application/json; charset=UTF-8");

        web::WebResponse res;
        if (fileID->empty())
        {
            req.bodyJSON(matjson::makeObject({
                {"name", fmt::format("save-{}", slot)},
                {"parents", std::vector<matjson::Value>({*parentID})},
            }));
            res = co_await req.post("https://www.googleapis.com/upload/drive/v3/files");
        }
        else
            res = co_await req.patch(fmt::format("https://www.googleapis.com/upload/drive/v3/files/{}", *fileID));

        if (res.ok())
        {
            if (auto header = res.getAllHeadersNamed("location"))
            {
                if (!header->empty() && utils::string::contains(header->at(0), "https://"))
                    resumableURL = header->at(0);
            }
        }
        else
            log::warn("{}", res.string());

        if (resumableURL.empty())
        {
            co_await waitForMainThread([&res, slot, this] {
                showError(fmt::format("Slot {} Save Failed", slot), fmt::format("error: {} {}", res.code(), res.json().unwrapOrDefault()["error"].get<std::string>("status").unwrapOrDefault()), false);
            });
            co_return false;
        }
    }

    co_await waitForMainThread([&data, this, slot] {
        if (m_saveQueue[slot])
        {
            m_saveQueue[slot]->setStatusMessage("Saving...");
            m_saveQueue[slot]->showPercentage(m_saveTotal);
        }
    });

    /* Upload Data */
    {
        responseReq.header("Authorization", fmt::format("Bearer {}", token));
        web::WebResponse res;
        int retries = 0;
        size_t i = 0;
        while (i < data.size())
        {
            size_t currentSize = std::min(m_chunkSize, data.size() - i);
            m_saveProgress = i;

            responseReq.body(ByteVector(data.data() + i, data.data() + i + currentSize));
            responseReq.removeHeader("content-length");
            responseReq.removeHeader("content-range");
            responseReq.header("content-length", std::to_string(currentSize));
            responseReq.header("content-range", fmt::format("bytes {}-{}/{}", i, i + currentSize - 1, data.size()));
            res = co_await responseReq.put(resumableURL);

            if (res.ok())
                i += currentSize;
            else if (res.badClient())
            {
                // try 15 times before giving up
                if (retries < 15)
                {
                    retries += 1;
                    continue;
                }
                else
                {
                    // assume the worst.
                    co_await waitForMainThread([&res, slot, this] {
                        showError(fmt::format("Slot {} Save Failed", slot), fmt::format("error: {} {}", res.code(), res.json().unwrapOrDefault()["error"].get<std::string>("status").unwrapOrDefault()), false);
                    });
                    co_return false;
                }
            }
            else
            {
                if (auto range = res.getAllHeadersNamed("range"))
                {
                    size_t r = utils::numFromString<size_t>(utils::string::split(range->at(0), "-").back()).unwrapOrDefault();
                    if (r == 0)
                    {
                        co_await waitForMainThread([&res, slot, this] {
                            showError(fmt::format("Slot {} Save Failed", slot), fmt::format("error: {} {}", res.code(), res.json().unwrapOrDefault()["error"].get<std::string>("status").unwrapOrDefault()), false);
                        });
                        co_return false;
                    }
                    i = r + 1;
                }
                else
                {
                    co_await waitForMainThread([&res, slot, this] {
                        showError(fmt::format("Slot {} Save Failed", slot), fmt::format("error: {} {}", res.code(), res.json().unwrapOrDefault()["error"].get<std::string>("status").unwrapOrDefault()), false);
                    });
                    co_return false;
                }
            }
        }
    }

    m_metadataMap[slot]["timestamp"] = std::to_string(std::time(nullptr));
    m_metadataMap[slot]["size"] = std::to_string(data.size());

    co_return true;
}

arc::Future<bool> GDriveManager::loadString(const int slot, web::WebRequest responseReq, GDriveLoadLayer *loadLayer)
{
    co_await waitForMainThread([&loadLayer] {
        if (loadLayer)
            loadLayer->setMessage("Preparing...");
    });

    m_loadProgress = 0;
    m_loadTotal = 0;
    std::string token = co_await getAccessToken();
    std::optional<std::string> parentID = co_await getUserFolderID(true);

    if (!parentID)
    {
        co_await waitForMainThread([this] { showError("Couldn't find user folder", "", false); });
        co_return false;
    }

    auto fileID = co_await getFileID(slot, true, fmt::format("Slot {} Save Failed", slot, *parentID));
    if (!fileID)
        co_return false;

    auto req = web::WebRequest();
    req.header("Authorization", fmt::format("Bearer {}", token));

    /* Get file size */
    req.param("fields", "size");
    auto res = co_await req.get(fmt::format("https://www.googleapis.com/drive/v3/files/{}", *fileID));
    if (res.ok())
    {
        auto size = numFromString<size_t>(res.json().unwrapOrDefault().get<std::string>("size").unwrapOrDefault()).unwrapOrDefault();

        if (size == 0)
        {
            co_await waitForMainThread([&res, slot, this] {
                showError(fmt::format("Slot {} load Failed", slot), fmt::format("error: {} {}", res.code(), res.json().unwrapOrDefault()["error"].get<std::string>("status").unwrapOrDefault()), false);
            });
            co_return false;
        }

        m_loadTotal = size;
        m_metadataMap[slot]["size"] = std::to_string(size);
    }
    else
        log::warn("{}", res.string());

    co_await waitForMainThread([&loadLayer, this, slot] {
        if (loadLayer)
        {
            loadLayer->setMessage(fmt::format("Downloading Slot {} Data...", slot));
            loadLayer->showPercentage(m_loadTotal);
        }
    });

    /* Download Data */
    ByteVector buf;
    responseReq.param("alt", "media");
    responseReq.header("Authorization", fmt::format("Bearer {}", token));
    int retries = 0;

    size_t i = 0;
    while (i < m_loadTotal)
    {
        size_t currentSize = std::min(m_chunkSize, m_loadTotal - i);
        m_loadProgress = i;

        responseReq.removeHeader("range");
        responseReq.header("range", fmt::format("bytes={}-{}", i, i + currentSize - 1));
        res = co_await responseReq.get(fmt::format("https://www.googleapis.com/drive/v3/files/{}", *fileID));

        if (res.ok())
        {
            buf.insert(buf.end(), res.data().begin(), res.data().end());
            i += currentSize;
        }
        else if (res.error())
        {
            if (auto range = res.getAllHeadersNamed("content-range"))
            {
                size_t r = utils::numFromString<size_t>(utils::string::split(utils::string::split(range->at(0), "-").back(), "/").front()).unwrapOrDefault();
                if (r == 0)
                {
                    co_await waitForMainThread([&res, slot, this] {
                        showError(fmt::format("Slot {} load Failed", slot), "Can't extract header", false);
                    });
                    co_return false;
                }

                buf.insert(buf.end(), res.data().begin(), res.data().end());
                i = r + 1;
            }
            else
            {
                co_await waitForMainThread([&res, slot, this] {
                    showError(fmt::format("Slot {} load Failed", slot), "Can't get header", false);
                });
                co_return false;
            }
        }
        else if (res.badClient())
        {
            // try a couple times before giving up ... maybe i should make this a seconds time out instead....
            if (retries < 15)
            {
                retries += 1;
                continue;
            }
            else
            {
                // assume the worst.
                co_await waitForMainThread([&res, slot, this] {
                    showError(fmt::format("Slot {} timeout Reached", slot), fmt::format("error: {} {}", res.code(), res.json().unwrapOrDefault()["error"].get<std::string>("status").unwrapOrDefault()), false);
                });
                co_return false;
            }
        }
        else
        {
            log::warn("{}", res.string());
            co_await waitForMainThread([&res, slot, this] {
                showError(fmt::format("Slot {} Load failed", slot), fmt::format("error: {} {}", res.code(), res.json().unwrapOrDefault()["error"].get<std::string>("status").unwrapOrDefault()), false);
            });
            co_return false;
        }
    }

    std::string_view sv(reinterpret_cast<const char *>(buf.data()), buf.size());
    auto strings = utils::string::splitView(sv, "|");

    if (m_loadTotal != buf.size() || strings.size() != 2)
    {
        co_await waitForMainThread(
            [&res, slot, this] { showError(fmt::format("Slot {} load failed", slot), "File isn't valid", false); });
        co_return false;
    }

    // here we go
    co_await waitForMainThread([this, &strings, &loadLayer, slot] {
        // make the level stats string...
        DS_Dictionary gs;
        gs.loadRootSubDictFromString(ZipUtils::decompressString(std::string(strings[0]), false, 0));
        std::string statsString;

        auto levelDic = gs.getDictForKey("GLM_03", false);
        size_t levelCount = 0;
        for (auto [id, level] : CCDictionaryExt<std::string_view, GJGameLevel *>(levelDic))
        {
            if (level == nullptr)
                continue;

            if (levelCount != 0)
                statsString += ",";

            statsString += id;
            statsString += ("," + std::to_string(level->m_stars.value()));

            ++levelCount;
        }

        if (levelDic->count() != levelCount)
        {
            showError(fmt::format("Slot {} load failed", slot), "Can't get levels", false);
            return;
        }

        // now lets make achievements silent
        auto am = AchievementManager::sharedState();
        bool oldDontNotify = am->m_dontNotify;
        am->m_dontNotify = true;

        // do whatever this is
        auto gsm = GameStatsManager::sharedState();
        bool oldSkipIncrementChallenge = gsm->m_skipIncrementChallenge;
        gsm->m_skipIncrementChallenge = true;
        gsm->preSaveGameStats();
        gsm->m_usePlayerStatsCCDictionary = true;

        // load both strings
        auto gm = GameManager::sharedState();
        gd::string gms(strings[0].data());
        gm->loadFromCompressedString(gms);

        auto llm = LocalLevelManager::sharedState();
        gd::string llms(strings[1].data());
        llm->loadFromCompressedString(llms);

        // i really really REALLY don't want to hardcode this but i can't think of something better if u know of a better solution PLEASE let me know
        std::string_view packsString = "1:1:3:11940,150245,215705:4:3:5:1|1:2:3:151245,61757,150906:4:4:5:1|1:3:3:59767,61982,65106:4:4:5:1|1:5:3:217631,3785,281148:4:5:5:1|1:6:3:167527,23420,88737:4:6:5:1|1:7:3:71485,77879,79275:4:6:5:1|1:8:3:8612,131259,85065:4:7:5:1|1:9:3:87960,116806,278956:4:8:5:1|1:10:3:269500,49229,169590:4:8:5:1|1:11:3:329929,188909,340602:4:9:5:1|1:19:3:341613,358750,369294:4:10:5:2|1:20:3:70059,10109,135561:4:10:5:2|1:21:3:57730,308891,102765:4:10:5:2|1:22:3:186646,13519,55520:4:10:5:2|1:26:3:199761,214523,130414:4:10:5:2|1:27:3:497514,380082,553327:4:10:5:2|1:28:3:541953,379772,449502:4:10:5:2|1:29:3:511533,350329,428765:4:10:5:2|1:30:3:393159,456678,396874:4:10:5:2|1:31:3:450920,316982,436624:4:10:5:2|1:32:3:490078,506009,513124:4:4:5:1|1:33:3:364445,411459,509393:4:5:5:1|1:34:3:422703,460862,124052:4:6:5:1|1:35:3:461472,516810,447766:4:8:5:1|1:36:3:456675,471354,457265:4:8:5:1|1:37:3:674454,750434,835854:4:5:5:1|1:38:3:809579,741941,577710:4:5:5:1|1:39:3:819956,540428,878743:4:6:5:1|1:40:3:714673,729521,661286:4:7:5:1|1:41:3:821459,692596,745177:4:8:5:1|1:42:3:857195,687938,804313:4:8:5:1|1:43:3:827829,664044,708901:4:8:5:1|1:44:3:882417,884256,551979:4:9:5:1|1:45:3:856066,862216,877915:4:6:5:1|1:46:3:874540,664867,700880:4:10:5:2|1:47:3:682941,897987,513137:4:10:5:2|1:48:3:776919,741635,735154:4:10:5:2|1:49:3:764038,897837,848722:4:10:5:2|1:50:3:840397,413504,839175:4:10:5:2|1:52:3:1512012,1602784,1649640:4:4:5:1|1:53:3:1244147,1389451,1642022:4:3:5:1|1:54:3:1314024,1629780,1721197:4:4:5:1|1:55:3:1446958,1063115,1734354:4:4:5:1|1:56:3:980341,1541962,1160937:4:5:5:1|1:57:3:1001204,1694003,1544084:4:6:5:1|1:58:3:1498893,1123276,1322487:4:7:5:1|1:59:3:1566116,946020,1100161:4:8:5:1|1:60:3:1350389,1215630,1724579:4:8:5:1|1:61:3:1267316,1670283,1205277:4:8:5:1|1:62:3:1447246,1132530,1683722:4:9:5:1|1:63:3:1728550,1799065,1311773:4:9:5:1|1:64:3:1018758,1326086,1698428:4:10:5:2|1:65:3:1668421,1703546,923264:4:10:5:2|1:66:3:1650666,1474319,1777565:4:10:5:2|1:67:3:4454123,11280109,6508283:4:2:5:1|1:68:3:10992098,9110646,9063899:4:3:5:1|1:69:3:8320596,2820124,8477262:4:3:5:1|1:70:3:5131543,8157377,8571598:4:4:5:1|1:71:3:12178580,11357573,11591917:4:4:5:1|1:72:3:4449079,6979485,10110092:4:4:5:1|1:73:3:13766381,13242284,13963465:4:4:5:1|1:74:3:8939774,9204593,6324840:4:5:5:1|1:75:3:7485599,5017264,6053464:4:5:5:1|1:76:3:13912771,12577409,11924846:4:5:5:1|1:77:3:3382569,3224853,3012870:4:6:5:1";
        auto mapPacks = GameLevelManager::sharedState()->createAndGetMapPacks(packsString.data());

        for (auto pack : CCArrayExt<GJMapPack *>(mapPacks))
        {
            if (pack->hasCompletedMapPack())
                gsm->completedMapPack(pack);

            gsm->setStarsForMapPack(pack->m_packID, pack->m_stars);
        }

        // now we re count stats with the awesome string i made
        gm->recountUserStats(statsString);

        // do these things
        gsm->verifyUserCoins();
        gsm->tryFixPathBug();
        gsm->verifyPathAchievements();

        // and put everything back!
        gsm->m_usePlayerStatsCCDictionary = false;
        gsm->postLoadGameStats();

        am->m_dontNotify = oldDontNotify;
        gsm->m_skipIncrementChallenge = oldSkipIncrementChallenge;

        // set every icon cuz they break sometimes lol
        gm->m_playerFrame = gs.getIntegerForKey("playerFrame");
        gm->m_playerShip = gs.getIntegerForKey("playerShip");
        gm->m_playerBall = gs.getIntegerForKey("playerBall");
        gm->m_playerBird = gs.getIntegerForKey("playerBird");
        gm->m_playerDart = gs.getIntegerForKey("playerDart");
        gm->m_playerRobot = gs.getIntegerForKey("playerRobot");
        gm->m_playerSpider = gs.getIntegerForKey("playerSpider");
        gm->m_playerSwing = gs.getIntegerForKey("playerSwing");
        gm->m_playerColor = gs.getIntegerForKey("playerColor");
        gm->m_playerColor2 = gs.getIntegerForKey("playerColor2");
        gm->m_playerGlowColor = (gs.getIntegerForKey("playerColor3") == -1) ? gs.getIntegerForKey("playerColor2") : gs.getIntegerForKey("playerColor3");
        gm->m_playerStreak = gs.getIntegerForKey("playerStreak");
        gm->m_playerShipFire = gs.getIntegerForKey("playerShipStreak");
        gm->m_playerDeathEffect = gs.getIntegerForKey("playerDeathEffect");
        gm->m_playerJetpack = gs.getIntegerForKey("playerJetpack");
        gm->m_playerGlow = gs.getBoolForKey("playerGlow");

        // setting game variables
        if (Mod::get()->getSettingValue<bool>("load-game-options"))
        {
            for (auto [key, value] : CCDictionaryExt<std::string_view, CCString *>(gs.getDictForKey("valueKeeper", false)))
            {
                if (!key.contains("gv_"))
                    continue;

                auto vint = value->intValue();
                auto kname = utils::string::remove(key, "gv_");
                auto kint = numFromString<int>(kname).unwrapOrDefault();
                switch (kint)
                {
                case 0:
                case 23:
                case 25:
                case 28:
                case 30:
                case 32:
                case 115:
                case 116:
                case 122:
                case 168:
                    continue;
                }

                if (vint == 0 || vint == 1)
                    gm->setGameVariable(kname.c_str(), static_cast<bool>(vint));
                else
                    gm->setIntGameVariable(kname.c_str(), vint);
            }
        }
    });

    co_return true;
}

arc::Future<std::optional<sizedata_map>> GDriveManager::getSizeInfo()
{
    /* Varz (vARS) (varaibles) */
    std::string token = co_await getAccessToken();
    auto parentID = co_await getMainFolderID(true);
    if (token.empty() || !parentID || (*parentID).empty())
        co_return std::nullopt;

    web::WebRequest req;
    web::WebResponse res;
    req.header("Authorization", fmt::format("Bearer {}", token));

    sizedata_map sizeDataMap;

    /* Get Data */
    std::string nextPageToken;
    do
    {
        req.removeParam("q");
        req.removeParam("pageToken");
        req.param("fields", "files/id,files/name,files/size, files/appProperties");
        req.param("q", fmt::format("'{}' in parents and trashed=false and mimeType = 'application/vnd.google-apps.folder'", *parentID));
        req.param("orderBy", "name_natural desc");
        if (!nextPageToken.empty())
            req.param("pageToken", nextPageToken);

        res = co_await req.get("https://www.googleapis.com/drive/v3/files");
        if (res.ok())
        {
            nextPageToken = res.json().unwrapOrDefault().get<std::string>("nextPageToken").unwrapOrDefault();
            auto folders = res.json().unwrapOrDefault()["files"];
            for (auto &value : folders)
            {
                auto id = value.get<std::string>("id").unwrapOrDefault();
                auto name = utils::string::trim(value.get<std::string>("name").unwrapOrDefault(), "'s saves");
                auto accountID = value.get<matjson::Value>("appProperties").unwrapOrDefault().get<std::string>("accountID").unwrapOrDefault();
                if (id.empty())
                    continue;

                // Get File Size! I like copy Paste :) I am Evil This is Evil I am So Sorry
                {
                    std::string nextPageToken;
                    do
                    {
                        req.removeParam("q");
                        req.removeParam("pageToken");
                        req.param("q", fmt::format("'{}' in parents and trashed=false", id));
                        if (!nextPageToken.empty())
                            req.param("pageToken", nextPageToken);

                        res = co_await req.get("https://www.googleapis.com/drive/v3/files");
                        if (res.ok())
                        {
                            nextPageToken = res.json().unwrapOrDefault().get<std::string>("nextPageToken").unwrapOrDefault();
                            auto files = res.json().unwrapOrDefault()["files"];
                            for (auto &value : files)
                            {
                                auto slotId = value.get<std::string>("id").unwrapOrDefault();
                                auto slotSize = value.get<std::string>("size").unwrapOrDefault();
                                auto slotName = value.get<std::string>("name").unwrapOrDefault();

                                sizeDataMap[utils::string::toLower(name)][fmt::format("Slot {}", utils::string::filter(slotName, "0123456789"))] = slotSize;
                                sizeDataMap[name]["account id"] = accountID;
                            }
                        }
                        else
                            co_return std::nullopt;

                    } while (!nextPageToken.empty());
                }
            }
        }
        else
            co_return std::nullopt;

    } while (!nextPageToken.empty());

    if (sizeDataMap.empty())
        co_return std::nullopt;

    co_return sizeDataMap;
}

arc::Future<bool> GDriveManager::getMetadata()
{
    m_gettingMetadata = true;

    // get token & parentID
    std::string token = co_await getAccessToken();
    auto parentID = co_await getUserFolderID(true);
    std::string nextPageToken;
    if (!parentID)
    {
        log::warn("Couldn't find parentID while getting metadata");
        m_gettingMetadata = false;
        co_return false;
    }
    // setup req
    auto req = web::WebRequest();
    web::WebResponse res;
    req.header("Authorization", fmt::format("Bearer {}", token));
    req.param("fields", "files/name, files/size, files/modifiedTime, files/description");
    req.param("q", fmt::format("'{}' in parents and trashed = false", *parentID));

    // do-while loop to go through pages of the res
    do
    {
        req.removeParam("pageToken");
        if (!nextPageToken.empty())
            req.param("pageToken", nextPageToken);

        res = co_await req.get("https://www.googleapis.com/drive/v3/files");
        if (res.ok())
        {
            // loop through file objects & save their data to userdata
            nextPageToken = res.json().unwrapOrDefault().get<std::string>("nextPageToken").unwrapOrDefault();
            auto folders = res.json().unwrapOrDefault()["files"];
            for (auto &value : folders)
            {
                auto slot = utils::numFromString<int>(utils::string::filter(value.get<std::string>("name").unwrapOrDefault(), "0123456789")).unwrapOr(-1);

                if (slot < 1)
                {
                    log::debug("Slot name is invalid [{}], skipping it...", slot);
                    continue;
                }
                auto description = value.get<std::string>("description").unwrapOrDefault();
                auto modifiedTime = value.get<std::string>("modifiedTime").unwrapOrDefault();
                auto size = value.get<std::string>("size").unwrapOrDefault();

                if (modifiedTime.empty() && (size.empty() || size == "0"))
                {
                    log::debug("Slot {} is empty, skipping it...", slot);
                    continue;
                }

                // parse modifiedTime
                std::tm tm;
                std::istringstream ss(modifiedTime);
                ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%SZ");

                time_t localTime;

                // lol
#ifdef GEODE_IS_WINDOWS
                localTime = _mkgmtime(&tm);
#else
                localTime = timegm(&tm);
#endif

                m_metadataMap[slot]["timestamp"] = std::to_string(localTime);
                m_metadataMap[slot]["size"] = size;
                m_metadataMap[slot]["description"] = description;
            }
        }
        else
        {
            log::warn("Couldn't get metadata: {}", res.string());
            m_gettingMetadata = false;
            co_return false;
        }
    } while (!nextPageToken.empty());

    m_gettingMetadata = false;

    co_return true;
}

arc::Future<bool> GDriveManager::setDescription(const std::string description, const int slot)
{
    std::string token = co_await getAccessToken();
    auto fileID = co_await getFileID(slot, false, fmt::format("Slot {} title update failed", slot));
    if (!fileID)
        co_return false;

    auto req = web::WebRequest();
    req.header("Authorization", fmt::format("Bearer {}", token));

    auto body = matjson::Value();
    body.set("description", description);
    req.bodyJSON(body);

    auto res = co_await req.patch(fmt::format("https://www.googleapis.com/drive/v3/files/{}", *fileID));
    if (res.ok())
        m_metadataMap[slot]["description"] = description.data();
    else
    {
        co_await waitForMainThread([this, slot, &res] {
            showError(fmt::format("Slot {} title update failed", slot), fmt::format("error: {} {}", res.code(), res.json().unwrapOrDefault()["error"].get<std::string>("status").unwrapOrDefault()), false);
        });
        co_return false;
    }

    co_return true;
}

arc::Future<bool> GDriveManager::deleteFile(const int slot)
{
    std::string token = co_await getAccessToken();
    auto fileID = co_await getFileID(slot, false, fmt::format("Slot {} delete failed", slot));
    if (!fileID)
        co_return false;

    auto req = web::WebRequest();
    req.header("Authorization", fmt::format("Bearer {}", token));
    req.method("DELETE");

    auto res = co_await req.send("DELETE", fmt::format("https://www.googleapis.com/drive/v3/files/{}", *fileID));
    if (res.error())
    {
        co_await waitForMainThread([this, slot, &res] {
            showError(fmt::format("Slot {} delete failed", slot), fmt::format("error: {} {}", res.code(), res.json().unwrapOrDefault()["error"].get<std::string>("status").unwrapOrDefault()), false);
        });
        co_return false;
    }
    co_return true;
}

arc::Future<std::vector<GDriveManager::fileRevision>> GDriveManager::getRevisionList(const int slot)
{
    std::vector<GDriveManager::fileRevision> arr = {};

    std::string token = co_await getAccessToken();
    auto fileID = co_await getFileID(slot, false, "", "", true);
    if (!fileID)
        co_return arr;

    auto req = web::WebRequest();
    req.header("Authorization", fmt::format("Bearer {}", token));

    req.param("pageSize", "1000");
    std::string nextPageToken = {};

    do
    {
        req.removeParam("pageToken");
        if (!nextPageToken.empty())
            req.param("pageToken", nextPageToken);

        auto res = co_await req.get(fmt::format("https://www.googleapis.com/drive/v3/files/{}/revisions", *fileID));

        if (res.ok())
        {
            nextPageToken = res.json().unwrapOrDefault().get<std::string>("nextPageToken").unwrapOrDefault();
            log::debug("{}", res.string());
        }
        else
        {
            log::warn("{}", res.string());
            co_return std::vector<GDriveManager::fileRevision>();
        }
    } while (!nextPageToken.empty());

    co_return arr;
}

arc::Future<std::string> GDriveManager::getRefreshToken()
{
    auto refreshToken = GDriveEncrypt::create()->decryptString(Mod::get()->getSavedValue<EncStr>("refresh_token"));
    if (refreshToken.empty())
    {
        co_await waitForMainThread([this] {
            signout(false);
            showError("Refresh Token");
        });
    }

    co_return refreshToken;
}

arc::Future<std::string> GDriveManager::getAccessToken()
{
    auto token = GDriveEncrypt::create()->decryptString(Mod::get()->getSavedValue<EncStr>("access_token"));
    if ((!token.empty()) && Mod::get()->getSavedValue<time_t>("access_expires_at") > std::time(nullptr))
        co_return token;

    /* Create request */
    auto req = web::WebRequest();
    req.param("refresh_token", co_await getRefreshToken());

    /* get access token and save it to var */
    auto res = co_await req.get("https://api.deaen.top/gdb/getaccesstoken/");
    if (res.ok())
    {
        auto token = res.json().unwrapOrDefault().get<std::string>("access_token").unwrapOrDefault();
        auto expires_at = res.json().unwrapOrDefault().get<time_t>("expires_in").unwrapOrDefault();
        if (token != "" && expires_at != 0)
        {
            Mod::get()->setSavedValue<EncStr>("access_token", GDriveEncrypt::create()->encryptString(token));
            Mod::get()->setSavedValue<time_t>("access_expires_at", std::time(nullptr) + expires_at);
        }
    }
    else
        log::warn("{}", res.string());

    co_return GDriveEncrypt::create()->decryptString(Mod::get()->getSavedValue<EncStr>("access_token"));
}

arc::Future<std::string> GDriveManager::getEmail()
{
    auto email = Mod::get()->getSavedValue<std::string>("email");
    if (!email.empty())
        co_return email;

    /* Create request */
    auto req = web::WebRequest();
    req.header("Authorization", fmt::format("Bearer {}", co_await getAccessToken()));
    req.param("fields", "user");

    /* get access token and save it to var */
    auto res = co_await req.get("https://www.googleapis.com/drive/v3/about");
    if (res.ok())
    {
        auto email = res.json()
                         .unwrapOrDefault()
                         .get<matjson::Value>("user")
                         .unwrapOrDefault()
                         .get<std::string>("emailAddress")
                         .unwrapOrDefault();
        if (email != "")
        {
            Mod::get()->setSavedValue<std::string>("email", email);
        }
    }
    else
    {
        log::warn("{}", res.string());
        auto error = res.json().unwrapOrDefault().get<matjson::Value>("error").unwrapOrDefault().get<std::string>("status").unwrapOr(fmt::format("error code: {}", res.code()));
        co_await waitForMainThread([this, error] { showError("Email", error); });
    }

    co_return Mod::get()->getSavedValue<std::string>("email");
}

void GDriveManager::updateQueue()
{
    if (!m_saveQueue.empty() && m_saveQueue.begin()->first)
    {

        saveData(m_saveQueue.begin()->first);
    }
}

void GDriveManager::addToQueue(GDriveSlotBox *box)
{
    m_saveQueue.insert_or_assign(box->getSlot(), box);
    if (m_saveQueue.begin()->first == box->getSlot())
        updateQueue();
}

void GDriveManager::removeFromQueue(const int slot)
{
    if (!m_saveQueue.empty() && m_saveQueue.begin()->first == slot)
    {
        m_saveListener.cancel();
    }

    if (m_saveQueue.contains(slot))
    {
        m_saveQueue.erase(slot);
        updateQueue();
    }
}

GDriveManager::Status GDriveManager::checkStatus(GDriveSlotBox *box)
{
    if (m_saveQueue.empty())
        return Idle;
    if (m_saveQueue.begin()->first == box->getSlot())
    {
        m_saveQueue.insert_or_assign(box->getSlot(), box);
        return Working;
    }
    else if (m_saveQueue.contains(box->getSlot()))
    {
        m_saveQueue.insert_or_assign(box->getSlot(), box);
        return Waiting;
    }

    return Idle;
}

void GDriveManager::removeBoxPointer(const int slot)
{
    if (!m_saveQueue.contains(slot))
        return;

    m_saveQueue.at(slot) = nullptr;
}

size_t GDriveManager::getSaveProgress()
{
    return m_saveProgress;
}

size_t GDriveManager::getSaveTotal()
{
    return m_saveTotal;
};

size_t GDriveManager::getLoadProgress()
{
    return m_loadProgress;
}

size_t GDriveManager::getLoadTotal()
{
    return m_loadTotal;
};
void GDriveManager::setGettingMetadataStatus(bool status)
{
    m_gettingMetadata = status;
}
bool GDriveManager::getMetadataStatus()
{
    return m_gettingMetadata;
}
metadata_map *GDriveManager::getMetadataMap()
{
    return &m_metadataMap;
}

void GDriveManager::setCurrentPopup(GDrivePopup *popup)
{
    GDriveManager::getInstance()->clearMetadata();
    m_currentPopup = popup;
}
void GDriveManager::setCurrentSigninPopup(GDriveSigninPopup *signinPopup)
{
    m_currentSigninPopup = signinPopup;
}

GDrivePopup *GDriveManager::getCurrentPopup()
{
    return m_currentPopup;
}

GDriveSigninPopup *GDriveManager::getCurrentSigninPopup()
{
    return m_currentSigninPopup;
}

void GDriveManager::clearMetadata()
{
    auto map = getMetadataMap();
    if (map)
        map->clear();
}
