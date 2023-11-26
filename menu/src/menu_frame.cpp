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
                     std::function<void(CloseOperation)> onClose, bool isControllerUsed)
  : context(std::move(context_)),
    theme(std::make_shared<menu::ColorTheme>(menu::ColorThemeType::blue)),
    localization(std::make_shared<menu::MessageResources>(menu::LocalizationType::en)),
    sectionMode(mode),
    activeProfileId(activeProfileId),
    profiles(std::move(profiles)),
    displayMonitor(window.displayMonitor().handle()),
    onClose(std::move(onClose)) {
  assert(this->context != nullptr);
  initGraphicsPipelines(window);

  // load first page
  createSectionTabs(0);
  createBackground(); // uses section tab width -> done after
  createPage(PageId::mainMenu, isControllerUsed);
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
  sectionTabs.release();
  pageTabs.release();
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
      displayMonitor = sender->displayMonitor().handle();
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
      if (context->onSizeChange(clientArea.width, clientArea.height)) { // size changed
        resizeGraphicsPipelines();
        
        if (sectionTabs.width() != Control::sectionTabWidth(context->clientWidth()) + 1u) // section tab width changed -> rebuild
          createSectionTabs(sectionTabs.activeTabIndex());
        else { // simple resize
          sectionTabs.move(*context, 0, 0, clientArea.height - activePage->tooltipHeight());
          logoMesh.move(context->renderer(), context->pixelSizeX(), context->pixelSizeY(),
                        (sectionTabs.width() + 1u - logoMesh.width()) >> 1, (logoAreaHeight + 1u - logoMesh.height()) >> 1);
        }
        moveBackground(); // uses section tab width -> done after

        const int32_t pageX = (int32_t)sectionTabs.width();
        if (pageTabs.width()) {
          pageTabs.move(*context, pageX, 0, clientArea.width);
        }
        activePage->move(pageX, (int32_t)pageTabs.height(), 
                         clientArea.width - sectionTabs.width(),
                         clientArea.height - pageTabs.height());
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

bool MenuFrame::onMouseEvent(Window*, MouseEvent event, int32_t x, int32_t y, int32_t index) {
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
          pageTabs.click(*context, x);
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

void MenuFrame::draw() {
  if (isInvalidated) {
    // process page change here (not in handlers: we don't want to destroy an instance in use)
    if (pageToLoad != PageId::none)
      createPage(pageToLoad, activePage->isControllerUsed());
    
    auto& renderer = context->renderer();
    renderer.setFragmentSamplerStates(0, textureSampler.handlePtr(), 1);
    renderer.setActiveRenderTarget(swapChain.getRenderTargetView());
    
    // draw page controls - backgrounds
    renderer.bindGraphicsPipeline(controlPipeline.handle());

    buffers->bindFixedLocationBuffer(renderer, ScissorRectangle(0, 0, context->clientWidth(), context->clientHeight()));
    buffers->bindControlBuffer(renderer, ControlBufferType::regular);
    backgroundMesh.draw(renderer);
    sectionTabs.drawBackground(*context, *buffers);
    if (pageTabs.width())
      pageTabs.drawBackground(*context, *buffers);

    bool drawForeground = activePage->drawBackgrounds(mouseX, mouseY);
    
    // draw page controls - icons
    renderer.bindGraphicsPipeline(iconPipeline.handle());

    buffers->bindFixedLocationBuffer(renderer, ScissorRectangle(0, 0, context->clientWidth(), context->clientHeight()));
    buffers->bindIconBuffer(renderer, ControlBufferType::regular);
    logoMesh.draw(context->renderer());
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
  buffers = std::make_shared<menu::RendererStateBuffers>(renderer, *theme);
  
  // framebuffers (swap-chain)
  const uint32_t width = context->clientWidth();
  const uint32_t height = context->clientHeight();
  swapChain = SwapChain(DisplaySurface(renderer, window.handle()),
                        SwapChainDescriptor(width, height, 2u, PresentMode::fifo),
                        DataFormat::rgba8_sRGB);
  
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
  TextureWrap textureWrapUVW[]{ TextureWrap::repeat, TextureWrap::repeat, TextureWrap::repeat };
  textureSampler = samplerBuilder.createSampler(SamplerParams(TextureFilter::linear, TextureFilter::linear, TextureFilter::nearest,
                                                       textureWrapUVW, SamplerParams::highestLod(), SamplerParams::infiniteLod()));
  //uint32_t sampleCount = renderer->isColorSampleCountAvailable(DataFormat::rgba8_sRGB, 4) ? 4 : 1;
}

void MenuFrame::resizeGraphicsPipelines() {
  const uint32_t width = context->clientWidth();
  const uint32_t height = context->clientHeight();
  swapChain.resize(width, height);

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
  const uint32_t pageTabHeight = context->getFont(FontType::labels).XHeight()
                               + (Control::pageTabPaddingY() << 1) + 3u;
  
  std::vector<ControlVertex> vertices(static_cast<size_t>(13 + 5));
  GeometryGenerator::fillRadialGradientRectangleVertices(vertices.data() + 5, theme->backgroundColor(),
                                                         theme->backgroundGradientColor(),
                                                         (float)sectionTabs.width(), (float)width,
                                                         -(float)pageTabHeight, -(float)height);
  
  const float* topLeftColor = vertices[5].color;
  const float* bottomLeftColor = vertices[vertices.size() - 3].color;
  GeometryGenerator::fillControlVertex(vertices[0], bottomLeftColor, (float)sectionTabs.width(), -(float)height);
  GeometryGenerator::fillControlVertex(vertices[1], bottomLeftColor, 0.f,                        -(float)height);
  GeometryGenerator::fillControlVertex(vertices[2], topLeftColor,    (float)sectionTabs.width(), -(float)(height >> 1));
  GeometryGenerator::fillControlVertex(vertices[3], topLeftColor,    0.f,                        0.f);
  GeometryGenerator::fillControlVertex(vertices[4], topLeftColor,    (float)width,               0.f);
  
  std::vector<uint32_t> indices{ 0,1,2,1,3,2,2,3,5,  3,6,5,3,4,6,4,7,6,
                                 5,6,8,5,8,10,      6,7,9,7,11,9,      8,6,12,6,9,12,     8,12,10,9,11,12,
                                 10,12,13,12,16,13, 10,13,15,13,16,15, 12,11,14,12,14,16, 11,17,14,14,17,16 };
  backgroundMesh = ControlMesh(context->renderer(), std::move(vertices), indices,
                               context->pixelSizeX(), context->pixelSizeY(), 0, 0, width, height);
}

void MenuFrame::moveBackground() {
  const uint32_t width = context->clientWidth();
  const uint32_t height = context->clientHeight();
  const uint32_t pageTabHeight = context->getFont(FontType::labels).XHeight()
                               + (Control::pageTabPaddingY() << 1) + 3u;
  
  auto vertices = backgroundMesh.relativeVertices();
  GeometryGenerator::moveRadialGradientRectangleVertices(vertices.data() + 5,
                                                         (float)sectionTabs.width(), (float)width,
                                                         -(float)pageTabHeight, -(float)height);
  vertices[0].position[0] = (float)sectionTabs.width();
  vertices[0].position[1] = -(float)height;
  vertices[1].position[1] = -(float)height;
  vertices[2].position[0] = (float)sectionTabs.width();
  vertices[2].position[1] = -(float)(height >> 1);
  vertices[4].position[0] = (float)width;
  
  backgroundMesh.update(context->renderer(), std::move(vertices), context->pixelSizeX(),
                        context->pixelSizeY(), 0, 0, width, height);
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
        case 0: this->pageToLoad = PageId::generalSettings; break;
        case 1: this->pageToLoad = PageId::profileSelector; break;
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
        case 0: this->pageToLoad = PageId::mainMenu; break;
        case 1: this->pageToLoad = PageId::profileSelector; break;
        case 2: this->pageToLoad = PageId::profileSettings; break;
        case 3: this->pageToLoad = PageId::generalSettings; break;
        default: break;
      }
    };
  }
  
  uint32_t tabBarHeight = context->clientHeight();
  if (activePage != nullptr)
    tabBarHeight -= activePage->tooltipHeight();
  const uint32_t sectionTabWidth = Control::sectionTabWidth(context->clientWidth());
  sectionTabs = VerticalTabControl(*context, 0, 0, sectionTabWidth, tabBarHeight, 9, logoAreaHeight,
                                   theme->verticalTabControlColor(), theme->verticalTabBorderColor(),
                                   sectionTabOptions, sectionTabCount, activeTabIndex, std::move(sectionChangeHandler));
  

  const uint32_t logoHeight = context->pandoraLogoImage()->height() / (uint32_t)ColorThemeType::COUNT;
  uint32_t logoWidth = (context->pandoraLogoImage()->rowBytes() >> 2);
  if (sectionTabs.width() < Control::sectionWideTabWidth())
    logoWidth -= 25;
  const uint32_t logoX = (sectionTabs.width() + 1u - logoWidth) >> 1;
  const uint32_t logoY = (logoAreaHeight + 1u - logoHeight) >> 1;

  logoMesh = IconMesh(context->renderer(), context->pandoraLogoImage(),
                      context->pixelSizeX(), context->pixelSizeY(), logoX, logoY, 0,
                      logoHeight*(uint32_t)theme->themeType(), logoWidth, logoHeight);
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
  tabSizes.paddingY = (pageHeight >= 600u) ? Control::pageTabPaddingY() : (Control::pageTabPaddingY() - 2u);

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
  }
  else // change active tab
    pageTabs.selectIndex(*context, activeTabIndex);
}

// -----------------------------------------------------------------------------

void MenuFrame::createPage(PageId id, bool isControllerUsed) {
  const int32_t pageX = (int32_t)sectionTabs.width();
  const uint32_t pageWidth = context->clientWidth() - sectionTabs.width();
  
  switch (id) {
    // main pages
    case PageId::mainMenu: {
      createPageTabs(TabMode::none, 0);
      activePage = std::make_unique<MainMenu>(context, buffers, theme, *localization,
                                              pageX, 0, pageWidth, context->clientHeight(),
                                              activeProfileId, profiles);
      break;
    }
    case PageId::profileSelector: {
      createPageTabs(TabMode::none, 0);
      activePage = std::make_unique<ProfileSelector>(context, buffers, theme, *localization,
                                                     pageX, 0, pageWidth, context->clientHeight(),
                                                     activeProfileId, profiles);
      break;
    }
    // general pages
    case PageId::generalSettings: {
      auto onThemeChange = [this]() { // theme color/language change
        createSectionTabs(sectionTabs.activeTabIndex());
        createBackground();
        createPageTabs(TabMode::general, pageTabs.activeTabIndex(), true);
      };
      createPageTabs(TabMode::general, (uint32_t)id - (uint32_t)PageId::generalSettings);
      activePage = std::make_unique<GeneralSettings>(context, buffers, theme, localization,
                                                     DisplayMonitor(displayMonitor, true),
                                                     pageX, (int32_t)pageTabs.height(), pageWidth,
                                                     context->clientHeight() - pageTabs.height(),
                                                     std::move(onThemeChange));
      break;
    }
    case PageId::generalHotkeyBindings: {
      createPageTabs(TabMode::general, (uint32_t)id - (uint32_t)PageId::generalSettings);
      activePage = std::make_unique<HotkeyBindings>(context, buffers, *theme, *localization,
                                                    pageX, (int32_t)pageTabs.height(), pageWidth,
                                                    context->clientHeight() - pageTabs.height());
      break;
    }
    case PageId::generalOsdSettings: {
      createPageTabs(TabMode::general, (uint32_t)id - (uint32_t)PageId::generalSettings);
      activePage = std::make_unique<OsdSettings>(context, buffers, *theme, *localization,
                                                 pageX, (int32_t)pageTabs.height(), pageWidth,
                                                 context->clientHeight() - pageTabs.height());
      break;
    }
    // profile pages
    case PageId::profileSettings: {
      createPageTabs(TabMode::profile, (uint32_t)id - (uint32_t)PageId::profileSettings);
      activePage = std::make_unique<ProfileSettings>(context, buffers, theme, *localization,
                                                     pageX, (int32_t)pageTabs.height(), pageWidth,
                                                     context->clientHeight() - pageTabs.height(),
                                                     activeProfileId, profiles, presets);
      break;
    }
    case PageId::profileScreenStretching: {
      createPageTabs(TabMode::profile, (uint32_t)id - (uint32_t)PageId::profileSettings);
      activePage = std::make_unique<ScreenStretching>(context, buffers, *theme, *localization,
                                                      pageX, (int32_t)pageTabs.height(), pageWidth,
                                                      context->clientHeight() - pageTabs.height());
      break;
    }
    case PageId::profileSmoothingUpscaling: {
      createPageTabs(TabMode::profile, (uint32_t)id - (uint32_t)PageId::profileSettings);
      activePage = std::make_unique<SmoothingUpscaling>(context, buffers, *theme, localization,
                                                        pageX, (int32_t)pageTabs.height(), pageWidth,
                                                        context->clientHeight() - pageTabs.height());
      break;
    }
    case PageId::profileAdvancedEffects: {
      createPageTabs(TabMode::profile, (uint32_t)id - (uint32_t)PageId::profileSettings);
      activePage = std::make_unique<AdvancedEffects>(context, buffers, *theme, *localization,
                                                     pageX, (int32_t)pageTabs.height(), pageWidth,
                                                     context->clientHeight() - pageTabs.height());
      break;
    }
    default: break;
  }
  activePage->setControllerUsed(isControllerUsed);
  sectionTabs.move(*context, context->clientHeight() - activePage->tooltipHeight());
  pageToLoad = PageId::none;
}
