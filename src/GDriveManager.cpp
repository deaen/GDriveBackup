#include "GDriveManager.hpp"

#include <Geode/utils/base64.hpp>
#include <ctime>

#include "GDriveEncrypt.hpp"

GDriveManager::GDriveManager()
{
}

GDriveManager *GDriveManager::getInstance()
{
    static GDriveManager *instance = new GDriveManager();
    return instance;
}

void GDriveManager::showError(const std::string &title, const std::string &error, bool invasive)
{
    std::string body = "<cg>GDriveBackup</c> has encountered an unexpected <cr>error</c>, please check your internet "
                       "<cl>connection</c>.\nIf this keeps happening, please report this issue to the developer.";
    if (error != "")
        body += "\n <cr>" + error + "</c>";

    FLAlertLayer::create((title + " Error").c_str(), body, "OK")->show();
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
        if (!res.error() && res.code() == 200)
        {
            auto URL = res.json().unwrap().get<std::string>("authorizationUrl").unwrapOr("");
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

        showError("Sign in", res.string().unwrap());
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
        if (!res.error() && res.code() == 200)
        {
            auto token = res.json().unwrap().get<std::string>("refresh_token").unwrapOr("");
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

        showError("Verify", res.string().unwrap());
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
void GDriveManager::saveData()
{
}
void GDriveManager::getMetadata()
{
}
void GDriveManager::loadData()
{
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
    if (!res.error() && res.code() == 200)
    {
        auto token = res.json().unwrap().get<std::string>("access_token").unwrapOr("");
        auto expires_at = res.json().unwrap().get<time_t>("expires_in").unwrapOr(0);
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
    auto token = co_await getAccessToken();
    req.param("access_token", token);
    req.param("fields", "user");

    /* get access token and save it to var */
    auto res = req.getSync("https://www.googleapis.com/drive/v3/about");
    if (!res.error() && res.code() == 200)
    {

        auto email = res.json()
                         .unwrap()
                         .get<matjson::Value>("user")
                         .unwrapOr(matjson::Value())
                         .get<std::string>("emailAddress")
                         .unwrapOr("");
        if (email != "")
        {
            Mod::get()->setSavedValue<std::string>("email", email);
        }
    }
    else
    {
        auto error = res.json()
                         .unwrap()
                         .get<matjson::Value>("error")
                         .unwrapOr(matjson::Value())
                         .get<std::string>("status")
                         .unwrapOr(std::to_string(res.code()));
        showError("Email", error);
    }

    co_return Mod::get()->getSavedValue<std::string>("email");
}
