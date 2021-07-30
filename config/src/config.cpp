/*******************************************************************************
Pandora GS - PSEmu-compatible GPU driver
Copyright (C) 2021  Romain Vinders

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation, version 2
of the License.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details (LICENSE file).
*******************************************************************************/
#include <cstring>
#include <video/window_keycodes.h>
#include "config/config.h"

using namespace config;


// -- config --

void ActionsConfig::initDefaultMapping() noexcept {
  keyboardMapping[keyboardMap::showHideMenu()] = _P_VK_GRAVE_EXP;
  keyboardMapping[keyboardMap::pause()] = _P_VK_PAUSE;
  keyboardMapping[keyboardMap::turbo()] = _P_VK_PG_UP;
  keyboardMapping[keyboardMap::toggleSlowMotion()] = _P_VK_PG_DOWN;
  keyboardMapping[keyboardMap::toggleVsync()] = _P_VK_F11;
  keyboardMapping[keyboardMap::toggleWindowMode()] = _P_VK_F12;
  keyboardMapping[keyboardMap::toggleWideSource()] = _P_VK_DELETE;
  keyboardMapping[keyboardMap::toggleOSD()] = _P_VK_INSERT;
  memset(controllerMapping, disabledKey(), sizeof(*controllerMapping)*controllerMap::length());
}


// -- other implementation files -- --------------------------------------------

#include "./types.hpp" // include -> single object file (less overhead + smaller lib size)
