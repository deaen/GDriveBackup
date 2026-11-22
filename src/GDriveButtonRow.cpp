#include "Geode/ui/Popup.hpp"
#include <GDriveManager.hpp>
#include <Geode/loader/Mod.hpp>
#include <Geode/loader/SettingV3.hpp>

class GDriveButtonRow : public SettingV3
{
  public:
    static Result<std::shared_ptr<SettingV3>> parse(std::string const &key, std::string const &modID, matjson::Value const &json)
    {
        auto res = std::make_shared<GDriveButtonRow>();
        auto root = checkJson(json, "GDriveButtonRow");

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

/* I don't remember this */
class GDriveButtonRowNode : public SettingNodeV3
{
  protected:
    bool init(std::shared_ptr<GDriveButtonRow> setting, float width)
    {
        if (!SettingNodeV3::init(setting, width))
            return false;

        getNameLabel()->setVisible(false);
        auto m_signoutButtonSprite = ButtonSprite::create("Sign Out", "goldFont.fnt", "GJ_button_06.png");
        m_signoutButtonSprite->setScale(.7f);
        auto m_signoutButton = Button::createWithNode(m_signoutButtonSprite, onSignoutButton);

        auto m_loadButtonSprite = ButtonSprite::create("Load File", "goldFont.fnt", "GJ_button_01.png");
        m_loadButtonSprite->setScale(.7f);
        auto m_loadButton = Button::createWithNode(m_loadButtonSprite, onLoadButton);

        this->getButtonMenu()->setLayout(RowLayout::create());
        this->getButtonMenu()->setAnchorPoint({0.5f, 0.5f});
        this->getButtonMenu()->setContentSize(getBG()->getContentSize());
        this->getButtonMenu()->setPosition(getBG()->getPosition());

        this->getButtonMenu()->addChild(m_loadButton);
        this->getButtonMenu()->addChild(m_signoutButton);

        this->getButtonMenu()->updateLayout();

        this->updateState(nullptr);

        if (Mod::get()->getSavedValue<std::array<std::string, 3>>("refresh_token")[0].empty())
        {
            for (auto button : CCArrayExt<Button>(this->getButtonMenu()->getChildren()))
            {
                if (!button)
                    continue;
                button->setEnabled(false);
                auto sprite = static_cast<ButtonSprite *>(button->getDisplayNode());
                if (!sprite)
                    continue;
                sprite->setCascadeColorEnabled(true);
                sprite->setCascadeOpacityEnabled(true);
                sprite->setOpacity(155);
                sprite->setColor(ccGRAY);
            }
        }

        return true;
    }

    static void onSignoutButton(Button *sender)
    {
        auto popDown = createQuickPopup(
            "Google Account", "Do you want to sign out of GDrive Backup?\n<cy>Note:</c> This does <cg>not</c> delete your saved data.",
            "Cancel", "Sign Out",
            [sender](auto, bool btn2) {
                if (btn2)
                {
                    GDriveManager::getInstance()->signout(false);

                    for (auto button : CCArrayExt<Button>(sender->getParent()->getChildren()))
                    {
                        if (!button)
                            continue;
                        button->setEnabled(false);
                        auto sprite = static_cast<ButtonSprite *>(button->getDisplayNode());
                        if (!sprite)
                            continue;
                        sprite->setCascadeColorEnabled(true);
                        sprite->setCascadeOpacityEnabled(true);
                        sprite->setOpacity(155);
                        sprite->setColor(ccGRAY);
                    }
                }
            },
            false, true);
        popDown->m_button2->updateBGImage("GJ_button_06.png");
        popDown->show();
    }

    static void onLoadButton(Button *sender)
    {
        file::FilePickOptions options;

        file::FilePickOptions::Filter filter;
        filter.description = "GDriveBackup save file";
        options.filters.push_back(filter);

        async::spawn(file::pick(file::PickMode::OpenFile, options), [](file::PickResult result) {
            if (result.isOk() && result.unwrap().has_value())
            {
                auto filePath = result.unwrap().value_or("");
                if (filePath.isFile())
                {
                    auto popUp = createQuickPopup(
                        "Load local save file", "Do you want to <cg>load</c> this <cj>local</c> save file?\n<cy>This will merge your data!</c>",
                        "no", "yes",
                        [filePath](auto, bool btn2) {
                            if (btn2)
                            {
                                auto fileRes = utils::file::readBinary(filePath);
                                if (fileRes)
                                {
                                    auto fileData = std::move(fileRes).unwrap();

                                    auto loadLayer = GDriveManager::getInstance()->getCurrentPopup()->showLoadLayer();
                                    async::spawn(GDriveManager::getInstance()->downloadSave(-1, web::WebRequest(), loadLayer, (filePath.extension() == "gdbsave") ? GDriveConsts::slotType::New : GDriveConsts::slotType::Old, fileData), [](bool ok) {
                                        if (auto popup = GDriveManager::getInstance()->getCurrentPopup())
                                            popup->hideLoadLayer();

                                        if (ok)
                                            Notification::create("GDrive Backup: Load Complete!", NotificationIcon::Success, 5.f)->show();
                                    });
                                }
                                else
                                    GDriveManager::getInstance()->showError("Couldn't read save file,", "please try again", false);
                            }
                        },
                        false, true);
                    popUp->show();
                }
            }
        });
    }

    void onCommit() override
    {
    }
    void onResetToDefault() override
    {
    }

  public:
    static GDriveButtonRowNode *create(std::shared_ptr<GDriveButtonRow> setting, float width)
    {
        auto ret = new GDriveButtonRowNode();
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

SettingNodeV3 *GDriveButtonRow::createNode(float width)
{
    return GDriveButtonRowNode::create(std::static_pointer_cast<GDriveButtonRow>(shared_from_this()), width);
}

$execute
{
    (void)Mod::get()->registerCustomSettingType("gdrive-button-row", &GDriveButtonRow::parse);
}
