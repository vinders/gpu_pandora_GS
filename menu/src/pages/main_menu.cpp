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
#include "menu/controls/geometry_generator.h"
#include "menu/pages/page_content_builder.h"
#include "menu/pages/main_menu.h"

using namespace video_api;
using namespace display;
using namespace display::controls;
using namespace menu::controls;
using namespace menu::pages;
using namespace menu;

PageId MainMenu::PageType() const noexcept { return PageId::mainMenu; }


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
  grid.controlX = (x + (int32_t)((width - Control::scrollbarWidth() - activeButtonWidth + buttonWidth) >> 1) - (int32_t)(buttonWidth >> 1));
  grid.titleHeight = context.getFont(FontType::titles).XHeight();
  grid.buttonHeight = context.getFont(FontType::labels).XHeight() + (buttonPaddingY << 1);

  const uint32_t gridHeight = ((grid.buttonHeight + buttonSpacingY) * 5u);
  grid.offsetY = ((int32_t)height - (int32_t)gridHeight) / 2;
  if (grid.offsetY < (int32_t)Control::titleMarginTop() + (int32_t)Control::pageLineHeight() + (int32_t)grid.titleHeight)
    grid.offsetY = Control::titleMarginTop() + Control::pageLineHeight() + grid.titleHeight;
  else {
    grid.offsetY -= (int32_t)(height >> 4);
    if (grid.offsetY > (int32_t)gridHeight)
      grid.offsetY = (int32_t)gridHeight;
  }

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

#define HOVER_BORDER_RADIUS 3.f
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
                                    theme->buttonBorderColor(), theme->buttonBorderSize(), buttonWidth, buttonPaddingX, buttonPaddingY);
  int32_t currentLineY = y + grid.offsetY + (int32_t)buttonPaddingY;
  resume = Button(*context, localizedText.getMessage(MainMenuMessages::resume), grid.controlX, currentLineY, buttonStyle,
                  (uint32_t)MenuOperation::resume, buttonActionHandler);
  registry.emplace_back(resume, true, nullptr, 0, activeButtonWidth - buttonWidth);
  currentLineY += grid.buttonHeight + buttonSpacingY;
  loadState = Button(*context, localizedText.getMessage(MainMenuMessages::loadState), grid.controlX, currentLineY, buttonStyle,
                     (uint32_t)MenuOperation::loadState, buttonActionHandler);
  registry.emplace_back(loadState, true, nullptr, 0, activeButtonWidth - buttonWidth);
  currentLineY += grid.buttonHeight + buttonSpacingY;
  saveState = Button(*context, localizedText.getMessage(MainMenuMessages::saveState), grid.controlX, currentLineY, buttonStyle,
                     (uint32_t)MenuOperation::saveState, buttonActionHandler);
  registry.emplace_back(saveState, true, nullptr, 0, activeButtonWidth - buttonWidth);
  currentLineY += grid.buttonHeight + buttonSpacingY;
  resetGame = Button(*context, localizedText.getMessage(MainMenuMessages::resetGame), grid.controlX, currentLineY, buttonStyle,
                     (uint32_t)MenuOperation::resetGame, buttonActionHandler);
  registry.emplace_back(resetGame, true, nullptr, 0, activeButtonWidth - buttonWidth);
  currentLineY += grid.buttonHeight + buttonSpacingY;
  exitGame = Button(*context, localizedText.getMessage(MainMenuMessages::exitGame), grid.controlX, currentLineY, buttonStyle,
                    (uint32_t)MenuOperation::exitGame, buttonActionHandler);
  registry.emplace_back(exitGame, true, nullptr, 0, activeButtonWidth - buttonWidth);
  currentLineY += grid.buttonHeight + Control::pageLineHeight();

  // create save state selector
  ComboBoxOption saveSlots[saveSlotCount];
  fillSaveSlotOptions(saveSlots, saveSlotCount, localizedText.getMessage(MainMenuMessages::activeSaveSlot));
  activeSaveSlot = Slider(*context, nullptr, grid.controlX, currentLineY, 0, buttonWidth,
                          theme->sliderArrowColor(), 0, nullptr, saveSlots, saveSlotCount, 0);
  registry.emplace_back(activeSaveSlot, true);
  currentLineY += Control::pageLineHeight();

  // create save state selector hover mesh
  constexpr const uint32_t saveSlotHoverWidth = buttonWidth + (Control::lineHoverPaddingX() << 1);
  constexpr const uint32_t saveSlotHoverHeight = Control::pageLineHeight() + (Control::pageLineHeight() >> 2);
  std::vector<ControlVertex> saveSlotHoverVertices(GeometryGenerator::getRoundedRectangleVertexCount(HOVER_BORDER_RADIUS));
  GeometryGenerator::fillRoundedRectangleVertices(saveSlotHoverVertices.data(), theme->lineSelectorControlColor(),
                                                  0.f, (float)saveSlotHoverWidth, 0.f, -(float)saveSlotHoverHeight,
                                                  HOVER_BORDER_RADIUS);
  std::vector<uint32_t> indices(GeometryGenerator::getRoundedRectangleVertexIndexCount(HOVER_BORDER_RADIUS));
  GeometryGenerator::fillRoundedRectangleIndices(indices.data(), 0, HOVER_BORDER_RADIUS);
  
  saveSlotHoverMesh = ControlMesh(context->renderer(), std::move(saveSlotHoverVertices), indices, context->pixelSizeX(),
                                  context->pixelSizeY(), resume.x() - (int32_t)Control::lineHoverPaddingX(),
                                  activeSaveSlot.y() - static_cast<int32_t>((saveSlotHoverHeight - activeSaveSlot.height()) >> 1) - 1,
                                  saveSlotHoverWidth, saveSlotHoverHeight);

  // create controller action info
  navigateControllerInfo = Label(*context, localizedText.getMessage(CommonMessages::navigate),
                                 x + (int32_t)(width >> 1), grid.controllerInfoY,
                                 TextAlignment::right, ControlIconType::buttonDpad);
  selectControllerInfo = Label(*context, localizedText.getMessage(CommonMessages::select),
                               navigateControllerInfo.x() + (int32_t)navigateControllerInfo.width()
                               + (int32_t)Control::controlSideMargin() + (int32_t)Control::buttonPaddingX(),
                               grid.controllerInfoY, TextAlignment::left, ControlIconType::buttonCross);

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
  saveSlotHoverMesh.release();

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

  constexpr const uint32_t saveSlotHoverHeight = Control::pageLineHeight() + (Control::pageLineHeight() >> 2);
  saveSlotHoverMesh.move(context->renderer(), context->pixelSizeX(), context->pixelSizeY(),
                         resume.x() - (int32_t)Control::lineHoverPaddingX(),
                         activeSaveSlot.y() - static_cast<int32_t>((saveSlotHoverHeight - activeSaveSlot.height()) >> 1) - 1);

  navigateControllerInfo.move(*context, x + (int32_t)(width >> 1), grid.controllerInfoY, TextAlignment::right);
  selectControllerInfo.move(*context, navigateControllerInfo.x() + (int32_t)navigateControllerInfo.width()
                                      + (int32_t)Control::controlSideMargin() + (int32_t)Control::buttonPaddingX(),
                            grid.controllerInfoY, TextAlignment::left);

  Page::moveScrollbarThumb(currentLineY); // required after a move
  registerControls(std::move(registry));
  resetPopup.move(*context);
  selectControlIndex(0);
}

// ---

void MainMenu::onButtonAction(uint32_t id) {
  if ((MenuOperation)id == MenuOperation::resetGame) {
    setActivePopup(resetPopup, [this](uint32_t action) {
      if (action == 0)
        onMenuAction(MenuOperation::resetGame);
    });
  }
  else
    onMenuAction((MenuOperation)id);
}


// -- rendering -- -------------------------------------------------------------

void MainMenu::adaptButtonStyle(Button& button, const Control* activeControl) {
  if (activeControl == &button) {
    if (button.width() != activeButtonWidth)
      button.move(*context, activeButtonWidth, theme->buttonSpecialColor(), theme->buttonSpecialColor());
  }
  else if (button.width() != buttonWidth && activeControl != nullptr)
    button.move(*context, buttonWidth, theme->buttonControlColor(), theme->buttonBorderColor());
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
  adaptButtonStyle(resume, hoverControl);
  resume.drawBackground(*context, *buffers, (resume.width() == activeButtonWidth), (isMouseDown() && hoverControl));
  adaptButtonStyle(loadState, hoverControl);
  loadState.drawBackground(*context, *buffers, (loadState.width() == activeButtonWidth), (isMouseDown() && hoverControl));
  adaptButtonStyle(saveState, hoverControl);
  saveState.drawBackground(*context, *buffers, (saveState.width() == activeButtonWidth), (isMouseDown() && hoverControl));
  adaptButtonStyle(resetGame, hoverControl);
  resetGame.drawBackground(*context, *buffers, (resetGame.width() == activeButtonWidth), (isMouseDown() && hoverControl));
  adaptButtonStyle(exitGame, hoverControl);
  exitGame.drawBackground(*context, *buffers, (exitGame.width() == activeButtonWidth), (isMouseDown() && hoverControl));

  if (hoverControl == &activeSaveSlot)
    saveSlotHoverMesh.draw(context->renderer());
  activeSaveSlot.drawBackground(*context, mouseX, *buffers, (hoverControl == &activeSaveSlot));
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
