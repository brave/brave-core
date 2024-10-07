// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ui/color/material_ui_color_mixer.h"

#define AddMaterialUiColorMixer AddMaterialUiColorMixer_UnUsed

#include "ui/color/color_transform.h"
#include "ui/gfx/color_palette.h"

#include "src/ui/color/material_ui_color_mixer.cc"

#undef AddMaterialUiColorMixer

namespace ui {

void AddMaterialUiColorMixer(ColorProvider* provider,
                             const ColorProviderKey& key) {
  ColorMixer& mixer = provider->AddMixer();
  const bool is_dark = key.color_mode == ColorProviderKey::ColorMode::kDark;
  mixer[kColorListItemUrlFaviconBackground] = {is_dark ? gfx::kGoogleGrey800
                                                       : gfx::kGoogleGrey050};
  mixer[kColorListItemFolderIconBackground] = {is_dark ? gfx::kGoogleGrey800
                                                       : gfx::kGoogleGrey050};
  mixer[kColorListItemFolderIconForeground] = {is_dark ? gfx::kGoogleGrey100
                                                       : SK_ColorBLACK};
  mixer[kColorCheckboxCheck] = {kColorSysOnPrimary};
  mixer[kColorCheckboxCheckDisabled] = {kColorSysStateDisabled};
  mixer[kColorCheckboxContainer] = {kColorSysPrimary};
  mixer[kColorCheckboxContainerDisabled] = {kColorSysStateDisabledContainer};
  mixer[kColorCheckboxOutline] = {kColorSysOutline};
  mixer[kColorCheckboxOutlineDisabled] = {kColorSysStateDisabledContainer};
  mixer[kColorToggleButtonHover] = {is_dark ? SkColorSetRGB(0x44, 0x36, 0xE1)
                                            : SkColorSetRGB(0x4C, 0x54, 0xD2)};
  mixer[kColorComboboxInkDropHovered] = {kColorSysStateHoverOnSubtle};
  mixer[kColorComboboxInkDropRipple] = {kColorSysStateRippleNeutralOnSubtle};
  mixer[kColorComboboxBackground] = {kColorSysSurface};
  mixer[kColorComboboxBackgroundDisabled] = {GetResultingPaintColor(
      {kColorSysStateDisabledContainer}, {kColorComboboxBackground})};
  mixer[kColorComboboxContainerOutline] = {kColorSysNeutralOutline};
  mixer[kColorTabBorderSelected] = {kColorSysPrimary};
  mixer[kColorTabForeground] = {kColorSecondaryForeground};
  mixer[kColorTabForegroundSelected] = {kColorSysPrimary};
  mixer[kColorTableBackground] = {kColorPrimaryBackground};
  mixer[kColorTableBackgroundAlternate] = {kColorTableBackground};
  mixer[kColorTableBackgroundSelectedFocused] = {kColorSysTonalContainer};
  mixer[kColorTableBackgroundSelectedUnfocused] = {
      kColorTableBackgroundSelectedFocused};
  mixer[kColorTableForeground] = {kColorPrimaryForeground};
  mixer[kColorTableForegroundSelectedFocused] = {kColorTableForeground};
  mixer[kColorTableForegroundSelectedUnfocused] = {
      kColorTableForegroundSelectedFocused};
  mixer[kColorToolbarSearchFieldBackground] = {kColorSysBaseContainerElevated};
  mixer[kColorToolbarSearchFieldBackgroundHover] = {
      is_dark ? SetAlpha({kColorRefNeutral10}, 0x0F)
              : SetAlpha({kColorRefNeutral20}, 0x1F)};
  mixer[kColorToolbarSearchFieldBackgroundPressed] = {
      kColorSysStateRippleNeutralOnSubtle};
  mixer[kColorToolbarSearchFieldForeground] = {kColorSysOnSurface};
  mixer[kColorToolbarSearchFieldForegroundPlaceholder] = {
      kColorSysOnSurfaceSubtle};
  mixer[kColorToolbarSearchFieldIcon] = {kColorSysOnSurfaceSubtle};
  mixer[kColorToastBackground] = {is_dark ? gfx::kGoogleGrey800
                                          : gfx::kGoogleGrey100};
  mixer[kColorToastButton] = {kColorSysInversePrimary};
  mixer[kColorToastForeground] = {kColorSysOnSurfaceSubtle};
  mixer[kColorToggleButtonHover] = {kColorSysStateHover};
  mixer[kColorToggleButtonPressed] = {kColorSysStatePressed};
  mixer[kColorToggleButtonShadow] = {kColorSysOutline};
  mixer[kColorToggleButtonThumbOff] = {kColorSysOutline};
  mixer[kColorToggleButtonThumbOffDisabled] = {kColorSysStateDisabled};
  mixer[kColorToggleButtonThumbOn] = {kColorSysOnPrimary};
  mixer[kColorToggleButtonThumbOnDisabled] = {kColorSysSurface};
  mixer[kColorToggleButtonThumbOnHover] = {kColorSysPrimaryContainer};
  mixer[kColorToggleButtonTrackOff] = {kColorSysSurfaceVariant};
  mixer[kColorToggleButtonTrackOffDisabled] = {kColorSysSurfaceVariant};
  mixer[kColorToggleButtonTrackOn] = {kColorSysPrimary};
  mixer[kColorToggleButtonTrackOnDisabled] = {kColorSysStateDisabledContainer};

  mixer[kColorAppMenuRowBackgroundHovered] = {kColorSysStateHoverOnSubtle};
  mixer[kColorAppMenuUpgradeRowBackground] = {
      is_dark ? SkColorSetRGB(0x37, 0x2C, 0xBF)
              : SkColorSetRGB(0xDF, 0xE1, 0xFF)};
}

}  // namespace ui
