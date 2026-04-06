#include "GDriveManager.hpp"

#include <Geode/binding/LocalLevelManager.hpp>
#include <Geode/utils/base64.hpp>
#include <ctime>

#include "GDriveEncrypt.hpp"

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
    m_metadataListener.setName("gdrive-metadata-listener");
    m_loadListener.setName("gdrive-load-listener");

// Get the android hardware ID and store in here to avoid all the threading nonsense
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

void GDriveManager::showError(std::string_view title, std::string_view error, bool invasive)
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
        log::warn("Sign in Error Code: {}", res.code());
        showError("Sign in", fmt::format("Error code: {}", res.code()));
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
                Mod::get()->setSavedValue<EncStr>("refresh_token", GDriveEncypt::create()->encryptString(token));
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
        showError("Verify", fmt::format("Error code: {}", res.code()));
        if (m_currentSigninPopup)
            m_currentSigninPopup->showVerify();
    });
}

void GDriveManager::signout(bool openAgain)
{
    m_saveListener.cancel();
    m_loadListener.cancel();
    m_metadataListener.cancel();
    m_saveQueue.clear();
    m_metadataQueue.clear();

    if (m_currentPopup)
        m_currentPopup->removeFromParent();
    if (m_currentSigninPopup)
        m_currentSigninPopup->removeFromParent();

    Mod::get()->getSaveContainer().clear();

    if (openAgain)
        GDrivePopup::create();
}

arc::Future<std::string> GDriveManager::getFolderID(const int slot, const bool autoCreate)
{
    std::string latestID;

    for (const auto &folder : utils::string::split(fmt::format("GDrive Backup/{}", GJAccountManager::sharedState()->m_accountID, slot), "/"))
    {
        if (folder.empty())
            continue;

        auto localID = Mod::get()->getSavedValue<std::string>((fmt::format("{}-folder-id", folder)));
        if (!localID.empty())
        {
            latestID = localID;
            continue;
        }

        auto req = web::WebRequest();
        req.param("access_token", co_await getAccessToken());

        std::string query = fmt::format("name='{}' and trashed=false and mimeType = 'application/vnd.google-apps.folder'", folder);

        if (!latestID.empty())
            query += fmt::format(" and '{}' in parents", latestID);

        req.param("q", query);
        auto res = co_await req.get("https://www.googleapis.com/drive/v3/files");

        if (res.ok())
        {
            auto id = res.json().unwrapOrDefault()["files"][0].get<std::string>("id").unwrapOrDefault();
            if (!id.empty())
            {
                latestID = id;
                Mod::get()->setSavedValue<std::string>(fmt::format("{}-folder-id", folder), id);
                continue;
            }
        }
        else
            log::warn("Folder id find error code: {}", res.code());

        if (!autoCreate)
            co_return "";

        // if all else fails just create the folder man
        req.removeParam("q");
        auto body = matjson::makeObject({
            {"name", folder},
            {"mimeType", "application/vnd.google-apps.folder"},
        });

        if (!latestID.empty())
        {
            body.set("parents", std::vector<matjson::Value>({latestID}));
        }
        req.bodyJSON(body);
        res = co_await req.post("https://www.googleapis.com/drive/v3/files");
        if (res.ok())
        {
            auto id = res.json().unwrapOrDefault().get<std::string>("id").unwrapOrDefault();
            if (!id.empty())
            {
                latestID = id;
                Mod::get()->setSavedValue<std::string>(fmt::format("{}-folder-id", folder), id);
                continue;
            }
        }
        else
            log::warn("Folder creation error code: {}", res.code());

        latestID = "";
    }

    co_return latestID;
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
            m_saveQueue[slot]->updateInfo();
        }

        if (ok)
            Notification::create(fmt::format("GDrive Backup: Slot {} Save Complete!", slot), NotificationIcon::Success, 5.f)->show();

        removeFromQueue(Save, slot);
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

void GDriveManager::loadMetadata(const int slot)
{
    m_metadataListener.spawn(setMetadata(slot), [this, slot](bool ok) {
        if (!ok)
        {
            Mod::get()->setSavedValue<time_t>(fmt::format("{}-{}-timestamp", GJAccountManager::sharedState()->m_accountID, slot), 0);
            Mod::get()->setSavedValue<size_t>(fmt::format("{}-{}-size", GJAccountManager::sharedState()->m_accountID, slot), 0);
        }

        if (m_metadataQueue[slot] && checkStatus(Save, m_metadataQueue[slot]) == Idle)
        {
            m_metadataQueue[slot]->setStatusVisiblity(false);
            m_metadataQueue[slot]->updateInfo();
        }

        removeFromQueue(Metadata, slot);
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
    std::string parentID = co_await getFolderID(slot);
    std::string token = co_await getAccessToken();
    std::string fileID;

    if (parentID.empty())
    {
        co_await waitForMainThread([this] { showError("Couldn't find folder", "", false); });
        co_return false;
    }

    /* Look 4 if file alr exists */
    {
        auto req = web::WebRequest();
        req.param("q", fmt::format("name='{}' and '{}' in parents and trashed=false", fmt::format("save-{}.dat", slot), parentID));
        req.param("access_token", token);

        auto res = co_await req.get("https://www.googleapis.com/drive/v3/files");
        if (res.ok())
        {
            auto id = res.json().unwrapOrDefault()["files"][0].get<std::string>("id").unwrapOrDefault();
            if (!id.empty())
            {
                fileID = std::move(id);
                Mod::get()->setSavedValue(fmt::format("{}-file-id", slot), fileID);
            }
        }
        else
        {
            log::warn("{}", res.code());
            co_await waitForMainThread([&res, slot, this] {
                showError(fmt::format("Slot {} Save Failed ", slot), fmt::format("Error code: {}", res.code()), false);
            });
            co_return false;
        }
    }

    /* Initial request */
    {
        auto req = web::WebRequest();
        req.param("uploadType", "resumable");
        req.param("access_token", token);

        req.header("X-Upload-Content-Length", std::to_string(data.size()));
        req.header("Content-Type", "application/json; charset=UTF-8");

        web::WebResponse res;
        if (fileID.empty())
        {
            req.bodyJSON(matjson::makeObject({
                {"name", fmt::format("save-{}.dat", slot)},
                {"parents", std::vector<matjson::Value>({parentID})},
            }));
            res = co_await req.post("https://www.googleapis.com/upload/drive/v3/files");
        }
        else
            res = co_await req.patch(fmt::format("https://www.googleapis.com/upload/drive/v3/files/{}", fileID));

        if (res.ok())
        {
            if (auto header = res.getAllHeadersNamed("location"))
            {
                if (!header->empty() && utils::string::contains(header->at(0), "https://"))
                    resumableURL = header->at(0);
            }
        }
        else
            log::warn("{}", res.code());

        if (resumableURL.empty())
        {
            co_await waitForMainThread([&res, slot, this] {
                showError(fmt::format("Slot {} Save Failed", slot), fmt::format("Error code: {}", res.code()), false);
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
        responseReq.param("access_token", token);
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
                // try 5 times before giving up
                if (retries < 15)
                {
                    retries += 1;
                    continue;
                }
                else
                {
                    // assume the worst.
                    co_await waitForMainThread([&res, slot, this] {
                        showError(fmt::format("Slot {} Save Failed", slot), fmt::format("Error code: {}", res.code()), false);
                    });
                    co_return false;
                }
            }
            else
            {
                if (auto range = res.getAllHeadersNamed("range"))
                {
                    size_t r = geode::utils::numFromString<size_t>(utils::string::split(range->at(0), "-").back())
                                   .unwrapOrDefault();
                    if (r == 0)
                    {
                        co_await waitForMainThread([&res, slot, this] {
                            showError(fmt::format("Slot {} Save Failed", slot), fmt::format("Error code: {}", res.code()), false);
                        });
                        co_return false;
                    }
                    i = r + 1;
                }
                else
                {
                    co_await waitForMainThread([&res, slot, this] {
                        showError(fmt::format("Slot {} Save Failed", slot), fmt::format("Error code: {}", res.code()), false);
                    });
                    co_return false;
                }
            }
        }

        auto id = res.json().unwrapOrDefault().get<std::string>("id").unwrapOrDefault();
        if (!id.empty())
            Mod::get()->setSavedValue(fmt::format("{}-file-id", slot), id);
    }

    Mod::get()->setSavedValue<time_t>(fmt::format("{}-{}-timestamp", GJAccountManager::sharedState()->m_accountID, slot), std::time(nullptr));
    Mod::get()->setSavedValue<size_t>(fmt::format("{}-{}-size", GJAccountManager::sharedState()->m_accountID, slot), data.size());

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

    std::string parentID = co_await getFolderID(slot);
    std::string token = co_await getAccessToken();
    std::string fileID;

    if (parentID.empty())
    {
        co_await waitForMainThread([this] { showError("Couldn't find folder", "", false); });
        co_return false;
    }

    /* Get file ID */

    auto req = web::WebRequest();
    req.param("access_token", token);

    req.param("q", fmt::format("name='{}' and '{}' in parents and trashed=false", fmt::format("save-{}.dat", slot),
                               parentID));

    auto res = co_await req.get("https://www.googleapis.com/drive/v3/files");
    if (res.ok())
    {
        auto id = res.json().unwrapOrDefault()["files"][0].get<std::string>("id").unwrapOrDefault();
        if (!id.empty())
            fileID = std::move(id);
        else
        {
            co_await waitForMainThread([&res, slot, this] {
                showError(fmt::format("Slot {} Load Failed ", slot), "Can't find file", false);
                // Mod::get()->setSavedValue<time_t>(fmt::format("{}-{}-timestamp", GJAccountManager::sharedState()->m_accountID, slot), 0);
                // Mod::get()->setSavedValue<size_t>(fmt::format("{}-{}-size", GJAccountManager::sharedState()->m_accountID, slot), 0);
            });
            co_return false;
        }
    }
    else
    {

        log::warn("{}", res.code());
        co_await waitForMainThread([&res, slot, this] {
            showError(fmt::format("Slot {} Load Failed ", slot), fmt::format("Error code: {}", res.code()), false);
        });
        co_return false;
    }

    /* Get file size */
    req.removeParam("q");
    req.param("fields", "size");
    res = co_await req.get(fmt::format("https://www.googleapis.com/drive/v3/files/{}", fileID));

    if (res.ok())
    {
        auto size = numFromString<size_t>(res.json().unwrapOrDefault().get<std::string>("size").unwrapOrDefault()).unwrapOrDefault();

        if (size == 0)
        {
            co_await waitForMainThread([&res, slot, this] {
                showError(fmt::format("Slot {} Load Failed ", slot), fmt::format("Error code: {}", res.code()), false);
            });
            co_return false;
        }

        m_loadTotal = size;
        Mod::get()->setSavedValue<size_t>(fmt::format("{}-{}-size", GJAccountManager::sharedState()->m_accountID, slot), size);
    }
    else
        log::warn("{}", res.code());

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
        res = co_await responseReq.get(fmt::format("https://www.googleapis.com/drive/v3/files/{}", fileID));

        if (res.ok())
        {
            buf.insert(buf.end(), res.data().begin(), res.data().end());
            i += currentSize;
        }
        else if (res.error())
        {
            if (auto range = res.getAllHeadersNamed("content-range"))
            {
                size_t r = geode::utils::numFromString<size_t>(utils::string::split(utils::string::split(range->at(0), "-").back(), "/").front()).unwrapOrDefault();
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
                    showError(fmt::format("Slot {} timeout reached", slot), fmt::format("Error code: {}", res.code()), false);
                });
                co_return false;
            }
        }
        else
        {
            log::warn("{}", res.code());
            co_await waitForMainThread([&res, slot, this] {
                showError(fmt::format("Slot {} load failed", slot), fmt::format("Error code: {}", res.code()), false);
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
        // if (loadLayer)
        // {
        //     loadLayer->setMessage("Loading data...");
        //     loadLayer->hidePercentage();
        // }

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

        // setting game varaibles
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

arc::Future<bool> GDriveManager::setMetadata(const int slot)
{
    // auto savedTimestamp = Mod::get()->getSavedValue<time_t>(
    //     fmt::format("{}-{}-timestamp", GJAccountManager::sharedState()->m_accountID, slot), -1);
    // auto savedSize = Mod::get()->getSavedValue<size_t>(
    //     fmt::format("{}-{}-size", GJAccountManager::sharedState()->m_accountID, slot));
    // if (savedTimestamp != -1)
    //     co_return;

    std::string fileID;
    std::string token = co_await getAccessToken();

    auto req = web::WebRequest();
    req.param("access_token", token);
    web::WebResponse res;

    /* Gtet id.......*/

    std::string parentID = co_await getFolderID(slot, false);
    if (parentID.empty())
        co_return false;

    req.param("q", fmt::format("name='save-{}.dat' and trashed=false and '{}' in parents", slot, parentID));

    res = co_await req.get("https://www.googleapis.com/drive/v3/files");
    if (res.ok())
    {
        auto id = res.json().unwrapOrDefault()["files"][0].get<std::string>("id").unwrapOrDefault();
        if (!id.empty())
        {
            fileID = std::move(id);
            Mod::get()->setSavedValue(fmt::format("{}-file-id", slot), fileID);
        }
        else
            co_return false;
    }
    else
    {
        log::warn("{}", res.code());
        co_return false;
    }

    req.param("fields", "size,modifiedTime");
    res = co_await req.get(fmt::format("https://www.googleapis.com/drive/v3/files/{}", fileID));

    if (res.ok())
    {
        auto timestamp = res.json().unwrapOrDefault().get<std::string>("modifiedTime").unwrapOrDefault();
        auto size = numFromString<size_t>(res.json().unwrapOrDefault().get<std::string>("size").unwrapOrDefault()).unwrapOrDefault();

        if (timestamp.empty() && size == 0)
            co_return false;

        std::tm tm;
        std::istringstream ss(timestamp);
        ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%SZ");

        time_t localtime;

        // lol
#ifdef GEODE_IS_WINDOWS
        localtime = _mkgmtime(&tm);
#else
        localtime = timegm(&tm);
#endif

        Mod::get()->setSavedValue<time_t>(fmt::format("{}-{}-timestamp", GJAccountManager::sharedState()->m_accountID, slot), localtime);
        Mod::get()->setSavedValue<size_t>(fmt::format("{}-{}-size", GJAccountManager::sharedState()->m_accountID, slot), size);
    }
    else
    {
        log::warn("{}", res.code());
        co_return false;
    }

    co_return true;
}

void GDriveManager::setCurrentPopup(GDrivePopup *popup)
{
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

arc::Future<std::string> GDriveManager::getRefreshToken()
{
    auto refreshToken = GDriveEncypt::create()->decryptString(Mod::get()->getSavedValue<EncStr>("refresh_token"));
    if (refreshToken == "")
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
    auto token = GDriveEncypt::create()->decryptString(Mod::get()->getSavedValue<EncStr>("access_token"));
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
            Mod::get()->setSavedValue<EncStr>("access_token", GDriveEncypt::create()->encryptString(token));
            Mod::get()->setSavedValue<time_t>("access_expires_at", std::time(nullptr) + expires_at);
        }
    }
    else
        log::warn("{}", res.code());

    co_return GDriveEncypt::create()->decryptString(Mod::get()->getSavedValue<EncStr>("access_token"));
}

arc::Future<std::string> GDriveManager::getEmail()
{
    auto email = Mod::get()->getSavedValue<std::string>("email");
    if (!email.empty())
        co_return email;

    /* Create request */
    auto req = web::WebRequest();
    req.param("access_token", co_await getAccessToken());
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
        log::warn("{}", res.code());
        auto error = res.json()
                         .unwrapOrDefault()
                         .get<matjson::Value>("error")
                         .unwrapOrDefault()
                         .get<std::string>("status")
                         .unwrapOr(fmt::format("Error code: {}", res.code()));
        co_await waitForMainThread([this, &error] { showError("Email", error); });
    }

    co_return Mod::get()->getSavedValue<std::string>("email");
}

void GDriveManager::updateQueue(QueueType queueType)
{
    auto *queue = (queueType == Save) ? &m_saveQueue : &m_metadataQueue;
    if (!queue->empty() && queue->begin()->first)
    {
        if (queueType == Save)
            saveData(queue->begin()->first);
        else if (queueType == Metadata)
            loadMetadata(queue->begin()->first);
    }
}

void GDriveManager::addToQueue(QueueType queueType, GDriveSlotBox *box)
{
    auto *queue = (queueType == Save) ? &m_saveQueue : &m_metadataQueue;
    queue->insert_or_assign(box->getSlot(), box);
    if (queue->begin()->first == box->getSlot())
        updateQueue(queueType);
}

void GDriveManager::removeFromQueue(QueueType queueType, const int slot)
{
    auto *queue = (queueType == Save) ? &m_saveQueue : &m_metadataQueue;

    if (!queue->empty() && queue->begin()->first == slot)
    {
        if (queueType == Save)
            m_saveListener.cancel();
        else if (queueType == Metadata)
            m_metadataListener.cancel();
    }

    if (queue->contains(slot))
    {
        queue->erase(slot);
        updateQueue(queueType);
    }
}

GDriveManager::Status GDriveManager::checkStatus(QueueType queueType, GDriveSlotBox *box)
{
    auto *queue = (queueType == Save) ? &m_saveQueue : &m_metadataQueue;

    if (queue->empty())
        return Idle;
    if (queue->begin()->first == box->getSlot())
    {
        queue->insert_or_assign(box->getSlot(), box);
        return Working;
    }
    else if (queue->contains(box->getSlot()))
    {
        queue->insert_or_assign(box->getSlot(), box);
        return Waiting;
    }

    return Idle;
}

void GDriveManager::removeBoxPointer(QueueType queueType, const int slot)
{
    auto *queue = (queueType == Save) ? &m_saveQueue : &m_metadataQueue;
    if (!queue->contains(slot))
        return;

    queue->at(slot) = nullptr;

    if (queueType == Metadata)
        removeFromQueue(Metadata, slot);
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
