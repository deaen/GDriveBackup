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
            {
                auto pos = Mod::get()->getSettingValue<std::string>("button-position");
                if (pos == "bottom right")
                    menu->setPosition({m_listLayer->getPositionX() + m_listLayer->getContentWidth(), m_listLayer->getPositionY()});
                else if (pos == "center right")
                {
                    menu->setAnchorPoint({0.4f, 0.5f});
                    menu->setPositionX(m_listLayer->getPositionX() + m_listLayer->getContentWidth());
                }
                else if (pos == "top right")
                {
                    menu->setAnchorPoint({0.4f, 0.25f});
                    menu->setPosition({m_listLayer->getPositionX() + m_listLayer->getContentWidth(), m_listLayer->getPositionY() + m_listLayer->getContentHeight()});
                }
                else if (pos == "bottom left")
                    menu->setPosition({m_listLayer->getPositionX(), m_listLayer->getPositionY()});
                else if (pos == "center left")
                {
                    menu->setAnchorPoint({0.6f, 0.5f});
                    menu->setPositionX(m_listLayer->getPositionX());
                }
                else if (pos == "top left")
                {
                    menu->setAnchorPoint({0.6f, 0.25f});
                    menu->setPosition({m_listLayer->getPositionX(), m_listLayer->getPositionY() + m_listLayer->getContentHeight()});
                }
                // else if (pos == "top")
                // {
                //     menu->setAnchorPoint({0.5f, 0.250f});
                //     menu->setPositionY(m_listLayer->getPositionY() + m_listLayer->getContentHeight());
                // }
                else if (pos == "bottom")
                    menu->setPositionY(m_listLayer->getPositionY());
            }
            if (m_mainLayer)
                m_mainLayer->addChild(menu);
        }
    }
    void onGDriveButton(CCObject *)
    {
        GDrivePopup::create();
    }
};