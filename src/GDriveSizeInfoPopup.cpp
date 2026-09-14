#include "GDriveSizeInfoPopup.hpp"

GDriveSizeInfoPopup *GDriveSizeInfoPopup::create(sizedata_map &sizeDataMap)
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

bool GDriveSizeInfoPopup::init(sizedata_map &sizeDataMap)
{
    if (!Popup::init(tableWidth + 30.f, tableHeight + 75.f))
        return false;

    this->setID("gdrive-size-info-popup"_spr);

    /* Popup Title */
    auto popupTitle = CCLabelBMFont::create("Size Information Breakdown", "bigFont.fnt");
    popupTitle->setID("popup-title"_spr);
    popupTitle->setScale((tableWidth / 1.7f) / popupTitle->getContentWidth());
    this->m_mainLayer->addChildAtPosition(popupTitle, Anchor::Top, {0, -16.f});

    /* Table Background */ 
    auto tableBackground = CCLayerColor::create({98, 51, 25, 255}, tableWidth, tableHeight);
    tableBackground->setID("table-background"_spr);

    /* Table Container */
    auto tableContainer = ScrollLayer::create({tableWidth, tableHeight}, true, true);
    tableContainer->setID("table-container"_spr);

    /* Table Entries */
    tableContainer->m_contentLayer->addChild(createTableRow("Account", "Slot", "Size", rowColor::DARKER));

    bool darkColor = false;
    float grandTotal = .0f;

    for (const auto& [userName, slotMap] : sizeDataMap)
    {
        bool doneDidUser = false;
        bool shouldProfileButton = false;
        float userTotal = .0f;
        size_t slotCount = 0;

        for (const auto& [slotName, slotSize] : slotMap)
        {
            ++slotCount;

            if (slotName == "account id")
                continue;

            std::string_view name;
            if (doneDidUser)
                name = "";
            else
            {
                name = userName;
                shouldProfileButton = true;
                doneDidUser = true;
            }
            auto size = utils::numFromString<float>(slotSize).unwrapOrDefault() / (1024.f * 1024.f);

            grandTotal += size;
            userTotal += size;

            if (slotCount == (slotMap.size() - 1)  && slotMap.size() > 2)
                tableContainer->m_contentLayer->addChild(createTableRow(fmt::format("Total: {:.2f} MB", userTotal), slotName, fmt::format("{:.2f} MB", size), (darkColor) ? rowColor::DARK : rowColor::INVISIBLE, true));

            else
                tableContainer->m_contentLayer->addChild(createTableRow(name.data(), slotName, fmt::format("{:.2f} MB", size), (darkColor) ? rowColor::DARK : rowColor::INVISIBLE, false, true, false, false, shouldProfileButton, sizeDataMap[userName]["account id"]));

            if (shouldProfileButton)
                shouldProfileButton = false;
        }

        darkColor = !darkColor;
    }

    /* Table Background & Container Setup */
    tableContainer->m_contentLayer->setLayout(ScrollLayer::createDefaultListLayout(0.f));
    tableContainer->scrollToTop();

    CCPoint tableOffset = {(tableWidth / -2), (tableHeight / -2)  + 5.f};
    this->m_mainLayer->addChildAtPosition(tableBackground, Anchor::Center, tableOffset);
    this->m_mainLayer->addChildAtPosition(tableContainer, Anchor::Center, tableOffset);

    /* Sticky Grand Row */
    auto grandRow = createTableRow("Grand Total:", "", fmt::format("{:.3f} MB", grandTotal), rowColor::LIGHT, false, false, false, true, false, "", 25.f, true);
    this->m_mainLayer->addChildAtPosition(grandRow, Anchor::Center, {tableOffset.x, tableOffset.y - grandRow->getContentHeight()});

    /* Scrollbar */
    auto scrollbar = Scrollbar::create(tableContainer);
    scrollbar->setID("table-scrollbar"_spr);
    scrollbar->getTrack()->setOpacity(0);
    scrollbar->getThumb()->setOpacity(170);
    scrollbar->setAnchorPoint({1.f, 0.5});
    this->m_mainLayer->addChildAtPosition(scrollbar, Anchor::Center, {tableContainer->getContentWidth() / 2.f, -scrollbar->getContentWidth()});
    
    /* Borders */
    auto topBorder = NineSlice::createWithSpriteFrameName("GJ_commentTop_001.png");
    topBorder->setInsetLeft(topBorder->getContentWidth() / 2);
    topBorder->setContentWidth(tableWidth + 11.f);
    topBorder->setID("top-border"_spr);
    this->m_mainLayer->addChildAtPosition(topBorder, Anchor::Center, {0, -tableOffset.y + 6.f});

    auto bottomBorder = NineSlice::createWithSpriteFrameName("GJ_commentTop_001.png");
    bottomBorder->setInsetLeft(bottomBorder->getContentWidth() / 2);
    bottomBorder->setContentWidth(tableWidth + 11.f);
    bottomBorder->setScaleY(-1.f);
    bottomBorder->setID("bottom-border"_spr);
    this->m_mainLayer->addChildAtPosition(bottomBorder, Anchor::Center, {0, tableOffset.y - grandRow->getContentHeight() + 4.f});

    auto rightBorder = NineSlice::createWithSpriteFrameName("GJ_commentSide_001.png");
    rightBorder->setContentHeight(tableHeight + 5.f);
    rightBorder->setScaleX(-1.f);
    rightBorder->setID("right-border"_spr);
    this->m_mainLayer->addChildAtPosition(rightBorder, Anchor::Center, {-tableOffset.x + 1.9f, -10.f});

    auto leftBorder = NineSlice::createWithSpriteFrameName("GJ_commentSide_001.png");
    leftBorder->setContentHeight(tableHeight + 5.f);
    leftBorder->setID("left-border"_spr);
    this->m_mainLayer->addChildAtPosition(leftBorder, Anchor::Center, {tableOffset.x - 1.9f, -10.f});

    return true;
}

cocos2d::CCNode *GDriveSizeInfoPopup::createTableRow(const std::string_view firstColumnLabel, const std::string_view secondColumnLabel, const std::string_view thirdColumnLabel, const rowColor color, const bool firstColumnGreen, const bool firstColumnGolden, const bool secondColumnGolden, const bool thirdColumnGolden, const bool profileButton, const std::string accountID, const GLfloat rowHeight, const bool noGaps)
{
    /* Setup vars & create the row */
    auto tableRow = CCNode::create();
    tableRow->setID("table-row"_spr);

    const GLfloat columnWidth = tableWidth / ((noGaps) ? 3.f : 3.05f);
    ccColor4B colorCode;
    switch (color)
    {
    case INVISIBLE:
        colorCode = {0, 0, 0, 0};
        break;
    case LIGHT:
        colorCode = {129, 64, 32, 255};
        break;
    case DARK:
        colorCode = {79, 41, 20, 255};
        break;
    case DARKER:
        colorCode = {59, 32, 15, 255};
        break;
    }

    tableRow->setContentSize({tableWidth, rowHeight});

    /* create the column backgrounds & the containers */
    auto firstColumnBackground = CCLayerColor::create(colorCode, columnWidth, rowHeight);
    firstColumnBackground->setID("first-column-background"_spr);
    tableRow->addChild(firstColumnBackground);

    auto secondColumnBackground = CCLayerColor::create(colorCode, columnWidth, rowHeight);
    secondColumnBackground->setID("second-column-background"_spr);
    tableRow->addChild(secondColumnBackground);

    auto thirdColumnBackground = CCLayerColor::create(colorCode, columnWidth, rowHeight);
    thirdColumnBackground->setID("third-column-background"_spr);
    tableRow->addChild(thirdColumnBackground);

    auto firstColumn = CCMenu::create();
    firstColumn->setID("first-column"_spr);
    firstColumn->setContentSize({columnWidth, rowHeight});
    firstColumn->setAnchorPoint({0.5, 0.5});
    firstColumnBackground->addChildAtPosition(firstColumn, Anchor::Center);

    auto secondColumn = CCNode::create();
    secondColumn->setID("second-column"_spr);
    secondColumn->setContentSize({columnWidth, rowHeight});
    secondColumn->setAnchorPoint({0.5, 0.5});
    secondColumnBackground->addChildAtPosition(secondColumn, Anchor::Center);

    auto thirdColumn = CCNode::create();
    thirdColumn->setID("third-column"_spr);
    thirdColumn->setContentSize({columnWidth, rowHeight});
    thirdColumn->setAnchorPoint({0.5, 0.5});
    thirdColumnBackground->addChildAtPosition(thirdColumn, Anchor::Center);

    /* Profile Button */
    if (profileButton && !accountID.empty())
    {
        auto profileButtonSpr = CCSprite::createWithSpriteFrameName("GJ_profileButton_001.png");
        auto profileBtn = CCMenuItemExt::createSpriteExtra(profileButtonSpr, [accountID](CCMenuItemSpriteExtra *) {
            auto res = numFromString<int>(accountID.c_str());

            if (res.isErr())
                FLAlertLayer::create("Profile error", "<cr>Couldn't find this account</c>\n...you shouldn't be seeing this, actually.", "ok?")->show();
            else
                ProfilePage::create(res.unwrap(), false)->show();
        });
        profileBtn->setID("account-button"_spr);
        profileBtn->setAnchorPoint({0.5f, 0.4f});
        profileBtn->setLayoutOptions(SimpleAxisLayoutOptions::create()->setMinRelativeScale(0.3f)->setScalingPriority(ScalingPriority::Never));
        firstColumn->addChild(profileBtn);
    }

    /* Column Labels */
    auto firstLabel = CCLabelBMFont::create(firstColumnLabel.data(), (firstColumnGolden) ? "goldFont.fnt" : "bigFont.fnt");
    firstLabel->setID("first-column-label"_spr);
    if (firstColumnGreen)
        firstLabel->setColor({53, 229, 92});
    firstColumn->addChild(firstLabel);

    auto secondLabel = CCLabelBMFont::create(secondColumnLabel.data(), (secondColumnGolden) ? "goldFont.fnt" : "bigFont.fnt");
    secondLabel->setID("second-column-label"_spr);
    secondColumn->addChild(secondLabel);

    auto thirdLabel = CCLabelBMFont::create(thirdColumnLabel.data(), (thirdColumnGolden) ? "goldFont.fnt" : "bigFont.fnt");
    thirdLabel->setID("third-column-label"_spr);
    thirdColumn->addChild(thirdLabel);

    auto columnLayout = SimpleRowLayout::create()
                            ->setPadding({5.f, 5.f, 5.f, 5.f})
                            ->setMainAxisAlignment(MainAxisAlignment::Start)
                            ->setCrossAxisAlignment(CrossAxisAlignment::Center)
                            ->setMainAxisScaling(AxisScaling::Scale)
                            ->setCrossAxisScaling(AxisScaling::Scale)
                            ->setMinRelativeScale(0.f)
                            ->setGap(5.f);
    firstColumn->setLayout(columnLayout);
    secondColumn->setLayout(columnLayout);
    thirdColumn->setLayout(columnLayout);
    tableRow->setLayout(SimpleRowLayout::create()->setMainAxisAlignment(MainAxisAlignment::Start)->setGap((noGaps) ? 0.f : 3.f));

    return tableRow;
}