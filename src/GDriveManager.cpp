#include "GDriveManager.hpp"

#include <Geode/utils/base64.hpp>
#include <ctime>

#include "GDriveEncrypt.hpp"

GDriveManager::GDriveManager()
{
    m_saveListener.setName("gdrive-save-listener");
    m_metadataListener.setName("gdrive-metadata-listener");
    m_loadListener.setName("gdrive-load-listener");
}

GDriveManager *GDriveManager::getInstance()
{
    static GDriveManager *instance = new GDriveManager();
    return instance;
}

void GDriveManager::showError(std::string_view title, std::string_view error, bool invasive)
{
    if (invasive)
    {
        std::string body =
            "<cg>GDriveBackup</c> has encountered an unexpected <cr>error</c>, please check your internet "
            "<cl>connection</c>.\nIf this keeps happening, please report this issue to the developer.";
        if (error != "")
            body += fmt::format("\n <cr>{}</c>", error);

        FLAlertLayer::create(fmt::format("{} Error", title).c_str(), body, "OK")->show();
    }
    else
    {
        Notification::create(fmt::format("GDrive Backup: {} {}", title, error), NotificationIcon::Error, 5.f)->show();
    }
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

void GDriveManager::signin()
{
    /* Generate new uuid and current timestamp */
    m_uuid = utils::random::generateUUID();
    m_timestamp = std::time(nullptr);

    /* Create request */
    auto req = web::WebRequest();
    req.param("uuid", m_uuid);
    req.param("timestamp", m_timestamp);

    /* Get url from api and then open in browser */
    async::spawn(req.get("https://api.deaen.top/gdb/getgoogleauth/"), [this](web::WebResponse res) {
        if (res.ok())
        {
            auto URL = res.json().unwrapOrDefault().get<std::string>("authorizationUrl").unwrapOrDefault();
            if (URL != "")
            {
                web::openLinkInBrowser(fmt::format(
                    "https://gdb.deaen.top/auth/google.html?uuid={}&timestamp={}&url={}", this->m_uuid.c_str(),
                    this->m_timestamp, utils::base64::encode(URL, base64::Base64Variant::UrlWithPad).c_str()));

                if (m_currentSigninPopup)
                    m_currentSigninPopup->showVerify();

                return;
            }
        }
        else
            log::warn("{}", res.string());

        if (m_currentSigninPopup)
            m_currentSigninPopup->showSignin();

        showError("Sign in", res.string().unwrapOrDefault());
    });
}
void GDriveManager::verify()
{
    /* Create request */
    auto req = web::WebRequest();
    req.param("uuid", m_uuid);
    req.param("timestamp", m_timestamp);

    /* Get token and save it in file */
    async::spawn(req.get("https://api.deaen.top/gdb/getrefreshtoken/"), [this](web::WebResponse res) {
        if (res.ok())
        {
            auto token = res.json().unwrapOrDefault().get<std::string>("refresh_token").unwrapOrDefault();
            if (token != "")
            {
                Mod::get()->setSavedValue<EncStr>("refresh_token", GDriveEncypt::create()->encryptString(token));
            }

            async::spawn(getAccessToken(), [this](std::string token) {
                if (m_currentSigninPopup)
                    m_currentSigninPopup->finishUp();
            });
        }
        else
        {

            log::warn("{}", res.string());
            if (m_currentSigninPopup)
                m_currentSigninPopup->showVerify();
            showError("Verify", res.string().unwrapOrDefault());
            return;
        }
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

arc::Future<std::string> GDriveManager::getFolderID(int slot, bool autoCreate)
{
    std::string latestID;

    for (const auto &folder :
         utils::string::split(fmt::format("GDrive Backup/{}", GJAccountManager::sharedState()->m_accountID, slot), "/"))
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

        std::string query =
            fmt::format("name='{}' and trashed=false and mimeType = 'application/vnd.google-apps.folder'", folder);
        if (!latestID.empty())
            query += fmt::format(" and '{}' in parents", latestID);
        req.param("q", query);

        auto res = req.getSync("https://www.googleapis.com/drive/v3/files");
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
            log::warn("{}", res.string());

        // if all else fails just create the folder man
        if (!autoCreate)
            co_return "";

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
        res = req.postSync("https://www.googleapis.com/drive/v3/files");
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
            log::warn("{}", res.string());

        latestID = "";
    }

    co_return latestID;
}

void GDriveManager::saveData(const int slot)
{
    auto req = web::WebRequest().onProgress([this, slot](web::WebProgress const &p) {
        if (m_saveQueue[slot])
            m_saveQueue[slot]->setStatusPercentage(m_saveProgress);
    });

    m_saveListener.spawn(saveString(static_cast<std::string>(GameManager::sharedState()->getCompressedSaveString()) +
                                        "|" +
                                        static_cast<std::string>(GameManager::sharedState()->getCompressedSaveString()),
                                    slot, req),
                         [this, slot](bool ok) {
                             if (m_saveQueue[slot])
                             {
                                 m_saveQueue[slot]->setStatusVisiblity(false);
                                 m_saveQueue[slot]->updateInfo();
                             }

                             removeFromQueue(Save, slot);

                             if (ok)
                                 Notification::create(fmt::format("GDrive Backup: Slot {} Save Complete!", slot),
                                                      NotificationIcon::Success, 5.f)
                                     ->show();
                         });
}

void GDriveManager::loadData(const int slot)
{
    auto loadLayer = GDriveManager::getInstance()->getCurrentPopup()->showLoadLayer();

    auto req = web::WebRequest().onProgress([this, loadLayer](web::WebProgress const &p) {
        if (loadLayer)
            loadLayer->setPercentage(m_loadProgress);
    });

    m_loadListener.spawn(loadString(slot, req, loadLayer), [slot](bool ok) {
        if (auto popup = GDriveManager::getInstance()->getCurrentPopup())
            popup->hideLoadLayer();

        if (ok)
            Notification::create(fmt::format("GDrive Backup: Slot {} Load Complete!", slot), NotificationIcon::Success,
                                 5.f)
                ->show();
    });
}

void GDriveManager::loadMetadata(const int slot)
{
    m_metadataListener.spawn(setMetadata(slot), [this, slot](bool ok) {
        if (!ok)
        {
            Mod::get()->setSavedValue<time_t>(
                fmt::format("{}-{}-timestamp", GJAccountManager::sharedState()->m_accountID, slot), 0);
            Mod::get()->setSavedValue<size_t>(
                fmt::format("{}-{}-size", GJAccountManager::sharedState()->m_accountID, slot), 0);
        }

        if (m_metadataQueue[slot])
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

    constexpr size_t chunkSize = 256 * 1024;
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
        req.param("q", fmt::format("name='{}' and '{}' in parents and trashed=false", fmt::format("save-{}.dat", slot),
                                   parentID));
        req.param("access_token", token);

        auto res = req.getSync("https://www.googleapis.com/drive/v3/files");
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

            log::warn("{}", res.string());
            co_await waitForMainThread([&res, slot, this] {
                showError(fmt::format("Slot {} Save Failed ", slot), res.errorMessage().data(), false);
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
            res = req.postSync("https://www.googleapis.com/upload/drive/v3/files");
        }
        else
            res = req.patchSync(fmt::format("https://www.googleapis.com/upload/drive/v3/files/{}", fileID));

        if (res.ok())
        {
            if (auto header = res.getAllHeadersNamed("location"))
            {
                if (!header->empty() && utils::string::contains(header->at(0), "https://"))
                {
                    resumableURL = header->at(0);
                }
            }
        }
        else
            log::warn("{}", res.string());

        if (resumableURL.empty())
        {
            co_await waitForMainThread([&res, slot, this] {
                showError(fmt::format("Slot {} Save Failed", slot), res.errorMessage().data(), false);
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
            size_t currentSize = std::min(chunkSize, data.size() - i);
            m_saveProgress = i + currentSize;

            responseReq.body(ByteVector(data.data() + i, data.data() + i + currentSize));
            responseReq.removeHeader("content-length");
            responseReq.removeHeader("content-range");
            responseReq.header("content-length", std::to_string(currentSize));
            responseReq.header("content-range", fmt::format("bytes {}-{}/{}", i, i + currentSize - 1, data.size()));
            res = responseReq.putSync(resumableURL);

            if (res.ok())
            {
                i += currentSize;
            }
            else if (res.badClient())
            {
                // try 5 times before giving up
                if (retries < 5)
                {
                    retries += 1;
                    continue;
                }
                else
                {
                    // assume the worst.
                    co_await waitForMainThread([&res, slot, this] {
                        showError(fmt::format("Slot {} Save Failed", slot), res.errorMessage().data(), false);
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
                            showError(fmt::format("Slot {} Save Failed", slot), res.errorMessage().data(), false);
                        });
                        co_return false;
                    }
                    i = r + 1;
                }
                else
                {
                    co_await waitForMainThread([&res, slot, this] {
                        showError(fmt::format("Slot {} Save Failed", slot), res.errorMessage().data(), false);
                    });
                    co_return false;
                }
            }
        }

        auto id = res.json().unwrapOrDefault().get<std::string>("id").unwrapOrDefault();
        if (!id.empty())
            Mod::get()->setSavedValue(fmt::format("{}-file-id", slot), id);
    }

    Mod::get()->setSavedValue<time_t>(
        fmt::format("{}-{}-timestamp", GJAccountManager::sharedState()->m_accountID, slot), std::time(nullptr));
    Mod::get()->setSavedValue<size_t>(fmt::format("{}-{}-size", GJAccountManager::sharedState()->m_accountID, slot),
                                      data.size());
    co_return true;
}

arc::Future<bool> GDriveManager::loadString(const int slot, web::WebRequest responseReq, GDriveLoadLayer *loadLayer)
{
    co_await waitForMainThread([&loadLayer] {
        if (loadLayer)
            loadLayer->setMessage("Preparing...");
    });

    constexpr size_t chunkSize = 10000 * 1024;
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

    auto res = req.getSync("https://www.googleapis.com/drive/v3/files");
    if (res.ok())
    {
        auto id = res.json().unwrapOrDefault()["files"][0].get<std::string>("id").unwrapOrDefault();
        if (!id.empty())
            fileID = std::move(id);
        else
        {
            co_await waitForMainThread([&res, slot, this] {
                showError(fmt::format("Slot {} Load Failed ", slot), "Can't find file", false);
                Mod::get()->setSavedValue<time_t>(
                    fmt::format("{}-{}-timestamp", GJAccountManager::sharedState()->m_accountID, slot), 0);
                Mod::get()->setSavedValue<size_t>(
                    fmt::format("{}-{}-size", GJAccountManager::sharedState()->m_accountID, slot), 0);
            });
            co_return false;
        }
    }
    else
    {

        log::warn("{}", res.string());
        co_await waitForMainThread([&res, slot, this] {
            showError(fmt::format("Slot {} Load Failed ", slot), res.errorMessage().data(), false);
        });
        co_return false;
    }

    /* Get file size */

    req.removeParam("q");
    req.param("fields", "size");
    res = req.getSync(fmt::format("https://www.googleapis.com/drive/v3/files/{}", fileID));

    if (res.ok())
    {
        auto size = numFromString<size_t>(res.json().unwrapOrDefault().get<std::string>("size").unwrapOrDefault())
                        .unwrapOrDefault();

        if (size == 0)
        {
            co_await waitForMainThread([&res, slot, this] {
                showError(fmt::format("Slot {} Load Failed ", slot), res.errorMessage().data(), false);
            });
            co_return false;
        }

        m_loadTotal = size;
        Mod::get()->setSavedValue<size_t>(fmt::format("{}-{}-size", GJAccountManager::sharedState()->m_accountID, slot),
                                          size);
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
        size_t currentSize = std::min(chunkSize, m_loadTotal - i);
        m_loadProgress = i + currentSize;

        responseReq.removeHeader("range");
        responseReq.header("range", fmt::format("bytes={}-{}", i, i + currentSize - 1));
        res = responseReq.getSync(fmt::format("https://www.googleapis.com/drive/v3/files/{}", fileID));

        if (res.ok())
        {
            buf.insert(buf.end(), res.data().begin(), res.data().end());
            i += currentSize;
        }
        else if (res.error())
        {
            if (auto range = res.getAllHeadersNamed("content-range"))
            {
                size_t r = geode::utils::numFromString<size_t>(
                               utils::string::split(utils::string::split(range->at(0), "-").back(), "/").front())
                               .unwrapOrDefault();
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
            // try 5 times before giving up
            if (retries < 5)
            {
                retries += 1;
                continue;
            }
            else
            {
                // assume the worst.
                co_await waitForMainThread([&res, slot, this] {
                    showError(fmt::format("Slot {} timeout reached", slot), res.errorMessage().data(), false);
                });
                co_return false;
            }
        }
        else
        {
            log::warn("{}", res.string());
            co_await waitForMainThread([&res, slot, this] {
                showError(fmt::format("Slot {} load failed", slot), std::to_string(res.code()), false);
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
            showError(fmt::format("Slot {} load failed", slot), "Can't get levels", false);

        // n the mappack string.. hopefully
        std::string packsString;
        size_t packCount = 0;
        auto packDic = gs.getDictForKey("GS_5", false);
        for (auto [pack, stars] : CCDictionaryExt<std::string_view, CCString *>(packDic))
        {
            if (!stars->compare(""))
                continue;

            if (packCount != 0)
                packsString += "|";

            packsString += "1:" + utils::string::remove(pack, "pack_") + ":4:" + stars->getCString();

            ++packCount;
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

        // lets load em packs
        auto mapPacks = GameLevelManager::sharedState()->createAndGetMapPacks(packsString);

        for (auto pack : CCArrayExt<GJMapPack *>(mapPacks))
        {
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

        // set the colors cuz they break sometimes lol
        gm->m_playerColor = gs.getIntegerForKey("playerColor");
        gm->m_playerColor2 = gs.getIntegerForKey("playerColor2");
        gm->m_playerGlowColor = (gs.getIntegerForKey("playerColor3") == -1) ? gs.getIntegerForKey("playerColor2")
                                                                            : gs.getIntegerForKey("playerColor3");
        gm->m_playerGlow = gs.getBoolForKey("playerGlow");
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

    std::string fileID; // = Mod::get()->getSavedValue<std::string>(fmt::format("{}-file-id", slot));
    std::string token = co_await getAccessToken();

    auto req = web::WebRequest();
    req.param("access_token", token);
    web::WebResponse res;

    /* Gtet id.......*/

    std::string parentID = co_await getFolderID(slot, false);
    if (parentID.empty())
        co_return false;

    req.param("q", fmt::format("name='save-{}.dat' and trashed=false and '{}' in parents", slot, parentID));

    res = req.getSync("https://www.googleapis.com/drive/v3/files");
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
        log::warn("{}", res.string());
        co_return false;
    }

    req.param("fields", "size,modifiedTime");
    res = req.getSync(fmt::format("https://www.googleapis.com/drive/v3/files/{}", fileID));

    if (res.ok())
    {
        auto timestamp = res.json().unwrapOrDefault().get<std::string>("modifiedTime").unwrapOrDefault();
        auto size = numFromString<size_t>(res.json().unwrapOrDefault().get<std::string>("size").unwrapOrDefault())
                        .unwrapOrDefault();

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
        log::warn("{}", res.string());
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
    auto res = req.getSync("https://api.deaen.top/gdb/getaccesstoken/");
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
        log::warn("{}", res.string());

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
    auto res = req.getSync("https://www.googleapis.com/drive/v3/about");
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
        auto error = res.json()
                         .unwrapOrDefault()
                         .get<matjson::Value>("error")
                         .unwrapOrDefault()
                         .get<std::string>("status")
                         .unwrapOr(std::to_string(res.code()));
        showError("Email", error);
    }

    co_return Mod::get()->getSavedValue<std::string>("email");
}

void GDriveManager::addToQueue(QueueType queueType, GDriveSlotBox *box)
{
    auto *queue = (queueType == Save) ? &m_saveQueue : &m_metadataQueue;
    queue->insert_or_assign(box->getSlot(), box);
    if (queue->begin()->first == box->getSlot())
    {
        updateQueue(queueType);
    }
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
