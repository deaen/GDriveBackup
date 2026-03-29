#include <Geode/Geode.hpp>
#include <Geode/binding/FLAlertLayer.hpp>
using namespace geode::prelude;
#include "GDrivePopup.hpp"
#include "GDriveEncrypt.hpp"
#include <Geode/modify/MenuLayer.hpp>

class $modify(MyMenuLayer, MenuLayer)
{

    bool init()
    {

        if (!MenuLayer::init())
        {
            return false;
        }

        auto myButton = CCMenuItemSpriteExtra::create(CircleButtonSprite::createWithSprite("icon.png"_spr), this,
                                                      menu_selector(MyMenuLayer::onMyButton));
        auto menu = this->getChildByID("bottom-menu");
        menu->addChild(myButton);
        myButton->setID("my-button"_spr);
        menu->updateLayout();

        return true;
    }
    void onMyButton(CCObject *)
    {
        // GDrivePopup::create();
        FLAlertLayer::create("hardware id", GDriveEncypt::create()->getHardwareID(), "i see")->show();
    }
};