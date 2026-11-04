#include "GDriveConsts.hpp"
#include <Geode/loader/Mod.hpp>
#include <Geode/loader/SettingV3.hpp>
using namespace geode::prelude;

template <>
struct matjson::Serialize<GDriveConsts::backupSettingValue>
{
    static matjson::Value toJson(GDriveConsts::backupSettingValue const &value)
    {
        matjson::Value json;
        json.set("save", value.save);
        json.set("load", value.load);

        return json;
    }
    static Result<GDriveConsts::backupSettingValue> fromJson(matjson::Value const &value)
    {
        GDriveConsts::backupSettingValue valueStruct;
        valueStruct.save = value.get<bool>("save").unwrapOr(false);
        valueStruct.load = value.get<bool>("load").unwrapOr(false);

        return Ok(valueStruct);
    }
};

class GDriveBackupSetting : public SettingBaseValueV3<GDriveConsts::backupSettingValue>
{
  public:
    static Result<std::shared_ptr<SettingV3>> parse(std::string const &key, std::string const &modID, matjson::Value const &json)
    {
        auto res = std::make_shared<GDriveBackupSetting>();
        auto root = checkJson(json, "GDriveBackupSetting");
        res->parseBaseProperties(key, modID, root);
        root.checkUnknownKeys();
        return root.ok(std::static_pointer_cast<SettingV3>(res));
    }

    SettingNodeV3 *createNode(float width) override;
};

template <>
struct geode::SettingTypeForValueType<GDriveConsts::backupSettingValue>
{
    using SettingType = GDriveBackupSetting;
};

class GDriveBackupSettingNode : public SettingValueNodeV3<GDriveBackupSetting>
{
  protected:
    CCMenuItemToggler *saveToggle;
    CCMenuItemToggler *loadToggle;

    GDriveConsts::backupSettingValue togglesValues;
    bool init(std::shared_ptr<GDriveBackupSetting> setting, float width)
    {
        if (!SettingValueNodeV3::init(setting, width))
            return false;

        // save menu
        auto saveMenu = CCMenu::create();
        saveMenu->setID("save-menu"_spr);

        saveToggle = CCMenuItemToggler::createWithStandardSprites(this, menu_selector(GDriveBackupSettingNode::onSaveToggle), .5f);
        saveToggle->setID("save-toggle"_spr);
        saveMenu->addChild(saveToggle);

        auto saveLabel = Label::create("Save", "bigFont.fnt");
        saveLabel->setScale(0.3f);
        saveLabel->setID("save-label"_spr);
        saveMenu->addChild(saveLabel);

        this->getButtonMenu()->addChild(saveMenu);

        // load menu
        auto loadMenu = CCMenu::create();
        loadMenu->setID("load-menu"_spr);

        loadToggle = CCMenuItemToggler::createWithStandardSprites(this, menu_selector(GDriveBackupSettingNode::onLoadToggle), .5f);
        loadToggle->setID("load-toggle"_spr);
        loadMenu->addChild(loadToggle);

        auto loadLabel = Label::create("load", "bigFont.fnt");
        loadLabel->setScale(0.3f);
        loadLabel->setID("load-label"_spr);
        loadMenu->addChild(loadLabel);

        this->getButtonMenu()->addChild(loadMenu);

        this->getButtonMenu()->setContentHeight(30.f);
        saveMenu->setContentHeight(30.f);
        loadMenu->setContentHeight(30.f);

        auto menuLayout = ColumnLayout::create()
                              ->setAxisReverse(true)
                              ->setAutoScale(false)
                              ->setPadding({0.f, 10.f, 0.f, 1.f})
                              ->setGap(5.f)
                              ->setAxisAlignment(AxisAlignment::Between);

        saveMenu->setLayout(menuLayout);
        loadMenu->setLayout(menuLayout);

        this->getButtonMenu()->setLayout(RowLayout::create()->setAxisAlignment(AxisAlignment::End)->setGap(30.f));

        this->updateState(nullptr);

        return true;
    }

    void updateState(CCNode *invoker) override
    {
        SettingValueNodeV3::updateState(invoker);
        // auto shouldEnable = this->getSetting()->shouldEnable();

        togglesValues = this->getValue();
        saveToggle->toggle(togglesValues.save);
        loadToggle->toggle(togglesValues.load);

        log::debug("save {} & load {}", togglesValues.save, togglesValues.load);
        // saveToggle->setEnabled(shouldEnable);
        // saveToggle->setCascadeColorEnabled(true);
        // saveToggle->setCascadeOpacityEnabled(true);
        // saveToggle->setOpacity(shouldEnable ? 255 : 155);
        // saveToggle->setColor(shouldEnable ? ccWHITE : ccGRAY);

        
        // loadToggle->setEnabled(shouldEnable);
        // loadToggle->setCascadeColorEnabled(true);
        // loadToggle->setCascadeOpacityEnabled(true);
        // loadToggle->setOpacity(shouldEnable ? 255 : 155);
        // loadToggle->setColor(shouldEnable ? ccWHITE : ccGRAY);
    }

    void onSaveToggle(CCObject *sender)
    {
        auto toggle = static_cast<CCMenuItemToggler *>(sender);
        togglesValues.save = !toggle->m_toggled;
        this->setValue(togglesValues, toggle);

        log::debug("save: {}", togglesValues.save);
    }

    void onLoadToggle(CCObject *sender)
    {
        auto toggle = static_cast<CCMenuItemToggler *>(sender);
        togglesValues.load = !toggle->m_toggled;
        this->setValue(togglesValues, toggle);

                log::debug("load: {}", togglesValues.load);

    }

  public:
    static GDriveBackupSettingNode *create(std::shared_ptr<GDriveBackupSetting> setting, float width)
    {
        auto ret = new GDriveBackupSettingNode();
        if (ret && ret->init(setting, width))
        {
            ret->autorelease();
            return ret;
        }
        CC_SAFE_DELETE(ret);
        return nullptr;
    }
};

SettingNodeV3 *GDriveBackupSetting::createNode(float width)
{
    return GDriveBackupSettingNode::create(std::static_pointer_cast<GDriveBackupSetting>(shared_from_this()), width);
}

$execute
{
    auto ret = Mod::get()->registerCustomSettingType("gdrive-backup-setting", &GDriveBackupSetting::parse);
    if (!ret)
    {
        log::error("Unable to register setting type: {}", ret.unwrapErr());
    }
}