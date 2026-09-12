#include "GDriveSizeInfoPopup.hpp"

GDriveSizeInfoPopup *GDriveSizeInfoPopup::create(const sizedata_map &sizeDataMap)
{
    auto ret = new GDriveSizeInfoPopup();
    if (ret->init(sizeDataMap))
    {
        ret->autorelease();
        ret->show();
        return ret;
    }

    delete ret;
    return nullptr;
}

bool GDriveSizeInfoPopup::init(const sizedata_map &sizeDataMap)
{

    if (!Popup::init(tableWidth + 20.f, tableHeight + 50.f))
        return false;

    this->setID("gdrive-size-info-popup"_spr);
    this->setTitle("size information breakdown");

    ScrollLayer *tableContainer = ScrollLayer::create({tableWidth, tableHeight}, true, true);
    tableContainer->setID("table-container"_spr);

    tableContainer->m_contentLayer->addChild(createTableRow("Account", "", "Slot", "Size", true, true, true));

    bool darkColor = false;
    float grandTotal = .0f;

    for (auto &key : sizeDataMap)
    {
        bool doneDidUser = false;
        float userTotal = .0f;

        std::string accountID;
        if (key.second.contains("account id"))
            accountID = key.second.at("account id");

        for (auto &obj : key.second)
        {
            if (obj.first == "account id")
                continue;

            std::string_view name;
            if (doneDidUser)
                name = "";
            else
            {
                name = key.first;
                doneDidUser = true;
            }

            auto size = utils::numFromString<float>(obj.second).unwrapOrDefault() / (1024.f * 1024.f);
            tableContainer->m_contentLayer->addChild(createTableRow(name.data(), accountID, obj.first, fmt::format("{:.2f} MB", size), darkColor));
            grandTotal += size;
            userTotal += size;
        }

        tableContainer->m_contentLayer->addChild(createTableRow("Total", "","", fmt::format("{:.2f} MB", userTotal), darkColor, true, true));
        darkColor = !darkColor;
    }

    tableContainer->m_contentLayer->addChild(createTableRow("GRAND TOTAL", "","", fmt::format("{:.3f} MB", grandTotal), darkColor, true, true));

    tableContainer->m_contentLayer->setLayout(ScrollLayer::createDefaultListLayout(0.f));
    tableContainer->scrollToTop();
    this->m_mainLayer->addChildAtPosition(tableContainer, Anchor::Center, {tableWidth / -2, (tableHeight / -2) - 12.f});

    return true;
}

cocos2d::CCLayerColor *GDriveSizeInfoPopup::createTableRow(std::string buttonLabel, std::string accountID, std::string_view slot, std::string_view size, bool darkColor, bool goldFont, bool accountLabel)
{
    auto tableRow = CCLayerColor::create({123, 67, 40});
    tableRow->setID("table-row"_spr);
    tableRow->setContentSize({tableWidth, 25.f});

    if (darkColor)
        tableRow->setOpacity(255);

    constexpr GLfloat columnWidth = tableWidth / 3.f;
    constexpr GLfloat columnHeight = 25.f;
    constexpr float labelPercentage = 0.7f;

    auto accountColumn = CCMenu::create();
    accountColumn->setContentSize({columnWidth, columnHeight});
    accountColumn->setID("account-column"_spr);
    tableRow->addChild(accountColumn);

    auto slotColumn = CCNode::create();
    slotColumn->setContentSize({columnWidth, columnHeight});
    slotColumn->setID("slot-column"_spr);
    tableRow->addChild(slotColumn);

    auto sizeColumn = CCNode::create();
    sizeColumn->setContentSize({columnWidth, columnHeight});
    sizeColumn->setID("size-column"_spr);
    tableRow->addChild(sizeColumn);

    if (!buttonLabel.empty())
    {
        if (accountLabel || accountID == "0")
        {
            if (accountID == "0")
                buttonLabel = "Unregistered";

            auto label = CCLabelBMFont::create(buttonLabel.c_str(), "goldFont.fnt");
            label->setID("account-label"_spr);
            label->setScale(calculatePercentageScale(label->getContentSize(), accountColumn->getContentSize(), (buttonLabel != "GRAND TOTAL") ? labelPercentage : 1.f));
            label->setAnchorPoint({0, 0.4f});

            accountColumn->addChildAtPosition(label, Anchor::Left);
        }
        else
        {
            auto buttonBtnSpr = ButtonSprite::create(buttonLabel.c_str(), "goldFont.fnt", "GJ_button_01.png");
            auto label = CCMenuItemExt::createSpriteExtra(buttonBtnSpr, [accountID](CCMenuItemSpriteExtra *) {
                auto res = numFromString<int>(accountID);
                if (res.isErr())
                    FLAlertLayer::create("Profile error", "<cr>Couldn't find this account</c>\n...You shouldn't be seeing this, actually.", "ok?")->show();
                else
                    ProfilePage::create(res.unwrap(), false)->show();
            });
            label->setID("account-button"_spr);
            auto scale = calculatePercentageScale(label->getContentSize(), accountColumn->getContentSize(), labelPercentage);
            label->setScale(scale);
            label->m_baseScale = scale;
            label->setAnchorPoint({0, 0.4f});

            accountColumn->addChildAtPosition(label, Anchor::Left);
        }
    }

    if (!slot.empty())
    {
        auto label = CCLabelBMFont::create(slot.data(), (goldFont) ? "goldFont.fnt" : "bigFont.fnt");
        label->setID("slot-label"_spr);
        label->setScale(calculatePercentageScale(label->getContentSize(), slotColumn->getContentSize(), labelPercentage));
        label->setAnchorPoint({0, 0.4f});

        slotColumn->addChildAtPosition(label, Anchor::Left);
    }

    if (!size.empty())
    {
        auto label = CCLabelBMFont::create(size.data(), (goldFont) ? "goldFont.fnt" : "bigFont.fnt");
        label->setID("size-label"_spr);
        label->setScale(calculatePercentageScale(label->getContentSize(), sizeColumn->getContentSize(), labelPercentage));
        label->setAnchorPoint({0, 0.4f});

        sizeColumn->addChildAtPosition(label, Anchor::Left);
    }

    tableRow->setLayout(SimpleRowLayout::create()->setMainAxisAlignment(MainAxisAlignment::Start)->setPadding(Padding(2.f, 4.f, 2.f, 0.f)));

    return tableRow;
}
float GDriveSizeInfoPopup::calculatePercentageScale(const CCSize dividend, const CCSize divisor, const float percentage)
{
    return std::min((divisor.height * std::abs(percentage)) / dividend.height, (divisor.width * std::abs(percentage) / dividend.width));
}
