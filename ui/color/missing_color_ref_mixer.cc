// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ui/color/missing_color_ref_mixer.h"

#include "ui/color/color_provider.h"
#include "ui/color/color_recipe.h"
#include "ui/color/dynamic_color/palette.h"
#include "ui/color/dynamic_color/palette_factory.h"

namespace ui {
namespace {
void AddBaseLine(ColorProvider* provider) {
  auto& mixer = provider->AddMixer();

  mixer[kColorRefNeutral5] = {SkColorSetRGB(17, 17, 20)};
  mixer[kColorRefNeutral35] = {SkColorSetRGB(82, 82, 86)};

  mixer[kColorRefNeutralVariant5] = {SkColorSetRGB(16, 16, 24)};
  mixer[kColorRefNeutralVariant25] = {SkColorSetRGB(59, 58, 67)};
  mixer[kColorRefNeutralVariant35] = {SkColorSetRGB(82, 81, 91)};
  mixer[kColorRefNeutralVariant98] = {SkColorSetRGB(252, 248, 255)};

  mixer[kColorRefPrimary5] = {SkColorSetRGB(5, 0, 76)};
  mixer[kColorRefPrimary15] = {SkColorSetRGB(25, 11, 125)};
  mixer[kColorRefPrimary35] = {SkColorSetRGB(62, 55, 212)};
  mixer[kColorRefPrimary98] = {SkColorSetRGB(248, 248, 255)};

  mixer[kColorRefSecondary5] = {SkColorSetRGB(38, 6, 0)};
  mixer[kColorRefSecondary98] = {SkColorSetRGB(255, 248, 246)};

  mixer[kColorRefTertiary5] = {SkColorSetRGB(43, 0, 20)};
  mixer[kColorRefTertiary15] = {SkColorSetRGB(81, 0, 42)};
  mixer[kColorRefTertiary25] = {SkColorSetRGB(120, 0, 66)};
  mixer[kColorRefTertiary35] = {SkColorSetRGB(163, 0, 91)};
  mixer[kColorRefTertiary98] = {SkColorSetRGB(255, 248, 248)};
}

void AddGenerated(ColorProvider* provider,
                  SkColor seed_color,
                  ColorProviderKey::SchemeVariant variant) {
  auto palette = GeneratePalette(seed_color, variant);

  auto& mixer = provider->AddMixer();

  mixer[kColorRefNeutral5] = {palette->neutral().get(5)};
  mixer[kColorRefNeutral35] = {palette->neutral().get(35)};

  mixer[kColorRefPrimary5] = {palette->primary().get(5)};
  mixer[kColorRefPrimary15] = {palette->primary().get(15)};
  mixer[kColorRefPrimary35] = {palette->primary().get(35)};
  mixer[kColorRefPrimary98] = {palette->primary().get(98)};

  mixer[kColorRefSecondary5] = {palette->secondary().get(5)};
  mixer[kColorRefSecondary98] = {palette->secondary().get(98)};

  mixer[kColorRefTertiary5] = {palette->tertiary().get(5)};
  mixer[kColorRefTertiary15] = {palette->tertiary().get(15)};
  mixer[kColorRefTertiary25] = {palette->tertiary().get(25)};
  mixer[kColorRefTertiary35] = {palette->tertiary().get(35)};
  mixer[kColorRefTertiary98] = {palette->tertiary().get(98)};
}
}  // namespace

void AddMissingRefColorMixerForNala(ColorProvider* provider,
                                    const ColorProviderKey& key) {
  // Note: The logic for determining whether to use the baseline or generated
  // color is taken from AddRefMixer in
  // ui/color/ref_color_mixer.cc
  if (!key.user_color.has_value() ||
      key.user_color_source == ColorProviderKey::UserColorSource::kBaseline ||
      key.user_color_source == ColorProviderKey::UserColorSource::kGrayscale) {
    AddBaseLine(provider);
  } else {
    auto variant = key.scheme_variant.value_or(
        ColorProviderKey::SchemeVariant::kTonalSpot);
    SkColor user_color = key.user_color.value();
    if (user_color == SK_ColorBLACK) {
      user_color = SkColorSetRGB(0x01, 0x01, 0x01);
    }

    AddGenerated(provider, user_color, variant);
  }
}
}  // namespace ui
