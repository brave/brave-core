/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/color/material_brave_color_mixer.h"

#include "base/check.h"
#include "base/feature_list.h"
#include "brave/ui/color/nala/nala_color_id.h"
#include "chrome/browser/ui/color/chrome_color_id.h"
#include "ui/color/color_provider.h"
#include "ui/color/color_recipe.h"
#include "ui/gfx/color_palette.h"

#if defined(TOOLKIT_VIEWS)
#include "brave/browser/ui/darker_theme/features.h"
#endif  // defined(TOOLKIT_VIEWS)

void AddMaterialBraveColorMixer(ui::ColorProvider* provider,
                                const ui::ColorProviderKey& key) {
  CHECK(provider);
  ui::ColorMixer& mixer = provider->AddMixer();
  const bool is_dark = key.color_mode == ui::ColorProviderKey::ColorMode::kDark;

  // Use leo color when it's ready.
  mixer[kColorTabSearchScrollbarThumb] = {
      is_dark ? SkColorSetRGB(0x58, 0x58, 0x58)
              : SkColorSetRGB(0xB4, 0xB4, 0xB4)};

#if defined(TOOLKIT_VIEWS)
  if (!key.custom_theme &&
      (!base::FeatureList::IsEnabled(
           darker_theme::features::kBraveDarkerTheme) ||
       key.scheme_variant != ui::ColorProviderKey::SchemeVariant::kDarker)) {
    return;
  }

  // Override saved tab group button / bookmark folder icon color in darker
  // theme.
  mixer[kColorBookmarkFolderIcon] = {nala::kColorPrimitiveNeutral40};
  mixer[kColorBookmarkButtonIcon] = {nala::kColorPrimitiveNeutral40};
#endif  // defined(TOOLKIT_VIEWS)
}
