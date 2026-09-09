#include "Geode/ui/Popup.hpp"
#include <Geode/loader/Mod.hpp>
#include <Geode/loader/SettingV3.hpp>

#include <GDriveManager.hpp>

using namespace geode::prelude;

class GDriveSettingsButton : public SettingV3
{
  public:
    static Result<std::shared_ptr<SettingV3>> parse(std::string const &key, std::string const &modID, matjson::Value const &json)
    {
        auto res = std::make_shared<GDriveSettingsButton>();
        auto root = checkJson(json, "GDriveSettingsButton");

        res->init(key, modID, root);

        root.checkUnknownKeys();
        return root.ok(std::static_pointer_cast<SettingV3>(res));
    }

    bool load(matjson::Value const &json) override
    {
        return true;
    }
    bool save(matjson::Value &json) const override
    {
        return true;
    }

    bool isDefaultValue() const override
    {
        return true;
    }
    void reset() override
    {
    }

    SettingNodeV3 *createNode(float width) override;
};

class GDriveSettingsButtonNode : public SettingNodeV3
{
  protected:
    ButtonSprite *m_signinButtonSprite;
    CCMenuItemSpriteExtra *m_signinButton;

    ButtonSprite *m_cacheButtonSprite;
    CCMenuItemSpriteExtra *m_cacheButton;

    bool init(std::shared_ptr<GDriveSettingsButton> setting, float width)
    {
        if (!SettingNodeV3::init(setting, width))
            return false;

        getNameLabel()->setVisible(false);
        m_signinButtonSprite = ButtonSprite::create("Sign Out", "goldFont.fnt", "GJ_button_01.png");
        m_signinButtonSprite->setScale(.7f);
        m_signinButton = CCMenuItemSpriteExtra::create(m_signinButtonSprite, this, menu_selector(GDriveSettingsButtonNode::onSigninButton));

        if (Mod::get()->getSavedValue<std::array<std::string, 3>>("refresh_token")[0].empty())
        {
            m_signinButton->setEnabled(false);
            m_signinButtonSprite->setCascadeColorEnabled(true);
            m_signinButtonSprite->setCascadeOpacityEnabled(true);
            m_signinButtonSprite->setOpacity(155);
            m_signinButtonSprite->setColor(ccGRAY);
        }

        m_cacheButtonSprite = ButtonSprite::create("Clear Cache", "goldFont.fnt", "GJ_button_01.png");
        m_cacheButtonSprite->setScale(.7f);
        m_cacheButton = CCMenuItemSpriteExtra::create(m_cacheButtonSprite, this, menu_selector(GDriveSettingsButtonNode::onCacheButton));

        this->getButtonMenu()->setLayout(RowLayout::create());
        this->getButtonMenu()->setAnchorPoint({0.5f, 0.5f});
        this->getButtonMenu()->setContentSize(getBG()->getContentSize());
        this->getButtonMenu()->setPosition(getBG()->getPosition());

        this->getButtonMenu()->addChild(m_cacheButton);
        this->getButtonMenu()->addChild(m_signinButton);
        
        this->getButtonMenu()->updateLayout();

        this->updateState(nullptr);

        return true;
    }

    void onSigninButton(CCObject *)
    {
        auto popDown = createQuickPopup(
            "Google Account", "Do you want to sign out of GDrive Backup?\n<cy>Note:</c> This does <cg>not</c> delete your saved data.",
            "Cancel", "Sign Out",
            [this](auto, bool btn2) {
                if (btn2)
                {
                    GDriveManager::getInstance()->signout(false);

                    m_signinButton->setEnabled(false);
                    m_signinButtonSprite->setCascadeColorEnabled(true);
                    m_signinButtonSprite->setCascadeOpacityEnabled(true);
                    m_signinButtonSprite->setOpacity(155);
                    m_signinButtonSprite->setColor(ccGRAY);
                }
            },
            false, true);
        popDown->m_button2->updateBGImage("GJ_button_06.png");
        popDown->show();
    }

    void onCacheButton(CCObject *)
    {
        auto popDown = createQuickPopup(
            "Clear ID cache", "Do you want to clear ID cache?\n(this doesn't delete your save data)\n<cg>this may be useful if you're facing file & folder Find issues when saving/loading.</c>",
            "no", "yes",
            [this](auto, bool btn2) {
                if (btn2)
                {
                    GDriveManager::getInstance()->clearIDCache();
                }
            }, false, true);
        popDown->show();
    }

    void onCommit() override
    {
    }
    void onResetToDefault() override
    {
    }

  public:
    static GDriveSettingsButtonNode *create(std::shared_ptr<GDriveSettingsButton> setting, float width)
    {
        auto ret = new GDriveSettingsButtonNode();
        if (ret->init(setting, width))
        {
            ret->autorelease();
            return ret;
        }
        delete ret;
        return nullptr;
    }

    bool hasUncommittedChanges() const override
    {
        return false;
    }
    bool hasNonDefaultValue() const override
    {
        return false;
    }
};

// Create node as before
SettingNodeV3 *GDriveSettingsButton::createNode(float width)
{
    return GDriveSettingsButtonNode::create(std::static_pointer_cast<GDriveSettingsButton>(shared_from_this()), width);
}

// Register as before
$execute
{
    (void)Mod::get()->registerCustomSettingType("signout-button", &GDriveSettingsButton::parse);
}
