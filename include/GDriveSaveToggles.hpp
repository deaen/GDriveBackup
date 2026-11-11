#include "GDriveConsts.hpp"
#include <Geode/loader/Mod.hpp>
#include <Geode/loader/SettingV3.hpp>
#include <Geode/ui/Toggler.hpp>
using namespace geode::prelude;

template <>
struct matjson::Serialize<GDriveConsts::backupSettingValue>
{
    static matjson::Value toJson(GDriveConsts::backupSettingValue const &value)
    {
        matjson::Value json;
        json.set("save", value.save);
        json.set("load", value.load);
        json.set("saveToggleEnabled", value.saveToggleEnabled);
        json.set("loadToggleEnabled", value.loadToggleEnabled);

        return json;
    }
    static Result<GDriveConsts::backupSettingValue> fromJson(matjson::Value const &value)
    {
        GDriveConsts::backupSettingValue valueStruct;
        valueStruct.save = value.get<bool>("save").unwrapOrDefault();
        valueStruct.load = value.get<bool>("load").unwrapOrDefault();
        valueStruct.saveToggleEnabled = value.get<bool>("saveToggleEnabled").unwrapOrDefault();
        valueStruct.loadToggleEnabled = value.get<bool>("loadToggleEnabled").unwrapOrDefault();

        return Ok(valueStruct);
    }
};

class GDriveSaveToggles : public SettingBaseValueV3<GDriveConsts::backupSettingValue>
{
  public:
    static Result<std::shared_ptr<SettingV3>> parse(std::string const &key, std::string const &modID, matjson::Value const &json)
    {
        auto res = std::make_shared<GDriveSaveToggles>();
        auto root = checkJson(json, "GDriveSaveToggles");
        res->parseBaseProperties(key, modID, root);
        root.checkUnknownKeys();
        return root.ok(std::static_pointer_cast<SettingV3>(res));
    }

    SettingNodeV3 *createNode(float width) override;
};

template <>
struct geode::SettingTypeForValueType<GDriveConsts::backupSettingValue> {
    using SettingType = GDriveSaveToggles;
};

class GDriveSaveTogglesNode : public SettingValueNodeV3<GDriveSaveToggles>
{
  protected:
    geode::Toggler *saveToggle;
    geode::Toggler *loadToggle;
    GDriveConsts::backupSettingValue togglesValues;

    bool init(std::shared_ptr<GDriveSaveToggles> setting, float width)
    {
        if (!SettingValueNodeV3::init(setting, width))
            return false;

        auto *menu = this->getButtonMenu();

        // save menu
        auto saveMenu = CCLayerRGBA::create();
        saveMenu->setID("save-menu"_spr);

        saveToggle = geode::Toggler::createWithStandardSprites(.5f, [this](geode::Toggler *sender, bool isToggled) {
            togglesValues.save = isToggled;
            this->setValue(togglesValues, sender);
        });
        saveToggle->setID("save-toggle"_spr);
        saveMenu->addChild(saveToggle);

        auto saveLabel = Label::create("Save", "bigFont.fnt");
        saveLabel->setScale(0.3f);
        saveLabel->setID("save-label"_spr);
        saveMenu->addChild(saveLabel);
        menu->addChild(saveMenu);

        // load menu
        auto loadMenu = CCLayerRGBA::create();
        loadMenu->setID("load-menu"_spr);

        loadToggle = geode::Toggler::createWithStandardSprites(.5f, [this](geode::Toggler *sender, bool isToggled) {
            togglesValues.load = isToggled;
            this->setValue(togglesValues, sender);
        });
        loadToggle->setID("load-toggle"_spr);
        loadMenu->addChild(loadToggle);

        auto loadLabel = Label::create("load", "bigFont.fnt");
        loadLabel->setScale(0.3f);
        loadLabel->setID("load-label"_spr);
        loadMenu->addChild(loadLabel);
        menu->addChild(loadMenu);

        menu->setPositionX(menu->getPositionX() - 2.7f);
        menu->setContentHeight(30.f);
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
        menu->setLayout(RowLayout::create()
                            ->setAxisAlignment(AxisAlignment::End)
                            ->setGap(30.f));

        this->updateState(nullptr);

        // set toggle ui state
        for (auto value : {
                 std::make_pair(this->getValue().saveToggleEnabled, saveMenu),
                 std::make_pair(this->getValue().loadToggleEnabled, loadMenu),
             })
        {
            if(auto toggle = value.second->getChildByType<geode::Toggler>())
                toggle->setEnabled(value.first);
            value.second->setCascadeColorEnabled(true);
            value.second->setCascadeOpacityEnabled(true);
            value.second->setOpacity(value.first ? 255 : 100);
            value.second->setColor(value.first ? ccWHITE : ccGRAY);
        }

        return true;
    }

    void updateState(CCNode *invoker) override
    {
        SettingValueNodeV3::updateState(invoker);

        togglesValues = this->getValue();
        saveToggle->setToggled(togglesValues.save, false);
        loadToggle->setToggled(togglesValues.load, false);
    }

  public:
    static GDriveSaveTogglesNode *create(std::shared_ptr<GDriveSaveToggles> setting, float width)
    {
        auto ret = new GDriveSaveTogglesNode();
        if (ret && ret->init(setting, width))
        {
            ret->autorelease();
            return ret;
        }
        CC_SAFE_DELETE(ret);
        return nullptr;
    }
};

inline SettingNodeV3 *GDriveSaveToggles::createNode(float width)
{
    return GDriveSaveTogglesNode::create(std::static_pointer_cast<GDriveSaveToggles>(shared_from_this()), width);
}