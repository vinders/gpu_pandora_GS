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
#include <cstring>
#include <cmath>
#include "menu/pages/page_content_builder.h"
#include "menu/pages/main_menu.h"

using namespace video_api;
using namespace display;
using namespace display::controls;
using namespace menu::controls;
using namespace menu::pages;
using namespace menu;


// -- helpers -- ---------------------------------------------------------------

static constexpr const uint32_t buttonSpacingY = 14;
static constexpr const uint32_t buttonPaddingX = 30;
static constexpr const uint32_t buttonPaddingY = 14;
static constexpr const uint32_t buttonWidth = 270;
static constexpr const uint32_t activeButtonWidth = 290;

static constexpr const uint32_t fieldsetTitleHeight = (Control::fieldsetContentMarginBottom() << 2);
static constexpr const uint32_t tilePaddingX = Control::fieldsetContentMarginBottom();
static constexpr const uint32_t tileWidth = (Control::tileContentWidth() + (Control::maxTilePaddingX() << 1))*5u/6u;

struct MainMenuGrid final {
  int32_t controlX;
  int32_t offsetY;
  uint32_t titleHeight;
  uint32_t buttonHeight;
  int32_t controllerInfoY;
  uint32_t fieldsetWidth;
  int32_t recentProfilesGroupX;
};

static MainMenuGrid computeGrid(RendererContext& context, int32_t x, int32_t y, uint32_t width, uint32_t height) {
  MainMenuGrid grid;
  if (width >= 980u)
    grid.controlX = (x + (int32_t)width / 3 - (int32_t)buttonWidth/3);
  else if (width >= 780u)
    grid.controlX = (x + (int32_t)(width >> 2) - (int32_t)buttonWidth/4);
  else if (width >= 650u)
    grid.controlX = (x + (int32_t)(width >> 2) - (int32_t)buttonWidth/3);
  else
    grid.controlX = (x + (int32_t)(width >> 2) - (int32_t)buttonWidth/3 - (int32_t)buttonWidth/8);
  grid.titleHeight = context.getFont(FontType::titles).XHeight();
  grid.buttonHeight = context.getFont(FontType::labels).XHeight() + (buttonPaddingY << 1);

  grid.offsetY = ((int32_t)height - (int32_t)((grid.buttonHeight + buttonSpacingY)*5u)) / 2;
  if (grid.offsetY < (int32_t)Control::titleMarginTop() + (int32_t)Control::pageLineHeight() + (int32_t)grid.titleHeight)
    grid.offsetY = Control::titleMarginTop() + Control::pageLineHeight() + grid.titleHeight;
  else
    grid.offsetY -= (int32_t)(height >> 4);

  grid.controllerInfoY = y + (int32_t)height - (int32_t)Control::pageLineHeight() - 2;
  grid.fieldsetWidth = (int32_t)tileWidth + (int32_t)(tilePaddingX << 1);
  grid.recentProfilesGroupX = x + (int32_t)width - (grid.controlX - x) - (int32_t)grid.fieldsetWidth;
  if (grid.recentProfilesGroupX - grid.controlX - (int32_t)buttonWidth > (int32_t)(grid.fieldsetWidth >> 1))
    grid.recentProfilesGroupX = grid.controlX + (int32_t)(grid.fieldsetWidth >> 1) + (int32_t)buttonWidth;
  return grid;
}

static inline void fillSaveSlotOptions(ComboBoxOption* saveSlots, uint32_t saveSlotCount, const char16_t* slotNamePrefix) {
  char16_t slotNameBuffer[24]{};
  size_t slotNamePrefixSize = TextMesh::getStringLength(slotNamePrefix);
  if (slotNamePrefixSize > (size_t)21)
    slotNamePrefixSize = (size_t)21;

  memcpy(slotNameBuffer, slotNamePrefix, slotNamePrefixSize*sizeof(char16_t));
  slotNameBuffer[slotNamePrefixSize] = u' ';
  slotNameBuffer[++slotNamePrefixSize] = u'#';
  ++slotNamePrefixSize;

  for (uint32_t i = 0; i < saveSlotCount; ++i) {
    slotNameBuffer[slotNamePrefixSize] = u'1' + (char16_t)i;
    saveSlots[i] = ComboBoxOption(slotNameBuffer, (ComboValue)i);
  }
}


// -- page -- ------------------------------------------------------------------

#define RESUME_ID     1
#define LOAD_STATE_ID 2
#define SAVE_STATE_ID 3
#define RESET_GAME_ID 4
#define EXIT_GAME_ID  5

static constexpr const uint32_t saveSlotCount = 6;
static constexpr const uint32_t recentTileCount = 5;

void MainMenu::init(const MessageResources& localizedText, int32_t x, int32_t y, uint32_t width, uint32_t height) {
  auto grid = computeGrid(*context, x, y, width, height);

  // create main menu buttons
  const int32_t titleY = y + grid.offsetY - (int32_t)grid.titleHeight - (int32_t)buttonSpacingY;
  title = TextMesh(context->renderer(), context->getFont(FontType::titles), localizedText.getMessage(MainMenuMessages::title),
                   context->pixelSizeX(), context->pixelSizeY(), grid.controlX, titleY, TextAlignment::left);

  std::vector<ControlRegistration> registry;
  registry.reserve(11);

  auto buttonActionHandler = std::bind(&MainMenu::onButtonAction,this,std::placeholders::_1);
  ButtonStyleProperties buttonStyle(ButtonStyle::fromTopLeft, FontType::labels, ControlIconType::none, theme->buttonControlColor(),
                                    theme->buttonBorderColor(), 2, buttonWidth, buttonPaddingX, buttonPaddingY);
  int32_t currentLineY = y + grid.offsetY + (int32_t)buttonPaddingY;
  resume = Button(*context, localizedText.getMessage(MainMenuMessages::resume), grid.controlX, currentLineY, buttonStyle, RESUME_ID, buttonActionHandler);
  registry.emplace_back(resume, true, nullptr, 0, activeButtonWidth - buttonWidth);
  currentLineY += grid.buttonHeight + buttonSpacingY;
  loadState = Button(*context, localizedText.getMessage(MainMenuMessages::loadState), grid.controlX, currentLineY, buttonStyle, LOAD_STATE_ID, buttonActionHandler);
  registry.emplace_back(loadState, true, nullptr, 0, activeButtonWidth - buttonWidth);
  currentLineY += grid.buttonHeight + buttonSpacingY;
  saveState = Button(*context, localizedText.getMessage(MainMenuMessages::saveState), grid.controlX, currentLineY, buttonStyle, SAVE_STATE_ID, buttonActionHandler);
  registry.emplace_back(saveState, true, nullptr, 0, activeButtonWidth - buttonWidth);
  currentLineY += grid.buttonHeight + buttonSpacingY;
  resetGame = Button(*context, localizedText.getMessage(MainMenuMessages::resetGame), grid.controlX, currentLineY, buttonStyle, RESET_GAME_ID, buttonActionHandler);
  registry.emplace_back(resetGame, true, nullptr, 0, activeButtonWidth - buttonWidth);
  currentLineY += grid.buttonHeight + buttonSpacingY;
  exitGame = Button(*context, localizedText.getMessage(MainMenuMessages::exitGame), grid.controlX, currentLineY, buttonStyle, EXIT_GAME_ID, buttonActionHandler);
  registry.emplace_back(exitGame, true, nullptr, 0, activeButtonWidth - buttonWidth);
  currentLineY += grid.buttonHeight + Control::pageLineHeight();

  // create save state selector
  ComboBoxOption saveSlots[saveSlotCount];
  fillSaveSlotOptions(saveSlots, saveSlotCount, localizedText.getMessage(MainMenuMessages::activeSaveSlot));
  activeSaveSlot = Slider(*context, nullptr, grid.controlX, currentLineY, 0, buttonWidth,
                          theme->sliderArrowColor(), 0, nullptr, saveSlots, saveSlotCount, 0);
  registry.emplace_back(activeSaveSlot, true);
  currentLineY += Control::pageLineHeight();

  // create controller action info
  navigateControllerInfo = Label(*context, localizedText.getMessage(CommonMessages::navigate),
                                 x + (int32_t)(width >> 1), grid.controllerInfoY,
                                 TextAlignment::right, ControlIconType::buttonDpad);
  selectControllerInfo = Label(*context, localizedText.getMessage(CommonMessages::select),
                               navigateControllerInfo.x() + (int32_t)navigateControllerInfo.width()
                               + (int32_t)Control::controlSideMargin() + (int32_t)Control::buttonPaddingX(),
                               grid.controllerInfoY, TextAlignment::left, ControlIconType::buttonCross);

  // create list of recent profiles
  /*if (profiles->size() > 1u) {
    const uint32_t tileHeight = Control::tileContentHeight(context->getFont(FontType::inputText).XHeight()) + (Control::tilePaddingY() << 1);
    uint32_t tileCount = (height - Control::fieldsetContentMarginBottom() - fieldsetTitleHeight) / (tileHeight + 1u);
    if ((uint32_t)profiles->size() - 1u < recentTileCount) {
      if (tileCount > (uint32_t)profiles->size() - 1u)
        tileCount = (uint32_t)profiles->size() - 1u;
    }
    else if (tileCount > recentTileCount)
      tileCount = recentTileCount;

    // fieldset group
    const uint32_t fieldsetContentHeight = 5u * (tileHeight + 1u) + Control::fieldsetContentPaddingTop() + 4u + Control::fieldsetContentPaddingBottom();
    int32_t recentLineY = resume.y() - (int32_t)(Control::fieldsetTitlePaddingY() << 1) + 3;
    recentProfilesGroup = Fieldset(*context, localizedText.getMessage(MainMenuMessages::recentProfiles), theme->fieldsetControlColor(),
                                   grid.recentProfilesGroupX, recentLineY, grid.fieldsetWidth, fieldsetContentHeight);
    // tiles
    const int32_t tileX = recentProfilesGroup.x() + (int32_t)tilePaddingX;
    auto tileActionHandler = [this](uint32_t id, TileAction action) { this->onTileAction(id, action); };
    recentLineY += (int32_t)Control::pageLineHeight() + (int32_t)Control::fieldsetContentPaddingTop() + 3;
    recentProfiles.reserve((size_t)tileCount);

    uint32_t insertIndex = 1;
    float tileColor[4]{ 0.f,0.f,0.f,0.65f };
    for (uint32_t i = 0; i < tileCount; ++i, insertIndex += 2) {
      const auto& profile = (*profiles)[i + 1];
      memcpy(tileColor, theme->tileColor(profile.color), sizeof(float)*3u);
      recentProfiles.emplace_back(*context, profile.name.get(), tileX, recentLineY, tileWidth,
                                  tileColor, profile.id, tileActionHandler, false);
      registry.insert(registry.begin() + insertIndex, ControlRegistration(recentProfiles.back(), true));
      recentLineY += (int32_t)tileHeight + 1;
    }
  }*/

  // control registry
  Page::moveScrollbarThumb(currentLineY);
  registerControls(std::move(registry));
  selectControlIndex(0);

  // confirmation popup
  const char16_t* popupButtons[]{ localizedText.getMessage(CommonMessages::ok),
                                  localizedText.getMessage(CommonMessages::cancel) };
  resetPopup = Popup(*context, *theme, localizedText.getMessage(MainMenuMessages::resetPopupTitle),
                     localizedText.getMessage(MainMenuMessages::resetPopupMessage),
                     nullptr, popupButtons, sizeof(popupButtons)/sizeof(*popupButtons));
  resetPopup.close();
}

MainMenu::~MainMenu() noexcept {
  title.release();

  resume.release();
  loadState.release();
  saveState.release();
  resetGame.release();
  exitGame.release();
  activeSaveSlot.release();

  //recentProfilesGroup.release();
  //recentProfiles.clear();

  navigateControllerInfo.release();
  selectControllerInfo.release();
}


// -- window events -- ---------------------------------------------------------

void MainMenu::move(int32_t x, int32_t y, uint32_t width, uint32_t height) {
  Page::moveBase(x, y, width, height);
  auto grid = computeGrid(*context, x, y, width, height);

  // create main menu buttons
  const int32_t titleY = y + grid.offsetY - (int32_t)grid.titleHeight - (int32_t)buttonSpacingY;
  title.move(context->renderer(), context->pixelSizeX(), context->pixelSizeY(), grid.controlX, titleY);

  std::vector<ControlRegistration> registry;
  registry.reserve(11);

  int32_t currentLineY = y + grid.offsetY + (int32_t)buttonPaddingY;
  resume.move(*context, grid.controlX, currentLineY);
  registry.emplace_back(resume, true, nullptr, 0, activeButtonWidth - buttonWidth);
  currentLineY += grid.buttonHeight + buttonSpacingY;
  loadState.move(*context, grid.controlX, currentLineY);
  registry.emplace_back(loadState, true, nullptr, 0, activeButtonWidth - buttonWidth);
  currentLineY += grid.buttonHeight + buttonSpacingY;
  saveState.move(*context, grid.controlX, currentLineY);
  registry.emplace_back(saveState, true, nullptr, 0, activeButtonWidth - buttonWidth);
  currentLineY += grid.buttonHeight + buttonSpacingY;
  resetGame.move(*context, grid.controlX, currentLineY);
  registry.emplace_back(resetGame, true, nullptr, 0, activeButtonWidth - buttonWidth);
  currentLineY += grid.buttonHeight + buttonSpacingY;
  exitGame.move(*context, grid.controlX, currentLineY);
  registry.emplace_back(exitGame, true, nullptr, 0, activeButtonWidth - buttonWidth);
  currentLineY += grid.buttonHeight + Control::pageLineHeight();

  activeSaveSlot.move(*context, grid.controlX, currentLineY);
  registry.emplace_back(activeSaveSlot, true);
  currentLineY += Control::pageLineHeight();

  navigateControllerInfo.move(*context, x + (int32_t)(width >> 1), grid.controllerInfoY, TextAlignment::right);
  selectControllerInfo.move(*context, navigateControllerInfo.x() + (int32_t)navigateControllerInfo.width()
                                      + (int32_t)Control::controlSideMargin() + (int32_t)Control::buttonPaddingX(),
                            grid.controllerInfoY, TextAlignment::left);

  // recent profiles
  /*if (!recentProfiles.empty()) {
    int32_t recentLineY = resume.y() - (int32_t)(Control::fieldsetTitlePaddingY() << 1) + 3;
    recentProfilesGroup.move(*context, grid.recentProfilesGroupX, recentLineY);

    const int32_t tileX = recentProfilesGroup.x() + (int32_t)tilePaddingX;
    recentLineY += (int32_t)Control::pageLineHeight() + (int32_t)Control::fieldsetContentPaddingTop() + 3;

    uint32_t insertIndex = 1;
    for (uint32_t i = 0; i < (uint32_t)recentProfiles.size(); ++i, insertIndex += 2) {
      auto& profile = recentProfiles[i];
      profile.move(*context, tileX, recentLineY, tileWidth);
      registry.insert(registry.begin() + insertIndex, ControlRegistration(profile, true));
      recentLineY += (int32_t)profile.height() + 1;
    }
  }*/

  Page::moveScrollbarThumb(currentLineY); // required after a move
  registerControls(std::move(registry));
  resetPopup.move(*context);
  selectControlIndex(0);
}

// ---

void MainMenu::onButtonAction(uint32_t id) {
  switch (id) {
    case RESUME_ID: break;
    case LOAD_STATE_ID: break;
    case SAVE_STATE_ID: break;
    case RESET_GAME_ID:
      setActivePopup(resetPopup, [this](uint32_t action) {
        if (action == 0)
          ;//...
      });
      break;
    case EXIT_GAME_ID: break;
  }


    
}

void MainMenu::onTileAction(uint32_t id, TileAction type) {
  if (type == TileAction::select) {
    //...
  }
}


// -- rendering -- -------------------------------------------------------------

void MainMenu::adaptButtonStyle(Button& button, const Control* activeControl) {
  if (activeControl == &button) {
    if (button.width() != activeButtonWidth)
      button.move(*context, activeButtonWidth, theme->buttonBorderColor());
  }
  else if (button.width() != buttonWidth && activeControl != nullptr)
    button.move(*context, buttonWidth, theme->buttonControlColor());
}

// ---

void MainMenu::drawIcons() {
  if (isControllerUsed()) {
    if (!buffers->isFixedLocationBuffer())
      buffers->bindFixedLocationBuffer(context->renderer(), ScissorRectangle(x(), y(), width(), height()));

    navigateControllerInfo.drawIcon(*context, *buffers);
    selectControllerInfo.drawIcon(*context, *buffers);
  }
}

void MainMenu::drawPageBackgrounds(int32_t mouseX, int32_t) {
  // scrollable geometry
  if (buffers->isFixedLocationBuffer())
    buffers->bindScrollLocationBuffer(context->renderer(), ScissorRectangle(x(), y(), width(), contentHeight()));

  auto* hoverControl = getActiveControl();
  resume.drawBackground(*context, *buffers, (hoverControl == &resume), isMouseDown());
  adaptButtonStyle(resume, hoverControl);
  loadState.drawBackground(*context, *buffers, (hoverControl == &loadState), isMouseDown());
  adaptButtonStyle(loadState, hoverControl);
  saveState.drawBackground(*context, *buffers, (hoverControl == &saveState), isMouseDown());
  adaptButtonStyle(saveState, hoverControl);
  resetGame.drawBackground(*context, *buffers, (hoverControl == &resetGame), isMouseDown());
  adaptButtonStyle(resetGame, hoverControl);
  exitGame.drawBackground(*context, *buffers, (hoverControl == &exitGame), isMouseDown());
  adaptButtonStyle(exitGame, hoverControl);

  activeSaveSlot.drawBackground(*context, mouseX, *buffers, (hoverControl == &activeSaveSlot));

  /*if (recentProfilesGroup.width() && (int32_t)width() - (int32_t)activeButtonWidth > (int32_t)tileWidth + 60) {
    recentProfilesGroup.drawBackground(*context, *buffers);
    for (auto& tile : recentProfiles)
      tile.drawBackground(*context, mouseX, mouseY, *buffers, false, (hoverControl == &tile));
  }*/
}

void MainMenu::drawPageLabels() {
  // scrollable geometry
  auto& renderer = context->renderer();
  if (buffers->isFixedLocationBuffer())
    buffers->bindScrollLocationBuffer(renderer, ScissorRectangle(x(), y(), width(), contentHeight()));

  buffers->bindLabelBuffer(renderer, LabelBufferType::title);
  title.draw(renderer);

  auto* hoverControl = getActiveControl();
  resume.drawLabel(*context, *buffers, (hoverControl == &resume));
  loadState.drawLabel(*context, *buffers, (hoverControl == &loadState));
  saveState.drawLabel(*context, *buffers, (hoverControl == &saveState));
  resetGame.drawLabel(*context, *buffers, (hoverControl == &resetGame));
  exitGame.drawLabel(*context, *buffers, (hoverControl == &exitGame));

  activeSaveSlot.drawLabels(*context, *buffers, (hoverControl == &activeSaveSlot));

  /*if (recentProfilesGroup.width() && (int32_t)width() - (int32_t)activeButtonWidth > (int32_t)tileWidth + 60) {
    recentProfilesGroup.drawLabel(*context, *buffers);
    for (auto& tile : recentProfiles)
      tile.drawLabel(*context, *buffers, false, (hoverControl == &tile));
  }*/

  buffers->bindFixedLocationBuffer(context->renderer(), ScissorRectangle(x(), y(), width(), height()));
  if (isControllerUsed()) {
    navigateControllerInfo.drawLabel(*context, *buffers);
    selectControllerInfo.drawLabel(*context, *buffers);
  }
}
