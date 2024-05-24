// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ui/color/material_ui_color_mixer.h"

#define AddMaterialUiColorMixer AddMaterialUiColorMixer_UnUsed

#include "src/ui/color/material_ui_color_mixer.cc"
#include "ui/gfx/color_palette.h"

#undef AddMaterialUiColorMixer

namespace ui {

void AddMaterialUiColorMixer(ColorProvider* provider,
                             const ColorProviderKey& key) {
  ui::ColorMixer& mixer = provider->AddMixer();
  const bool is_dark = key.color_mode == ui::ColorProviderKey::ColorMode::kDark;
  mixer[ui::kColorListItemUrlFaviconBackground] = {
      is_dark ? gfx::kGoogleGrey800 : gfx::kGoogleGrey050};
  mixer[ui::kColorListItemFolderIconBackground] = {
      is_dark ? gfx::kGoogleGrey800 : gfx::kGoogleGrey050};
  mixer[ui::kColorListItemFolderIconForeground] = {is_dark ? gfx::kGoogleGrey100
                                                           : SK_ColorBLACK};
  mixer[ui::kColorCheckboxCheck] = {ui::kColorSysOnPrimary};
  mixer[ui::kColorCheckboxCheckDisabled] = {ui::kColorSysStateDisabled};
  mixer[ui::kColorCheckboxContainer] = {ui::kColorSysPrimary};
  mixer[ui::kColorCheckboxContainerDisabled] = {
      ui::kColorSysStateDisabledContainer};
  mixer[ui::kColorCheckboxOutline] = {ui::kColorSysOutline};
  mixer[ui::kColorCheckboxOutlineDisabled] = {
      ui::kColorSysStateDisabledContainer};
  mixer[ui::kColorToggleButtonHover] = {is_dark
                                            ? SkColorSetRGB(0x44, 0x36, 0xE1)
                                            : SkColorSetRGB(0x4C, 0x54, 0xD2)};
  mixer[ui::kColorComboboxInkDropHovered] = {ui::kColorSysStateHoverOnSubtle};
  mixer[ui::kColorComboboxInkDropRipple] = {
      ui::kColorSysStateRippleNeutralOnSubtle};
  mixer[ui::kColorToolbarSearchFieldBackground] = {
      ui::kColorSysBaseContainerElevated};
  mixer[ui::kColorToolbarSearchFieldBackgroundHover] = {
      ui::kColorSysStateHoverCutout};
  mixer[ui::kColorToolbarSearchFieldBackgroundPressed] = {
      ui::kColorSysStateRippleNeutralOnSubtle};
  mixer[ui::kColorToolbarSearchFieldForeground] = {ui::kColorSysOnSurface};
  mixer[ui::kColorToolbarSearchFieldForegroundPlaceholder] = {
      ui::kColorSysOnSurfaceSubtle};
  mixer[ui::kColorToolbarSearchFieldIcon] = {ui::kColorSysOnSurfaceSubtle};
  mixer[ui::kColorToastBackground] = {is_dark ? gfx::kGoogleGrey800
                                              : gfx::kGoogleGrey100};
  mixer[ui::kColorToastButton] = {ui::kColorSysInversePrimary};
  mixer[ui::kColorToastForeground] = {ui::kColorSysOnSurfaceSubtle};
  mixer[ui::kColorToggleButtonHover] = {ui::kColorSysStateHover};
  mixer[ui::kColorToggleButtonPressed] = {ui::kColorSysStatePressed};
  mixer[ui::kColorToggleButtonShadow] = {ui::kColorSysOutline};
  mixer[ui::kColorToggleButtonThumbOff] = {ui::kColorSysOutline};
  mixer[ui::kColorToggleButtonThumbOffDisabled] = {ui::kColorSysStateDisabled};
  mixer[ui::kColorToggleButtonThumbOn] = {ui::kColorSysOnPrimary};
  mixer[ui::kColorToggleButtonThumbOnDisabled] = {ui::kColorSysSurface};
  mixer[ui::kColorToggleButtonThumbOnHover] = {ui::kColorSysPrimaryContainer};
  mixer[ui::kColorToggleButtonTrackOff] = {ui::kColorSysSurfaceVariant};
  mixer[ui::kColorToggleButtonTrackOffDisabled] = {ui::kColorSysSurfaceVariant};
  mixer[ui::kColorToggleButtonTrackOn] = {ui::kColorSysPrimary};
  mixer[ui::kColorToggleButtonTrackOnDisabled] = {
      ui::kColorSysStateDisabledContainer};
}

}  // namespace ui
