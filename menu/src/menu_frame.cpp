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
#define USE_SHADERS
#include <thread>
#include <video/window_keycodes.h>
#include "menu/controls/geometry_generator.h"
#include "menu/pages/main_menu.h"
#include "menu/pages/general_settings.h"
#include "menu/pages/hotkey_bindings.h"
#include "menu/pages/osd_settings.h"
#include "menu/pages/profile_selector.h"
#include "menu/pages/profile_settings.h"
#include "menu/pages/screen_stretching.h"
#include "menu/pages/smoothing_upscaling.h"
#include "menu/pages/advanced_effects.h"
#include "menu/menu_frame.h"

using namespace pandora::hardware;
using namespace pandora::video;
using namespace video_api;
using namespace display;
using namespace display::controls;
using namespace menu::controls;
using namespace menu::pages;
using namespace menu;


static constexpr const uint32_t logoAreaHeight = 48;

MenuFrame::MenuFrame(MenuMode mode, std::shared_ptr<RendererContext> context_,
                     Window& window, std::vector<ConfigProfile>&& profiles, uint32_t activeProfileId,
                     std::function<void(MenuOperation)> onClose, bool isControllerUsed)
  : context(std::move(context_)),
    theme(std::make_shared<menu::ColorTheme>(menu::ColorThemeType::blue)),
    localization(std::make_shared<menu::MessageResources>(menu::LocalizationType::en)),
    sectionMode(mode),
    activeProfileId(activeProfileId),
    editedProfileId(activeProfileId),
    profiles(std::move(profiles)),
    windowMonitor(window.displayMonitor().handle()),
    onClose(std::move(onClose)) {
  assert(this->context != nullptr);
  initGraphicsPipelines(window);

  // load first page
  createSectionTabs(0);
  createBackground(); // uses section tab width -> done after
  createPage(PageId::mainMenu, isControllerUsed, true);
  isInvalidated = true;
  
  // create presets
  presets.emplace_back(0, u"All-purpose");
  presets.emplace_back(1, u"Cartoon 2D");
  presets.emplace_back(2, u"Cartoon 3D");
  presets.emplace_back(3, u"Realistic 2D");
  presets.emplace_back(4, u"Realistic 3D");
  presets.emplace_back(5, u"Nostalgia");
  presets.emplace_back(6, u"Fastest");
}

// ---
  
MenuFrame::~MenuFrame() noexcept {
  backgroundMesh.release();
  backgroundGradientMesh.release();
  sectionTabs.release();
  logoMesh.release();
  sectionUpButtonMesh.release();
  sectionDownButtonMesh.release();
  pageTabs.release();
  pagePreviousButton.release();
  pageNextButton.release();
  activePage = nullptr;
  
  controlPipeline.release();
  iconPipeline.release();
  labelPipeline.release();
  textureSampler.release();
  buffers = nullptr;
  theme = nullptr;
  localization = nullptr;
  
  swapChain.release();
  context = nullptr;
}

// ---

bool MenuFrame::onWindowEvent(Window* sender, WindowEvent event, uint32_t, int32_t, int32_t, void*) {
  switch (event) {
    case WindowEvent::monitorChanged:
      windowMonitor = sender->displayMonitor().handle();
      isInvalidated = true;
      break;
    default: isInvalidated = true; break;
  }
  return false;
}

bool MenuFrame::onPositionEvent(Window* sender, PositionEvent event, uint32_t, uint32_t) {
  switch (event) {
    case PositionEvent::sizePositionChanged: {
      auto clientArea = sender->getClientSize();
      auto previousScaling = context->scaling();
      if (context->onSizeChange(clientArea.width, clientArea.height)) { // size changed
        resizeGraphicsPipelines();
        
        if (previousScaling == context->scaling()) {
          if (sectionTabs.width() != Control::sectionTabWidth(context->clientWidth()) + 1u) // section tab width changed -> rebuild
            createSectionTabs(sectionTabs.activeTabIndex());
          else // simple resize
            moveSectionTabs();
          moveBackground(); // uses section tab width -> done after

          if (pageTabs.width())
            movePageTabs();
          activePage->move((int32_t)sectionTabs.width(), (int32_t)pageTabs.height(), 
                           context->clientWidth() - sectionTabs.width(),
                           context->clientHeight() - pageTabs.height());
        }
        else {
          auto sectionIndex = sectionTabs.activeTabIndex();
          sectionTabs.release();
          pageTabs.release();
          auto pageId = activePage->PageType();
          bool isControllerUsed = activePage->isControllerUsed();
          activePage = nullptr;

          createSectionTabs(sectionIndex);
          createPage(pageId, isControllerUsed, true);
        }
      }
      isInvalidated = true; // always re-draw if position or size changes
      break;
    }
    default: break;
  }
  return false;
}

bool MenuFrame::onKeyboardEvent(Window*, KeyboardEvent event, uint32_t keyCode, uint32_t) {
  switch (event) {
    case KeyboardEvent::keyDown:
    case KeyboardEvent::altKeyDown: {
      isInvalidated = true;
      return activePage->vkeyDown(keyCode);
    }
    case KeyboardEvent::charInput: {
      isInvalidated = true;
      activePage->keyDown((char32_t)keyCode);
      break;
    }
    default: break;
  }
  return false;
}

bool MenuFrame::onControllerEvent(KeyboardEvent event, uint32_t virtualKeyCode, int32_t analogX, int32_t analogY) {
  switch (event) {
    case KeyboardEvent::keyDown:
    case KeyboardEvent::altKeyDown: {
      isInvalidated = true;
      if (activePage->padButtonDown(virtualKeyCode))
        return true;

      switch (virtualKeyCode) {
        case /*XINPUT_GAMEPAD_LEFT_SHOULDER*/0x0100:  sectionTabs.selectPrevious(*context); break;
        case /*XINPUT_GAMEPAD_RIGHT_SHOULDER*/0x0200: sectionTabs.selectNext(*context); break;
        case /*bLeftTrigger*/0xFFFFFF00:
          if (pageTabs.width())
            pageTabs.selectPrevious(*context);
          break;
        case /*bRightTrigger*/0xFFFFFF01:
          if (pageTabs.width())
            pageTabs.selectNext(*context);
          break;
        default: break;
      }
      break;
    }
    default: break;
  }
  return false;
}

bool MenuFrame::onMouseEvent(Window*, MouseEvent event, int32_t x, int32_t y, int32_t index) {
  if (context->scaling() != 1u) {
    x /= (int32_t)context->scaling();
    y /= (int32_t)context->scaling();
  }

  switch (event) {
    case MouseEvent::mouseMove:
      this->mouseX = x;
      this->mouseY = y;
      activePage->mouseMove(x, y);
      break;
    case MouseEvent::mouseLeave:
      this->mouseX = this->mouseY = -1;
      activePage->mouseLeave();
      break;
    case MouseEvent::buttonDown:
      if (x >= (int32_t)sectionTabs.width()) {
        if (y >= (int32_t)pageTabs.height())
          activePage->mouseDown(x, y, (MouseButton)index);
        else if (y >= 0)
          pageTabs.click(*context, x, y);
      }
      else if (x >= 0)
        sectionTabs.click(*context, y);
      break;
    case MouseEvent::buttonUp:
      if ((MouseButton)index == MouseButton::left)
        activePage->mouseUp(x, y);
      break;
    case MouseEvent::mouseWheelV:
      activePage->mouseScroll(index);
      break;
    default: break;
  }
  isInvalidated = true; // all mouse events trigger a re-draw
  return false;
}

void MenuFrame::onProfileSelection(uint32_t profileId, bool isEditing) {
  if (isEditing) {
    sectionTabs.selectIndex(*context, 2u);
    editedProfileId = profileId;
    pageToLoad = PageId::profileSettings;
  }
  else {
    activeProfileId = editedProfileId = profileId;
    onClose(MenuOperation::resume);
  }
}

// ---

void MenuFrame::draw() {
  if (isInvalidated) {
    // process page change here (not in handlers: we don't want to destroy an instance in use)
    if (pageToLoad != PageId::none)
      createPage(pageToLoad, activePage->isControllerUsed(), false);
    
    auto& renderer = context->renderer();
    renderer.setFragmentSamplerStates(0, textureSampler.handlePtr(), 1);
    renderer.setActiveRenderTarget(swapChain.getRenderTargetView());
    
    // draw page background with gradient
    renderer.bindGraphicsPipeline(controlPipeline.handle());
    buffers->bindFixedLocationBuffer(renderer, ScissorRectangle(0, 0, context->clientWidth(), context->clientHeight()));
    buffers->bindControlBuffer(renderer, ControlBufferType::regular);
    backgroundMesh.draw(renderer);
    renderer.bindGraphicsPipeline(iconPipeline.handle());
    buffers->bindFixedLocationBuffer(renderer, ScissorRectangle(0, 0, context->clientWidth(), context->clientHeight()));
    buffers->bindIconBuffer(renderer, ControlBufferType::regular);
    backgroundGradientMesh.draw(renderer);

    // draw page controls - backgrounds
    renderer.bindGraphicsPipeline(controlPipeline.handle());
    buffers->bindFixedLocationBuffer(renderer, ScissorRectangle(0, 0, context->clientWidth(), context->clientHeight()));
    sectionTabs.drawBackground(*context, *buffers);
    if (pageTabs.width())
      pageTabs.drawBackground(*context, mouseX, mouseY, *buffers);
    bool drawForeground = activePage->drawBackgrounds(mouseX, mouseY);
    
    // draw page controls - icons
    renderer.bindGraphicsPipeline(iconPipeline.handle());
    buffers->bindFixedLocationBuffer(renderer, ScissorRectangle(0, 0, context->clientWidth(), context->clientHeight()));
    buffers->bindIconBuffer(renderer, ControlBufferType::regular);
    logoMesh.draw(context->renderer());
    if (activePage->isControllerUsed()) {
      sectionUpButtonMesh.draw(renderer);
      sectionDownButtonMesh.draw(renderer);
      if (pageTabs.width()) {
        pagePreviousButton.draw(renderer);
        pageNextButton.draw(renderer);
      }
    }
    sectionTabs.drawIcons(*context, mouseX, mouseY, *buffers);
    activePage->drawIcons();
    
    // draw page controls - labels/titles
    renderer.bindGraphicsPipeline(labelPipeline.handle());
    buffers->bindFixedLocationBuffer(renderer, ScissorRectangle(0, 0, context->clientWidth(), context->clientHeight()));
    if (sectionTabs.width() >= Control::sectionWideTabWidth())
      sectionTabs.drawLabels(*context, mouseX, mouseY, *buffers);
    if (pageTabs.width())
      pageTabs.drawLabels(*context, mouseX, mouseY, *buffers);
    activePage->drawLabels();

    // draw foreground controls (open combo-box...)
    if (drawForeground) {
      renderer.bindGraphicsPipeline(controlPipeline.handle());
      activePage->drawForegrounds();
      
      renderer.bindGraphicsPipeline(labelPipeline.handle());
      activePage->drawForegroundLabels();
    }
    
    // display framebuffer
    swapChain.swapBuffers(); // vsync -> no need for sleep
  }
  else // no change and no system event -> no need to re-draw
    std::this_thread::sleep_for(std::chrono::milliseconds(15LL));
}


// -- graphics -- --------------------------------------------------------------

void MenuFrame::initGraphicsPipelines(Window& window) {
  // UI state buffers (uniforms)
  auto& renderer = context->renderer();
  buffers = std::make_shared<menu::RendererStateBuffers>(renderer, *theme, context->scaling());
  
  // framebuffers (swap-chain)
  uint32_t width = context->originalWidth();
  uint32_t height = context->originalHeight();
  swapChain = SwapChain(DisplaySurface(renderer, window.handle()),
                        SwapChainDescriptor(width, height, 2u, PresentMode::fifo),
                        DataFormat::rgba8_sRGB);
  if (context->scaling() != 1u) {
    width += width % context->scaling();
    height += height % context->scaling();
  }
  
  // graphics pipelines
  GraphicsPipeline::Builder builder(renderer);
  Viewport viewport(width, height);
  ScissorRectangle scissor(0, 0, width, height);
  builder.setVertexTopology(VertexTopology::triangles)
         .setRasterizerState(RasterizerParams(CullMode::cullBack, FillMode::fill, true, false, true, 1u))
         .setDepthStencilState(DepthStencilParams(false, false))
         .setBlendState(BlendParams(BlendFactor::sourceAlpha, BlendFactor::sourceInvAlpha, BlendOp::add,
                                    BlendFactor::one, BlendFactor::zero, BlendOp::add, ColorComponentFlag::all))
         .setViewports(&viewport, 1, &scissor, 1, true);
  
# include "./shaders/control.hlsl.hpp"
  auto vertexBuilder = Shader::Builder::compile(ShaderType::vertex, controlVertexShader, sizeof(controlVertexShader)/sizeof(char));
  auto fragmentBuilder = Shader::Builder::compile(ShaderType::fragment, controlPixelShader, sizeof(controlPixelShader)/sizeof(char));
  controlPipeline = builder.setInputLayout(vertexBuilder.createInputLayout(renderer.device(), controlLayout, sizeof(controlLayout)/sizeof(*controlLayout)))
                           .attachShaderStage(vertexBuilder.createShader(renderer.device()))
                           .attachShaderStage(fragmentBuilder.createShader(renderer.device()))
                           .build();
  
# include "./shaders/icon.hlsl.hpp"
  vertexBuilder = Shader::Builder::compile(ShaderType::vertex, iconVertexShader, sizeof(iconVertexShader)/sizeof(char));
  fragmentBuilder = Shader::Builder::compile(ShaderType::fragment, iconPixelShader, sizeof(iconPixelShader)/sizeof(char));
  iconPipeline = builder.setInputLayout(vertexBuilder.createInputLayout(renderer.device(), iconLayout, sizeof(iconLayout)/sizeof(*iconLayout)))
                        .attachShaderStage(vertexBuilder.createShader(renderer.device()))
                        .attachShaderStage(fragmentBuilder.createShader(renderer.device()))
                        .build();
  
# include "./shaders/label.hlsl.hpp"
  vertexBuilder = Shader::Builder::compile(ShaderType::vertex, labelVertexShader, sizeof(labelVertexShader)/sizeof(char));
  fragmentBuilder = Shader::Builder::compile(ShaderType::fragment, labelPixelShader, sizeof(labelPixelShader)/sizeof(char));
  labelPipeline = builder.setInputLayout(vertexBuilder.createInputLayout(renderer.device(), labelLayout, sizeof(labelLayout)/sizeof(*labelLayout)))
                         .attachShaderStage(vertexBuilder.createShader(renderer.device()))
                         .attachShaderStage(fragmentBuilder.createShader(renderer.device()))
                         .build();

  Sampler::Builder samplerBuilder(renderer.resourceManager());
  TextureWrap textureWrapUVW[]{ TextureWrap::clampToEdge, TextureWrap::clampToEdge, TextureWrap::clampToEdge };
  textureSampler = samplerBuilder.createSampler(SamplerParams(TextureFilter::linear, TextureFilter::linear, TextureFilter::nearest,
                                                       textureWrapUVW, SamplerParams::highestLod(), SamplerParams::infiniteLod()));
}

void MenuFrame::resizeGraphicsPipelines() {
  uint32_t width = context->originalWidth();
  uint32_t height = context->originalHeight();
  swapChain.resize(width, height);
  buffers->updateScaling(context->scaling());
  if (context->scaling() != 1u) {
    width += width % context->scaling();
    height += height % context->scaling();
  }

  Viewport viewport(width, height);
  ScissorRectangle scissor(0, 0, width, height);
  controlPipeline.handle()->viewports[0] = *(D3D11_VIEWPORT*)&viewport;
  controlPipeline.handle()->scissorTests[0] = *(D3D11_RECT*)&scissor;
  iconPipeline.handle()->viewports[0] = *(D3D11_VIEWPORT*)&viewport;
  iconPipeline.handle()->scissorTests[0] = *(D3D11_RECT*)&scissor;
  labelPipeline.handle()->viewports[0] = *(D3D11_VIEWPORT*)&viewport;
  labelPipeline.handle()->scissorTests[0] = *(D3D11_RECT*)&scissor;
}

// -----------------------------------------------------------------------------

void MenuFrame::createBackground() {
  const uint32_t width = context->clientWidth();
  const uint32_t height = context->clientHeight();
  const float* backColor = theme->backgroundColor();

  std::vector<ControlVertex> vertices(static_cast<size_t>(4));
  ControlVertex* vertexIt = vertices.data();
  GeometryGenerator::fillControlVertex(*vertexIt,     backColor, 0.f,          0.f);
  GeometryGenerator::fillControlVertex(*(++vertexIt), backColor, (float)width, 0.f);
  GeometryGenerator::fillControlVertex(*(++vertexIt), backColor, 0.f,          -(float)height);
  GeometryGenerator::fillControlVertex(*(++vertexIt), backColor, (float)width, -(float)height);
  std::vector<uint32_t> indices{ 0,1,2,2,1,3 };
  backgroundMesh = ControlMesh(context->renderer(), std::move(vertices), indices,
                               context->pixelSizeX(), context->pixelSizeY(), 0, 0, width, height);

  const uint32_t cropX = (width >> 4);
  const uint32_t cropY = (height >> 5);
  const int32_t gradientX = (int32_t)sectionTabs.width() - (int32_t)cropX;
  backgroundGradientMesh = IconMesh(context->renderer(),
                                    context->imageLoader().loadRadialGradient(theme->backgroundGradientColor()),
                                    context->pixelSizeX(), context->pixelSizeY(),
                                    gradientX, -(int32_t)cropY, width + cropX - gradientX, height + (cropY << 1));
}

void MenuFrame::moveBackground() {
  const uint32_t width = context->clientWidth();
  const uint32_t height = context->clientHeight();
  
  auto vertices = backgroundMesh.relativeVertices();
  vertices[1].position[0] = (float)width;
  vertices[2].position[1] = -(float)height;
  vertices[3].position[0] = (float)width;
  vertices[3].position[1] = -(float)height;
  backgroundMesh.update(context->renderer(), std::move(vertices), context->pixelSizeX(),
                        context->pixelSizeY(), 0, 0, width, height);
  
  const uint32_t cropX = (width >> 4);
  const uint32_t cropY = (height >> 5);
  const int32_t gradientX = (int32_t)sectionTabs.width() - (int32_t)cropX;
  backgroundGradientMesh.resize(context->renderer(), context->pixelSizeX(), context->pixelSizeY(),
                                gradientX, -(int32_t)cropY, width + cropX - gradientX, height + (cropY << 1));
}


// -- pages/tabs -- ------------------------------------------------------------

void MenuFrame::createSectionTabs(uint32_t activeTabIndex) {
  VerticalTabOption sectionTabOptions[4];
  size_t sectionTabCount;
  std::function<void(uint32_t)> sectionChangeHandler;
  
  if (sectionMode == MenuMode::configDialog) {
    sectionTabCount = 3;
    sectionTabOptions[0] = VerticalTabOption(localization->getMessage(CommonMessages::globalSettings), ControlIconType::tabSettings);
    sectionTabOptions[1] = VerticalTabOption(localization->getMessage(CommonMessages::profileSelector), ControlIconType::tabSelector);
    sectionTabOptions[2] = VerticalTabOption(localization->getMessage(CommonMessages::profileSettings), ControlIconType::tabProfile);
    sectionChangeHandler = [this](uint32_t tabIndex) {
      switch (tabIndex) {
        case 0: this->pageToLoad = PageId::generalSettings; editedProfileId = activeProfileId; break;
        case 1: this->pageToLoad = PageId::profileSelector; editedProfileId = activeProfileId; break;
        case 2: this->pageToLoad = PageId::profileSettings; break;
        default: break;
      }
    };
  }
  else {
    sectionTabCount = 4;
    sectionTabOptions[0] = VerticalTabOption(localization->getMessage(CommonMessages::mainMenu), ControlIconType::tabHome);
    sectionTabOptions[1] = VerticalTabOption(localization->getMessage(CommonMessages::profileSelector), ControlIconType::tabSelector);
    sectionTabOptions[2] = VerticalTabOption(localization->getMessage(CommonMessages::profileSettings), ControlIconType::tabProfile);
    sectionTabOptions[3] = VerticalTabOption(localization->getMessage(CommonMessages::globalSettings), ControlIconType::tabSettings);
    sectionChangeHandler = [this](uint32_t tabIndex) {
      switch (tabIndex) {
        case 0: this->pageToLoad = PageId::mainMenu; editedProfileId = activeProfileId; break;
        case 1: this->pageToLoad = PageId::profileSelector; editedProfileId = activeProfileId; break;
        case 2: this->pageToLoad = PageId::profileSettings; break;
        case 3: this->pageToLoad = PageId::generalSettings; editedProfileId = activeProfileId; break;
        default: break;
      }
    };
  }
  
  uint32_t tabBarHeight = context->clientHeight();
  if (activePage != nullptr)
    tabBarHeight -= activePage->tooltipHeight();
  const uint32_t sectionTabWidth = Control::sectionTabWidth(context->clientWidth());
  sectionTabs = VerticalTabControl(*context, 0, 0, sectionTabWidth, tabBarHeight, 9, logoAreaHeight,
                                   theme->verticalTabControlColorParams(), sectionTabOptions, sectionTabCount,
                                   activeTabIndex, std::move(sectionChangeHandler));
  
  // top logo
  const uint32_t logoHeight = context->pandoraLogoImage()->height() / (uint32_t)ColorThemeType::COUNT;
  uint32_t logoWidth = (context->pandoraLogoImage()->rowBytes() >> 2);
  if (sectionTabs.width() < Control::sectionWideTabWidth())
    logoWidth -= 25;
  const uint32_t logoX = (sectionTabs.width() + 1u - logoWidth) >> 1;
  const uint32_t logoY = (logoAreaHeight + 1u - logoHeight) >> 1;

  logoMesh = IconMesh(context->renderer(), context->pandoraLogoImage(),
                      context->pixelSizeX(), context->pixelSizeY(), logoX, logoY, 0,
                      logoHeight*(uint32_t)theme->themeType(), logoWidth, logoHeight);

  // bottom controller indicators
  auto upButtonIcon = context->imageLoader().getIcon(ControlIconType::buttonL1);
  auto downButtonIcon = context->imageLoader().getIcon(ControlIconType::buttonR1);
  int32_t upButtonX, upButtonY, downButtonX, downButtonY;
  if (sectionTabs.width() > upButtonIcon.width() + downButtonIcon.width() + 2u) {
    upButtonX = (sectionTabs.width() - (upButtonIcon.width() + downButtonIcon.width() + 2u) + 1u) >> 1;
    downButtonX = upButtonX + (int32_t)upButtonIcon.width() + 2;
    upButtonY = downButtonY = (int32_t)(sectionTabs.height() - upButtonIcon.height() - (Control::comboBoxPaddingY() << 1) - 1u);
  }
  else {
    upButtonX = downButtonX = (int32_t)((sectionTabs.width() - upButtonIcon.width()) >> 1);
    downButtonY = (int32_t)(sectionTabs.height() - upButtonIcon.height() - Control::comboBoxPaddingY() + 2u);
    upButtonY = downButtonY - (int32_t)downButtonIcon.height() - 2;
  }
  sectionUpButtonMesh = IconMesh(context->renderer(), upButtonIcon.texture(),
                                 context->pixelSizeX(), context->pixelSizeY(), upButtonX, upButtonY,
                                 upButtonIcon.offsetX(), upButtonIcon.offsetY(), upButtonIcon.width(), upButtonIcon.height());
  sectionDownButtonMesh = IconMesh(context->renderer(), downButtonIcon.texture(),
                                   context->pixelSizeX(), context->pixelSizeY(), downButtonX, downButtonY,
                                   downButtonIcon.offsetX(), downButtonIcon.offsetY(), downButtonIcon.width(), downButtonIcon.height());
}

void MenuFrame::moveSectionTabs() {
  sectionTabs.move(*context, 0, 0, context->clientHeight() - activePage->tooltipHeight());
  logoMesh.move(context->renderer(), context->pixelSizeX(), context->pixelSizeY(),
                (sectionTabs.width() + 1u - logoMesh.width()) >> 1, (logoAreaHeight + 1u - logoMesh.height()) >> 1);

  int32_t upButtonY, downButtonY;
  if (sectionTabs.width() > sectionUpButtonMesh.width() + sectionDownButtonMesh.width() + 2u) {
    upButtonY = downButtonY = (int32_t)(sectionTabs.height() - sectionUpButtonMesh.height() - (Control::comboBoxPaddingY() << 1) - 1u);
  }
  else {
    downButtonY = (int32_t)(sectionTabs.height() - sectionUpButtonMesh.height() - Control::comboBoxPaddingY() + 2u);
    upButtonY = downButtonY - (int32_t)sectionDownButtonMesh.height() - 2;
  }
  sectionUpButtonMesh.move(context->renderer(), context->pixelSizeX(), context->pixelSizeY(),
                           sectionUpButtonMesh.x(), upButtonY);
  sectionDownButtonMesh.move(context->renderer(), context->pixelSizeX(), context->pixelSizeY(),
                             sectionDownButtonMesh.x(), downButtonY);
}

// -----------------------------------------------------------------------------

struct PageTabSizes final {
  uint32_t tabWidth;
  uint32_t paddingX;
  uint32_t paddingY;
};
static PageTabSizes computePageTabSizes(uint32_t tabCount, uint32_t pageWidth, uint32_t pageHeight) {
  uint32_t availableSpacePerTab = pageWidth / tabCount;
  
  PageTabSizes tabSizes;
  tabSizes.paddingX = (pageWidth >= 800u) ? (uint32_t)(availableSpacePerTab*0.15f) : availableSpacePerTab/10u;
  if (tabSizes.paddingX > Control::maxPageTabPaddingX())
    tabSizes.paddingX = Control::maxPageTabPaddingX();
  tabSizes.paddingY = (pageHeight >= 600u) ? Control::pageTabPaddingY() : (Control::pageTabPaddingY() - 1u);

  availableSpacePerTab -= (tabSizes.paddingX << 1);
  tabSizes.tabWidth = (availableSpacePerTab >= Control::maxPageTabWidth())
                    ? Control::maxPageTabWidth() : availableSpacePerTab;
  return tabSizes;
}

// ---

void MenuFrame::createPageTabs(TabMode mode, uint32_t activeTabIndex, bool forceRegen) {
  if (this->tabMode != mode || forceRegen) { // change tab mode
    const int32_t pageX = (int32_t)sectionTabs.width();
    const uint32_t pageWidth = context->clientWidth() - sectionTabs.width();
    
    switch (mode) {
      case TabMode::general: {
        const char16_t* tabLabels[] {
          localization->getMessage(GeneralSettingsMessages::title),
          localization->getMessage(HotkeyBindingsMessages::title),
          localization->getMessage(OsdSettingsMessages::title)
        };
        constexpr const size_t tabCount = sizeof(tabLabels)/sizeof(*tabLabels);
        auto tabSizes = computePageTabSizes((uint32_t)tabCount, pageWidth, context->clientHeight());
        
        pageTabs = TabControl(*context, pageX, 0, pageWidth, tabSizes.paddingX, tabSizes.paddingY,
                              tabSizes.tabWidth, theme->tabControlColorParams(), tabLabels, tabCount,
                              [this](uint32_t index) {
                                this->pageToLoad = static_cast<PageId>((uint32_t)PageId::generalSettings + index);
                                this->isInvalidated = true;
                              }, activeTabIndex);
        break;
      }
      case TabMode::profile: {
        const char16_t* tabLabels[] {
          localization->getMessage(ProfileSettingsMessages::title),
          localization->getMessage(ScreenStretchingMessages::title),
          localization->getMessage(SmoothingUpscalingMessages::title),
          localization->getMessage(AdvancedEffectsMessages::title)
        };
        constexpr const size_t tabCount = sizeof(tabLabels)/sizeof(*tabLabels);
        auto tabSizes = computePageTabSizes((uint32_t)tabCount, pageWidth, context->clientHeight());
        
        pageTabs = TabControl(*context, pageX, 0, pageWidth, tabSizes.paddingX, tabSizes.paddingY,
                              tabSizes.tabWidth, theme->tabControlColorParams(), tabLabels, tabCount,
                              [this](uint32_t index) {
                                this->pageToLoad = static_cast<PageId>((uint32_t)PageId::profileSettings + index);
                                this->isInvalidated = true;
                              }, activeTabIndex);
        break;
      }
      case TabMode::none:
      default: pageTabs = TabControl{}; break;
    }
    this->tabMode = mode;

    // create controller indicators
    if (mode != TabMode::none) {
      auto prevButtonIcon = context->imageLoader().getIcon(ControlIconType::buttonSmallL2);
      const int32_t buttonY = (int32_t)((pageTabs.height() - prevButtonIcon.height() + 1u) >> 1);
      pagePreviousButton = IconMesh(context->renderer(), prevButtonIcon.texture(), context->pixelSizeX(), context->pixelSizeY(),
                                    (int32_t)(sectionTabs.width() + Control::controlSideMargin()), buttonY,
                                    prevButtonIcon.offsetX(), prevButtonIcon.offsetY(), prevButtonIcon.width(), prevButtonIcon.height());
      auto nextButtonIcon = context->imageLoader().getIcon(ControlIconType::buttonSmallR2);
      pageNextButton = IconMesh(context->renderer(), nextButtonIcon.texture(), context->pixelSizeX(), context->pixelSizeY(),
                                (int32_t)(context->clientWidth() - nextButtonIcon.width() - Control::controlSideMargin()), buttonY,
                                nextButtonIcon.offsetX(), nextButtonIcon.offsetY(), nextButtonIcon.width(), nextButtonIcon.height());
    }
  }
  else // change active tab
    pageTabs.selectIndex(*context, activeTabIndex);
}

void MenuFrame::movePageTabs() {
  pageTabs.move(*context, (int32_t)sectionTabs.width(), 0, context->clientWidth() - sectionTabs.width());

  const int32_t buttonY = (int32_t)((pageTabs.height() - pagePreviousButton.height() + 1u) >> 1);
  pagePreviousButton.move(context->renderer(), context->pixelSizeX(), context->pixelSizeY(),
                          (int32_t)(sectionTabs.width() + Control::controlSideMargin()), buttonY);
  pageNextButton.move(context->renderer(), context->pixelSizeX(), context->pixelSizeY(),
                      (int32_t)(context->clientWidth() - pageNextButton.width() - Control::controlSideMargin()), buttonY);
}

// -----------------------------------------------------------------------------

void MenuFrame::createPage(PageId id, bool isControllerUsed, bool forceRegen) {
  const int32_t pageX = (int32_t)sectionTabs.width();
  const uint32_t pageWidth = context->clientWidth() - sectionTabs.width();
  
  switch (id) {
    // main pages
    case PageId::mainMenu: {
      createPageTabs(TabMode::none, 0, forceRegen);
      activePage = std::make_unique<MainMenu>(context, buffers, theme, *localization,
                                              pageX, 0, pageWidth, context->clientHeight(),
                                              activeProfileId, profiles, onClose);
      break;
    }
    case PageId::profileSelector: {
      createPageTabs(TabMode::none, 0, forceRegen);
      activePage = std::make_unique<ProfileSelector>(context, buffers, theme, *localization,
                                                     pageX, 0, pageWidth, context->clientHeight(),
                                                     activeProfileId, profiles,
                                                     std::bind(&MenuFrame::onProfileSelection,this,std::placeholders::_1,std::placeholders::_2));
      break;
    }
    // general pages
    case PageId::generalSettings: {
      auto onThemeChange = [this]() { // theme color/language change
        createSectionTabs(sectionTabs.activeTabIndex());
        createBackground();
        createPageTabs(TabMode::general, pageTabs.activeTabIndex(), true);
      };
      createPageTabs(TabMode::general, (uint32_t)id - (uint32_t)PageId::generalSettings, forceRegen);
      activePage = std::make_unique<GeneralSettings>(context, buffers, theme, localization, windowMonitor,
                                                     pageX, (int32_t)pageTabs.height(), pageWidth,
                                                     context->clientHeight() - pageTabs.height(),
                                                     std::move(onThemeChange));
      break;
    }
    case PageId::generalHotkeyBindings: {
      createPageTabs(TabMode::general, (uint32_t)id - (uint32_t)PageId::generalSettings, forceRegen);
      activePage = std::make_unique<HotkeyBindings>(context, buffers, *theme, *localization,
                                                    pageX, (int32_t)pageTabs.height(), pageWidth,
                                                    context->clientHeight() - pageTabs.height());
      break;
    }
    case PageId::generalOsdSettings: {
      createPageTabs(TabMode::general, (uint32_t)id - (uint32_t)PageId::generalSettings, forceRegen);
      activePage = std::make_unique<OsdSettings>(context, buffers, *theme, *localization,
                                                 pageX, (int32_t)pageTabs.height(), pageWidth,
                                                 context->clientHeight() - pageTabs.height());
      break;
    }
    // profile pages
    case PageId::profileSettings: {
      createPageTabs(TabMode::profile, (uint32_t)id - (uint32_t)PageId::profileSettings, forceRegen);
      activePage = std::make_unique<ProfileSettings>(context, buffers, theme, *localization,
                                                     pageX, (int32_t)pageTabs.height(), pageWidth,
                                                     context->clientHeight() - pageTabs.height(),
                                                     editedProfileId, profiles, presets);
      break;
    }
    case PageId::profileScreenStretching: {
      createPageTabs(TabMode::profile, (uint32_t)id - (uint32_t)PageId::profileSettings, forceRegen);
      activePage = std::make_unique<ScreenStretching>(context, buffers, *theme, *localization,
                                                      pageX, (int32_t)pageTabs.height(), pageWidth,
                                                      context->clientHeight() - pageTabs.height());
      break;
    }
    case PageId::profileSmoothingUpscaling: {
      createPageTabs(TabMode::profile, (uint32_t)id - (uint32_t)PageId::profileSettings, forceRegen);
      activePage = std::make_unique<SmoothingUpscaling>(context, buffers, *theme, localization,
                                                        pageX, (int32_t)pageTabs.height(), pageWidth,
                                                        context->clientHeight() - pageTabs.height());
      break;
    }
    case PageId::profileAdvancedEffects: {
      createPageTabs(TabMode::profile, (uint32_t)id - (uint32_t)PageId::profileSettings, forceRegen);
      activePage = std::make_unique<AdvancedEffects>(context, buffers, *theme, *localization,
                                                     pageX, (int32_t)pageTabs.height(), pageWidth,
                                                     context->clientHeight() - pageTabs.height());
      break;
    }
    default: break;
  }
  activePage->setControllerUsed(isControllerUsed);
  moveSectionTabs();
  pageToLoad = PageId::none;
}
