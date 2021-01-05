/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/themes/theme_properties.h"

#include "base/notreached.h"
#include "chrome/browser/themes/theme_properties.h"
#include "ui/gfx/color_palette.h"

namespace {

const SkColor kLightToolbar = SkColorSetRGB(0xf3, 0xf3, 0xf3);
const SkColor kLightFrame = SkColorSetRGB(0xd5, 0xd9, 0xdc);
const SkColor kLightToolbarIcon = SkColorSetRGB(0x42, 0x42, 0x42);

base::Optional<SkColor> MaybeGetDefaultColorForBraveLightUi(int id) {
  switch (id) {
    // Applies when the window is active, tabs and also tab bar everywhere
    // except active tab
    case ThemeProperties::COLOR_FRAME_ACTIVE:
    case ThemeProperties::COLOR_TAB_BACKGROUND_INACTIVE_FRAME_ACTIVE:
    case ThemeProperties::COLOR_TOOLBAR_CONTENT_AREA_SEPARATOR:
      return kLightFrame;
    // Window when the window is innactive, tabs and also tab bar everywhere
    // except active tab
    case ThemeProperties::COLOR_FRAME_INACTIVE:
    case ThemeProperties::COLOR_TAB_BACKGROUND_INACTIVE_FRAME_INACTIVE:
      return color_utils::HSLShift(kLightFrame, { -1, -1, 0.6 });
    // Active tab and also the URL toolbar
    // Parts of this color show up as you hover over innactive tabs too
    case ThemeProperties::COLOR_TOOLBAR:
    case ThemeProperties::COLOR_TOOLBAR_TOP_SEPARATOR:
    case ThemeProperties::COLOR_TOOLBAR_TOP_SEPARATOR_INACTIVE:
    case ThemeProperties::COLOR_TAB_BACKGROUND_ACTIVE_FRAME_ACTIVE:
    case ThemeProperties::COLOR_TAB_BACKGROUND_ACTIVE_FRAME_INACTIVE:
      return kLightToolbar;
    case ThemeProperties::COLOR_TAB_FOREGROUND_ACTIVE_FRAME_ACTIVE:
    case ThemeProperties::COLOR_BOOKMARK_TEXT:
    case BraveThemeProperties::COLOR_BOOKMARK_BAR_INSTRUCTIONS_TEXT:
    case ThemeProperties::COLOR_TAB_FOREGROUND_INACTIVE_FRAME_ACTIVE:
    case ThemeProperties::COLOR_TOOLBAR_BUTTON_ICON:
      return kLightToolbarIcon;
    case ThemeProperties::COLOR_TOOLBAR_BUTTON_ICON_INACTIVE:
      return color_utils::AlphaBlend(kLightToolbarIcon, kLightToolbar, 0.3f);
    case BraveThemeProperties::COLOR_SIDEBAR_ARROW_BACKGROUND_HOVERED:
      return SkColorSetRGB(0xE3, 0xE3, 0xE3);
    case BraveThemeProperties::COLOR_SIDEBAR_ARROW_NORMAL:
      return SkColorSetRGB(0x21, 0x25, 0x29);
    case BraveThemeProperties::COLOR_SIDEBAR_BACKGROUND:
      return SkColorSetRGB(0xF3, 0xF3, 0xF5);
    case BraveThemeProperties::COLOR_SIDEBAR_ARROW_DISABLED:
      return SkColorSetARGB(0x8A, 0x49, 0x50, 0x57);
    case BraveThemeProperties::COLOR_SIDEBAR_BUTTON_BASE:
      return SkColorSetRGB(0x49, 0x50, 0x57);
    case BraveThemeProperties::COLOR_SIDEBAR_ADD_BUTTON_DISABLED:
      return SkColorSetARGB(0X66, 0x49, 0x50, 0x57);
    case BraveThemeProperties::COLOR_SIDEBAR_BORDER:
      return SkColorSetRGB(0xD9, 0xDC, 0xDF);
    case BraveThemeProperties::COLOR_SIDEBAR_ITEM_BACKGROUND:
      return SkColorSetRGB(0xE8, 0xE8, 0xE8);
    case BraveThemeProperties::COLOR_SIDEBAR_SEPARATOR:
      return SkColorSetRGB(0xE9, 0xE9, 0xF4);
    case BraveThemeProperties::COLOR_FOR_TEST:
      return BraveThemeProperties::kLightColorForTest;
    default:
      return base::nullopt;
  }
}

const SkColor kDarkToolbar = SkColorSetRGB(0x30, 0x34, 0x43);
const SkColor kDarkFrame = SkColorSetRGB(0x0C, 0x0C, 0x17);
const SkColor kDarkToolbarIcon = SkColorSetRGB(0xed, 0xed, 0xed);

base::Optional<SkColor> MaybeGetDefaultColorForBraveDarkUi(int id) {
  switch (id) {
    // Applies when the window is active, tabs and also tab bar everywhere
    // except active tab
    case ThemeProperties::COLOR_FRAME_ACTIVE:
    case ThemeProperties::COLOR_TAB_BACKGROUND_INACTIVE_FRAME_ACTIVE:
      return kDarkFrame;
    // Window when the window is innactive, tabs and also tab bar everywhere
    // except active tab
    case ThemeProperties::COLOR_FRAME_INACTIVE:
    case ThemeProperties::COLOR_TAB_BACKGROUND_INACTIVE_FRAME_INACTIVE:
      return color_utils::HSLShift(kDarkFrame, { -1, -1, 0.6 });
    // Active tab and also the URL toolbar
    // Parts of this color show up as you hover over innactive tabs too
    case ThemeProperties::COLOR_TOOLBAR:
    case ThemeProperties::COLOR_TOOLBAR_TOP_SEPARATOR:
    case ThemeProperties::COLOR_TOOLBAR_TOP_SEPARATOR_INACTIVE:
    case ThemeProperties::COLOR_TOOLBAR_CONTENT_AREA_SEPARATOR:
    case ThemeProperties::COLOR_TAB_BACKGROUND_ACTIVE_FRAME_ACTIVE:
    case ThemeProperties::COLOR_TAB_BACKGROUND_ACTIVE_FRAME_INACTIVE:
      return kDarkToolbar;
    case ThemeProperties::COLOR_TAB_FOREGROUND_ACTIVE_FRAME_ACTIVE:
      return SkColorSetRGB(0xF3, 0xF3, 0xF3);
    case ThemeProperties::COLOR_BOOKMARK_TEXT:
    case BraveThemeProperties::COLOR_BOOKMARK_BAR_INSTRUCTIONS_TEXT:
    case ThemeProperties::COLOR_TAB_FOREGROUND_INACTIVE_FRAME_ACTIVE:
      return SkColorSetRGB(0xFF, 0xFF, 0xFF);
    case ThemeProperties::COLOR_TOOLBAR_BUTTON_ICON:
      return kDarkToolbarIcon;
    case ThemeProperties::COLOR_TOOLBAR_BUTTON_ICON_INACTIVE:
      return color_utils::AlphaBlend(kDarkToolbarIcon, kDarkToolbar, 0.3f);
    case BraveThemeProperties::COLOR_SIDEBAR_BACKGROUND:
      return SkColorSetRGB(0x30, 0x34, 0x43);
    case BraveThemeProperties::COLOR_SIDEBAR_ARROW_BACKGROUND_HOVERED:
      return SkColorSetRGB(0x42, 0x45, 0x51);
    case BraveThemeProperties::COLOR_SIDEBAR_ARROW_DISABLED:
      return SkColorSetARGB(0x8A, 0xAE, 0xB1, 0xC2);
    case BraveThemeProperties::COLOR_SIDEBAR_ARROW_NORMAL:
      FALLTHROUGH;
    case BraveThemeProperties::COLOR_SIDEBAR_BUTTON_BASE:
      return SkColorSetRGB(0xC2, 0xC4, 0xCF);
    case BraveThemeProperties::COLOR_SIDEBAR_ADD_BUTTON_DISABLED:
      return SkColorSetARGB(0x66, 0xC2, 0xC4, 0xCF);
    case BraveThemeProperties::COLOR_SIDEBAR_BORDER:
      return SkColorSetRGB(0x3B, 0x3E, 0x4F);
    case BraveThemeProperties::COLOR_SIDEBAR_ITEM_BACKGROUND:
      return SkColorSetRGB(0x41, 0x44, 0x51);
    case BraveThemeProperties::COLOR_SIDEBAR_SEPARATOR:
      return SkColorSetRGB(0xE9, 0xE9, 0xF4);
    case BraveThemeProperties::COLOR_FOR_TEST:
      return BraveThemeProperties::kDarkColorForTest;
    default:
      return base::nullopt;
  }
}

const SkColor kPrivateFrame = SkColorSetRGB(0x19, 0x16, 0x2F);
const SkColor kPrivateToolbar = SkColorSetRGB(0x32, 0x25, 0x60);

base::Optional<SkColor> MaybeGetDefaultColorForPrivateUi(int id) {
  switch (id) {
    // Applies when the window is active, tabs and also tab bar everywhere
    // except active tab
    case ThemeProperties::COLOR_FRAME_ACTIVE:
    case ThemeProperties::COLOR_TAB_BACKGROUND_INACTIVE_FRAME_ACTIVE:
      return kPrivateFrame;
    // Window when the window is innactive, tabs and also tab bar everywhere
    // except active tab
    case ThemeProperties::COLOR_FRAME_INACTIVE:
    case ThemeProperties::COLOR_TAB_BACKGROUND_INACTIVE_FRAME_INACTIVE:
      return color_utils::HSLShift(kPrivateFrame, { -1, -1, 0.55 });
    // Active tab and also the URL toolbar
    // Parts of this color show up as you hover over innactive tabs too
    case ThemeProperties::COLOR_TOOLBAR:
    case ThemeProperties::COLOR_TOOLBAR_CONTENT_AREA_SEPARATOR:
    case ThemeProperties::COLOR_TAB_BACKGROUND_ACTIVE_FRAME_ACTIVE:
    case ThemeProperties::COLOR_TAB_BACKGROUND_ACTIVE_FRAME_INACTIVE:
      return kPrivateToolbar;
    case ThemeProperties::COLOR_TAB_FOREGROUND_ACTIVE_FRAME_ACTIVE:
      return SkColorSetRGB(0xF3, 0xF3, 0xF3);
    case ThemeProperties::COLOR_BOOKMARK_TEXT:
    case BraveThemeProperties::COLOR_BOOKMARK_BAR_INSTRUCTIONS_TEXT:
    case ThemeProperties::COLOR_TAB_FOREGROUND_INACTIVE_FRAME_ACTIVE:
      return SkColorSetRGB(0xFF, 0xFF, 0xFF);
    case ThemeProperties::COLOR_TOOLBAR_BUTTON_ICON:
      return kDarkToolbarIcon;
    case ThemeProperties::COLOR_TOOLBAR_BUTTON_ICON_INACTIVE:
      return color_utils::AlphaBlend(kDarkToolbarIcon, kPrivateToolbar, 0.3f);
    case BraveThemeProperties::COLOR_FOR_TEST:
      return BraveThemeProperties::kPrivateColorForTest;
    // The rest is covered by a dark-appropriate value
    default:
      return MaybeGetDefaultColorForBraveDarkUi(id);
  }
}

const SkColor kPrivateTorFrame = SkColorSetRGB(0x19, 0x0E, 0x2A);
const SkColor kPrivateTorToolbar = SkColorSetRGB(0x49, 0x2D, 0x58);
base::Optional<SkColor> MaybeGetDefaultColorForPrivateTorUi(int id) {
  switch (id) {
    // Applies when the window is active, tabs and also tab bar everywhere
    // except active tab
    case ThemeProperties::COLOR_FRAME_ACTIVE:
    case ThemeProperties::COLOR_TAB_BACKGROUND_INACTIVE_FRAME_ACTIVE:
      return kPrivateTorFrame;
    // Window when the window is innactive, tabs and also tab bar everywhere
    // except active tab
    case ThemeProperties::COLOR_FRAME_INACTIVE:
    case ThemeProperties::COLOR_TAB_BACKGROUND_INACTIVE_FRAME_INACTIVE:
      return color_utils::HSLShift(kPrivateTorFrame, { -1, -1, 0.55 });
    // Active tab and also the URL toolbar
    // Parts of this color show up as you hover over innactive tabs too
    case ThemeProperties::COLOR_TOOLBAR:
    case ThemeProperties::COLOR_TOOLBAR_CONTENT_AREA_SEPARATOR:
    case ThemeProperties::COLOR_TAB_BACKGROUND_ACTIVE_FRAME_ACTIVE:
    case ThemeProperties::COLOR_TAB_BACKGROUND_ACTIVE_FRAME_INACTIVE:
      return kPrivateTorToolbar;
    case ThemeProperties::COLOR_TOOLBAR_BUTTON_ICON_INACTIVE:
      return color_utils::AlphaBlend(kDarkToolbarIcon,
                                     kPrivateTorToolbar,
                                     0.3f);
    // The rest is covered by a private value
    default:
      return MaybeGetDefaultColorForPrivateUi(id);
  }
}

}  // namespace

namespace BraveThemeProperties {

bool IsBraveThemeProperties(int id) {
  return id >= BRAVE_THEME_PROPERTIES_START &&
         id <= BRAVE_THEME_PROPERTIES_LAST;
}

}  // namespace BraveThemeProperties
// Returns a |nullopt| if the UI color is not handled by Brave.
base::Optional<SkColor> MaybeGetDefaultColorForBraveUi(
    int id, bool incognito,
    bool is_tor, dark_mode::BraveDarkModeType dark_mode) {
  // Consistent (and stable) values across all themes
  switch (id) {
    case ThemeProperties::COLOR_TAB_THROBBER_SPINNING:
      return SkColorSetRGB(0xd7, 0x55, 0x26);
    default:
      break;
  }

  if (is_tor) {
    return MaybeGetDefaultColorForPrivateTorUi(id);
  }

  // Allow Private Window theme to override dark vs light
  if (incognito) {
    return MaybeGetDefaultColorForPrivateUi(id);
  }
  // Get Dark or Light value
  switch (dark_mode) {
    case dark_mode::BraveDarkModeType::BRAVE_DARK_MODE_TYPE_LIGHT:
      return MaybeGetDefaultColorForBraveLightUi(id);
    case dark_mode::BraveDarkModeType::BRAVE_DARK_MODE_TYPE_DARK:
      return MaybeGetDefaultColorForBraveDarkUi(id);
    default:
      NOTREACHED();
  }
  return base::nullopt;
}
