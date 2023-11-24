/*******************************************************************************
Pandora GS - PSEmu-compatible GPU driver
Copyright (C) 2023  Romain Vinders

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation, version 2 of the License.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details (LICENSE file).
*******************************************************************************/
#include <cassert>
#include "menu/pages/profile_selector.h"

using namespace video_api;
using namespace display;
using namespace display::controls;
using namespace menu::controls;
using namespace menu::pages;
using namespace menu;


// -- helpers -- ---------------------------------------------------------------

struct TileGrid final {
  uint32_t gridWidth;
  uint32_t rowCount;
  uint32_t lineCount;
  uint32_t tileWidth;
  uint32_t tileHeight;
};

static TileGrid computeTileGrid(uint32_t width, uint32_t height, uint32_t fontHeight, uint32_t profileCount) {
  TileGrid grid{};
  const uint32_t tileContentHeight = Control::tileContentHeight(fontHeight);
  const uint32_t visibleTileLines = height / (tileContentHeight + (Control::tilePaddingY() << 1) + 1u);

  constexpr const uint32_t maxWidth = (Control::tileContentWidth() + (Control::maxTilePaddingX() << 1))*8u + 7u;
  if (width > maxWidth)
    width = maxWidth;

  grid.rowCount = width / (Control::tileContentWidth() + (Control::minTilePaddingX() << 1) + 1u);
  if (grid.rowCount > profileCount) {
    if (profileCount < 2u)
      profileCount = 2u;
    grid.rowCount = profileCount;
  }
  else {
    const uint32_t idealTileLines = (visibleTileLines*3u) >> 2;
    while (profileCount <= (grid.rowCount - 1u)*idealTileLines && grid.rowCount > 3u)
      --(grid.rowCount);
  }
  grid.lineCount = (profileCount + grid.rowCount - 1u) / grid.rowCount;

  uint32_t doublePaddingX = (width + 1u - grid.rowCount*(Control::tileContentWidth()+1u)) / grid.rowCount;
  if (doublePaddingX > (Control::maxTilePaddingX() << 1))
    doublePaddingX = (Control::maxTilePaddingX() << 1);
  grid.tileWidth = Control::tileContentWidth() + doublePaddingX;

  grid.tileHeight = tileContentHeight + (Control::tilePaddingY() << 1);

  grid.gridWidth = grid.rowCount*(grid.tileWidth + 1u) - 1u;
  return grid;
}


// -- page -- ------------------------------------------------------------------

#define CREATE_PROFILE_ID 1
#define EDIT_PROFILE_ID   2
#define DELETE_PROFILE_ID 3

void ProfileSelector::init(const MessageResources& localizedText, int32_t x, int32_t y, uint32_t width, uint32_t height) {
  const int32_t marginX = Control::fieldsetMarginX(width);
  const int32_t controlX = x + marginX;
  title = TextMesh(context->renderer(), context->getFont(FontType::titles), localizedText.getMessage(ProfileSelectorMessages::title),
                   context->pixelSizeX(), context->pixelSizeY(), controlX, y + Control::titleMarginTop(), TextAlignment::left);
  int32_t currentLineY = title.y() + (int32_t)title.height() + (int32_t)Control::pageLineHeight();

  // create tiles
  auto grid = computeTileGrid(width - ((uint32_t)marginX << 1), height - (uint32_t)(currentLineY - y) - buttonBarHeight(),
                              context->getFont(FontType::labels).XHeight(), (uint32_t)(profiles->size()));
  std::vector<ControlRegistration> registry;
  registry.reserve(profiles->size() + 3u);
  profileTiles.reserve(profiles->size());
  auto tileActionHandler = [this](uint32_t id, TileAction action) { this->onTileAction(id, action); };
  
  bool isActiveProfileFound = false;
  uint32_t row = 0;
  int32_t currentRowX = controlX;
  for (const auto& profile : *profiles) {
    profileTiles.emplace_back(*context, profile.name.get(), currentRowX, currentLineY, grid.tileWidth,
                              theme->tileColor(profile.color), profile.id, tileActionHandler, true);
    registry.emplace_back(profileTiles.back(), true);
    if (profile.id == activeProfileId)
      isActiveProfileFound = true;

    if (++row >= grid.rowCount) {
      row = 0;
      currentRowX = controlX;
      currentLineY += (int32_t)grid.tileHeight + 1;
    }
    else
      currentRowX += (int32_t)grid.tileWidth + 1;
  }
  if (row != 0)
    currentLineY += (int32_t)grid.tileHeight + 1;
  if (!isActiveProfileFound)
    activeProfileId = (!profileTiles.empty()) ? profileTiles[0].id() : 0;

  // create action buttons
  ButtonStyleProperties buttonStyle(ButtonStyle::fromTopLeft, FontType::inputText, ControlIconType::none,
                                    theme->buttonControlColor(), theme->buttonBorderColor(), 0, 0,
                                    Control::buttonPaddingX(), Control::comboBoxPaddingY());
  int32_t buttonLabelY = y + (int32_t)height - (int32_t)buttonBarHeight() + (int32_t)Control::comboBoxPaddingY() + 4u;
  if (buttonLabelY > currentLineY + (int32_t)Control::controlSideMargin())
    buttonLabelY = currentLineY + (int32_t)Control::controlSideMargin();
  auto buttonActionHandler = std::bind(&ProfileSelector::onButtonAction,this,std::placeholders::_1);
  isDeleteEnabled = (profileTiles.size() > (size_t)1u);

  buttonStyle.icon = ControlIconType::add;
  createProfile = Button(*context, localizedText.getMessage(ProfileSelectorMessages::createProfile),
                         controlX, buttonLabelY, buttonStyle, CREATE_PROFILE_ID, buttonActionHandler);
  buttonStyle.icon = ControlIconType::remove;
  buttonStyle.style = ButtonStyle::fromBottomLeft;
  deleteProfile = Button(*context, localizedText.getMessage(ProfileSelectorMessages::deleteProfile),
                         controlX, buttonLabelY, buttonStyle, DELETE_PROFILE_ID, buttonActionHandler, &isDeleteEnabled);
  deleteProfile.move(*context, controlX + (int32_t)grid.gridWidth - (int32_t)deleteProfile.width(), buttonLabelY);
  buttonStyle.icon = ControlIconType::edit;
  editProfile = Button(*context, localizedText.getMessage(ProfileSelectorMessages::editProfile),
                       controlX, buttonLabelY, buttonStyle, EDIT_PROFILE_ID, buttonActionHandler);
  editProfile.move(*context, deleteProfile.x() - (int32_t)editProfile.width() - (int32_t)Control::controlSideMargin(), buttonLabelY);

  registry.emplace_back(createProfile, false);
  registry.emplace_back(editProfile, false);
  registry.emplace_back(deleteProfile, false);
  currentLineY += (int32_t)buttonBarHeight();

  // create controller action info
  selectProfileControllerInfo = Label(*context, localizedText.getMessage(CommonMessages::select),
                                      controlX, buttonLabelY, TextAlignment::left, ControlIconType::buttonCross);
  createProfileControllerInfo = Label(*context, localizedText.getMessage(CommonMessages::create),
                                      selectProfileControllerInfo.x() + (int32_t)selectProfileControllerInfo.width()
                                      + (int32_t)Control::controlSideMargin() + (int32_t)Control::buttonPaddingX(),
                                      buttonLabelY, TextAlignment::left, ControlIconType::buttonSquare);
  deleteProfileControllerInfo = Label(*context, localizedText.getMessage(ProfileSelectorMessages::deleteProfile),
                                      controlX + (int32_t)grid.gridWidth, buttonLabelY, TextAlignment::right, ControlIconType::buttonCircle);
  editProfileControllerInfo = Label(*context, localizedText.getMessage(ProfileSelectorMessages::editProfile),
                                    deleteProfileControllerInfo.x() - (int32_t)Control::controlSideMargin() - (int32_t)Control::buttonPaddingX(),
                                    buttonLabelY, TextAlignment::right, ControlIconType::buttonTriangle);

  // control registry
  Page::moveScrollbarThumb(currentLineY);
  registerControls(std::move(registry));

  // confirmation popup
  const char16_t* popupButtons[]{ localizedText.getMessage(CommonMessages::ok),
                                  localizedText.getMessage(CommonMessages::cancel) };
  confirmationPopup = Popup(*context, *theme, localizedText.getMessage(ProfileSelectorMessages::removePopupTitle),
                            localizedText.getMessage(ProfileSelectorMessages::removePopupMessage),
                            nullptr, popupButtons, sizeof(popupButtons)/sizeof(*popupButtons));
  confirmationPopup.close();
}

ProfileSelector::~ProfileSelector() noexcept {
  profileTiles.clear();

  createProfile.release();
  editProfile.release();
  deleteProfile.release();

  selectProfileControllerInfo.release();
  createProfileControllerInfo.release();
  editProfileControllerInfo.release();
  deleteProfileControllerInfo.release();
}


// -- window events -- ---------------------------------------------------------

void ProfileSelector::move(int32_t x, int32_t y, uint32_t width, uint32_t height) {
  Page::moveBase(x, y, width, height);

  const int32_t marginX = Control::fieldsetMarginX(width);
  const int32_t controlX = x + marginX;
  title.move(context->renderer(), context->pixelSizeX(), context->pixelSizeY(), controlX, y + Control::titleMarginTop());
  int32_t currentLineY = title.y() + (int32_t)title.height() + (int32_t)Control::pageLineHeight();

  // tiles
  auto grid = computeTileGrid(width - ((uint32_t)marginX << 1), height - (uint32_t)(currentLineY - y) - buttonBarHeight(),
                              context->getFont(FontType::labels).XHeight(), (uint32_t)(profiles->size()));
  std::vector<ControlRegistration> registry;
  registry.reserve(profiles->size() + 3u);

  uint32_t row = 0, line = 0;
  int32_t currentRowX = controlX;
  for (auto& tile : profileTiles) {
    tile.move(*context, currentRowX, currentLineY, grid.tileWidth);
    registry.emplace_back(tile, true);

    if (++row >= grid.rowCount) {
      row = 0;
      ++line;
      currentRowX = controlX;
      currentLineY += (int32_t)grid.tileHeight + 1;
    }
    else
      currentRowX += (int32_t)grid.tileWidth + 1;
  }
  if (row != 0)
    currentLineY += (int32_t)grid.tileHeight + 1;

  // action buttons
  int32_t buttonLabelY = y + (int32_t)height - (int32_t)buttonBarHeight() + (int32_t)Control::comboBoxPaddingY() + 4u;
  if (buttonLabelY > currentLineY + (int32_t)Control::controlSideMargin())
    buttonLabelY = currentLineY + (int32_t)Control::controlSideMargin();
  createProfile.move(*context, controlX, buttonLabelY);
  deleteProfile.move(*context, controlX + (int32_t)grid.gridWidth - (int32_t)deleteProfile.width(), buttonLabelY);
  editProfile.move(*context, deleteProfile.x() - (int32_t)editProfile.width() - (int32_t)Control::controlSideMargin(), buttonLabelY);
  registry.emplace_back(createProfile, false);
  registry.emplace_back(editProfile, false);
  registry.emplace_back(deleteProfile, false);
  currentLineY += (int32_t)buttonBarHeight();

  selectProfileControllerInfo.move(*context, controlX, buttonLabelY);
  createProfileControllerInfo.move(*context, selectProfileControllerInfo.x() + (int32_t)selectProfileControllerInfo.width()
                                   + (int32_t)Control::controlSideMargin() + (int32_t)Control::buttonPaddingX(), buttonLabelY);
  deleteProfileControllerInfo.move(*context, controlX + (int32_t)grid.gridWidth - (int32_t)deleteProfileControllerInfo.width(), buttonLabelY);
  editProfileControllerInfo.move(*context, deleteProfileControllerInfo.x() - editProfileControllerInfo.width()
                                 - (int32_t)Control::controlSideMargin() - (int32_t)Control::buttonPaddingX(), buttonLabelY);

  Page::moveScrollbarThumb(currentLineY); // required after a move
  registerControls(std::move(registry));
  confirmationPopup.move(*context);
}

// ---

uint32_t ProfileSelector::getProfileIndex(uint32_t id) const noexcept {
  uint32_t index = 0;
  for (const auto& profile : profileTiles) {
    if (profile.id() == id)
      break;
    ++index;
  }
  return (index < (uint32_t)profileTiles.size()) ? index : notFound();
}

void ProfileSelector::onButtonAction(uint32_t id) {
  switch (id) {
    case CREATE_PROFILE_ID: break;
    case EDIT_PROFILE_ID: break;
    case DELETE_PROFILE_ID: {
      if (profileTiles.size() > (size_t)1u) {
        setActivePopup(confirmationPopup, [this](uint32_t action) {
          if (action == 0)
            this->onProfileRemoved(this->activeProfileId);
        });
      }
      break;
    }
    default: assert(false); break;
  }
}

void ProfileSelector::onTileAction(uint32_t id, TileAction type) {
  switch (type) {
    case TileAction::select:
      activeProfileId = id;
      break;
    case TileAction::edit:
      break;
    case TileAction::remove: {
      if (profileTiles.size() > (size_t)1u) {
        setActivePopup(confirmationPopup, [this, id](uint32_t action) {
          if (action == 0)
            this->onProfileRemoved(id);
        });
      }
      break;
    }
    default: break;
  }
}

void ProfileSelector::onProfileRemoved(uint32_t id) {
  auto index = getProfileIndex(id);
  if (index == notFound())
    return;

  registerControls(std::vector<ControlRegistration>{}); // unregister before removal
  profiles->erase(profiles->begin() + (intptr_t)index);
  profileTiles.erase(profileTiles.begin() + (intptr_t)index);
  if (activeProfileId == id)
    activeProfileId = profileTiles[(index != 0u) ? (index - 1u) : 0u].id();

  isDeleteEnabled = (profileTiles.size() > (size_t)1u);
  move(x(), y(), width(), height());
}


// -- rendering -- -------------------------------------------------------------

void ProfileSelector::drawIcons() {
  if (!buffers->isFixedLocationBuffer())
    buffers->bindFixedLocationBuffer(context->renderer(), ScissorRectangle(x(), y(), width(), height()));

  auto* hoverControl = getActiveControl();
  if (isControllerUsed()) {
    selectProfileControllerInfo.drawIcon(*context, *buffers);
    createProfileControllerInfo.drawIcon(*context, *buffers);
    if (hoverControl != nullptr) {
      editProfileControllerInfo.drawIcon(*context, *buffers);
      deleteProfileControllerInfo.drawIcon(*context, *buffers);
    }
  }
  else {
    createProfile.drawIcon(*context, *buffers, (hoverControl == &createProfile));
    editProfile.drawIcon(*context, *buffers, (hoverControl == &editProfile));
    deleteProfile.drawIcon(*context, *buffers, (hoverControl == &deleteProfile));
  }
}

void ProfileSelector::drawPageBackgrounds(int32_t mouseX, int32_t mouseY) {
  // scrollable geometry
  if (buffers->isFixedLocationBuffer())
    buffers->bindScrollLocationBuffer(context->renderer(), ScissorRectangle(x(), y(), width(), contentHeight() - buttonBarHeight()));

  auto* hoverControl = getActiveControl();
  for (auto& tile : profileTiles)
    tile.drawBackground(*context, mouseX, mouseY, *buffers, (activeProfileId == tile.id()), (hoverControl == &tile));

  if (!isControllerUsed()) {
    buffers->bindFixedLocationBuffer(context->renderer(), ScissorRectangle(x(), y(), width(), height()));
    createProfile.drawBackground(*context, *buffers, (hoverControl == &createProfile), isMouseDown());
    editProfile.drawBackground(*context, *buffers, (hoverControl == &editProfile), isMouseDown());
    deleteProfile.drawBackground(*context, *buffers, (hoverControl == &deleteProfile), isMouseDown());
  }
}

void ProfileSelector::drawPageLabels() {
  // scrollable geometry
  auto& renderer = context->renderer();
  if (buffers->isFixedLocationBuffer())
    buffers->bindScrollLocationBuffer(renderer, ScissorRectangle(x(), y(), width(), contentHeight() - buttonBarHeight()));

  buffers->bindLabelBuffer(renderer, LabelBufferType::title);
  title.draw(renderer);

  auto* hoverControl = getActiveControl();
  for (auto& tile : profileTiles)
    tile.drawLabel(*context, *buffers, (activeProfileId == tile.id()), (hoverControl == &tile));

  buffers->bindFixedLocationBuffer(context->renderer(), ScissorRectangle(x(), y(), width(), height()));
  if (isControllerUsed()) {
    selectProfileControllerInfo.drawLabel(*context, *buffers);
    createProfileControllerInfo.drawLabel(*context, *buffers);
    if (hoverControl != nullptr) {
      editProfileControllerInfo.drawLabel(*context, *buffers);
      deleteProfileControllerInfo.drawLabel(*context, *buffers);
    }
  }
  else {
    createProfile.drawLabel(*context, *buffers, (hoverControl == &createProfile));
    editProfile.drawLabel(*context, *buffers, (hoverControl == &editProfile));
    deleteProfile.drawLabel(*context, *buffers, (hoverControl == &deleteProfile));
  }
}
