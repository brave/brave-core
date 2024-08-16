/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/tabs/brave_tab_color_mixer.h"

#include "base/containers/contains.h"
#include "base/containers/fixed_flat_map.h"
#include "base/no_destructor.h"
#include "brave/browser/ui/color/brave_color_id.h"
#include "brave/browser/ui/color/color_palette.h"
#include "brave/ui/color/nala/nala_color_id.h"
#include "chrome/browser/ui/color/chrome_color_id.h"
#include "ui/color/color_mixer.h"
#include "ui/color/color_provider.h"
#include "ui/color/color_provider_key.h"
#include "ui/color/color_recipe.h"

namespace {

std::optional<ChromeColorIds> GetMappedChromeColorId(
    BraveColorIds brave_color_id) {
  static constexpr const auto kChromiumColorMap =
      base::MakeFixedFlatMap<BraveColorIds, ChromeColorIds>(
          // Note that we mapped inactive tab to active tab background and
          // vice versa. Vertical tabs are not in frame color, we should flip
          // them for our design goal.
          {
              {kColorBraveVerticalTabActiveBackground,
               kColorTabBackgroundInactiveFrameActive},
              {kColorBraveVerticalTabInactiveBackground,
               kColorTabBackgroundActiveFrameActive},
              {kColorBraveVerticalTabSeparator, kColorToolbarSeparator},
              {kColorBraveVerticalTabNTBIconColor,
               kColorTabForegroundInactiveFrameActive},
              {kColorBraveVerticalTabNTBTextColor,
               kColorTabForegroundInactiveFrameActive},
              {kColorBraveVerticalTabNTBShortcutTextColor,
               kColorTabForegroundInactiveFrameActive},
              {kColorBraveSplitViewTileBackground,
               kColorTabBackgroundInactiveFrameActive},
          });

  auto iter = kChromiumColorMap.find(brave_color_id);
  if (iter == kChromiumColorMap.end()) {
    return std::nullopt;
  }

  return iter->second;
}

ui::ColorTransform GetCustomColorOrDefaultColor(
    const scoped_refptr<ui::ColorProviderKey::ThemeInitializerSupplier>&
        custom_theme,
    BraveColorIds color_id,
    SkColor color) {
  if (!custom_theme) {
    return {color};
  }

  auto chrome_color_id = GetMappedChromeColorId(color_id);
  if (!chrome_color_id) {
    return {color};
  }

  if (custom_theme->GetColor(*chrome_color_id, &color)) {
    return {color};
  }

  if (color_utils::HSL hsl; custom_theme->GetTint(*chrome_color_id, &hsl)) {
    return {ui::HSLShift(color, hsl)};
  }

  return {*chrome_color_id};
}

}  // namespace

namespace tabs {

void AddBraveTabLightThemeColorMixer(ui::ColorProvider* provider,
                                     const ui::ColorProviderKey& key) {
  ui::ColorMixer& mixer = provider->AddMixer();

  static const base::NoDestructor<base::flat_map<BraveColorIds, SkColor>>
      kDefaultColorMap({
          {kColorBraveVerticalTabActiveBackground,
           mixer.GetResultColor(kColorTabBackgroundActiveFrameActive)},
          {kColorBraveVerticalTabInactiveBackground,
           mixer.GetResultColor(kColorToolbar)},
          {kColorBraveVerticalTabSeparator,
           SkColorSetA(SK_ColorBLACK, 0.05 * 255)},
          {kColorBraveVerticalTabNTBIconColor,
           SkColorSetARGB(0.6 * 255, 0x1D, 0x1F, 0x25)},
          {kColorBraveVerticalTabNTBTextColor, SkColorSetRGB(0x6B, 0x70, 0x84)},
          {kColorBraveVerticalTabNTBShortcutTextColor,
           SkColorSetRGB(0x85, 0x89, 0x89)},
          {kColorBraveSplitViewTileBackground, SkColorSetRGB(0xDA, 0xDF, 0xE1)},
          {kColorBraveSharedPinnedTabDummyViewBackground,
           nala::kColorContainerBackground},
          {kColorBraveSharedPinnedTabDummyViewTitle, nala::kColorTextPrimary},
          {kColorBraveSharedPinnedTabDummyViewDescription,
           nala::kColorTextSecondary},
          {kColorBraveSharedPinnedTabDummyViewThumbnailBorder,
           nala::kColorDividerSubtle},
          {kColorBraveSplitViewMenuButtonIcon, nala::kColorIconInteractive},
          {kColorBraveSplitViewMenuButtonBackground,
           nala::kColorContainerBackground},
          {kColorBraveSplitViewMenuButtonBorder, nala::kColorDividerSubtle},
          {kColorBraveSplitViewActiveWebViewBorder,
           nala::kColorIconInteractive},
          {kColorBraveSplitViewInactiveWebViewBorder, kLightFrame},
          {kColorBraveSplitViewMenuItemIcon, nala::kColorIconDefault},
          {kColorBraveSplitViewUrl, nala::kColorTextTertiary},
      });
  for (const auto& [color_id, default_color] : *kDefaultColorMap) {
    mixer[color_id] =
        GetCustomColorOrDefaultColor(key.custom_theme, color_id, default_color);
  }
}

void AddBraveTabDarkThemeColorMixer(ui::ColorProvider* provider,
                                    const ui::ColorProviderKey& key) {
  ui::ColorMixer& mixer = provider->AddMixer();

  static const base::NoDestructor<base::flat_map<BraveColorIds, SkColor>>
      kDefaultColorMap({
          {kColorBraveVerticalTabActiveBackground,
           mixer.GetResultColor(kColorTabBackgroundActiveFrameActive)},
          {kColorBraveVerticalTabInactiveBackground,
           mixer.GetResultColor(kColorToolbar)},
          {kColorBraveVerticalTabSeparator,
           SkColorSetA(SK_ColorWHITE, 0.1 * 255)},
          {kColorBraveVerticalTabNTBIconColor,
           SkColorSetA(SK_ColorWHITE, 0.6 * 255)},
          {kColorBraveVerticalTabNTBTextColor, SkColorSetRGB(0x8C, 0x90, 0xA1)},
          {kColorBraveVerticalTabNTBShortcutTextColor,
           SkColorSetRGB(0x68, 0x6D, 0x7D)},
          {kColorBraveSplitViewTileBackground, SkColorSetRGB(0x0D, 0x12, 0x14)},
          {kColorBraveSharedPinnedTabDummyViewBackground,
           nala::kColorContainerBackground},
          {kColorBraveSharedPinnedTabDummyViewTitle, nala::kColorTextPrimary},
          {kColorBraveSharedPinnedTabDummyViewDescription,
           nala::kColorTextSecondary},
          {kColorBraveSharedPinnedTabDummyViewThumbnailBorder,
           nala::kColorDividerSubtle},
          {kColorBraveSplitViewMenuButtonIcon, nala::kColorIconInteractive},
          {kColorBraveSplitViewMenuButtonBackground,
           nala::kColorContainerBackground},
          {kColorBraveSplitViewMenuButtonBorder, nala::kColorDividerSubtle},
          {kColorBraveSplitViewActiveWebViewBorder,
           nala::kColorIconInteractive},
          {kColorBraveSplitViewInactiveWebViewBorder, kDarkFrame},
          {kColorBraveSplitViewMenuItemIcon, nala::kColorIconDefault},
          {kColorBraveSplitViewUrl, nala::kColorTextTertiary},
      });
  for (const auto& [color_id, default_color] : *kDefaultColorMap) {
    auto color =
        GetCustomColorOrDefaultColor(key.custom_theme, color_id, default_color);
    if (color_id == kColorBraveVerticalTabActiveBackground ||
        color_id == kColorBraveVerticalTabInactiveBackground) {
      mixer[color_id] = ui::GetResultingPaintColor(
          /* foreground_transform= */ color,
          /* background_transform= */ kColorToolbar);
    } else {
      mixer[color_id] = color;
    }
  }
}

void AddBraveTabPrivateThemeColorMixer(ui::ColorProvider* provider,
                                       const ui::ColorProviderKey& key) {
  auto& mixer = provider->AddMixer();
  mixer[kColorBraveVerticalTabActiveBackground] = {
      mixer.GetResultColor(kColorTabBackgroundActiveFrameActive)};
  mixer[kColorBraveVerticalTabInactiveBackground] = {
      mixer.GetResultColor(kColorToolbar)};
  mixer[kColorBraveSplitViewTileBackground] = {SkColorSetRGB(0x2A, 0xD, 0x58)};
}

void AddBraveTabTorThemeColorMixer(ui::ColorProvider* provider,
                                   const ui::ColorProviderKey& key) {
  auto& mixer = provider->AddMixer();
  mixer[kColorBraveVerticalTabActiveBackground] = {
      mixer.GetResultColor(kColorTabBackgroundActiveFrameActive)};
  mixer[kColorBraveVerticalTabInactiveBackground] = {
      mixer.GetResultColor(kColorToolbar)};
  mixer[kColorBraveSplitViewTileBackground] = {SkColorSetRGB(0x35, 0x0B, 0x49)};
}

}  // namespace tabs
