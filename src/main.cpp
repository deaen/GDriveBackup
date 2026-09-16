#include "GDrivePopup.hpp"
#include <Geode/Geode.hpp>
#include <Geode/modify/AccountLayer.hpp>
#include <Geode/modify/MenuLayer.hpp>

using namespace geode::prelude;

struct GDriveMain
{
    void onButton(CCObject *)
    {
        GDrivePopup::create();
        /* Update popup */
        if (!Mod::get()->setSavedValue("shown-v1.3.0-popup", true) && !Mod::get()->getSavedValue<bool>("new-user", false))
        {
            FLAlertLayer::create(
                "update v1.3.0",
                "new update!! check it out:\n- <cy>New Edit mode!</c>\n- <cy> New size information menu!</c>\n- <cy>New drive icon option!</c>\n- <cy>Lots of bug fixes & optimizations!</c>\n thank you for using this mod! :D",
                "Okay")
                ->show();
        }
    }
};

class $modify(MenuLayer)
{

    $override bool init()
    {
        if (!MenuLayer::init())
            return false;

        if (!Mod::get()->getSettingValue<bool>("bottom-button"))
            return true;

        /* Main Menu GDrive Button */
        if (auto menu = this->getChildByID("bottom-menu"))
        {
            auto gdriveButton = CCMenuItemSpriteExtra::create(CircleButtonSprite::createWithSprite((Mod::get()->getSettingValue<bool>("new-icon")) ? "iconNew.png"_spr : "icon.png"_spr, 1.f, CircleBaseColor::Green, CircleBaseSize::MediumAlt), this, menu_selector(GDriveMain::onButton));
            gdriveButton->setID("gdrive-bottom-button"_spr);
            menu->addChild(gdriveButton);
            menu->updateLayout();
        }

        return true;
    }
};

class $modify(AccountLayer)
{

    $override void customSetup()
    {
        AccountLayer::customSetup();

        /* Account layer GDrive button */

        if (auto menu = CCMenu::create())
        {
            auto gdriveButton = CCMenuItemSpriteExtra::create(CircleButtonSprite::createWithSprite((Mod::get()->getSettingValue<bool>("new-icon")) ? "iconNew.png"_spr : "icon.png"_spr, 1.f, CircleBaseColor::Pink, CircleBaseSize::BigAlt), this, menu_selector(GDriveMain::onButton));
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
                {
                    menu->setPosition({m_listLayer->getPositionX(), m_listLayer->getPositionY()});
                }
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
                else if (pos == "bottom")
                {
                    menu->setPositionY(m_listLayer->getPositionY());
                }
            }
            if (m_mainLayer)
                m_mainLayer->addChild(menu);
        }
    }
};