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
#pragma once

#include <cstdint>
#include <cstddef>
#include <memory>
#include <array>
#include <functional>
#include <video/window.h>
#include <video/window_events.h>
#include <display/video_api.h>
#include <display/controls/control_mesh.h>
#include <display/controls/icon_mesh.h>
#include "menu/color_theme.h"
#include "menu/config_profile.h"
#include "menu/message_resources.h"
#include "menu/renderer_context.h"
#include "menu/renderer_state_buffers.h"
#include "menu/controls/tab_control.h"
#include "menu/controls/vertical_tab_control.h"
#include "menu/pages/page.h"

namespace menu {
  enum class MenuMode : uint32_t { ///< Menu variation
    configDialog = 0, ///< No main menu, no clock
    gameMenu          ///< Add main menu, add clock
  };
  enum class TabMode : uint32_t { ///< Type of page tabs displayed
    none = 0,
    general,
    profile
  };
  enum class CloseOperation : uint32_t { ///< Type of operation to perform when closing menu
    resume = 0,
    loadState,
    saveState,
    resetGame,
    exitGame
  };

  // ---
  
  /// @brief Menu window content frame -- main container with pipelines and pages
  class MenuFrame final {
  public:
    MenuFrame(MenuMode mode, std::shared_ptr<RendererContext> context,
              pandora::video::Window& window, std::vector<ConfigProfile>&& profiles, uint32_t activeProfileId,
              std::function<void(CloseOperation)> onClose, bool isControllerUsed);
    MenuFrame() = default;
    MenuFrame(const MenuFrame&) = delete;
    MenuFrame(MenuFrame&&) = delete;
    MenuFrame& operator=(const MenuFrame&) = delete;
    MenuFrame& operator=(MenuFrame&&) = delete;
    ~MenuFrame() noexcept;
    
    /// @brief Report window event
    bool onWindowEvent(pandora::video::Window* sender, pandora::video::WindowEvent event,
                       uint32_t status, int32_t posX, int32_t posY, void* data);
    /// @brief Report window position event
    bool onPositionEvent(pandora::video::Window* sender, pandora::video::PositionEvent event,
                         uint32_t sizeX, uint32_t sizeY);
    /// @brief Report keyboard event
    bool onKeyboardEvent(pandora::video::Window* sender, pandora::video::KeyboardEvent event,
                         uint32_t keyCode, uint32_t change);
    /// @brief Report controller event
    bool onControllerEvent(pandora::video::KeyboardEvent event, uint32_t virtualKeyCode,
                           int32_t analogX, int32_t analogY);
    /// @brief Report mouse event
    bool onMouseEvent(pandora::video::Window* sender, pandora::video::MouseEvent event,
                      int32_t x, int32_t y, int32_t index);
    
    void draw(); ///< Draw menu frame

  private:
    void initGraphicsPipelines(pandora::video::Window& window);
    void resizeGraphicsPipelines();
    void createBackground();
    void moveBackground();
    
    void createSectionTabs(uint32_t activeTabIndex);
    void moveSectionTabs();
    void createPageTabs(TabMode mode, uint32_t activeTabIndex, bool forceRegen = false);
    void movePageTabs();
    void createPage(pages::PageId id, bool isControllerUsed);
  
  private:
    std::shared_ptr<RendererContext> context = nullptr;
    std::shared_ptr<RendererStateBuffers> buffers = nullptr;
    std::shared_ptr<menu::ColorTheme> theme = nullptr;
    std::shared_ptr<menu::MessageResources> localization = nullptr;
    
    video_api::SwapChain swapChain;
    video_api::GraphicsPipeline controlPipeline;
    video_api::GraphicsPipeline iconPipeline;
    video_api::GraphicsPipeline labelPipeline;
    video_api::Sampler textureSampler;
    
    display::controls::ControlMesh backgroundMesh;
    display::controls::IconMesh backgroundGradientMesh;
    controls::VerticalTabControl sectionTabs;
    display::controls::IconMesh logoMesh;
    display::controls::IconMesh sectionUpButtonMesh;
    display::controls::IconMesh sectionDownButtonMesh;
    controls::TabControl pageTabs;
    display::controls::IconMesh pagePreviousButton;
    display::controls::IconMesh pageNextButton;
    std::unique_ptr<pages::Page> activePage;
    
    bool isInvalidated = false;
    int32_t mouseX = -1;
    int32_t mouseY = -1;
    TabMode tabMode = TabMode::none;
    pages::PageId pageToLoad = pages::PageId::none;
    MenuMode sectionMode = MenuMode::configDialog;
    
    uint32_t activeProfileId = 0;
    std::vector<ConfigProfile> profiles;
    std::vector<menu::ConfigProfile> presets;
    pandora::hardware::DisplayMonitor::Handle displayMonitor;
    std::function<void(CloseOperation)> onClose;
  };
}
