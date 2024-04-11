// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ui/color/material_ui_color_mixer.h"

#define AddMaterialUiColorMixer AddMaterialUiColorMixer_Chromium

#include "src/ui/color/material_ui_color_mixer.cc"
#include "ui/gfx/color_palette.h"

#undef AddMaterialUiColorMixer

namespace ui {

namespace {

void AddBraveMaterialUiColorMixer(ColorProvider* provider,
                                  const ColorProviderKey& key) {
  ui::ColorMixer& mixer = provider->AddMixer();
  const bool is_dark = key.color_mode == ui::ColorProviderKey::ColorMode::kDark;
  mixer[ui::kColorListItemUrlFaviconBackground] = {
      is_dark ? gfx::kGoogleGrey800 : gfx::kGoogleGrey050};
  mixer[ui::kColorToggleButtonHover] = {is_dark
                                            ? SkColorSetRGB(0x44, 0x36, 0xE1)
                                            : SkColorSetRGB(0x4C, 0x54, 0xD2)};
}

}  // namespace

void AddMaterialUiColorMixer(ColorProvider* provider,
                             const ColorProviderKey& key) {
  AddMaterialUiColorMixer_Chromium(provider, key);
  AddBraveMaterialUiColorMixer(provider, key);
}

}  // namespace ui
