#include <Geode/Geode.hpp>
using namespace geode::prelude;

#include <Geode/modify/AccountLayer.hpp>
#include <Geode/modify/MenuLayer.hpp>

#include "GDrivePopup.hpp"

class $modify(GDriveMenuLayer, MenuLayer)
{

    $override bool init()
    {
        if (!MenuLayer::init())
            return false;

        if (!Mod::get()->getSettingValue<bool>("bottom-button"))
            return true;

        if (auto menu = this->getChildByID("bottom-menu"))
        {
            auto gdriveButton = CCMenuItemSpriteExtra::create(CircleButtonSprite::createWithSprite("icon.png"_spr, 1.f, CircleBaseColor::Green, CircleBaseSize::MediumAlt), this, menu_selector(GDriveMenuLayer::onGDriveButton));
            gdriveButton->setID("gdrive-bottom-button"_spr);
            menu->addChild(gdriveButton);
            menu->updateLayout();
        }

        return true;
    }
    void onGDriveButton(CCObject *)
    {
        GDrivePopup::create();
    }
};

class $modify(GDriveAccountLayer, AccountLayer)
{

    $override void customSetup()
    {
        AccountLayer::customSetup();

        if (auto menu = CCMenu::create())
        {

            auto gdriveButton = CCMenuItemSpriteExtra::create(CircleButtonSprite::createWithSprite("icon.png"_spr, 1.f, CircleBaseColor::Pink, CircleBaseSize::BigAlt), this, menu_selector(GDriveAccountLayer::onGDriveButton));
            gdriveButton->setID("gdrive-button"_spr);

            menu->setContentSize(gdriveButton->getScaledContentSize());
            menu->setID("gdrive-button-menu"_spr);
            menu->addChildAtPosition(gdriveButton, Anchor::Center);

            if (m_listLayer)
                menu->setPosition({m_listLayer->getPositionX() + m_listLayer->getContentWidth(), m_listLayer->getPositionY()});
            if (m_mainLayer)
                m_mainLayer->addChild(menu);
        }
    }
    void onGDriveButton(CCObject *)
    {
        GDrivePopup::create();
    }
};