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
#include <video/window_keycodes.h>
#include <video/window_events.h>
#include "display/image_loader.h"
#include "menu/controls/geometry_generator.h"
#include "menu/controls/key_binding.h"

using namespace pandora::video;
using namespace display;
using namespace display::controls;
using namespace menu::controls;
using namespace menu;

ControlType KeyBinding::type() const noexcept { return ControlType::keyBinding; }


// -- helpers -- ---------------------------------------------------------------

static const char16_t* GetVirtualKeyboardKeyName(uint32_t virtualKeyCode) noexcept {
  switch (virtualKeyCode) {
    case _P_VK_L_CTRL:    return u"CTRL";
    case _P_VK_R_CTRL:    return u"CTRL-R";
    case _P_VK_L_ALT:     return u"ALT";
    case _P_VK_R_ALT:     return u"ALT-R";
    case _P_VK_L_SHIFT:   return u"SHIFT";
    case _P_VK_R_SHIFT:   return u"SHIFT-R";
    case _P_VK_L_SYSTEM:  return u"COMMAND";
    case _P_VK_R_SYSTEM:  return u"COMMAND-R";
    case _P_VK_APP_MENU:  return u"APPMENU";
    case _P_VK_BACKSPACE: return u"BACK";
    case _P_VK_CLEAR:     return u"CLEAR";
    case _P_VK_ENTER:     return u"ENTER";
    case _P_VK_ENTER_PAD: return u"ENTER-R";
    case _P_VK_EXECUTE:   return u"EXECUTE";
    case _P_VK_ESC:       return u"ESC";
    case _P_VK_PAUSE:     return u"PAUSE";
    case _P_VK_PRINT_SCR: return u"PRINTSCR";
    case _P_VK_SPACE:     return u"SPACE";
    case _P_VK_TAB:       return u"TAB";
    case _P_VK_DELETE:    return u"DEL";
    case _P_VK_END:       return u"END";
    case _P_VK_HOME:      return u"HOME";
    case _P_VK_INSERT:    return u"INS";
    case _P_VK_PG_DOWN:   return u"PAGE DOWN";
    case _P_VK_PG_UP:     return u"PAGE UP";
    case _P_VK_ARROW_DOWN:  return u"DOWN";
    case _P_VK_ARROW_LEFT:  return u"LEFT";
    case _P_VK_ARROW_RIGHT: return u"RIGHT";
    case _P_VK_ARROW_UP:    return u"UP";
    case _P_VK_F1:        return u"F1";
    case _P_VK_F2:        return u"F2";
    case _P_VK_F3:        return u"F3";
    case _P_VK_F4:        return u"F4";
    case _P_VK_F5:        return u"F5";
    case _P_VK_F6:        return u"F6";
    case _P_VK_F7:        return u"F7";
    case _P_VK_F8:        return u"F8";
    case _P_VK_F9:        return u"F9";
    case _P_VK_F10:       return u"F10";
    case _P_VK_F11:       return u"F11";
    case _P_VK_F12:       return u"F12";
    case _P_VK_F13:       return u"F13";
    case _P_VK_F14:       return u"F14";
    case _P_VK_NUMPAD_0:  return u"NUMPAD 0";
    case _P_VK_NUMPAD_1:  return u"NUMPAD 1";
    case _P_VK_NUMPAD_2:  return u"NUMPAD 2";
    case _P_VK_NUMPAD_3:  return u"NUMPAD 3";
    case _P_VK_NUMPAD_4:  return u"NUMPAD 4";
    case _P_VK_NUMPAD_5:  return u"NUMPAD 5";
    case _P_VK_NUMPAD_6:  return u"NUMPAD 6";
    case _P_VK_NUMPAD_7:  return u"NUMPAD 7";
    case _P_VK_NUMPAD_8:  return u"NUMPAD 8";
    case _P_VK_NUMPAD_9:  return u"NUMPAD 9";
    case _P_VK_DECIMAL:   return u"NUMPAD ,";
    case _P_VK_SEPARATOR: return u"NUMPAD .";
    case _P_VK_ADD:       return u"NUMPAD +";
    case _P_VK_MULTIPLY:  return u"NUMPAD *";
    case _P_VK_SUBTRACT:  return u"NUMPAD -";
    case _P_VK_DIVIDE:    return u"NUMPAD /";
    case _P_VK_PUNCT_BRACK1: return u"[";
    case _P_VK_PUNCT_BRACK2: return u"]";
    case _P_VK_PUNCT_COMMA:  return u",";
    case _P_VK_PUNCT_PERIOD: return u".";
    case _P_VK_PUNCT_MINUS:  return u"-";
    case _P_VK_PUNCT_PLUS:   return u"+";
    case _P_VK_BACKSLASH:    return u"\\";
    case _P_VK_MEDIA_NEXT: return u"NEXT";
    case _P_VK_MEDIA_PREV: return u"PREV";
    case _P_VK_MEDIA_PLAY: return u"PLAY";
    case _P_VK_MEDIA_STOP: return u"STOP";
    default: return nullptr;
  }
}

static const char16_t* GetMouseKeyName(uint32_t virtualKeyCode) noexcept {
  switch (KeyBinding::fromMouseKeyCode(virtualKeyCode)) {
    case MouseButton::left: return u"MOUSE-L";
    case MouseButton::middle: return u"MOUSE-M";
    case MouseButton::right: return u"MOUSE-R";
    case MouseButton::button4: return u"MOUSE-4";
    case MouseButton::button5: return u"MOUSE-5";
    default: return nullptr;
  }
}

static ControlIcon GetControllerKeyIcon(RendererContext& context, uint32_t virtualKeyCode) noexcept {
  // https://learn.microsoft.com/fr-fr/windows/win32/api/xinput/ns-xinput-xinput_gamepad?redirectedfrom=MSDN
  switch (virtualKeyCode) {
    case /*XINPUT_GAMEPAD_DPAD_UP*/0x0001:    return context.imageLoader().getIcon(ControlIconType::buttonDpadUp);
    case /*XINPUT_GAMEPAD_DPAD_DOWN*/0x0002:  return context.imageLoader().getIcon(ControlIconType::buttonDpadDown);
    case /*XINPUT_GAMEPAD_DPAD_LEFT*/0x0004:  return context.imageLoader().getIcon(ControlIconType::buttonDpadLeft);
    case /*XINPUT_GAMEPAD_DPAD_RIGHT*/0x0008: return context.imageLoader().getIcon(ControlIconType::buttonDpadRight);
    case /*XINPUT_GAMEPAD_START*/0x0010: return context.imageLoader().getIcon(ControlIconType::buttonStart);
    case /*XINPUT_GAMEPAD_BACK*/0x0020:  return context.imageLoader().getIcon(ControlIconType::buttonSelect);
    case /*XINPUT_GAMEPAD_LEFT_THUMB*/0x0040:  return context.imageLoader().getIcon(ControlIconType::buttonL3);
    case /*XINPUT_GAMEPAD_RIGHT_THUMB*/0x0080: return context.imageLoader().getIcon(ControlIconType::buttonR3);
    case /*XINPUT_GAMEPAD_LEFT_SHOULDER*/0x0100:  return context.imageLoader().getIcon(ControlIconType::buttonL1);
    case /*XINPUT_GAMEPAD_RIGHT_SHOULDER*/0x0200: return context.imageLoader().getIcon(ControlIconType::buttonR1);
    case /*bLeftTrigger*/0xFFFFFF00:              return context.imageLoader().getIcon(ControlIconType::buttonL2);
    case /*bRightTrigger*/0xFFFFFF01:             return context.imageLoader().getIcon(ControlIconType::buttonR2);
    case /*XINPUT_GAMEPAD_A*/0x1000: return context.imageLoader().getIcon(ControlIconType::buttonCross);
    case /*XINPUT_GAMEPAD_B*/0x2000: return context.imageLoader().getIcon(ControlIconType::buttonCircle);
    case /*XINPUT_GAMEPAD_X*/0x4000: return context.imageLoader().getIcon(ControlIconType::buttonSquare);
    case /*XINPUT_GAMEPAD_Y*/0x8000: return context.imageLoader().getIcon(ControlIconType::buttonTriangle);
    default: return ControlIcon{};
  }
}


// -- init/resize geometry -- --------------------------------------------------

static inline ControlMesh generateKeyboardKeyMesh(RendererContext& context, const TextMesh& keyLabel, const KeyboardKeyColors& keyColors) {
  const uint32_t totalWidth = keyLabel.width() + (Control::keyboardKeyPaddingX() << 1) + (Control::keyboardKeySideX() << 1);
  const uint32_t totalHeight = keyLabel.height() + (Control::keyboardKeyPaddingY() << 1) + (Control::keyboardKeySideY() << 1) - 1u;

  std::vector<ControlVertex> vertices(static_cast<size_t>(12));
  ControlVertex* vertexIt = vertices.data();
  GeometryGenerator::fillRectangleVertices(vertexIt, keyColors.colors[0], 0.f, (float)totalWidth, 0.f, -(float)totalHeight);
  vertexIt += 4;
  GeometryGenerator::fillRectangleVertices(vertexIt, keyColors.colors[1], 1.f, (float)(totalWidth-1u), -1.f, -(float)(totalHeight-1u));
  ++vertexIt;
  memcpy(vertexIt->color, keyColors.colors[2], sizeof(float)*4u);
  ++vertexIt;
  memcpy(vertexIt->color, keyColors.colors[3], sizeof(float)*4u);
  vertexIt += 2;
  GeometryGenerator::fillControlVertex(*vertexIt,     keyColors.colors[4],
                                       (float)Control::keyboardKeySideX(),                -(float)(Control::keyboardKeySideY()-1u));
  GeometryGenerator::fillControlVertex(*(++vertexIt), keyColors.colors[5],
                                       (float)(totalWidth - Control::keyboardKeySideX()), -(float)(Control::keyboardKeySideY()-1u));
  GeometryGenerator::fillControlVertex(*(++vertexIt), keyColors.colors[6],
                                       (float)Control::keyboardKeySideX(),                -(float)(totalHeight - Control::keyboardKeySideY()));
  GeometryGenerator::fillControlVertex(*(++vertexIt), keyColors.colors[4],
                                       (float)(totalWidth - Control::keyboardKeySideX()), -(float)(totalHeight - Control::keyboardKeySideY()));

  const int32_t x = keyLabel.x() - (int32_t)Control::keyboardKeySideX() - (int32_t)Control::keyboardKeyPaddingX();
  const int32_t y = keyLabel.y() - (int32_t)Control::keyboardKeySideY() - (int32_t)Control::keyboardKeyPaddingY();
  std::vector<uint32_t> indices{ 0,1,2,2,1,3,  4,5,6,6,5,7, 8,9,10,10,9,11 };
  return ControlMesh(context.renderer(), std::move(vertices), indices, context.pixelSizeX(), context.pixelSizeY(),
                     x, y, totalWidth, totalHeight);
}

static void resizeKeyboardKeyMesh(RendererContext& context, const TextMesh& keyLabel, ControlMesh& keyboardMesh) {
  const uint32_t oldLabelWidth = keyboardMesh.width() - (Control::keyboardKeyPaddingX() << 1) - (Control::keyboardKeySideX() << 1);
  const int32_t offsetX = (int32_t)keyLabel.width() - (int32_t)oldLabelWidth;

  std::vector<ControlVertex> vertices = keyboardMesh.relativeVertices();
  const auto* vertexEnd = vertices.data() + (intptr_t)vertices.size();
  for (ControlVertex* vertexIt = vertices.data() + 1; vertexIt < vertexEnd; vertexIt += 2) {
    const int32_t x = static_cast<int32_t>(vertexIt->position[0] + 0.5f) + offsetX;
    vertexIt->position[0] = (float)x;
  }
  const int32_t x = keyLabel.x() - (int32_t)Control::keyboardKeySideX() - (int32_t)Control::keyboardKeyPaddingX();
  const uint32_t width = keyLabel.width() + (Control::keyboardKeyPaddingX() << 1) + (Control::keyboardKeySideX() << 1);
  keyboardMesh.update(context.renderer(), std::move(vertices), context.pixelSizeX(), context.pixelSizeY(),
                      x, keyboardMesh.y(), width, keyboardMesh.height());
}

// ---

void KeyBinding::init(RendererContext& context, const char16_t* label, int32_t x, int32_t labelY,
                      uint32_t fixedWidth, const float color[4], const KeyboardKeyColors& keyColors, bool isUnbindable) {
  auto& inputFont = context.getFont(FontType::inputText);
  auto& labelFont = context.getFont(FontType::labels);
  uint32_t height = inputFont.XHeight() + ((Control::keyboardKeySideY() + Control::keyboardKeyPaddingY()) << 1);
  if (height & 0x1)
    ++height;

  // create label
  labelMesh = TextMesh(context.renderer(), labelFont, label, context.pixelSizeX(), context.pixelSizeY(), x, labelY);
  uint32_t labelWidthWithMargin = (minLabelWidth >= labelMesh.width()) ? minLabelWidth : labelMesh.width();
  if (labelWidthWithMargin)
    labelWidthWithMargin += labelMargin();

  // create background + border
  const float colorDarker[4]{ color[0]*0.9f, color[1]*0.9f, color[2]*0.9f, color[3] };
  std::vector<ControlVertex> vertices((bindingType == KeyBindingType::both) ? static_cast<size_t>(26) : static_cast<size_t>(22));
  ControlVertex* vertexIt = vertices.data();
  GeometryGenerator::fillControlVertex(*vertexIt,     colorDarker, 0.f,               0.f); // background (gradient from top)
  GeometryGenerator::fillControlVertex(*(++vertexIt), colorDarker, (float)fixedWidth, 0.f);
  GeometryGenerator::fillRectangleVertices(++vertexIt, color, 0.f, (float)fixedWidth, -(float)((height<<1)/3), -(float)height);
  vertexIt += 4;

  const float borderFactor = (color[0] + color[1] + color[2] >= 1.5f) ? 0.7f : 1.3f;
  float colorBorder[4]{ color[0]*borderFactor, color[1]*borderFactor, color[2]*borderFactor, color[3] };
  GeometryGenerator::fillRectangleVertices(vertexIt, colorBorder, 0.f, (float)fixedWidth, 0.f, -1.f); // border top
  vertexIt += 4;
  GeometryGenerator::fillRectangleVertices(vertexIt, colorBorder, 0.f, (float)fixedWidth, // border bottom
                                           -(float)(height-1), -(float)height);
  vertexIt += 4;
  GeometryGenerator::fillRectangleVertices(vertexIt, colorBorder, 0.f, 1.f, -1.f, -(float)(height-1)); // border left
  vertexIt += 4;
  GeometryGenerator::fillRectangleVertices(vertexIt, colorBorder, (float)(fixedWidth-1), (float)fixedWidth, // border right
                                           -1.f, -(float)(height-1));
  std::vector<uint32_t> indices;
  if (bindingType == KeyBindingType::both) { // central vertical line (if two values)
    colorBorder[3] *= 0.65f;
    vertexIt += 4;
    GeometryGenerator::fillRectangleVertices(vertexIt, colorBorder, (float)(fixedWidth >> 1),
                                             (float)((fixedWidth >> 1) + 1u), -2.f, -(float)(height-2));
    indices = { 0,1,2,2,1,3,       2,3,4,4,3,5,       6,7,8,8,7,9,
                10,11,12,12,11,13, 14,15,16,16,15,17, 18,19,20,20,19,21, 22,23,24,24,23,25 };
  }
  else
    indices = { 0,1,2,2,1,3,       2,3,4,4,3,5,       6,7,8,8,7,9,
                10,11,12,12,11,13, 14,15,16,16,15,17, 18,19,20,20,19,21 };

  const int32_t boxX = x + (int32_t)labelWidthWithMargin;
  const int32_t boxY = labelY - ((int32_t)height - (int32_t)labelFont.XHeight() + 3)/2;
  controlMesh = ControlMesh(context.renderer(), std::move(vertices), indices, context.pixelSizeX(), context.pixelSizeY(),
                            boxX, boxY, fixedWidth, height);

  // create unbind button
  if (isUnbindable) {
    vertices = std::vector<ControlVertex>(static_cast<size_t>(16));
    vertexIt = vertices.data();
    GeometryGenerator::fillRectangleVertices(vertexIt, color, 1.f, (float)(height - 1u), -1.f, -(float)(height - 1u)); // background
    vertexIt += 4;
    GeometryGenerator::fillRectangleVertices(vertexIt, colorBorder, 0.f, 1.f, -1.f, -(float)(height - 1u)); // border left
    vertexIt += 4;
    const float crossFactor = borderFactor*borderFactor;
    float colorCross[4]{ color[0]*crossFactor, color[1]*crossFactor, color[2]*crossFactor, 1.f };
    const uint32_t crossOffset = (height >> 2) + 2u;
    GeometryGenerator::fillCrossVertices(vertexIt, colorCross, (float)crossOffset, (float)(height - crossOffset),
                                         -(float)crossOffset, -(float)(height - crossOffset));

    indices = { 0,1,2,2,1,3,  4,5,6,6,5,7,  8,9,10,10,9,11,  12,13,14,14,13,15 };
    unbindMesh = ControlMesh(context.renderer(), std::move(vertices), indices, context.pixelSizeX(), context.pixelSizeY(),
                             boxX + (int32_t)fixedWidth - (int32_t)height, boxY, height - 1u, height - 2u);
  }

  // create binding value(s)
  int32_t inputX = (bindingType == KeyBindingType::both) ? boxX + (int32_t)(fixedWidth >> 2) : boxX + (int32_t)(fixedWidth >> 1);
  if ((uint32_t)bindingType & (uint32_t)KeyBindingType::keyboard) {
    char16_t buffer[2]{ u'\0' };
    const char16_t* value = nullptr;
    if (keyboardValue_ != emptyKeyValue()) {
      value = (keyboardValue_ < leftMouseKey()) ? GetVirtualKeyboardKeyName(keyboardValue_) : GetMouseKeyName(keyboardValue_);
      if (value == nullptr) {
        *buffer = (char16_t)pandora::video::virtualKeyToChar(keyboardValue_);
        if (*buffer != (char16_t)0) {
          if (*buffer >= u'a' && *buffer <= u'z')
            *buffer -= (char16_t)0x20;
          value = buffer;
        }
        else
          keyboardValue_ = emptyKeyValue();
      }
    }
    keyboardValueMesh = TextMesh(context.renderer(), inputFont, value, context.pixelSizeX(), context.pixelSizeY(),
                                 inputX, boxY + (int32_t)Control::keyboardKeySideY() + (int32_t)Control::keyboardKeyPaddingY() + 1,
                                 TextAlignment::center);
    keyboardMesh = generateKeyboardKeyMesh(context, keyboardValueMesh, keyColors);
    inputX += (int32_t)(fixedWidth >> 1);
  }
  if ((uint32_t)bindingType & (uint32_t)KeyBindingType::controller) {
    ControlIcon icon;
    if (controllerValue_ != emptyKeyValue()) {
      icon = GetControllerKeyIcon(context, controllerValue_);
      if (icon.texture() == nullptr)
        controllerValue_ = emptyKeyValue();
    }
    controllerValueMesh = IconMesh(context.renderer(), icon.texture(), context.pixelSizeX(), context.pixelSizeY(),
                                   inputX - (int32_t)(icon.width() >> 1), boxY + ((int32_t)height - (int32_t)icon.height())/2,
                                   icon.offsetX(), icon.offsetY(), icon.width(), icon.height());
  }
}

// ---

void KeyBinding::move(RendererContext& context, int32_t x, int32_t labelY) {
  const int32_t boxY = labelY - ((int32_t)controlMesh.height() - (int32_t)labelMesh.height() + 3)/2;

  labelMesh.move(context.renderer(), context.pixelSizeX(), context.pixelSizeY(), x, labelY);
  uint32_t labelWidthWithMargin = (minLabelWidth >= labelMesh.width()) ? minLabelWidth : labelMesh.width();
  if (labelWidthWithMargin)
    labelWidthWithMargin += labelMargin();

  const int32_t boxX = x + (int32_t)labelWidthWithMargin;
  controlMesh.move(context.renderer(), context.pixelSizeX(), context.pixelSizeY(), boxX, boxY);
  if (unbindMesh.width()) {
    unbindMesh.move(context.renderer(), context.pixelSizeX(), context.pixelSizeY(),
                    boxX + (int32_t)controlMesh.width() - (int32_t)controlMesh.height(), boxY);
  }

  int32_t inputX = (bindingType == KeyBindingType::both) ? boxX + (int32_t)(controlMesh.width() >> 2)
                                                         : boxX + (int32_t)(controlMesh.width() >> 1);
  if ((uint32_t)bindingType & (uint32_t)KeyBindingType::keyboard) {
    keyboardValueMesh.move(context.renderer(), context.pixelSizeX(), context.pixelSizeY(),
                      inputX - (int32_t)(keyboardValueMesh.width() >> 1),
                      boxY + (int32_t)Control::keyboardKeySideY() + (int32_t)Control::keyboardKeyPaddingY() + 1);
    keyboardMesh.move(context.renderer(), context.pixelSizeX(), context.pixelSizeY(),
                      keyboardValueMesh.x() - (int32_t)Control::keyboardKeySideX() - (int32_t)Control::keyboardKeyPaddingX(),
                      keyboardValueMesh.y() - (int32_t)Control::keyboardKeySideY() - (int32_t)Control::keyboardKeyPaddingY());
    inputX += (int32_t)(controlMesh.width() >> 1);
  }
  if ((uint32_t)bindingType & (uint32_t)KeyBindingType::controller) {
    controllerValueMesh.move(context.renderer(), context.pixelSizeX(), context.pixelSizeY(),
                             inputX - (int32_t)(controllerValueMesh.width() >> 1),
                             boxY + ((int32_t)controlMesh.height() - (int32_t)controllerValueMesh.height())/2);
  }
}


// -- accessors -- -------------------------------------------------------------

ControlStatus KeyBinding::getStatus(int32_t mouseX, int32_t mouseY) const noexcept {
  return isEnabled() ? (isHover(mouseX, mouseY) ? ControlStatus::hover : ControlStatus::regular) : ControlStatus::disabled;
}


// -- operations -- ------------------------------------------------------------

bool KeyBinding::click(RendererContext& context, int32_t mouseX, int32_t) {
  if (isEnabled()) {
    if (!isEditing) {
      if (unbindMesh.width() && mouseX >= unbindMesh.x() && mouseX < unbindMesh.x() + (int32_t)unbindMesh.width()
      && (keyboardValue_ != emptyKeyValue() || controllerValue_ != emptyKeyValue())) { // unbind button
        keyboardValue_ = emptyKeyValue();
        controllerValue_ = emptyKeyValue();
      }
      else // normal click -> set editing mode
        isEditing = true;
    }
    else {
      isEditing = false;
      if (mouseX != noMouseCoord() && (uint32_t)bindingType & (uint32_t)KeyBindingType::keyboard && keyboardValue_ != leftMouseKey()) {
        const int32_t inputX = (bindingType == KeyBindingType::both) ? controlMesh.x() + (int32_t)(controlMesh.width() >> 2)
                                                                     : controlMesh.x() + (int32_t)(controlMesh.width() >> 1);
        keyboardValue_ = leftMouseKey();
        keyboardValueMesh = TextMesh(context.renderer(), context.getFont(FontType::inputText),
                                     GetMouseKeyName(leftMouseKey()), context.pixelSizeX(), context.pixelSizeY(),
                                     inputX, keyboardValueMesh.y(), TextAlignment::center);
        resizeKeyboardKeyMesh(context, keyboardValueMesh, keyboardMesh);
      }
    }
  }
  else // close
    isEditing = false;
  return isEditing;
}

// ---

bool KeyBinding::setKeyboardValue(RendererContext& context, uint32_t virtualKeyCode) {
  if ((uint32_t)bindingType & (uint32_t)KeyBindingType::keyboard) {
    char16_t buffer[2]{ u'\0' };
    const char16_t* value = nullptr;
    if (virtualKeyCode != emptyKeyValue()) {
      value = (virtualKeyCode < leftMouseKey()) ? GetVirtualKeyboardKeyName(virtualKeyCode) : GetMouseKeyName(virtualKeyCode);
      if (value == nullptr) {
        *buffer = (char16_t)pandora::video::virtualKeyToChar(virtualKeyCode);
        if (*buffer == (char16_t)0)
          return isEditing; // unknown code -> ignore it (don't change value, don't exit edit mode)
        if (*buffer >= u'a' && *buffer <= u'z')
          *buffer -= (char16_t)0x20;
        value = buffer;
      }
      if (keyboardValue_ != virtualKeyCode) {
        const int32_t inputX = (bindingType == KeyBindingType::both) ? controlMesh.x() + (int32_t)(controlMesh.width() >> 2)
                                                                     : controlMesh.x() + (int32_t)(controlMesh.width() >> 1);
        keyboardValue_ = virtualKeyCode;
        keyboardValueMesh = TextMesh(context.renderer(), context.getFont(FontType::inputText), value, context.pixelSizeX(), context.pixelSizeY(),
                                     inputX, keyboardValueMesh.y(), TextAlignment::center);
        resizeKeyboardKeyMesh(context, keyboardValueMesh, keyboardMesh);
      }
    }
    else if (unbindMesh.width() != 0) // only use empty value if control is unbindable
      keyboardValue_ = virtualKeyCode;
  }
  isEditing = false;
  return false;
}

bool KeyBinding::setControllerValue(RendererContext& context, uint32_t virtualKeyCode) {
  if ((uint32_t)bindingType & (uint32_t)KeyBindingType::controller) {
    if (virtualKeyCode != emptyKeyValue()) {
      ControlIcon icon = GetControllerKeyIcon(context, controllerValue_);
      if (icon.texture() == nullptr)
        return isEditing; // unknown code -> ignore it (don't change value, don't exit edit mode)

      if (controllerValue_ != virtualKeyCode) {
        controllerValue_ = virtualKeyCode;
        controllerValueMesh = IconMesh(context.renderer(), icon.texture(), context.pixelSizeX(), context.pixelSizeY(),
                                       controllerValueMesh.x(), controllerValueMesh.y(),
                                       icon.offsetX(), icon.offsetY(), icon.width(), icon.height());
      }
    }
    else if (unbindMesh.width() != 0) // only use empty value if control is unbindable
      controllerValue_ = virtualKeyCode;
  }
  isEditing = false;
  return false;
}

void KeyBinding::close() {
  isEditing = false;
}


// -- rendering -- -------------------------------------------------------------

void KeyBinding::drawBackground(RendererContext& context, int32_t mouseX, RendererStateBuffers& buffers, bool isActive) {
  auto& renderer = context.renderer();
  if (isEnabled()) {
    auto bufferType = isEditing ? ControlBufferType::active // if not hover, draw semi-transparent -> use "disabled" buffer
                                : (isActive ? ControlBufferType::regular : ControlBufferType::disabled);
    buffers.bindControlBuffer(renderer, bufferType);
    controlMesh.draw(renderer);

    if (keyboardValue_ != emptyKeyValue()) {
      buffers.bindControlBuffer(renderer, ControlBufferType::regular);
      keyboardMesh.draw(renderer);
    }
    if (unbindMesh.width() && isActive && !isEditing && (keyboardValue_ != emptyKeyValue() || controllerValue_ != emptyKeyValue())) {
      if (mouseX >= unbindMesh.x() && mouseX < unbindMesh.x() + (int32_t)unbindMesh.width())
        buffers.bindControlBuffer(renderer, ControlBufferType::active);
      unbindMesh.draw(renderer);
    }
  }
  else { // disabled -> no background
    if (keyboardValue_ != emptyKeyValue()) {
      buffers.bindControlBuffer(renderer, ControlBufferType::disabled);
      keyboardMesh.draw(renderer);
    }
  }
}

void KeyBinding::drawIcon(RendererContext& context, RendererStateBuffers& buffers) {
  if (controllerValue_ != emptyKeyValue()) {
    buffers.bindIconBuffer(context.renderer(), isEnabled() ? ControlBufferType::regular : ControlBufferType::disabled);
    controllerValueMesh.draw(context.renderer());
  }
}

void KeyBinding::drawLabels(RendererContext& context, RendererStateBuffers& buffers, bool isActive) {
  auto& renderer = context.renderer();
  LabelBufferType labelBuffer, inputBuffer;
  if (isEnabled()) {
    labelBuffer = isActive ? LabelBufferType::active : LabelBufferType::regular;
    inputBuffer = LabelBufferType::keyboardKey;
  }
  else {
    labelBuffer = LabelBufferType::disabled;
    inputBuffer = LabelBufferType::keyboardKeyDisabled;
  }

  if (labelMesh.width()) {
    buffers.bindLabelBuffer(renderer, labelBuffer);
    labelMesh.draw(renderer);
  }
  if (keyboardValue_ != emptyKeyValue()) {
    buffers.bindLabelBuffer(renderer, inputBuffer);
    keyboardValueMesh.draw(renderer);
  }
}
