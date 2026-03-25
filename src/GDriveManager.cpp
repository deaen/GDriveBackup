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

void GDriveManager::showError(const std::string &title, const std::string &error, bool invasive)
{
    if (invasive)
    {
        std::string body =
            "<cg>GDriveBackup</c> has encountered an unexpected <cr>error</c>, please check your internet "
            "<cl>connection</c>.\nIf this keeps happening, please report this issue to the developer.";
        if (error != "")
            body += "\n <cr>" + error + "</c>";

        FLAlertLayer::create((title + " Error").c_str(), body, "OK")->show();
    }
    else
    {
        Notification::create("GDrive Backup: " + title + " " + error, NotificationIcon::Error, 5.f)->show();
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
                if (m_currentSigninPopup)
                    m_currentSigninPopup->finishUp();
                return;
            }
        }
        if (m_currentSigninPopup)
            m_currentSigninPopup->showVerify();

        showError("Verify", res.string().unwrapOrDefault());
    });
}
void GDriveManager::signout(bool openAgain)
{
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

    m_saveListener.spawn(saveString(GameManager::sharedState()->getCompressedSaveString() + "|" +
                                        LocalLevelManager::sharedState()->getCompressedSaveString(),
                                    slot, req),
                         [this, slot](bool ok) {
                             if (ok)
                                 Notification::create(fmt::format("GDrive Backup: Slot {} Save Complete!", slot),
                                                      NotificationIcon::Success, 5.f)
                                     ->show();
                             if (m_saveQueue[slot])
                             {
                                 m_saveQueue[slot]->setStatusVisiblity(false);
                                 m_saveQueue[slot]->updateInfo();
                             }

                             removeFromQueue(Save, slot);
                         });
}

void GDriveManager::loadData(const int slot)
{
    auto loadLayer = GDriveManager::getInstance()->getCurrentPopup()->showLoadLayer();

    auto req = web::WebRequest().onProgress([this, loadLayer](web::WebProgress const &p) {
        if (loadLayer)
            loadLayer->setPercentage(m_loadProgress);
    });

    m_loadListener.spawn(loadString(slot, req, loadLayer), [](bool ok) {
        if (auto popup = GDriveManager::getInstance()->getCurrentPopup())
            popup->hideLoadLayer();
    });
}

void GDriveManager::loadMetadata(const int slot)
{
    m_metadataListener.spawn(setMetadata(slot), [this, slot]() {
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

    Mod::get()->setSavedValue<std::time_t>(fmt::format("{}-timestamp", slot), std::time(nullptr));
    Mod::get()->setSavedValue<size_t>(fmt::format("{}-size", slot), data.size());
    co_return true;
}

arc::Future<bool> GDriveManager::loadString(const int slot, web::WebRequest responseReq, GDriveLoadLayer *loadLayer)
{
    co_await waitForMainThread([&loadLayer] {
        if (loadLayer)
            loadLayer->setMessage("Preparing...");
    });

    constexpr size_t chunkSize = 1000 * 1024;
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
    }
    else
    {
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
        Mod::get()->setSavedValue<size_t>(fmt::format("{}-size", slot), size);
    }

    co_await waitForMainThread([&loadLayer, this, slot] {
        if (loadLayer)
        {
            loadLayer->setMessage(fmt::format("Downloading Slot {} Data...", slot));
            loadLayer->showPercentage(m_loadTotal);
        }
    });

    /* Upload Data */

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
        // else if (res.code() == 206)
        // {

        //     if (auto range = res.getAllHeadersNamed("content-range"))
        //     {
        //         size_t r = geode::utils::numFromString<size_t>(
        //                        utils::string::split(utils::string::split(range->at(0), "-").back(), "/").front())
        //                        .unwrapOrDefault();
        //         if (r == 0)
        //         {
        //             co_await waitForMainThread([&res, slot, this] {
        //                 showError(fmt::format("Slot {} load Failed", slot), "Can't extract header", false);
        //             });
        //             co_return false;
        //         }

        //         buf.insert(buf.end(), res.data().begin(), res.data().end());
        //         i = r + 1;
        //     }
        //     else
        //     {
        //         co_await waitForMainThread([&res, slot, this] {
        //             showError(fmt::format("Slot {} load Failed", slot), "Can't get header", false);
        //         });
        //         co_return false;
        //     }
        // }
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
            co_await waitForMainThread([&res, slot, this] {
                showError(fmt::format("Slot {} load failed", slot), std::to_string(res.code()), false);
            });
            co_return false;
        }
    }

    std::string_view sv(reinterpret_cast<const char *>(buf.data()), buf.size());
    log::debug("{}", sv);
    log::debug("file size:{} sv:{}", m_loadTotal, sv.size());

    co_return true;
}

arc::Future<> GDriveManager::setMetadata(const int slot)
{
    auto savedTimestamp = Mod::get()->getSavedValue<time_t>(fmt::format("{}-timestamp", slot), -1);
    auto savedSize = Mod::get()->getSavedValue<size_t>(fmt::format("{}-size", slot));
    if (savedTimestamp != -1)
        co_return;

    std::string fileID = Mod::get()->getSavedValue<std::string>(fmt::format("{}-file-id", slot));
    std::string token = co_await getAccessToken();

    auto req = web::WebRequest();
    req.param("access_token", token);

    web::WebResponse res;

    /* Gtet id.......*/
    if (fileID.empty())
    {

        std::string parentID = co_await getFolderID(slot, false);
        if (parentID.empty())
            co_return;

        req.param("q", fmt::format("name='save-{}.dat' and '{}' in parents and trashed=false", slot, parentID));

        auto res = req.getSync("https://www.googleapis.com/drive/v3/files");
        if (res.ok())
        {
            auto id = res.json().unwrapOrDefault()["files"][0].get<std::string>("id").unwrapOrDefault();
            if (!id.empty())
            {
                fileID = std::move(id);
                Mod::get()->setSavedValue(fmt::format("{}-file-id", slot), fileID);
            }
            else
            {
                Mod::get()->setSavedValue<std::time_t>(fmt::format("{}-timestamp", slot), 0);
                Mod::get()->setSavedValue<size_t>(fmt::format("{}-size", slot), 0);
                co_return;
            }
        }
        else
            co_return;
    }

    req.param("fields", "size,modifiedTime");
    res = req.getSync(fmt::format("https://www.googleapis.com/drive/v3/files/{}", fileID));

    if (res.ok())
    {
        auto timestamp = res.json().unwrapOrDefault().get<std::string>("modifiedTime").unwrapOrDefault();
        auto size = numFromString<size_t>(res.json().unwrapOrDefault().get<std::string>("size").unwrapOrDefault())
                        .unwrapOrDefault();

        if (timestamp.empty() && size == 0)
            co_return;

        std::istringstream in(timestamp);
        std::chrono::sys_time<std::chrono::milliseconds> tp;
        in >> std::chrono::parse("%FT%TZ", tp);
        if (in.fail())
            co_return;

        Mod::get()->setSavedValue<std::time_t>(fmt::format("{}-timestamp", slot),
                                               std::chrono::system_clock::to_time_t(tp));
        Mod::get()->setSavedValue<size_t>(fmt::format("{}-size", slot), size);
    }
    co_return;
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

std::string GDriveManager::getRefreshToken()
{
    auto refreshToken = GDriveEncypt::create()->decryptString(Mod::get()->getSavedValue<EncStr>("refresh_token"));
    if (refreshToken == "")
    {
        signout(false);
        showError("Refresh Token");
    }
    return refreshToken;
}

arc::Future<std::string> GDriveManager::getAccessToken()
{
    auto token = GDriveEncypt::create()->decryptString(Mod::get()->getSavedValue<EncStr>("access_token"));
    if ((!token.empty()) && Mod::get()->getSavedValue<time_t>("access_expires_at") > std::time(nullptr))
        co_return token;

    /* Create request */
    auto req = web::WebRequest();
    req.param("refresh_token", getRefreshToken());

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
    if (queue->contains(slot))
        queue->at(slot) = nullptr;
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
