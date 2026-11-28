#include "GDriveManager.hpp"
#include "GDrivePopup.hpp"

#include <Geode/Geode.hpp>
#include <Geode/modify/AccountLayer.hpp>
#include <Geode/modify/MenuLayer.hpp>
using namespace geode::prelude;

struct GDriveMain
{
    void onButton(CCObject *)
    {
        if (!Mod::get()->getSavedValue<bool>("new-user", false))
            Mod::get()->setSavedValue("shown-v1.4.1-popup", true);
        GDrivePopup::create();
    }

    static void checkScene(CCNode *node, bool isLayer = false)
    {
        if (!node)
            return;

        CCLayer *layer = nullptr;
        if (isLayer)
            layer = typeinfo_cast<CCLayer *>(node);
        else if (auto scene = typeinfo_cast<CCTransitionScene *>(node); scene && scene->m_pInScene)
            layer = scene->m_pInScene->getChildByType<CCLayer *>();

        if (!layer)
            return;

        const std::string &id = layer->getID();
        if (id.empty() || id == " " || id == "LevelEditorLayer" || id == "PlayLayer" || id == "LevelInfoLayer" || id == "EditLevelLayer")
            return;

        // temp
        if (id != "SecretRewardsLayer")
            return;
         
        // skip menulayer if its from a transitionscene
        if (!isLayer && id == "MenuLayer")
            return;

        GDriveManager::getInstance()->checkForAutoBackup();
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

        GDriveMain::checkScene(this, true);
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

#include <Geode/modify/CCDirector.hpp>

class $modify(CCDirector)
{
    $override bool replaceScene(CCScene *pScene)
    {
        if (!CCDirector::replaceScene(pScene))
            return false;

        GDriveMain::checkScene(pScene);
        return true;
    };

    $override bool pushScene(CCScene *pScene)
    {
        if (!CCDirector::pushScene(pScene))
            return false;

        GDriveMain::checkScene(pScene);
        return true;
    };
};