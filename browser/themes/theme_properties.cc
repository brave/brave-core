/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/themes/theme_properties.h"

#include "base/notreached.h"
#include "brave/browser/ui/color/color_palette.h"
#include "chrome/browser/themes/theme_properties.h"
#include "ui/gfx/color_palette.h"

namespace {

absl::optional<SkColor> MaybeGetDefaultColorForBraveLightUi(int id) {
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
      return color_utils::HSLShift(kLightFrame, {-1, -1, 0.6});
    // Active tab and also the URL toolbar
    // Parts of this color show up as you hover over innactive tabs too
    case ThemeProperties::COLOR_TOOLBAR:
    case ThemeProperties::COLOR_TOOLBAR_TOP_SEPARATOR_FRAME_ACTIVE:
    case ThemeProperties::COLOR_TOOLBAR_TOP_SEPARATOR_FRAME_INACTIVE:
    case ThemeProperties::COLOR_TAB_STROKE_FRAME_ACTIVE:
    case ThemeProperties::COLOR_TAB_STROKE_FRAME_INACTIVE:
    case ThemeProperties::COLOR_TAB_BACKGROUND_ACTIVE_FRAME_ACTIVE:
    case ThemeProperties::COLOR_TAB_BACKGROUND_ACTIVE_FRAME_INACTIVE:
      return kLightToolbar;
    case ThemeProperties::COLOR_TAB_FOREGROUND_ACTIVE_FRAME_ACTIVE:
    case ThemeProperties::COLOR_BOOKMARK_TEXT:
    case ThemeProperties::COLOR_TAB_FOREGROUND_INACTIVE_FRAME_ACTIVE:
    case ThemeProperties::COLOR_TOOLBAR_BUTTON_ICON:
      return kLightToolbarIcon;
    case ThemeProperties::COLOR_TOOLBAR_BUTTON_ICON_INACTIVE:
      return color_utils::AlphaBlend(kLightToolbarIcon, kLightToolbar, 0.3f);
    case BraveThemeProperties::COLOR_ICON_BASE:
      return SkColorSetRGB(0x49, 0x50, 0x57);
    case BraveThemeProperties::COLOR_TOGGLE_BUTTON_THUMB_ON_COLOR:
      return SkColorSetRGB(0x4C, 0x54, 0xD2);
    case BraveThemeProperties::COLOR_TOGGLE_BUTTON_THUMB_OFF_COLOR:
      return SK_ColorWHITE;
    case BraveThemeProperties::COLOR_TOGGLE_BUTTON_TRACK_ON_COLOR:
      return SkColorSetRGB(0xE1, 0xE2, 0xF6);
    case BraveThemeProperties::COLOR_TOGGLE_BUTTON_TRACK_OFF_COLOR:
      return SkColorSetRGB(0xDA, 0xDC, 0xE8);
    case BraveThemeProperties::COLOR_MENU_ITEM_SUB_TEXT_COLOR:
      return SkColorSetRGB(0x86, 0x8E, 0x96);
#if BUILDFLAG(ENABLE_SIDEBAR)
    case BraveThemeProperties::COLOR_SIDEBAR_ARROW_NORMAL:
      return SkColorSetRGB(0x21, 0x25, 0x29);
    case BraveThemeProperties::COLOR_SIDEBAR_ARROW_DISABLED:
      return SkColorSetARGB(0x8A, 0x49, 0x50, 0x57);
    case BraveThemeProperties::COLOR_SIDEBAR_BUTTON_BASE:
      return SkColorSetRGB(0x49, 0x50, 0x57);
    case BraveThemeProperties::COLOR_SIDEBAR_ADD_BUTTON_DISABLED:
      return SkColorSetARGB(0X66, 0x49, 0x50, 0x57);
    case BraveThemeProperties::COLOR_SIDEBAR_ITEM_DRAG_INDICATOR_COLOR:
      return SkColorSetRGB(0x21, 0x25, 0x29);
    case BraveThemeProperties::COLOR_SIDEBAR_SEPARATOR:
      return SkColorSetRGB(0xE6, 0xE8, 0xF5);
    case BraveThemeProperties::COLOR_SIDEBAR_ADD_BUBBLE_BACKGROUND:
      return SK_ColorWHITE;
    case BraveThemeProperties::COLOR_SIDEBAR_ADD_BUBBLE_HEADER_TEXT:
      return SkColorSetRGB(0x17, 0x17, 0x1F);
    case BraveThemeProperties::COLOR_SIDEBAR_ADD_BUBBLE_ITEM_TEXT_NORMAL:
      return SkColorSetRGB(0x21, 0x25, 0x29);
    case BraveThemeProperties::COLOR_SIDEBAR_ADD_BUBBLE_ITEM_TEXT_HOVERED:
      return SkColorSetRGB(0xF0, 0xF2, 0xFF);
    case BraveThemeProperties::
        COLOR_SIDEBAR_ADD_BUBBLE_ITEM_TEXT_BACKGROUND_HOVERED:
      return SkColorSetRGB(0x4C, 0x54, 0xD2);
#endif
#if BUILDFLAG(ENABLE_SPEEDREADER)
    case BraveThemeProperties::COLOR_SPEEDREADER_ICON:
    case BraveThemeProperties::COLOR_SPEEDREADER_TOGGLE_THUMB:
      return SkColorSetRGB(0x4C, 0x54, 0xD2);
    case BraveThemeProperties::COLOR_SPEEDREADER_TOGGLE_TRACK:
      return SkColorSetRGB(0xE1, 0xE2, 0xF6);
#endif
#if BUILDFLAG(ENABLE_BRAVE_VPN)
    case BraveThemeProperties::COLOR_BRAVE_VPN_BUTTON_BORDER:
      return SkColorSetRGB(0xD0, 0xD3, 0xD6);
    case BraveThemeProperties::COLOR_BRAVE_VPN_BUTTON_TEXT_CONNECTED:
      return SkColorSetRGB(0x21, 0x25, 0x29);
    case BraveThemeProperties::COLOR_BRAVE_VPN_BUTTON_TEXT_DISCONNECTED:
      return SkColorSetRGB(0x86, 0x8E, 0x96);
#endif
    case BraveThemeProperties::COLOR_SEARCH_CONVERSION_BANNER_TYPE_DESC_TEXT:
      return SkColorSetRGB(0x2E, 0x30, 0x39);
    case BraveThemeProperties::
        COLOR_SEARCH_CONVERSION_BANNER_TYPE_BACKGROUND_BORDER:
      return SkColorSetRGB(0xE2, 0xE3, 0xF8);
    case BraveThemeProperties::
        COLOR_SEARCH_CONVERSION_BANNER_TYPE_BACKGROUND_BORDER_HOVERED:
      return SkColorSetRGB(0x83, 0x89, 0xE0);
    case BraveThemeProperties::
        COLOR_SEARCH_CONVERSION_BANNER_TYPE_BACKGROUND_GRADIENT_FROM:
      return SkColorSetARGB(104, 0xFF, 0xFF, 0xFF);
    case BraveThemeProperties::
        COLOR_SEARCH_CONVERSION_BANNER_TYPE_BACKGROUND_GRADIENT_TO:
      return SkColorSetARGB(104, 0xEF, 0xEF, 0xFB);
    case BraveThemeProperties::COLOR_SEARCH_CONVERSION_BUTTON_TYPE_INPUT_APPEND:
      return SkColorSetRGB(0x58, 0x5C, 0x6D);
    case BraveThemeProperties::
        COLOR_SEARCH_CONVERSION_BUTTON_TYPE_BACKGROUND_NORMAL:
      return SkColorSetRGB(0xED, 0xEE, 0xFA);
    case BraveThemeProperties::
        COLOR_SEARCH_CONVERSION_BUTTON_TYPE_BACKGROUND_HOVERED:
      return SkColorSetRGB(0xE2, 0xE3, 0xF8);
    case BraveThemeProperties::COLOR_SEARCH_CONVERSION_BUTTON_TYPE_DESC_NORMAL:
      return SkColorSetRGB(0x44, 0x4d, 0xd0);
    case BraveThemeProperties::COLOR_SEARCH_CONVERSION_BUTTON_TYPE_DESC_HOVERED:
      return SkColorSetRGB(0x1F, 0x25, 0x7A);
    case BraveThemeProperties::COLOR_FOR_TEST:
      return BraveThemeProperties::kLightColorForTest;
    default:
      return absl::nullopt;
  }
}

absl::optional<SkColor> MaybeGetDefaultColorForBraveDarkUi(int id) {
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
      return color_utils::HSLShift(kDarkFrame, {-1, -1, 0.6});
    // Active tab and also the URL toolbar
    // Parts of this color show up as you hover over innactive tabs too
    case ThemeProperties::COLOR_TOOLBAR:
    case ThemeProperties::COLOR_TOOLBAR_TOP_SEPARATOR_FRAME_ACTIVE:
    case ThemeProperties::COLOR_TOOLBAR_TOP_SEPARATOR_FRAME_INACTIVE:
    case ThemeProperties::COLOR_TAB_STROKE_FRAME_ACTIVE:
    case ThemeProperties::COLOR_TAB_STROKE_FRAME_INACTIVE:
    case ThemeProperties::COLOR_TOOLBAR_CONTENT_AREA_SEPARATOR:
    case ThemeProperties::COLOR_TAB_BACKGROUND_ACTIVE_FRAME_ACTIVE:
    case ThemeProperties::COLOR_TAB_BACKGROUND_ACTIVE_FRAME_INACTIVE:
      return kDarkToolbar;
    case ThemeProperties::COLOR_TAB_FOREGROUND_ACTIVE_FRAME_ACTIVE:
      return SkColorSetRGB(0xF3, 0xF3, 0xF3);
    case ThemeProperties::COLOR_BOOKMARK_TEXT:
    case ThemeProperties::COLOR_TAB_FOREGROUND_INACTIVE_FRAME_ACTIVE:
      return SkColorSetRGB(0xFF, 0xFF, 0xFF);
    case ThemeProperties::COLOR_TOOLBAR_BUTTON_ICON:
      return kDarkToolbarIcon;
    case ThemeProperties::COLOR_TOOLBAR_BUTTON_ICON_INACTIVE:
      return color_utils::AlphaBlend(kDarkToolbarIcon, kDarkToolbar, 0.3f);
    case BraveThemeProperties::COLOR_ICON_BASE:
      return SkColorSetRGB(0xC2, 0xC4, 0xCF);
    case BraveThemeProperties::COLOR_TOGGLE_BUTTON_THUMB_ON_COLOR:
      return SkColorSetRGB(0x44, 0x36, 0xE1);
    case BraveThemeProperties::COLOR_TOGGLE_BUTTON_THUMB_OFF_COLOR:
      return SK_ColorWHITE;
    case BraveThemeProperties::COLOR_TOGGLE_BUTTON_TRACK_ON_COLOR:
      return SkColorSetRGB(0x76, 0x79, 0xB1);
    case BraveThemeProperties::COLOR_TOGGLE_BUTTON_TRACK_OFF_COLOR:
      return SkColorSetRGB(0x5E, 0x61, 0x75);
    case BraveThemeProperties::COLOR_MENU_ITEM_SUB_TEXT_COLOR:
      return SkColorSetRGB(0x84, 0x88, 0x9C);
#if BUILDFLAG(ENABLE_SIDEBAR)
    case BraveThemeProperties::COLOR_SIDEBAR_ARROW_DISABLED:
      return SkColorSetARGB(0x8A, 0xAE, 0xB1, 0xC2);
    case BraveThemeProperties::COLOR_SIDEBAR_ARROW_NORMAL:
      [[fallthrough]];
    case BraveThemeProperties::COLOR_SIDEBAR_BUTTON_BASE:
      return SkColorSetRGB(0xC2, 0xC4, 0xCF);
    case BraveThemeProperties::COLOR_SIDEBAR_ADD_BUTTON_DISABLED:
      return SkColorSetARGB(0x66, 0xC2, 0xC4, 0xCF);
    case BraveThemeProperties::COLOR_SIDEBAR_ITEM_DRAG_INDICATOR_COLOR:
      return SkColorSetRGB(0xC2, 0xC4, 0xCF);
    case BraveThemeProperties::COLOR_SIDEBAR_SEPARATOR:
      return SkColorSetRGB(0x5E, 0x61, 0x75);
    case BraveThemeProperties::COLOR_SIDEBAR_ADD_BUBBLE_BACKGROUND:
      return gfx::kBraveGrey800;
    case BraveThemeProperties::COLOR_SIDEBAR_ADD_BUBBLE_HEADER_TEXT:
      return SkColorSetRGB(0xF0, 0xF0, 0xFF);
    case BraveThemeProperties::COLOR_SIDEBAR_ADD_BUBBLE_ITEM_TEXT_NORMAL:
      return SkColorSetRGB(0xF0, 0xF0, 0xFF);
    case BraveThemeProperties::COLOR_SIDEBAR_ADD_BUBBLE_ITEM_TEXT_HOVERED:
      return SkColorSetRGB(0xF0, 0xF0, 0xFF);
    case BraveThemeProperties::
        COLOR_SIDEBAR_ADD_BUBBLE_ITEM_TEXT_BACKGROUND_HOVERED:
      return SkColorSetRGB(0x4C, 0x54, 0xD2);
#endif
#if BUILDFLAG(ENABLE_SPEEDREADER)
    case BraveThemeProperties::COLOR_SPEEDREADER_ICON:
      return SkColorSetRGB(0x73, 0x7A, 0xDE);
    case BraveThemeProperties::COLOR_SPEEDREADER_TOGGLE_THUMB:
      return SkColorSetRGB(0x44, 0x36, 0xE1);
    case BraveThemeProperties::COLOR_SPEEDREADER_TOGGLE_TRACK:
      return SkColorSetRGB(0x76, 0x79, 0xB1);
#endif
#if BUILDFLAG(ENABLE_BRAVE_VPN)
    case BraveThemeProperties::COLOR_BRAVE_VPN_BUTTON_BORDER:
      return SkColorSetRGB(0x5E, 0x61, 0x75);
    case BraveThemeProperties::COLOR_BRAVE_VPN_BUTTON_TEXT_CONNECTED:
    case BraveThemeProperties::COLOR_BRAVE_VPN_BUTTON_TEXT_DISCONNECTED:
      return SkColorSetRGB(0xF0, 0xF2, 0xFF);
#endif
    case BraveThemeProperties::COLOR_SEARCH_CONVERSION_BANNER_TYPE_DESC_TEXT:
      return SkColorSetRGB(0xE2, 0xE3, 0xE7);
    case BraveThemeProperties::
        COLOR_SEARCH_CONVERSION_BANNER_TYPE_BACKGROUND_BORDER:
      return SkColorSetRGB(0x1F, 0x25, 0x7A);
    case BraveThemeProperties::
        COLOR_SEARCH_CONVERSION_BANNER_TYPE_BACKGROUND_BORDER_HOVERED:
      return SkColorSetRGB(0x5F, 0x67, 0xD7);
    case BraveThemeProperties::
        COLOR_SEARCH_CONVERSION_BANNER_TYPE_BACKGROUND_GRADIENT_FROM:
      return SkColorSetARGB(104, 0x17, 0x19, 0x1E);
    case BraveThemeProperties::
        COLOR_SEARCH_CONVERSION_BANNER_TYPE_BACKGROUND_GRADIENT_TO:
      return SkColorSetARGB(104, 0x1F, 0x25, 0x7A);
    case BraveThemeProperties::COLOR_SEARCH_CONVERSION_BUTTON_TYPE_INPUT_APPEND:
      return SkColorSetRGB(0xAC, 0xAF, 0xBB);
    case BraveThemeProperties::
        COLOR_SEARCH_CONVERSION_BUTTON_TYPE_BACKGROUND_NORMAL:
      return SkColorSetRGB(0x1A, 0x1C, 0x3B);
    case BraveThemeProperties::
        COLOR_SEARCH_CONVERSION_BUTTON_TYPE_BACKGROUND_HOVERED:
      return SkColorSetRGB(0x1F, 0x25, 0x7A);
    case BraveThemeProperties::COLOR_SEARCH_CONVERSION_BUTTON_TYPE_DESC_NORMAL:
      return SkColorSetRGB(0xA6, 0xAB, 0xE9);
    case BraveThemeProperties::COLOR_SEARCH_CONVERSION_BUTTON_TYPE_DESC_HOVERED:
      return SkColorSetRGB(0xE2, 0xE3, 0xF8);
    case BraveThemeProperties::COLOR_FOR_TEST:
      return BraveThemeProperties::kDarkColorForTest;
    default:
      return absl::nullopt;
  }
}

absl::optional<SkColor> MaybeGetDefaultColorForPrivateUi(int id) {
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
      return color_utils::HSLShift(kPrivateFrame, {-1, -1, 0.55});
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

absl::optional<SkColor> MaybeGetDefaultColorForPrivateTorUi(int id) {
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
      return color_utils::HSLShift(kPrivateTorFrame, {-1, -1, 0.55});
    // Active tab and also the URL toolbar
    // Parts of this color show up as you hover over innactive tabs too
    case ThemeProperties::COLOR_TOOLBAR:
    case ThemeProperties::COLOR_TOOLBAR_CONTENT_AREA_SEPARATOR:
    case ThemeProperties::COLOR_TAB_BACKGROUND_ACTIVE_FRAME_ACTIVE:
    case ThemeProperties::COLOR_TAB_BACKGROUND_ACTIVE_FRAME_INACTIVE:
      return kPrivateTorToolbar;
    case ThemeProperties::COLOR_TOOLBAR_BUTTON_ICON_INACTIVE:
      return color_utils::AlphaBlend(kDarkToolbarIcon, kPrivateTorToolbar,
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
absl::optional<SkColor> MaybeGetDefaultColorForBraveUi(
    int id,
    bool incognito,
    bool is_tor,
    dark_mode::BraveDarkModeType dark_mode) {
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
  return absl::nullopt;
}
