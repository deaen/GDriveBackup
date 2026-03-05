#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>

#include "GDrivePopup.hpp"

using namespace geode::prelude;

class $modify(MyMenuLayer, MenuLayer)
{

    bool init()
    {

        if (!MenuLayer::init())
        {
            return false;
        }
        auto myButton = CCMenuItemSpriteExtra::create(CCSprite::createWithSpriteFrameName("GJ_likeBtn_001.png"), this,
                                                      menu_selector(MyMenuLayer::onMyButton));
        auto menu = this->getChildByID("bottom-menu");
        menu->addChild(myButton);
        myButton->setID("my-button"_spr);
        menu->updateLayout();

        return true;
    }
    void onMyButton(CCObject *)
    {
        GDrivePopup::create()->show();
    }
};