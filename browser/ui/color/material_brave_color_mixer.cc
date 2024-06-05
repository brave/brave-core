/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/color/material_brave_color_mixer.h"

#include "chrome/browser/ui/color/chrome_color_id.h"
#include "ui/color/color_provider.h"
#include "ui/color/color_recipe.h"
#include "ui/gfx/color_palette.h"

void AddMaterialBraveColorMixer(ui::ColorProvider* provider,
                                const ui::ColorProviderKey& key) {
  CHECK(provider);
  ui::ColorMixer& mixer = provider->AddMixer();
  const bool is_dark = key.color_mode == ui::ColorProviderKey::ColorMode::kDark;

  // Use leo color when it's ready.
  mixer[kColorTabSearchScrollbarThumb] = {
      is_dark ? SkColorSetRGB(0x58, 0x58, 0x58)
              : SkColorSetRGB(0xB4, 0xB4, 0xB4)};
}
