/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#define AddCoreDefaultColorMixer AddCoreDefaultColorMixer_Chromium
#include "src/ui/color/core_default_color_mixer.cc"
#undef AddCoreDefaultColorMixer

namespace ui {

void AddBraveCoreDefaultColorMixer(ColorProvider* provider,
                                   const ColorProviderKey& key) {
  ColorMixer& mixer = provider->AddMixer();

  const bool dark_mode = key.color_mode == ColorProviderKey::ColorMode::kDark;

  mixer[kColorAlertMediumSeverityIcon] = {
      dark_mode ? SkColorSetRGB(0xBB, 0x88, 0x00)
                : SkColorSetRGB(0xE2, 0xA5, 0x00)};
}

void AddCoreDefaultColorMixer(ColorProvider* provider,
                              const ColorProviderKey& key) {
  AddCoreDefaultColorMixer_Chromium(provider, key);
  AddBraveCoreDefaultColorMixer(provider, key);
}

}  // namespace ui
