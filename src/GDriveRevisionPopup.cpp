#include "GDriveRevisionPopup.hpp"

#include "GDriveManager.hpp"
GDriveRevisionPopup *GDriveRevisionPopup::create(const int slot)
{
    auto ret = new GDriveRevisionPopup();
    if (ret->init(slot))
    {
        ret->autorelease();
        ret->show();
        return ret;
    }

    delete ret;
    return nullptr;
}

bool GDriveRevisionPopup::init(const int slot)
{
    if (!Popup::init(tableWidth + 50.f, tableHeight + 50.f, "GJ_square04.png"))
        return false;

    this->setID("gdrive-revision-popup"_spr);
    this->setTitle(fmt::format("Slot {} History", slot));

    CCLayerColor *tableBackground = CCLayerColor::create({35, 14, 56, 255}, tableWidth, tableHeight);
    tableBackground->setID("table-background"_spr);

    ScrollLayer *tableContainer = ScrollLayer::create({tableWidth, tableHeight}, true, true);
    tableContainer->setID("table-container"_spr);


    tableContainer->m_contentLayer->setLayout(ScrollLayer::createDefaultListLayout(0.f));
    tableContainer->scrollToTop();

    this->m_mainLayer->addChildAtPosition(tableBackground, Anchor::Center, {tableWidth / -2, (tableHeight / -2) - 12.f});
    this->m_mainLayer->addChildAtPosition(tableContainer, Anchor::Center, {tableWidth / -2, (tableHeight / -2) - 12.f});
    
    
    async::spawn(GDriveManager::getInstance()->getRevisionList(slot), [](){});
    
    bool darkColor = false;

    // for (auto &key : sizeData)
    // {
    //     bool doneDidUser = false;
    //     float userTotal = .0f;
    //     for (auto &obj : key.second)
    //     {
    //         std::string_view id;
    //         if (doneDidUser)
    //             id = "";
    //         else
    //         {
    //             id = key.first;
    //             doneDidUser = true;
    //         }

    //         tableContainer->m_contentLayer->addChild(createTableRow(id.data(), obj.first, fmt::format("{:.2f}MB", obj.second), darkColor));
    //         grandTotal += obj.second;
    //         userTotal += obj.second;
    //     }

    //     tableContainer->m_contentLayer->addChild(createTableRow("Total", "", fmt::format("{:.2f}MB", userTotal), darkColor, true, true));
    //     darkColor = !darkColor;
    // }

    return true;
}

cocos2d::CCLayerColor *GDriveRevisionPopup::createTableRow(std::string accountID, std::string_view slot, std::string_view size, bool darkColor, bool goldFont, bool accountLabel)
{
    auto tableRow = CCLayerColor::create({123, 67, 40});
    tableRow->setID("table-row"_spr);
    tableRow->setContentSize({tableWidth, 25.f});

    if (darkColor)
        tableRow->setOpacity(255);

    // constexpr GLfloat columnWidth = tableWidth / 3.f;
    // constexpr GLfloat columnHeight = 25.f;
    // constexpr float labelPercentage = 0.7f;

    // auto accountColumn = CCMenu::create();
    // accountColumn->setContentSize({columnWidth, columnHeight});
    // accountColumn->setID("account-column"_spr);
    // tableRow->addChild(accountColumn);

    // auto slotColumn = CCNode::create();
    // slotColumn->setContentSize({columnWidth, columnHeight});
    // slotColumn->setID("slot-column"_spr);
    // tableRow->addChild(slotColumn);

    // auto sizeColumn = CCNode::create();
    // sizeColumn->setContentSize({columnWidth, columnHeight});
    // sizeColumn->setID("size-column"_spr);
    // tableRow->addChild(sizeColumn);

    // if (!accountID.empty())
    // {
    //     if (accountLabel || accountID == "0")
    //     {
    //         if (accountID == "0")
    //             accountID = "Unregistered";

    //         auto label = CCLabelBMFont::create(accountID.data(), "goldFont.fnt");
    //         label->setID("account-label"_spr);
    //         label->setScale(calculatePercentageScale(label->getContentSize(), accountColumn->getContentSize(), (accountID != "GRAND TOTAL") ? labelPercentage : 1.f));
    //         label->setAnchorPoint({0, 0.4f});

    //         accountColumn->addChildAtPosition(label, Anchor::Left);
    //     }
    //     else
    //     {
    //         auto buttonBtnSpr = ButtonSprite::create(accountID.data(), "goldFont.fnt", "GJ_button_01.png");
    //         auto label = CCMenuItemExt::createSpriteExtra(buttonBtnSpr, [accountID](CCMenuItemSpriteExtra *) {
    //             auto res = numFromString<int>(accountID);
    //             if (res.isErr())
    //                 FLAlertLayer::create("Profile error", "<cr>Couldn't find this account</c>\n...You shouldn't be seeing this, actually.", "ok?")->show();
    //             else
    //                 ProfilePage::create(res.unwrap(), false)->show();
    //         });
    //         label->setID("account-button"_spr);
    //         auto scale = calculatePercentageScale(label->getContentSize(), accountColumn->getContentSize(), labelPercentage);
    //         label->setScale(scale);
    //         label->m_baseScale = scale;
    //         label->setAnchorPoint({0, 0.4f});

    //         accountColumn->addChildAtPosition(label, Anchor::Left);
    //     }
    // }

    // if (!slot.empty())
    // {
    //     auto label = CCLabelBMFont::create(slot.data(), (goldFont) ? "goldFont.fnt" : "bigFont.fnt");
    //     label->setID("slot-label"_spr);
    //     label->setScale(calculatePercentageScale(label->getContentSize(), slotColumn->getContentSize(), labelPercentage));
    //     label->setAnchorPoint({0, 0.4f});

    //     slotColumn->addChildAtPosition(label, Anchor::Left);
    // }

    // if (!size.empty())
    // {
    //     auto label = CCLabelBMFont::create(size.data(), (goldFont) ? "goldFont.fnt" : "bigFont.fnt");
    //     label->setID("size-label"_spr);
    //     label->setScale(calculatePercentageScale(label->getContentSize(), sizeColumn->getContentSize(), labelPercentage));
    //     label->setAnchorPoint({0, 0.4f});

    //     sizeColumn->addChildAtPosition(label, Anchor::Left);
    // }

    // tableRow->setLayout(SimpleRowLayout::create()->setMainAxisAlignment(MainAxisAlignment::Start)->setPadding(Padding(2.f, 4.f, 2.f, 0.f)));

    return tableRow;
}
