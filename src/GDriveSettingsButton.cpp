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
    ButtonSprite *m_buttonSprite;
    CCMenuItemSpriteExtra *m_button;

    bool init(std::shared_ptr<GDriveSettingsButton> setting, float width)
    {
        if (!SettingNodeV3::init(setting, width))
            return false;

        getNameLabel()->setVisible(false);
        m_buttonSprite = ButtonSprite::create("Sign Out", "goldFont.fnt", "GJ_button_01.png");
        m_buttonSprite->setScale(.7f);
        m_button = CCMenuItemSpriteExtra::create(m_buttonSprite, this, menu_selector(GDriveSettingsButtonNode::onButton));

        if (Mod::get()->getSavedValue<std::array<std::string, 3>>("refresh_token")[0].empty())
        {
            m_button->setEnabled(false);
            m_buttonSprite->setCascadeColorEnabled(true);
            m_buttonSprite->setCascadeOpacityEnabled(true);
            m_buttonSprite->setOpacity(155);
            m_buttonSprite->setColor(ccGRAY);
        }

        this->getButtonMenu()->addChildAtPosition(m_button, Anchor::Center);
        this->getButtonMenu()->setAnchorPoint({0.5f, 0.5f});
        this->getButtonMenu()->setContentSize(getBG()->getContentSize());
        this->getButtonMenu()->setPosition(getBG()->getPosition());
        this->getButtonMenu()->updateLayout();

        this->updateState(nullptr);

        return true;
    }

    void onButton(CCObject *)
    {
        createQuickPopup(
            "Google Account", "Do you want to sign out of GDrive Backup?\n<cy>Note:</c> This does <cg>not</c> delete your saved data.",
            "Cancel", "Sign Out",
            [](auto, bool btn2) {
                if (btn2)
                    GDriveManager::getInstance()->signout(false);
            },
            true, true);
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
