// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#define AddUiColorMixer AddUiColorMixer_Chromium
#include "src/ui/color/ui_color_mixer.cc"
#undef AddUiColorMixer

namespace ui {

void AddBraveUiColorMixer(ColorProvider* provider,
                          const ColorProviderManager::Key& key) {
  ColorMixer& mixer = provider->AddMixer();

  const bool dark_mode =
      key.color_mode == ColorProviderManager::ColorMode::kDark;

  // --------------------------------------------------------------------------
  // Border colors
  // --------------------------------------------------------------------------
  // For deprecated kColorId_FocusedBorderColor
  mixer[kColorFocusableBorderFocused] = {gfx::kBraveColorBrand};

  // --------------------------------------------------------------------------
  // Button colors
  // --------------------------------------------------------------------------
  // For deprecated kColorId_ProminentButtonColor
  mixer[kColorButtonBackgroundProminent] = {gfx::kBraveColorBrand};
  // For deprecated kColorId_ProminentButtonDisabledColor
  mixer[kColorButtonBackgroundProminentDisabled] = {gfx::kGoogleGrey800};
  // For deprecated kColorId_ProminentButtonFocusedColor
  mixer[kColorButtonBackgroundProminentFocused] = {gfx::kBraveColorBrand};
  // For deprecated kColorId_ButtonBorderColor
  // TODO(simonhong): Add this color to palette.
  mixer[kColorButtonBorder] = {SkColorSetRGB(0xc3, 0xc4, 0xcf)};
  // For deprecated kColorId_ButtonEnabledColor
  mixer[kColorButtonForeground] = {dark_mode ? SK_ColorWHITE
                                             : gfx::kBraveGrey800};
  mixer[kColorButtonForegroundChecked] = {kColorButtonForeground};
  // For deprecated kColorId_TextOnProminentButtonColor
  mixer[kColorButtonForegroundProminent] = {SK_ColorWHITE};

  // --------------------------------------------------------------------------
  // Label colors
  // --------------------------------------------------------------------------
  // For deprecated kColorId_LabelEnabledColor
  mixer[kColorLabelForeground] = {kColorButtonForeground};

  // --------------------------------------------------------------------------
  // Link colors
  // --------------------------------------------------------------------------
  // For deprecated kColorId_LinkEnabled & kColorId_LinkPressed
  mixer[kColorLinkForeground] = {dark_mode ? gfx::kBraveColorOrange300
                                           : gfx::kBraveColorBrand};
  mixer[kColorLinkForegroundPressed] = {kColorLinkForeground};

  // --------------------------------------------------------------------------
  // Table colors (e.g. Task Manager)
  // --------------------------------------------------------------------------
  // For deprecated kColorId_TableSelectionBackgroundFocused and
  // kColorId_TableSelectionBackgroundUnfocused (which were
  // AlphaBlend(kColorId_ProminentButtonColor, kColorId_WindowBackground,
  // SkAlpha{0x3C}))
  mixer[kColorTableBackgroundSelectedFocused] =
      AlphaBlend(gfx::kBraveColorBrand, kColorPrimaryBackground, 0x3C);
  mixer[kColorTableBackgroundSelectedUnfocused] = {
      kColorTableBackgroundSelectedFocused};
  // For deprecated kColorId_TableGroupingIndicatorColor (which was the same as
  // kColorId_FocusedBorderColor)
  mixer[kColorTableGroupingIndicator] = {gfx::kBraveColorBrand};

  // --------------------------------------------------------------------------
  // Text colors
  // --------------------------------------------------------------------------
  // For deprecated kColorId_TextfieldSelectionBackgroundFocused
  mixer[kColorTextfieldSelectionBackground] = {dark_mode ? gfx::kGoogleBlue800
                                                         : gfx::kGoogleBlue200};
}

void AddUiColorMixer(ColorProvider* provider,
                     const ColorProviderManager::Key& key) {
  AddUiColorMixer_Chromium(provider, key);
  AddBraveUiColorMixer(provider, key);
}

}  // namespace ui
