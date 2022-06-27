/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/color/brave_color_mixer.h"

#include "base/numerics/safe_conversions.h"
#include "brave/browser/ui/color/color_palette.h"
#include "chrome/browser/ui/color/chrome_color_id.h"
#include "ui/color/color_id.h"
#include "ui/color/color_provider.h"
#include "ui/color/color_recipe.h"
#include "ui/gfx/color_palette.h"
#include "ui/gfx/color_utils.h"
#include "ui/native_theme/native_theme.h"

namespace {

// Location bar colors
const SkColor kPrivateLocationBarBgBase = SkColorSetRGB(0x0B, 0x07, 0x24);
const SkColor kDarkLocationBarBgBase = SkColorSetRGB(0x18, 0x1A, 0x21);
const SkColor kDarkLocationBarHoverBg = SkColorSetRGB(0x23, 0x25, 0x2F);

// Copied from //chrome/browser/ui/omnibox//omnibox_theme.h
// As below values are not changed for several years, it would be safe to copy.
// Can't include //chrome/browser/ui/omnibox in here due to circular deps.
constexpr float kOmniboxOpacityHovered = 0.10f;
constexpr float kOmniboxOpacitySelected = 0.16f;

}  // namespace

SkColor GetLocationBarBackground(bool dark, bool priv, bool hover) {
  if (priv) {
    return hover ? color_utils::HSLShift(kPrivateLocationBarBgBase,
                                         {-1, -1, 0.54})
                 : kPrivateLocationBarBgBase;
  }

  if (dark) {
    return hover ? kDarkLocationBarHoverBg : kDarkLocationBarBgBase;
  }

  return hover ? color_utils::AlphaBlend(SK_ColorWHITE,
                                         SkColorSetRGB(0xf3, 0xf3, 0xf3), 0.7f)
               : SK_ColorWHITE;
}

// Omnibox result bg colors
SkColor GetOmniboxResultBackground(int id, bool dark, bool priv) {
  // For high contrast, selected rows use inverted colors to stand out more.
  ui::NativeTheme* native_theme = ui::NativeTheme::GetInstanceForNativeUi();
  bool high_contrast =
      native_theme && native_theme->UserHasContrastPreference();
  float omnibox_opacity = 0.0f;
  if (id == kColorOmniboxResultsBackgroundHovered) {
    omnibox_opacity = kOmniboxOpacityHovered;
  } else if (id == kColorOmniboxResultsBackgroundSelected) {
    omnibox_opacity = kOmniboxOpacitySelected;
  }

  SkColor color;
  if (priv) {
    color = high_contrast ? color_utils::HSLShift(kPrivateLocationBarBgBase,
                                                  {-1, -1, 0.45})
                          : kPrivateLocationBarBgBase;
  } else if (dark) {
    color = high_contrast ? gfx::kGoogleGrey900 : kDarkLocationBarBgBase;
  } else {
    color = SK_ColorWHITE;
  }
  return color_utils::BlendTowardMaxContrast(
      color, base::ClampRound(omnibox_opacity * 0xff));
}

void AddBraveLightThemeColorMixer(ui::ColorProvider* provider,
                                  const ui::ColorProviderManager::Key& key) {
  ui::ColorMixer& mixer = provider->AddMixer();

  mixer[kColorDownloadShelfButtonText] = {gfx::kBraveGrey800};

  mixer[ui::kColorFrameActive] = {kLightFrame};
  mixer[kColorTabBackgroundInactiveFrameActive] = {ui::kColorFrameActive};
  mixer[kColorNewTabButtonBackgroundFrameActive] = {ui::kColorFrameActive};
  mixer[kColorToolbarContentAreaSeparator] = {ui::kColorFrameActive};

  mixer[ui::kColorFrameInactive] = {
      color_utils::HSLShift(kLightFrame, {-1, -1, 0.6})};
  mixer[kColorTabBackgroundInactiveFrameInactive] = {ui::kColorFrameInactive};
  mixer[kColorNewTabButtonBackgroundFrameInactive] = {ui::kColorFrameInactive};

  mixer[kColorToolbar] = {kLightToolbar};
  mixer[kColorToolbarTopSeparatorFrameActive] = {kColorToolbar};
  mixer[kColorToolbarTopSeparatorFrameInactive] = {kColorToolbar};
  mixer[kColorTabBackgroundActiveFrameActive] = {kColorToolbar};
  mixer[kColorTabBackgroundActiveFrameInactive] = {kColorToolbar};

  mixer[kColorTabForegroundActiveFrameActive] = {kLightToolbarIcon};
  mixer[kColorTabForegroundInactiveFrameActive] = {
      kColorTabForegroundActiveFrameActive};
  mixer[kColorBookmarkBarForeground] = {kColorTabForegroundActiveFrameActive};
  mixer[kColorToolbarButtonIcon] = {kColorTabForegroundActiveFrameActive};

  mixer[kColorToolbarButtonIconInactive] = {
      color_utils::AlphaBlend(kLightToolbarIcon, kLightToolbar, 0.3f)};

  mixer[kColorNewTabPageBackground] = {kBraveNewTabBackgroundLight};
}

void AddBraveDarkThemeColorMixer(ui::ColorProvider* provider,
                                 const ui::ColorProviderManager::Key& key) {
  ui::ColorMixer& mixer = provider->AddMixer();

  mixer[kColorDownloadShelfButtonText] = {SK_ColorWHITE};

  mixer[ui::kColorFrameActive] = {kDarkFrame};
  mixer[kColorTabBackgroundInactiveFrameActive] = {ui::kColorFrameActive};
  mixer[kColorNewTabButtonBackgroundFrameActive] = {ui::kColorFrameActive};

  mixer[ui::kColorFrameInactive] = {
      color_utils::HSLShift(kDarkFrame, {-1, -1, 0.6})};
  mixer[kColorTabBackgroundInactiveFrameInactive] = {ui::kColorFrameInactive};
  mixer[kColorNewTabButtonBackgroundFrameInactive] = {ui::kColorFrameInactive};

  mixer[kColorToolbar] = {kDarkToolbar};
  mixer[kColorToolbarTopSeparatorFrameActive] = {kColorToolbar};
  mixer[kColorToolbarTopSeparatorFrameInactive] = {kColorToolbar};
  mixer[kColorToolbarContentAreaSeparator] = {kColorToolbar};
  mixer[kColorTabStrokeFrameActive] = {kColorToolbar};
  mixer[kColorTabStrokeFrameInactive] = {kColorToolbar};
  mixer[kColorTabBackgroundActiveFrameActive] = {kColorToolbar};
  mixer[kColorTabBackgroundActiveFrameInactive] = {kColorToolbar};

  mixer[kColorTabForegroundActiveFrameActive] = {
      SkColorSetRGB(0xF3, 0xF3, 0xF3)};
  mixer[kColorBookmarkBarForeground] = {kColorTabForegroundActiveFrameActive};
  mixer[kColorTabForegroundInactiveFrameActive] = {
      kColorTabForegroundActiveFrameActive};
  mixer[kColorToolbarButtonIcon] = {kDarkToolbarIcon};

  mixer[kColorToolbarButtonIconInactive] = {
      color_utils::AlphaBlend(kDarkToolbarIcon, kDarkToolbar, 0.3f)};

  mixer[kColorNewTabPageBackground] = {kBraveNewTabBackgroundDark};
}

void AddBravePrivateThemeColorMixer(ui::ColorProvider* provider,
                                    const ui::ColorProviderManager::Key& key) {
  AddBraveDarkThemeColorMixer(provider, key);

  ui::ColorMixer& mixer = provider->AddMixer();

  mixer[ui::kColorFrameActive] = {kPrivateFrame};
  mixer[kColorTabBackgroundInactiveFrameActive] = {ui::kColorFrameActive};
  mixer[kColorNewTabButtonBackgroundFrameActive] = {ui::kColorFrameActive};

  mixer[ui::kColorFrameInactive] = {
      color_utils::HSLShift(kPrivateFrame, {-1, -1, 0.55})};
  mixer[kColorTabBackgroundInactiveFrameInactive] = {ui::kColorFrameInactive};
  mixer[kColorNewTabButtonBackgroundFrameInactive] = {ui::kColorFrameInactive};

  mixer[kColorToolbar] = {kPrivateToolbar};
  mixer[kColorToolbarContentAreaSeparator] = {kColorToolbar};
  mixer[kColorTabBackgroundActiveFrameActive] = {kColorToolbar};
  mixer[kColorTabBackgroundActiveFrameInactive] = {kColorToolbar};

  mixer[kColorTabForegroundActiveFrameActive] = {
      SkColorSetRGB(0xF3, 0xF3, 0xF3)};

  mixer[kColorBookmarkBarForeground] = {SkColorSetRGB(0xFF, 0xFF, 0xFF)};
  mixer[kColorTabForegroundInactiveFrameActive] = {
      SkColorSetRGB(0xFF, 0xFF, 0xFF)};

  mixer[kColorToolbarButtonIcon] = {kDarkToolbarIcon};
  mixer[kColorToolbarButtonIconInactive] = {
      color_utils::AlphaBlend(kDarkToolbarIcon, kPrivateToolbar, 0.3f)};

  mixer[kColorNewTabPageBackground] = {kPrivateFrame};
}

void AddBraveTorThemeColorMixer(ui::ColorProvider* provider,
                                const ui::ColorProviderManager::Key& key) {
  AddBravePrivateThemeColorMixer(provider, key);

  ui::ColorMixer& mixer = provider->AddMixer();

  mixer[ui::kColorFrameActive] = {kPrivateTorFrame};
  mixer[kColorTabBackgroundInactiveFrameActive] = {ui::kColorFrameActive};
  mixer[kColorNewTabButtonBackgroundFrameActive] = {ui::kColorFrameActive};

  mixer[ui::kColorFrameInactive] = {
      color_utils::HSLShift(kPrivateTorFrame, {-1, -1, 0.55})};
  mixer[kColorTabBackgroundInactiveFrameInactive] = {ui::kColorFrameInactive};
  mixer[kColorNewTabButtonBackgroundFrameInactive] = {ui::kColorFrameInactive};

  mixer[kColorToolbar] = {kPrivateTorToolbar};
  mixer[kColorToolbarContentAreaSeparator] = {kColorToolbar};
  mixer[kColorTabBackgroundActiveFrameActive] = {kColorToolbar};
  mixer[kColorTabBackgroundActiveFrameInactive] = {kColorToolbar};

  mixer[kColorToolbarButtonIconInactive] = {
      color_utils::AlphaBlend(kDarkToolbarIcon, kPrivateTorToolbar, 0.3f)};

  mixer[kColorNewTabPageBackground] = {kPrivateTorFrame};
}

void AddBraveOmniboxLightThemeColorMixer(
    ui::ColorProvider* provider,
    const ui::ColorProviderManager::Key& key) {
  ui::ColorMixer& mixer = provider->AddMixer();

  mixer[kColorOmniboxBackground] = {GetLocationBarBackground(
      /*dark*/ false, /*private*/ false, /*hover*/ false)};
  mixer[kColorOmniboxBackgroundHovered] = {GetLocationBarBackground(
      /*dark*/ false, /*private*/ false, /*hover*/ true)};
  mixer[kColorOmniboxText] = {kLightOmniboxText};

  mixer[kColorOmniboxResultsBackground] = {GetOmniboxResultBackground(
      kColorOmniboxResultsBackground, /*dark*/ false, /*incognito*/ false)};
  mixer[kColorOmniboxResultsBackgroundHovered] = {
      GetOmniboxResultBackground(kColorOmniboxResultsBackgroundHovered,
                                 /*dark*/ false, /*incognito*/ false)};
  mixer[kColorOmniboxResultsBackgroundSelected] = {
      GetOmniboxResultBackground(kColorOmniboxResultsBackgroundSelected,
                                 /*dark*/ false, /*incognito*/ false)};
}

void AddBraveOmniboxDarkThemeColorMixer(
    ui::ColorProvider* provider,
    const ui::ColorProviderManager::Key& key) {
  ui::ColorMixer& mixer = provider->AddMixer();

  mixer[kColorOmniboxBackground] = {GetLocationBarBackground(
      /*dark*/ true, /*private*/ false, /*hover*/ false)};
  mixer[kColorOmniboxBackgroundHovered] = {GetLocationBarBackground(
      /*dark*/ true, /*private*/ false, /*hover*/ true)};
  mixer[kColorOmniboxText] = {kDarkOmniboxText};

  mixer[kColorOmniboxResultsBackground] = {GetOmniboxResultBackground(
      kColorOmniboxResultsBackground, /*dark*/ true, /*incognito*/ false)};
  mixer[kColorOmniboxResultsBackgroundHovered] = {
      GetOmniboxResultBackground(kColorOmniboxResultsBackgroundHovered,
                                 /*dark*/ true, /*incognito*/ false)};
  mixer[kColorOmniboxResultsBackgroundSelected] = {
      GetOmniboxResultBackground(kColorOmniboxResultsBackgroundSelected,
                                 /*dark*/ true, /*incognito*/ false)};
}

void AddBraveOmniboxPrivateThemeColorMixer(
    ui::ColorProvider* provider,
    const ui::ColorProviderManager::Key& key) {
  ui::ColorMixer& mixer = provider->AddMixer();

  mixer[kColorOmniboxBackground] = {GetLocationBarBackground(
      /*dark*/ false, /*private*/ true, /*hover*/ false)};
  mixer[kColorOmniboxBackgroundHovered] = {GetLocationBarBackground(
      /*dark*/ false, /*private*/ true, /*hover*/ true)};
  mixer[kColorOmniboxText] = {kDarkOmniboxText};

  mixer[kColorOmniboxResultsBackground] = {GetOmniboxResultBackground(
      kColorOmniboxResultsBackground, /*dark*/ false, /*incognito*/ true)};
  mixer[kColorOmniboxResultsBackgroundHovered] = {
      GetOmniboxResultBackground(kColorOmniboxResultsBackgroundHovered,
                                 /*dark*/ false, /*incognito*/ true)};
  mixer[kColorOmniboxResultsBackgroundSelected] = {
      GetOmniboxResultBackground(kColorOmniboxResultsBackgroundSelected,
                                 /*dark*/ false, /*incognito*/ true)};
}
