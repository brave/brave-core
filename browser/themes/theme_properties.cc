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
    case BraveThemeProperties::COLOR_FOR_TEST:
      return BraveThemeProperties::kLightColorForTest;
    default:
      return base::nullopt;
  }
}

const SkColor kDarkToolbar = SkColorSetRGB(0x39, 0x39, 0x39);
const SkColor kDarkFrame = SkColorSetRGB(0x22, 0x22, 0x22);
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
    case BraveThemeProperties::COLOR_FOR_TEST:
      return BraveThemeProperties::kDarkColorForTest;
    default:
      return base::nullopt;
  }
}

const SkColor kPrivateFrame = SkColorSetRGB(0x1b, 0x0e, 0x2c);
const SkColor kPrivateToolbar = SkColorSetRGB(0x3d, 0x28, 0x41);

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

}  // namespace

namespace BraveThemeProperties {

bool IsBraveThemeProperties(int id) {
  return id >= BRAVE_THEME_PROPERTIES_START &&
         id <= BRAVE_THEME_PROPERTIES_LAST;
}

}  // namespace BraveThemeProperties
// Returns a |nullopt| if the UI color is not handled by Brave.
base::Optional<SkColor> MaybeGetDefaultColorForBraveUi(
    int id, bool incognito, dark_mode::BraveDarkModeType dark_mode) {
  // Consistent (and stable) values across all themes
  switch (id) {
    case ThemeProperties::COLOR_TAB_THROBBER_SPINNING:
      return SkColorSetRGB(0xd7, 0x55, 0x26);
    default:
      break;
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
