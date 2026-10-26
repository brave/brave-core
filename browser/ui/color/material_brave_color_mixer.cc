/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/color/material_brave_color_mixer.h"

#include "base/check.h"
#include "brave/ui/color/nala/nala_color_id.h"
#include "chrome/browser/ui/color/chrome_color_id.h"
#include "ui/color/color_provider.h"
#include "ui/color/color_recipe.h"
#include "ui/gfx/color_palette.h"

void AddMaterialBraveColorMixer(ui::ColorProvider* provider,
                                const ui::ColorProviderKey& key) {
  CHECK(provider);
  ui::ColorMixer& mixer = provider->AddMixer();

  mixer[kColorTabSearchScrollbarThumb] = {nala::kColorMiscScrollbarThumbColor};
}
