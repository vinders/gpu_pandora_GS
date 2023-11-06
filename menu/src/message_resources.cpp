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
#include "menu/message_resources.h"

using namespace menu;


void MessageResources::updateLocalization(LocalizationType language) noexcept {
  this->language_ = language;
  switch (language) {
    case LocalizationType::en: fillMessagesEN(); break;
    case LocalizationType::fr: fillMessagesFR(); break;
    case LocalizationType::de: fillMessagesDE(); break;
    case LocalizationType::es: fillMessagesES(); break;
    default: fillMessagesEN(); break;
  }
}

const char32_t* LocalizationTypeHelper::toLanguageName(LocalizationType type) noexcept {
  switch (type) {
    case LocalizationType::en: return U"English";
    case LocalizationType::fr: return U"Français";
    case LocalizationType::de: return U"Deutsch";
    case LocalizationType::es: return U"Español";
    default: return U"Unknown";
  }
}

// ---

void MessageResources::fillMessagesEN() {
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::title) = U"General settings";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::windowGroup) = U"Window settings"; // ---
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::displayMode) = U"Display mode";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::displayMode_fullscreen) = U"Fullscreen";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::displayMode_borderless) = U"Borderless";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::displayMode_window) = U"Window mode";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::displayMode_tooltip) = U"Modes: fullscreen / borderless (fullscreen window) / window (resizable)";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::resolution) = U"Fullscreen resolution";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::resolution_tooltip) = U"Display resolution (pixels) in fullscreen mode";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::refreshRate_tooltip) = U"Display refresh rate (Hz) in fullscreen mode";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::windowSize) = U"Window size";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::windowSize_tooltip) = U"Display area height (pixels) in window mode";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::emulatorGroup) = U"Emulator compatibility"; // ---
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::precision) = U"Pixel precision";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::precision_original) = U"Original (integers)";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::precision_subprecision) = U"Subprecision (smoother)";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::precision_tooltip) = U"Anti-jitter framebuffer subprecision (if GTE-subprecision is enabled in emulator)";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::widescreen) = U"Widescreen hack";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::widescreen_tooltip) = U"Treat framebuffer coords as 16:9 (needed if widescreen hack is enabled in emulator)";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::autosaveOnExit) = U"Autosave on exit";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::autosaveOnExit_tooltip) = U"Automatically save the game state in the active slot when the emulator is closed";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::rateGroup) = U"Frame rate"; // ---
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::rateLimit) = U"Frame rate limit";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::rateLimit_disabled) = U"Disabled";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::rateLimit_autodetect) = U"Autodetect (NTSC/PAL)";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::rateLimit_custom) = U"Custom limit";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::rateLimit_tooltip) = U"Frame rate limiter, to ensure proper game speed (recommended)";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::customRate) = U"Custom frame rate";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::customRate_fps) = U"fps";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::customRate_tooltip) = U"Custom frame rate limit (frames per second)";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::frameSkip) = U"Frame skipping";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::frameSkip_tooltip) = U"Allow frame skipping to always keep a constant game speed";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::vsync) = U"Vertical sync";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::vsync_tooltip) = U"Fixes screen tearing and reduces power consumption (but may increase input delay)";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::uiGroup) = U"User interface"; // ---
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::theme) = U"Menu theme";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::theme_blue) = U"Blue theme";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::theme_green) = U"Green theme";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::theme_scifi) = U"Sci-fi theme";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::theme_yellow) = U"Yellow theme";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::theme_tooltip) = U"Choose color theme for the user interface";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::language) = U"Language";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::language_tooltip) = U"Choose language for the user interface";
  
  GET_UI_MESSAGE(hotkeyBindings, HotkeyBindingsMessages::title) = U"Hotkey bindings";
  GET_UI_MESSAGE(hotkeyBindings, HotkeyBindingsMessages::behaviorGroup) = U"Behavior settings";
  GET_UI_MESSAGE(hotkeyBindings, HotkeyBindingsMessages::menuHintMouseMove) = U"Menu hint on mouse move";
  GET_UI_MESSAGE(hotkeyBindings, HotkeyBindingsMessages::menuHintMouseMove_tooltip) = U"Show top-left shortcut to menu when the mouse moves (to this plugin config menu)";
  GET_UI_MESSAGE(hotkeyBindings, HotkeyBindingsMessages::enableKeyBindings) = U"Enable key bindings";
  GET_UI_MESSAGE(hotkeyBindings, HotkeyBindingsMessages::enableKeyBindings_tooltip) = U"Enable the hotkeys listed in the next section";
  GET_UI_MESSAGE(hotkeyBindings, HotkeyBindingsMessages::hotkey) = U"Controller master hotkey";
  GET_UI_MESSAGE(hotkeyBindings, HotkeyBindingsMessages::hotkey_tooltip) = U"Controller hotkey to press along with any controller binding key. Not needed with keyboard keys.";
  GET_UI_MESSAGE(hotkeyBindings, HotkeyBindingsMessages::toggle) = U"Toggle";
  GET_UI_MESSAGE(hotkeyBindings, HotkeyBindingsMessages::hold) = U"Hold";
  GET_UI_MESSAGE(hotkeyBindings, HotkeyBindingsMessages::slowMotionMode) = U"Slow-motion key mode";
  GET_UI_MESSAGE(hotkeyBindings, HotkeyBindingsMessages::slowMotionMode_tooltip) = U"Choose the behavior of the slow-motion key";
  GET_UI_MESSAGE(hotkeyBindings, HotkeyBindingsMessages::fastForwardMode) = U"Fast-forward key mode";
  GET_UI_MESSAGE(hotkeyBindings, HotkeyBindingsMessages::fastForwardMode_tooltip) = U"Choose the behavior of the fast-forward key";
  GET_UI_MESSAGE(hotkeyBindings, HotkeyBindingsMessages::bindingsGroup) = U"Keyboard/controller bindings";
  GET_UI_MESSAGE(hotkeyActions, HotkeyActions::showOptions) = U"Show options menu";
  GET_UI_MESSAGE(hotkeyActions, HotkeyActions::previousProfile) = U"Previous config profile";
  GET_UI_MESSAGE(hotkeyActions, HotkeyActions::nextProfile) = U"Next config profile";
  GET_UI_MESSAGE(hotkeyActions, HotkeyActions::nextSaveSlot) = U"Select next save slot";
  GET_UI_MESSAGE(hotkeyActions, HotkeyActions::quickSave) = U"Quick save to current slot";
  GET_UI_MESSAGE(hotkeyActions, HotkeyActions::quickLoad) = U"Quick load from current slot";
  GET_UI_MESSAGE(hotkeyActions, HotkeyActions::pauseResume) = U"Pause/resume game";
  GET_UI_MESSAGE(hotkeyActions, HotkeyActions::slowMotion) = U"Slow-motion mode";
  GET_UI_MESSAGE(hotkeyActions, HotkeyActions::fastForward) = U"Fast-forward mode";
  GET_UI_MESSAGE(hotkeyActions, HotkeyActions::resetEmulator) = U"Reset console";
  GET_UI_MESSAGE(hotkeyActions, HotkeyActions::exitToEmulator) = U"Exit to emulator";
  GET_UI_MESSAGE(hotkeyActions, HotkeyActions::screenshot) = U"Take a screenshot";
  GET_UI_MESSAGE(hotkeyActions, HotkeyActions::toggleWindowMode) = U"Toggle window mode";
  GET_UI_MESSAGE(hotkeyActions, HotkeyActions::toggleOsd) = U"Toggle on-screen display";
  GET_UI_MESSAGE(hotkeyActionsTooltips, HotkeyActions::showOptions) = U"Pause game and open menu (this plugin config menu)";
  GET_UI_MESSAGE(hotkeyActionsTooltips, HotkeyActions::previousProfile) = U"Reconfigure renderer with the previous config profile, if any (see profile settings page)";
  GET_UI_MESSAGE(hotkeyActionsTooltips, HotkeyActions::nextProfile) = U"Reconfigure renderer with the next config profile, if any (see profile settings page)";
  GET_UI_MESSAGE(hotkeyActionsTooltips, HotkeyActions::nextSaveSlot) = U"Change the active save state slot (for quick saves and quick loads)";
  GET_UI_MESSAGE(hotkeyActionsTooltips, HotkeyActions::quickSave) = U"Save current game state on disk (replace previous save in the active slot)";
  GET_UI_MESSAGE(hotkeyActionsTooltips, HotkeyActions::quickLoad) = U"Load the latest saved state (from the active slot)";
  GET_UI_MESSAGE(hotkeyActionsTooltips, HotkeyActions::pauseResume) = U"Pause or resume the game emulation";
  GET_UI_MESSAGE(hotkeyActionsTooltips, HotkeyActions::slowMotion) = U"Temporarily slow down the game (e.g. to make a tricky part easier)";
  GET_UI_MESSAGE(hotkeyActionsTooltips, HotkeyActions::fastForward) = U"Temporarily speed up the game (e.g. to skip cutscenes)";
  GET_UI_MESSAGE(hotkeyActionsTooltips, HotkeyActions::resetEmulator) = U"Completely restart the emulated game console";
  GET_UI_MESSAGE(hotkeyActionsTooltips, HotkeyActions::exitToEmulator) = U"Close game window and return to emulator (top menu)";
  GET_UI_MESSAGE(hotkeyActionsTooltips, HotkeyActions::screenshot) = U"Capture current display and save it as an image file";
  GET_UI_MESSAGE(hotkeyActionsTooltips, HotkeyActions::toggleWindowMode) = U"Toggle window mode: fullscreen / borderless / window";
  GET_UI_MESSAGE(hotkeyActionsTooltips, HotkeyActions::toggleOsd) = U"Show/hide on-screen display (technical information)";
  
  GET_UI_MESSAGE(osdSettings, OsdSettingsMessages::title) = U"On-screen display";
  GET_UI_MESSAGE(osdSettings, OsdSettingsMessages::clockGroup) = U"Clock settings"; // ---
  GET_UI_MESSAGE(osdSettings, OsdSettingsMessages::clockVisibility) = U"Show current time";
  GET_UI_MESSAGE(osdSettings, OsdSettingsMessages::clockVisibility_disabled) = U"Disabled";
  GET_UI_MESSAGE(osdSettings, OsdSettingsMessages::clockVisibility_mouseMove) = U"When the mouse moves";
  GET_UI_MESSAGE(osdSettings, OsdSettingsMessages::clockVisibility_always) = U"Always";
  GET_UI_MESSAGE(osdSettings, OsdSettingsMessages::clockFormat) = U"Clock format";
  GET_UI_MESSAGE(osdSettings, OsdSettingsMessages::clockLocationX) = U"Horizontal position";
  GET_UI_MESSAGE(osdSettings, OsdSettingsMessages::clockLocationX_left) = U"Left";
  GET_UI_MESSAGE(osdSettings, OsdSettingsMessages::clockLocationX_center) = U"Center";
  GET_UI_MESSAGE(osdSettings, OsdSettingsMessages::clockLocationX_right) = U"Right";
  GET_UI_MESSAGE(osdSettings, OsdSettingsMessages::clockLocationY) = U"Vertical position";
  GET_UI_MESSAGE(osdSettings, OsdSettingsMessages::clockLocationY_top) = U"Top";
  GET_UI_MESSAGE(osdSettings, OsdSettingsMessages::clockLocationY_bottom) = U"Bottom";
  GET_UI_MESSAGE(osdSettings, OsdSettingsMessages::techInfoGroup) = U"Technical information"; // ---
  GET_UI_MESSAGE(osdSettings, OsdSettingsMessages::techInfoVisibility) = U"Show technical message";
  GET_UI_MESSAGE(osdSettings, OsdSettingsMessages::techInfoType) = U"Information type";
  GET_UI_MESSAGE(osdSettings, OsdSettingsMessages::techInfoType_fps) = U"Frames per second";
  GET_UI_MESSAGE(osdSettings, OsdSettingsMessages::techInfoType_format) = U"Framebuffer format";
  GET_UI_MESSAGE(osdSettings, OsdSettingsMessages::techInfoType_all) = U"Everything";
}

void MessageResources::fillMessagesFR() {
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::title) = U"Options générales";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::windowGroup) = U"Paramètres de fenêtre"; // ---
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::displayMode) = U"Type d'affichage";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::displayMode_fullscreen) = U"Plein écran";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::displayMode_borderless) = U"Sans bordure";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::displayMode_window) = U"Mode fenêtré";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::displayMode_tooltip) = U"Modes : plein écran / sans bordure (fenêtre plein écran) / mode fenêtré (taille libre)";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::resolution) = U"Résolution plein écran";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::resolution_tooltip) = U"Résolution d'affichage (pixels) en mode plein écran";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::refreshRate_tooltip) = U"Fréquence de rafraîchissement (Hz) en mode plein écran";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::windowSize) = U"Taille de fenêtre";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::windowSize_tooltip) = U"Hauteur d'affichage (pixels) en mode fenêtré";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::emulatorGroup) = U"Compatibilité avec l'émulateur"; // ---
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::precision) = U"Précision des pixels";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::precision_original) = U"Originale (entiers)";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::precision_subprecision) = U"Sous-précision (plus fluide)";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::precision_tooltip) = U"Précision anti-saccade du framebuffer (si 'GTE-subprecision' est activé dans l'émulateur)";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::widescreen) = U"Ajustement écran large";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::widescreen_tooltip) = U"Traiter les coordonnées du framebuffer en 16:9 (si 'widescreen hack' est activé dans l'émulateur)";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::autosaveOnExit) = U"Sauvegarde à la fermeture";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::autosaveOnExit_tooltip) = U"Sauvegarder l'état du jeu automatiquement (dans l'emplacement actif) lorsque l'émulateur est fermé";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::rateGroup) = U"Fréquence d'échantillonage"; // ---
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::rateLimit) = U"Limite de fréquence";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::rateLimit_disabled) = U"Désactivée";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::rateLimit_autodetect) = U"Autodétection (NTSC/PAL)";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::rateLimit_custom) = U"Personnalisée";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::rateLimit_tooltip) = U"Limiteur de fréquence, pour assurer une vitesse de jeu appropriée (recommandé)";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::customRate) = U"Limite personnalisée";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::customRate_fps) = U"fps";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::customRate_tooltip) = U"Limite de fréquence personnalisée (images par secondes)";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::frameSkip) = U"Saut d'image";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::frameSkip_tooltip) = U"Autoriser le saut d'image pour assurer une vitesse de jeu constante";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::vsync) = U"Synchro verticale";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::vsync_tooltip) = U"Corrige le déchirement d'image et réduit la consommation (mais peut augmenter le délai d'affichage)";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::uiGroup) = U"Interface utilisateur"; // ---
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::theme) = U"Thème des menus";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::theme_blue) = U"Thème bleu";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::theme_green) = U"Thème vert";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::theme_scifi) = U"Thème sci-fi";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::theme_yellow) = U"Thème jaune";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::theme_tooltip) = U"Choisir le thème de couleurs de l'interface graphique";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::language) = U"Langue";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::language_tooltip) = U"Choisir la langue de l'interface graphique";
  
  GET_UI_MESSAGE(hotkeyBindings, HotkeyBindingsMessages::title) = U"";
  GET_UI_MESSAGE(hotkeyBindings, HotkeyBindingsMessages::behaviorGroup) = U"";
  GET_UI_MESSAGE(hotkeyBindings, HotkeyBindingsMessages::menuHintMouseMove) = U"";
  GET_UI_MESSAGE(hotkeyBindings, HotkeyBindingsMessages::menuHintMouseMove_tooltip) = U"";
  GET_UI_MESSAGE(hotkeyBindings, HotkeyBindingsMessages::enableKeyBindings) = U"";
  GET_UI_MESSAGE(hotkeyBindings, HotkeyBindingsMessages::enableKeyBindings_tooltip) = U"";
  GET_UI_MESSAGE(hotkeyBindings, HotkeyBindingsMessages::hotkey) = U"";
  GET_UI_MESSAGE(hotkeyBindings, HotkeyBindingsMessages::hotkey_tooltip) = U"";
  GET_UI_MESSAGE(hotkeyBindings, HotkeyBindingsMessages::toggle) = U"";
  GET_UI_MESSAGE(hotkeyBindings, HotkeyBindingsMessages::hold) = U"";
  GET_UI_MESSAGE(hotkeyBindings, HotkeyBindingsMessages::slowMotionMode) = U"";
  GET_UI_MESSAGE(hotkeyBindings, HotkeyBindingsMessages::slowMotionMode_tooltip) = U"";
  GET_UI_MESSAGE(hotkeyBindings, HotkeyBindingsMessages::fastForwardMode) = U"";
  GET_UI_MESSAGE(hotkeyBindings, HotkeyBindingsMessages::fastForwardMode_tooltip) = U"";
  GET_UI_MESSAGE(hotkeyBindings, HotkeyBindingsMessages::bindingsGroup) = U"";
  GET_UI_MESSAGE(hotkeyActions, HotkeyActions::showOptions) = U"";
  GET_UI_MESSAGE(hotkeyActions, HotkeyActions::previousProfile) = U"";
  GET_UI_MESSAGE(hotkeyActions, HotkeyActions::nextProfile) = U"";
  GET_UI_MESSAGE(hotkeyActions, HotkeyActions::nextSaveSlot) = U"";
  GET_UI_MESSAGE(hotkeyActions, HotkeyActions::quickSave) = U"";
  GET_UI_MESSAGE(hotkeyActions, HotkeyActions::quickLoad) = U"";
  GET_UI_MESSAGE(hotkeyActions, HotkeyActions::pauseResume) = U"";
  GET_UI_MESSAGE(hotkeyActions, HotkeyActions::slowMotion) = U"";
  GET_UI_MESSAGE(hotkeyActions, HotkeyActions::fastForward) = U"";
  GET_UI_MESSAGE(hotkeyActions, HotkeyActions::resetEmulator) = U"";
  GET_UI_MESSAGE(hotkeyActions, HotkeyActions::exitToEmulator) = U"";
  GET_UI_MESSAGE(hotkeyActions, HotkeyActions::screenshot) = U"";
  GET_UI_MESSAGE(hotkeyActions, HotkeyActions::toggleWindowMode) = U"";
  GET_UI_MESSAGE(hotkeyActions, HotkeyActions::toggleOsd) = U"";
  GET_UI_MESSAGE(hotkeyActionsTooltips, HotkeyActions::showOptions) = U"";
  GET_UI_MESSAGE(hotkeyActionsTooltips, HotkeyActions::previousProfile) = U"";
  GET_UI_MESSAGE(hotkeyActionsTooltips, HotkeyActions::nextProfile) = U"";
  GET_UI_MESSAGE(hotkeyActionsTooltips, HotkeyActions::nextSaveSlot) = U"";
  GET_UI_MESSAGE(hotkeyActionsTooltips, HotkeyActions::quickSave) = U"";
  GET_UI_MESSAGE(hotkeyActionsTooltips, HotkeyActions::quickLoad) = U"";
  GET_UI_MESSAGE(hotkeyActionsTooltips, HotkeyActions::pauseResume) = U"";
  GET_UI_MESSAGE(hotkeyActionsTooltips, HotkeyActions::slowMotion) = U"";
  GET_UI_MESSAGE(hotkeyActionsTooltips, HotkeyActions::fastForward) = U"";
  GET_UI_MESSAGE(hotkeyActionsTooltips, HotkeyActions::resetEmulator) = U"";
  GET_UI_MESSAGE(hotkeyActionsTooltips, HotkeyActions::exitToEmulator) = U"";
  GET_UI_MESSAGE(hotkeyActionsTooltips, HotkeyActions::screenshot) = U"";
  GET_UI_MESSAGE(hotkeyActionsTooltips, HotkeyActions::toggleWindowMode) = U"";
  GET_UI_MESSAGE(hotkeyActionsTooltips, HotkeyActions::toggleOsd) = U"";
  
  GET_UI_MESSAGE(osdSettings, OsdSettingsMessages::title) = U"Informations à l'écran";
  GET_UI_MESSAGE(osdSettings, OsdSettingsMessages::clockGroup) = U"Paramètres d'horloge"; // ---
  GET_UI_MESSAGE(osdSettings, OsdSettingsMessages::clockVisibility) = U"Afficher l'heure";
  GET_UI_MESSAGE(osdSettings, OsdSettingsMessages::clockVisibility_disabled) = U"Désactivé";
  GET_UI_MESSAGE(osdSettings, OsdSettingsMessages::clockVisibility_mouseMove) = U"Quand la souris bouge";
  GET_UI_MESSAGE(osdSettings, OsdSettingsMessages::clockVisibility_always) = U"Toujours";
  GET_UI_MESSAGE(osdSettings, OsdSettingsMessages::clockFormat) = U"Format d'horloge";
  GET_UI_MESSAGE(osdSettings, OsdSettingsMessages::clockLocationX) = U"Position horizontale";
  GET_UI_MESSAGE(osdSettings, OsdSettingsMessages::clockLocationX_left) = U"Gauche";
  GET_UI_MESSAGE(osdSettings, OsdSettingsMessages::clockLocationX_center) = U"Centre";
  GET_UI_MESSAGE(osdSettings, OsdSettingsMessages::clockLocationX_right) = U"Droite";
  GET_UI_MESSAGE(osdSettings, OsdSettingsMessages::clockLocationY) = U"Position verticale";
  GET_UI_MESSAGE(osdSettings, OsdSettingsMessages::clockLocationY_top) = U"Haut";
  GET_UI_MESSAGE(osdSettings, OsdSettingsMessages::clockLocationY_bottom) = U"Bas";
  GET_UI_MESSAGE(osdSettings, OsdSettingsMessages::techInfoGroup) = U"Informations techniques"; // ---
  GET_UI_MESSAGE(osdSettings, OsdSettingsMessages::techInfoVisibility) = U"Afficher message technique";
  GET_UI_MESSAGE(osdSettings, OsdSettingsMessages::techInfoType) = U"Type d'informations";
  GET_UI_MESSAGE(osdSettings, OsdSettingsMessages::techInfoType_fps) = U"Images par seconde";
  GET_UI_MESSAGE(osdSettings, OsdSettingsMessages::techInfoType_format) = U"Format du framebuffer";
  GET_UI_MESSAGE(osdSettings, OsdSettingsMessages::techInfoType_all) = U"Toutes les informations";
}

void MessageResources::fillMessagesDE() {
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::title) = U"Allgemain";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::windowGroup) = U"Fenstereinstellungen"; // ---
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::displayMode) = U"Anzeigemodus";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::displayMode_fullscreen) = U"Vollbild";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::displayMode_borderless) = U"Randlose";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::displayMode_window) = U"Fenster";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::displayMode_tooltip) = U"Modi: Vollbild / Randlose (Vollbildfenster) / Fenster (veränderbares)";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::resolution) = U"Vollbildauflösung";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::resolution_tooltip) = U"Anzeigeauflösung (Pixel) im Vollbildmodus";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::refreshRate_tooltip) = U"Bildwiederholfrequenz (Hz) im Vollbildmodus";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::windowSize) = U"Fenstergröße";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::windowSize_tooltip) = U"Anzeigehöhe (Pixel) im Fenstermodus";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::emulatorGroup) = U"Emulatorkompatibilität"; // ---
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::precision) = U"Pixelpräzision";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::precision_original) = U"Original (ganze Zahlen)";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::precision_subprecision) = U"Subpräzision (glatter)";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::precision_tooltip) = U"Anti-Bildschirmzittern (wenn 'GTE-subprecision' im Emulator aktiviert ist)";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::widescreen) = U"Breitbildanpassung";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::widescreen_tooltip) = U"Framebuffer-Daten als 16:9 behandeln (wenn 'widescreen hack' im Emulator aktiviert ist)";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::autosaveOnExit) = U"Speicherung beim Beenden";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::autosaveOnExit_tooltip) = U"Speichern Sie das Spiel automatisch im aktiven Slot, wenn der Emulator geschlossen wird";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::rateGroup) = U"Bildrate"; // ---
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::rateLimit) = U"Bildratenbegrenzung";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::rateLimit_disabled) = U"Deaktiviert";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::rateLimit_autodetect) = U"Automatisch (NTSC/PAL)";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::rateLimit_custom) = U"Anpassbar";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::rateLimit_tooltip) = U"Bildratenbegrenzer, um die richtige Spielgeschwindigkeit sicherzustellen (empfohlen)";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::customRate) = U"Benutzerdefiniertes";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::customRate_fps) = U"BpS";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::customRate_tooltip) = U"Benutzerdefiniertes Bildratenbegrenzung (Bilder pro Sekunde)";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::frameSkip) = U"Bildüberspringen";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::frameSkip_tooltip) = U"Bildüberspringen erlauben, um eine konstante Spielgeschwindigkeit aufrechtzuerhalten";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::vsync) = U"Vertikale Sync";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::vsync_tooltip) = U"Behebt Bildschirmrisse und reduziert den Stromverbrauch (kann die Verzögerung erhöhen)";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::uiGroup) = U"Benutzeroberfläche"; // ---
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::theme) = U"Menüthema";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::theme_blue) = U"Blaues Thema";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::theme_green) = U"Grünes Thema";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::theme_scifi) = U"Sci-Fi Theme";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::theme_yellow) = U"Gelbes Thema";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::theme_tooltip) = U"Farbthema für die Benutzeroberfläche";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::language) = U"Sprache";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::language_tooltip) = U"Sprache für die Benutzeroberfläche";
  
  GET_UI_MESSAGE(hotkeyBindings, HotkeyBindingsMessages::title) = U"";
  GET_UI_MESSAGE(hotkeyBindings, HotkeyBindingsMessages::behaviorGroup) = U"";
  GET_UI_MESSAGE(hotkeyBindings, HotkeyBindingsMessages::menuHintMouseMove) = U"";
  GET_UI_MESSAGE(hotkeyBindings, HotkeyBindingsMessages::menuHintMouseMove_tooltip) = U"";
  GET_UI_MESSAGE(hotkeyBindings, HotkeyBindingsMessages::enableKeyBindings) = U"";
  GET_UI_MESSAGE(hotkeyBindings, HotkeyBindingsMessages::enableKeyBindings_tooltip) = U"";
  GET_UI_MESSAGE(hotkeyBindings, HotkeyBindingsMessages::hotkey) = U"";
  GET_UI_MESSAGE(hotkeyBindings, HotkeyBindingsMessages::hotkey_tooltip) = U"";
  GET_UI_MESSAGE(hotkeyBindings, HotkeyBindingsMessages::toggle) = U"";
  GET_UI_MESSAGE(hotkeyBindings, HotkeyBindingsMessages::hold) = U"";
  GET_UI_MESSAGE(hotkeyBindings, HotkeyBindingsMessages::slowMotionMode) = U"";
  GET_UI_MESSAGE(hotkeyBindings, HotkeyBindingsMessages::slowMotionMode_tooltip) = U"";
  GET_UI_MESSAGE(hotkeyBindings, HotkeyBindingsMessages::fastForwardMode) = U"";
  GET_UI_MESSAGE(hotkeyBindings, HotkeyBindingsMessages::fastForwardMode_tooltip) = U"";
  GET_UI_MESSAGE(hotkeyBindings, HotkeyBindingsMessages::bindingsGroup) = U"";
  GET_UI_MESSAGE(hotkeyActions, HotkeyActions::showOptions) = U"";
  GET_UI_MESSAGE(hotkeyActions, HotkeyActions::previousProfile) = U"";
  GET_UI_MESSAGE(hotkeyActions, HotkeyActions::nextProfile) = U"";
  GET_UI_MESSAGE(hotkeyActions, HotkeyActions::nextSaveSlot) = U"";
  GET_UI_MESSAGE(hotkeyActions, HotkeyActions::quickSave) = U"";
  GET_UI_MESSAGE(hotkeyActions, HotkeyActions::quickLoad) = U"";
  GET_UI_MESSAGE(hotkeyActions, HotkeyActions::pauseResume) = U"";
  GET_UI_MESSAGE(hotkeyActions, HotkeyActions::slowMotion) = U"";
  GET_UI_MESSAGE(hotkeyActions, HotkeyActions::fastForward) = U"";
  GET_UI_MESSAGE(hotkeyActions, HotkeyActions::resetEmulator) = U"";
  GET_UI_MESSAGE(hotkeyActions, HotkeyActions::exitToEmulator) = U"";
  GET_UI_MESSAGE(hotkeyActions, HotkeyActions::screenshot) = U"";
  GET_UI_MESSAGE(hotkeyActions, HotkeyActions::toggleWindowMode) = U"";
  GET_UI_MESSAGE(hotkeyActions, HotkeyActions::toggleOsd) = U"";
  GET_UI_MESSAGE(hotkeyActionsTooltips, HotkeyActions::showOptions) = U"";
  GET_UI_MESSAGE(hotkeyActionsTooltips, HotkeyActions::previousProfile) = U"";
  GET_UI_MESSAGE(hotkeyActionsTooltips, HotkeyActions::nextProfile) = U"";
  GET_UI_MESSAGE(hotkeyActionsTooltips, HotkeyActions::nextSaveSlot) = U"";
  GET_UI_MESSAGE(hotkeyActionsTooltips, HotkeyActions::quickSave) = U"";
  GET_UI_MESSAGE(hotkeyActionsTooltips, HotkeyActions::quickLoad) = U"";
  GET_UI_MESSAGE(hotkeyActionsTooltips, HotkeyActions::pauseResume) = U"";
  GET_UI_MESSAGE(hotkeyActionsTooltips, HotkeyActions::slowMotion) = U"";
  GET_UI_MESSAGE(hotkeyActionsTooltips, HotkeyActions::fastForward) = U"";
  GET_UI_MESSAGE(hotkeyActionsTooltips, HotkeyActions::resetEmulator) = U"";
  GET_UI_MESSAGE(hotkeyActionsTooltips, HotkeyActions::exitToEmulator) = U"";
  GET_UI_MESSAGE(hotkeyActionsTooltips, HotkeyActions::screenshot) = U"";
  GET_UI_MESSAGE(hotkeyActionsTooltips, HotkeyActions::toggleWindowMode) = U"";
  GET_UI_MESSAGE(hotkeyActionsTooltips, HotkeyActions::toggleOsd) = U"";
  
  GET_UI_MESSAGE(osdSettings, OsdSettingsMessages::title) = U"Bildschirmanzeige";
  GET_UI_MESSAGE(osdSettings, OsdSettingsMessages::clockGroup) = U"Uhreinstellungen"; // ---
  GET_UI_MESSAGE(osdSettings, OsdSettingsMessages::clockVisibility) = U"Uhrzeit anzeigen";
  GET_UI_MESSAGE(osdSettings, OsdSettingsMessages::clockVisibility_disabled) = U"Deaktiviert";
  GET_UI_MESSAGE(osdSettings, OsdSettingsMessages::clockVisibility_mouseMove) = U"Beim Bewegen der Maus";
  GET_UI_MESSAGE(osdSettings, OsdSettingsMessages::clockVisibility_always) = U"Immer";
  GET_UI_MESSAGE(osdSettings, OsdSettingsMessages::clockFormat) = U"Uhrenformat";
  GET_UI_MESSAGE(osdSettings, OsdSettingsMessages::clockLocationX) = U"Horizontaler Position";
  GET_UI_MESSAGE(osdSettings, OsdSettingsMessages::clockLocationX_left) = U"Links";
  GET_UI_MESSAGE(osdSettings, OsdSettingsMessages::clockLocationX_center) = U"Center";
  GET_UI_MESSAGE(osdSettings, OsdSettingsMessages::clockLocationX_right) = U"Rechts";
  GET_UI_MESSAGE(osdSettings, OsdSettingsMessages::clockLocationY) = U"Vertikaler Position";
  GET_UI_MESSAGE(osdSettings, OsdSettingsMessages::clockLocationY_top) = U"Hoch";
  GET_UI_MESSAGE(osdSettings, OsdSettingsMessages::clockLocationY_bottom) = U"Runter";
  GET_UI_MESSAGE(osdSettings, OsdSettingsMessages::techInfoGroup) = U"Technische Information"; // ---
  GET_UI_MESSAGE(osdSettings, OsdSettingsMessages::techInfoVisibility) = U"Technische Info anzeigen";
  GET_UI_MESSAGE(osdSettings, OsdSettingsMessages::techInfoType) = U"Informationstyp";
  GET_UI_MESSAGE(osdSettings, OsdSettingsMessages::techInfoType_fps) = U"Bilder pro Sekunde";
  GET_UI_MESSAGE(osdSettings, OsdSettingsMessages::techInfoType_format) = U"Framebuffer-Format";
  GET_UI_MESSAGE(osdSettings, OsdSettingsMessages::techInfoType_all) = U"Alles";
}

void MessageResources::fillMessagesES() {
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::title) = U"Configuración general";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::windowGroup) = U"Gestión de ventanas"; // ---
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::displayMode) = U"Modo ventana";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::displayMode_fullscreen) = U"Pantalla completa";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::displayMode_borderless) = U"Sin bordes";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::displayMode_window) = U"Ventana";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::displayMode_tooltip) = U"Modos: pantalla completa / sin bordes (ventana de pantalla completa) / ventana (redimensionable)";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::resolution) = U"Resolución de la pantalla";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::resolution_tooltip) = U"Resolución de pantalla (píxeles) en modo de pantalla completa";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::refreshRate_tooltip) = U"Frecuencia de actualización (Hz) en modo de pantalla completa";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::windowSize) = U"Tamaño de ventana";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::windowSize_tooltip) = U"Altura de visualización (píxeles) en modo ventana";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::emulatorGroup) = U"Compatibilidad del emulador"; // ---
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::precision) = U"Precisión de píxeles";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::precision_original) = U"Original (enteros)";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::precision_subprecision) = U"Subprecisión (más fluido)";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::precision_tooltip) = U"Corrige el temblor del búfer de cuadros (si la 'GTE-subprecision' está habilitado en el emulador)";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::widescreen) = U"Truco de pantalla ancha";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::widescreen_tooltip) = U"Trate los datos del búfer de cuadros como 16:9 (si el 'widescreen hack' está habilitado en el emulador)";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::autosaveOnExit) = U"Autoguardado al salir";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::autosaveOnExit_tooltip) = U"Guarda automáticamente el juego en la ranura activa cuando el emulador está cerrado";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::rateGroup) = U"Frecuencia de muestreo"; // ---
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::rateLimit) = U"Límite de muestreo";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::rateLimit_disabled) = U"Desactivado";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::rateLimit_autodetect) = U"Automático (NTSC/PAL)";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::rateLimit_custom) = U"Personalizado";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::rateLimit_tooltip) = U"Limitador de cuadros por segundo, para garantizar una velocidad de juego adecuada (recomendado)";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::customRate) = U"Límite personalizado";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::customRate_fps) = U"fps";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::customRate_tooltip) = U"Límite personalizado (Cuadros por segundo)";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::frameSkip) = U"Saltos de imagen";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::frameSkip_tooltip) = U"Permitir saltos de fotogramas para mantener siempre una velocidad de juego constante";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::vsync) = U"Sincronización vertical";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::vsync_tooltip) = U"Corrige el desgarro de la pantalla y reduce el consumo de energía (puede aumentar el retraso de entrada)";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::uiGroup) = U"Interfaz de usuario"; // ---
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::theme) = U"Tema del menú";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::theme_blue) = U"Tema azul";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::theme_green) = U"Tema verde";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::theme_scifi) = U"Tema sci-fi";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::theme_yellow) = U"Tema amarillo";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::theme_tooltip) = U"Elija el tema para la interfaz de usuario";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::language) = U"Idioma";
  GET_UI_MESSAGE(generalSettings, GeneralSettingsMessages::language_tooltip) = U"Elija el idioma para la interfaz de usuario";
  
  GET_UI_MESSAGE(hotkeyBindings, HotkeyBindingsMessages::title) = U"";
  GET_UI_MESSAGE(hotkeyBindings, HotkeyBindingsMessages::behaviorGroup) = U"";
  GET_UI_MESSAGE(hotkeyBindings, HotkeyBindingsMessages::menuHintMouseMove) = U"";
  GET_UI_MESSAGE(hotkeyBindings, HotkeyBindingsMessages::menuHintMouseMove_tooltip) = U"";
  GET_UI_MESSAGE(hotkeyBindings, HotkeyBindingsMessages::enableKeyBindings) = U"";
  GET_UI_MESSAGE(hotkeyBindings, HotkeyBindingsMessages::enableKeyBindings_tooltip) = U"";
  GET_UI_MESSAGE(hotkeyBindings, HotkeyBindingsMessages::hotkey) = U"";
  GET_UI_MESSAGE(hotkeyBindings, HotkeyBindingsMessages::hotkey_tooltip) = U"";
  GET_UI_MESSAGE(hotkeyBindings, HotkeyBindingsMessages::toggle) = U"";
  GET_UI_MESSAGE(hotkeyBindings, HotkeyBindingsMessages::hold) = U"";
  GET_UI_MESSAGE(hotkeyBindings, HotkeyBindingsMessages::slowMotionMode) = U"";
  GET_UI_MESSAGE(hotkeyBindings, HotkeyBindingsMessages::slowMotionMode_tooltip) = U"";
  GET_UI_MESSAGE(hotkeyBindings, HotkeyBindingsMessages::fastForwardMode) = U"";
  GET_UI_MESSAGE(hotkeyBindings, HotkeyBindingsMessages::fastForwardMode_tooltip) = U"";
  GET_UI_MESSAGE(hotkeyBindings, HotkeyBindingsMessages::bindingsGroup) = U"";
  GET_UI_MESSAGE(hotkeyActions, HotkeyActions::showOptions) = U"";
  GET_UI_MESSAGE(hotkeyActions, HotkeyActions::previousProfile) = U"";
  GET_UI_MESSAGE(hotkeyActions, HotkeyActions::nextProfile) = U"";
  GET_UI_MESSAGE(hotkeyActions, HotkeyActions::nextSaveSlot) = U"";
  GET_UI_MESSAGE(hotkeyActions, HotkeyActions::quickSave) = U"";
  GET_UI_MESSAGE(hotkeyActions, HotkeyActions::quickLoad) = U"";
  GET_UI_MESSAGE(hotkeyActions, HotkeyActions::pauseResume) = U"";
  GET_UI_MESSAGE(hotkeyActions, HotkeyActions::slowMotion) = U"";
  GET_UI_MESSAGE(hotkeyActions, HotkeyActions::fastForward) = U"";
  GET_UI_MESSAGE(hotkeyActions, HotkeyActions::resetEmulator) = U"";
  GET_UI_MESSAGE(hotkeyActions, HotkeyActions::exitToEmulator) = U"";
  GET_UI_MESSAGE(hotkeyActions, HotkeyActions::screenshot) = U"";
  GET_UI_MESSAGE(hotkeyActions, HotkeyActions::toggleWindowMode) = U"";
  GET_UI_MESSAGE(hotkeyActions, HotkeyActions::toggleOsd) = U"";
  GET_UI_MESSAGE(hotkeyActionsTooltips, HotkeyActions::showOptions) = U"";
  GET_UI_MESSAGE(hotkeyActionsTooltips, HotkeyActions::previousProfile) = U"";
  GET_UI_MESSAGE(hotkeyActionsTooltips, HotkeyActions::nextProfile) = U"";
  GET_UI_MESSAGE(hotkeyActionsTooltips, HotkeyActions::nextSaveSlot) = U"";
  GET_UI_MESSAGE(hotkeyActionsTooltips, HotkeyActions::quickSave) = U"";
  GET_UI_MESSAGE(hotkeyActionsTooltips, HotkeyActions::quickLoad) = U"";
  GET_UI_MESSAGE(hotkeyActionsTooltips, HotkeyActions::pauseResume) = U"";
  GET_UI_MESSAGE(hotkeyActionsTooltips, HotkeyActions::slowMotion) = U"";
  GET_UI_MESSAGE(hotkeyActionsTooltips, HotkeyActions::fastForward) = U"";
  GET_UI_MESSAGE(hotkeyActionsTooltips, HotkeyActions::resetEmulator) = U"";
  GET_UI_MESSAGE(hotkeyActionsTooltips, HotkeyActions::exitToEmulator) = U"";
  GET_UI_MESSAGE(hotkeyActionsTooltips, HotkeyActions::screenshot) = U"";
  GET_UI_MESSAGE(hotkeyActionsTooltips, HotkeyActions::toggleWindowMode) = U"";
  GET_UI_MESSAGE(hotkeyActionsTooltips, HotkeyActions::toggleOsd) = U"";
  
  GET_UI_MESSAGE(osdSettings, OsdSettingsMessages::title) = U"Información en pantalla";
  GET_UI_MESSAGE(osdSettings, OsdSettingsMessages::clockGroup) = U"Configuraciones del reloj"; // ---
  GET_UI_MESSAGE(osdSettings, OsdSettingsMessages::clockVisibility) = U"Tiempo de la funcion";
  GET_UI_MESSAGE(osdSettings, OsdSettingsMessages::clockVisibility_disabled) = U"Desactivado";
  GET_UI_MESSAGE(osdSettings, OsdSettingsMessages::clockVisibility_mouseMove) = U"Cuando el ratón se mueve";
  GET_UI_MESSAGE(osdSettings, OsdSettingsMessages::clockVisibility_always) = U"Siempre";
  GET_UI_MESSAGE(osdSettings, OsdSettingsMessages::clockFormat) = U"Formato de reloj";
  GET_UI_MESSAGE(osdSettings, OsdSettingsMessages::clockLocationX) = U"Posición horizontal";
  GET_UI_MESSAGE(osdSettings, OsdSettingsMessages::clockLocationX_left) = U"Izquierdo";
  GET_UI_MESSAGE(osdSettings, OsdSettingsMessages::clockLocationX_center) = U"centrado";
  GET_UI_MESSAGE(osdSettings, OsdSettingsMessages::clockLocationX_right) = U"Derecho";
  GET_UI_MESSAGE(osdSettings, OsdSettingsMessages::clockLocationY) = U"Posición vertical";
  GET_UI_MESSAGE(osdSettings, OsdSettingsMessages::clockLocationY_top) = U"Superior";
  GET_UI_MESSAGE(osdSettings, OsdSettingsMessages::clockLocationY_bottom) = U"Inferior";
  GET_UI_MESSAGE(osdSettings, OsdSettingsMessages::techInfoGroup) = U"Información técnica"; // ---
  GET_UI_MESSAGE(osdSettings, OsdSettingsMessages::techInfoVisibility) = U"Mostrar mensaje técnico";
  GET_UI_MESSAGE(osdSettings, OsdSettingsMessages::techInfoType) = U"Tipo de información";
  GET_UI_MESSAGE(osdSettings, OsdSettingsMessages::techInfoType_fps) = U"Cuadros por segundo";
  GET_UI_MESSAGE(osdSettings, OsdSettingsMessages::techInfoType_format) = U"Formato de framebuffer";
  GET_UI_MESSAGE(osdSettings, OsdSettingsMessages::techInfoType_all) = U"Todo";
}
