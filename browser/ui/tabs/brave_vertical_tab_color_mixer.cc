/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/tabs/brave_vertical_tab_color_mixer.h"

#include "base/containers/fixed_flat_map.h"
#include "brave/browser/ui/color/brave_color_id.h"
#include "chrome/browser/ui/color/chrome_color_id.h"
#include "ui/color/color_mixer.h"
#include "ui/color/color_provider.h"
#include "ui/color/color_recipe.h"

namespace {

ChromeColorIds GetMappedChromeColorId(BraveColorIds brave_color_id) {
  static constexpr const auto kChromiumColorMap =
      base::MakeFixedFlatMap<BraveColorIds, ChromeColorIds>(
          // Note that we mapped inactive tab to active tab background and
          // vice versa. Vertical tabs are not in frame color, we should flip
          // them for our design goal.
          {{kColorBraveVerticalTabActiveBackground,
            kColorTabBackgroundInactiveFrameActive},
           {kColorBraveVerticalTabInactiveBackground,
            kColorTabBackgroundActiveFrameActive},
           {kColorBraveVerticalTabSeparator, kColorToolbarSeparator},
           {kColorBraveVerticalTabHeaderButtonColor,
            kColorTabForegroundActiveFrameActive}});
  return kChromiumColorMap.at(brave_color_id);
}

ui::ColorRecipe GetCustomColorOrDefaultColor(
    const scoped_refptr<ui::ColorProviderManager::ThemeInitializerSupplier>&
        custom_theme,
    BraveColorIds color_id,
    SkColor color) {
  if (!custom_theme) {
    return {color};
  }

  auto chrome_color_id = GetMappedChromeColorId(color_id);
  if (custom_theme->GetColor(chrome_color_id, &color)) {
    return {color};
  }

  if (color_utils::HSL hsl; custom_theme->GetTint(chrome_color_id, &hsl)) {
    return {ui::HSLShift(color, hsl)};
  }

  return {chrome_color_id};
}

}  // namespace

namespace tabs {

void AddBraveVerticalTabLightThemeColorMixer(
    ui::ColorProvider* provider,
    const ui::ColorProviderManager::Key& key) {
  ui::ColorMixer& mixer = provider->AddMixer();

  static constexpr const auto kDefaultColorMap =
      base::MakeFixedFlatMap<BraveColorIds, SkColor>({
          {kColorBraveVerticalTabActiveBackground, SK_ColorWHITE},
          {kColorBraveVerticalTabInactiveBackground,
           SkColorSetRGB(0xf3, 0xf3, 0xf3)},
          {kColorBraveVerticalTabSeparator, SkColorSetRGB(0xE2, 0xE3, 0xE7)},
          {kColorBraveVerticalTabHeaderButtonColor,
           SkColorSetRGB(0x6B, 0x70, 0x84)},
      });
  for (const auto& [color_id, default_color] : kDefaultColorMap) {
    mixer[color_id] =
        GetCustomColorOrDefaultColor(key.custom_theme, color_id, default_color);
  }
}

void AddBraveVerticalTabDarkThemeColorMixer(
    ui::ColorProvider* provider,
    const ui::ColorProviderManager::Key& key) {
  ui::ColorMixer& mixer = provider->AddMixer();

  static constexpr const auto kDefaultColorMap =
      base::MakeFixedFlatMap<BraveColorIds, SkColor>({
          {kColorBraveVerticalTabActiveBackground,
           SkColorSetRGB(0x18, 0x1A, 0x21)},
          {kColorBraveVerticalTabInactiveBackground,
           SkColorSetRGB(0x30, 0x34, 0x43)},
          {kColorBraveVerticalTabSeparator, SkColorSetRGB(0x5E, 0x61, 0x75)},
          {kColorBraveVerticalTabHeaderButtonColor,
           SkColorSetRGB(0x6B, 0x70, 0x84)},
      });
  for (const auto& [color_id, default_color] : kDefaultColorMap) {
    mixer[color_id] =
        GetCustomColorOrDefaultColor(key.custom_theme, color_id, default_color);
  }
}

}  // namespace tabs
