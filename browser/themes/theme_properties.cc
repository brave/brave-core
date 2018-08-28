/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/themes/theme_properties.h"

#include "brave/browser/themes/brave_theme_service.h"
#include "brave/common/pref_names.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/themes/theme_properties.h"
#include "chrome/common/channel_info.h"
#include "components/prefs/pref_service.h"
#include "components/version_info/channel.h"

namespace {

base::Optional<SkColor> MaybeGetDefaultColorForBraveLightUi(int id, bool incognito) {
  switch (id) {
    // Applies when the window is active, tabs and also tab bar everywhere except active tab
    case ThemeProperties::COLOR_FRAME:
    case ThemeProperties::COLOR_BACKGROUND_TAB:
      return incognito ? SkColorSetRGB(0x81, 0x85, 0x89) : SkColorSetRGB(0xD8, 0xDE, 0xE1);
    // Window when the window is innactive, tabs and also tab bar everywhere except active tab
    case ThemeProperties::COLOR_FRAME_INACTIVE:
    case ThemeProperties::COLOR_BACKGROUND_TAB_INACTIVE:
      return incognito ? SkColorSetRGB(0x71, 0x75, 0x79) : SkColorSetRGB(0xC8, 0xCE, 0xC8);
    // Active tab and also the URL toolbar
    // Parts of this color show up as you hover over innactive tabs too
    case ThemeProperties::COLOR_TOOLBAR:
    case ThemeProperties::COLOR_DETACHED_BOOKMARK_BAR_BACKGROUND:
    case ThemeProperties::COLOR_CONTROL_BACKGROUND:
    case ThemeProperties::COLOR_TOOLBAR_CONTENT_AREA_SEPARATOR:
      return incognito ? SkColorSetRGB(0x91, 0x95, 0x99) : SkColorSetRGB(0xF6, 0xF7, 0xF9);
    case ThemeProperties::COLOR_TAB_TEXT:
      return SkColorSetRGB(0x22, 0x23, 0x26);
    case ThemeProperties::COLOR_BOOKMARK_TEXT:
    case ThemeProperties::COLOR_BACKGROUND_TAB_TEXT:
      return SkColorSetRGB(0x22, 0x23, 0x26);
    default:
      return base::nullopt;
  }
}

base::Optional<SkColor> MaybeGetDefaultColorForBraveDarkUi(int id, bool incognito) {
  switch (id) {
    // Applies when the window is active, tabs and also tab bar everywhere except active tab
    case ThemeProperties::COLOR_FRAME:
    case ThemeProperties::COLOR_BACKGROUND_TAB:
      return incognito ? SkColorSetRGB(0x68, 0x6B, 0x6E) : SkColorSetRGB(0x58, 0x5B, 0x5E);
    // Window when the window is innactive, tabs and also tab bar everywhere except active tab
    case ThemeProperties::COLOR_FRAME_INACTIVE:
    case ThemeProperties::COLOR_BACKGROUND_TAB_INACTIVE:
      return incognito ? SkColorSetRGB(0x58, 0x5B, 0x5E) : SkColorSetRGB(0x48, 0x4B, 0x4E);
    // Active tab and also the URL toolbar
    // Parts of this color show up as you hover over innactive tabs too
    case ThemeProperties::COLOR_TOOLBAR:
    case ThemeProperties::COLOR_DETACHED_BOOKMARK_BAR_BACKGROUND:
    case ThemeProperties::COLOR_CONTROL_BACKGROUND:
    case ThemeProperties::COLOR_TOOLBAR_CONTENT_AREA_SEPARATOR:
      return incognito ? SkColorSetRGB(0x32, 0x33, 0x36) : SkColorSetRGB(0x22, 0x23, 0x26);
    case ThemeProperties::COLOR_TAB_TEXT:
      return SkColorSetRGB(0xF7, 0xF8, 0xF9);
    case ThemeProperties::COLOR_BOOKMARK_TEXT:
    case ThemeProperties::COLOR_BACKGROUND_TAB_TEXT:
      return SkColorSetRGB(0x81, 0x85, 0x89);
    default:
      return base::nullopt;
  }
}

}  // namespace

// Returns a |nullopt| if the UI color is not handled by Brave.
base::Optional<SkColor> MaybeGetDefaultColorForBraveUi(int id, bool incognito, Profile* profile) {
  switch (BraveThemeService::GetBraveThemeType(profile)) {
    case BraveThemeService::BRAVE_THEME_TYPE_DEFAULT:
      switch (chrome::GetChannel()) {
        case version_info::Channel::STABLE:
        case version_info::Channel::BETA:
          return MaybeGetDefaultColorForBraveLightUi(id, incognito);
        case version_info::Channel::DEV:
        case version_info::Channel::CANARY:
        case version_info::Channel::UNKNOWN:
        default:
          return MaybeGetDefaultColorForBraveDarkUi(id, incognito);
      }
    case BraveThemeService::BRAVE_THEME_TYPE_LIGHT:
      return MaybeGetDefaultColorForBraveLightUi(id, incognito);
    case BraveThemeService::BRAVE_THEME_TYPE_DARK:
      return MaybeGetDefaultColorForBraveDarkUi(id, incognito);
    default:
      NOTREACHED();

  }
  return base::nullopt;
}
