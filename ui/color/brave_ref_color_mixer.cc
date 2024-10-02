// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ui/color/brave_ref_color_mixer.h"

#include "ui/color/color_provider.h"
#include "ui/color/color_recipe.h"
#include "ui/color/dynamic_color/palette_factory.h"

namespace ui {
namespace {

void AddBaseline(ColorProvider* provider) {
  auto& mixer = provider->AddMixer();

  // These baseline colors come from Nala. We override the baseline from Chrome
  // because they use a warmer palette, while we have a more blue one.
  // There's a tool for generating these here:
  // https://github.com/fallaciousreasoning/brave-material-base
  mixer[kColorRefNeutral0] = {SkColorSetRGB(0x00, 0x00, 0x00)};
  mixer[kColorRefNeutral4] = {SkColorSetRGB(0x0d, 0x0e, 0x11)};
  mixer[kColorRefNeutral5] = {SkColorSetRGB(0x11, 0x11, 0x14)};
  mixer[kColorRefNeutral6] = {SkColorSetRGB(0x12, 0x13, 0x16)};
  mixer[kColorRefNeutral8] = {SkColorSetRGB(0x17, 0x18, 0x1b)};
  mixer[kColorRefNeutral10] = {SkColorSetRGB(0x1b, 0x1b, 0x1f)};
  mixer[kColorRefNeutral12] = {SkColorSetRGB(0x1f, 0x1f, 0x23)};
  mixer[kColorRefNeutral15] = {SkColorSetRGB(0x25, 0x25, 0x29)};
  mixer[kColorRefNeutral17] = {SkColorSetRGB(0x29, 0x2a, 0x2d)};
  mixer[kColorRefNeutral20] = {SkColorSetRGB(0x30, 0x30, 0x34)};
  mixer[kColorRefNeutral22] = {SkColorSetRGB(0x34, 0x35, 0x38)};
  mixer[kColorRefNeutral24] = {SkColorSetRGB(0x38, 0x39, 0x3c)};
  mixer[kColorRefNeutral25] = {SkColorSetRGB(0x3b, 0x3b, 0x3f)};
  mixer[kColorRefNeutral30] = {SkColorSetRGB(0x46, 0x46, 0x4a)};
  mixer[kColorRefNeutral35] = {SkColorSetRGB(0x52, 0x52, 0x56)};
  mixer[kColorRefNeutral40] = {SkColorSetRGB(0x5e, 0x5e, 0x62)};
  mixer[kColorRefNeutral50] = {SkColorSetRGB(0x76, 0x76, 0x7a)};
  mixer[kColorRefNeutral60] = {SkColorSetRGB(0x90, 0x90, 0x94)};
  mixer[kColorRefNeutral70] = {SkColorSetRGB(0xaa, 0xaa, 0xaf)};
  mixer[kColorRefNeutral80] = {SkColorSetRGB(0xc7, 0xc7, 0xcc)};
  mixer[kColorRefNeutral87] = {SkColorSetRGB(0xdb, 0xd9, 0xdd)};
  mixer[kColorRefNeutral90] = {SkColorSetRGB(0xe3, 0xe3, 0xe8)};
  mixer[kColorRefNeutral92] = {SkColorSetRGB(0xe9, 0xe7, 0xec)};
  mixer[kColorRefNeutral94] = {SkColorSetRGB(0xed, 0xed, 0xf2)};
  mixer[kColorRefNeutral95] = {SkColorSetRGB(0xf0, 0xf0, 0xf4)};
  mixer[kColorRefNeutral96] = {SkColorSetRGB(0xf5, 0xf3, 0xf7)};
  mixer[kColorRefNeutral98] = {SkColorSetRGB(0xfa, 0xfa, 0xfb)};
  mixer[kColorRefNeutral99] = {SkColorSetRGB(0xfc, 0xfc, 0xfe)};
  mixer[kColorRefNeutral100] = {SkColorSetRGB(0xff, 0xff, 0xff)};

  mixer[kColorRefNeutralVariant0] = {SkColorSetRGB(0x00, 0x00, 0x00)};
  mixer[kColorRefNeutralVariant5] = {SkColorSetRGB(0x10, 0x10, 0x18)};
  mixer[kColorRefNeutralVariant10] = {SkColorSetRGB(0x1b, 0x1b, 0x23)};
  mixer[kColorRefNeutralVariant15] = {SkColorSetRGB(0x25, 0x25, 0x2d)};
  mixer[kColorRefNeutralVariant20] = {SkColorSetRGB(0x30, 0x30, 0x38)};
  mixer[kColorRefNeutralVariant25] = {SkColorSetRGB(0x3b, 0x3a, 0x43)};
  mixer[kColorRefNeutralVariant30] = {SkColorSetRGB(0x47, 0x46, 0x4f)};
  mixer[kColorRefNeutralVariant35] = {SkColorSetRGB(0x52, 0x51, 0x5b)};
  mixer[kColorRefNeutralVariant40] = {SkColorSetRGB(0x5e, 0x5d, 0x67)};
  mixer[kColorRefNeutralVariant50] = {SkColorSetRGB(0x77, 0x76, 0x80)};
  mixer[kColorRefNeutralVariant60] = {SkColorSetRGB(0x91, 0x8f, 0x9a)};
  mixer[kColorRefNeutralVariant70] = {SkColorSetRGB(0xac, 0xaa, 0xb4)};
  mixer[kColorRefNeutralVariant80] = {SkColorSetRGB(0xc8, 0xc5, 0xd0)};
  mixer[kColorRefNeutralVariant90] = {SkColorSetRGB(0xe4, 0xe1, 0xec)};
  mixer[kColorRefNeutralVariant95] = {SkColorSetRGB(0xf2, 0xef, 0xfa)};
  mixer[kColorRefNeutralVariant98] = {SkColorSetRGB(0xfc, 0xf8, 0xff)};
  mixer[kColorRefNeutralVariant99] = {SkColorSetRGB(0xff, 0xfb, 0xff)};
  mixer[kColorRefNeutralVariant100] = {SkColorSetRGB(0xff, 0xff, 0xff)};

  mixer[kColorRefPrimary0] = {SkColorSetRGB(0x00, 0x00, 0x00)};
  mixer[kColorRefPrimary5] = {SkColorSetRGB(0x05, 0x00, 0x4c)};
  mixer[kColorRefPrimary10] = {SkColorSetRGB(0x0f, 0x06, 0x63)};
  mixer[kColorRefPrimary15] = {SkColorSetRGB(0x19, 0x0b, 0x7d)};
  mixer[kColorRefPrimary20] = {SkColorSetRGB(0x25, 0x14, 0x91)};
  mixer[kColorRefPrimary25] = {SkColorSetRGB(0x2e, 0x19, 0xb0)};
  mixer[kColorRefPrimary30] = {SkColorSetRGB(0x37, 0x2c, 0xbf)};
  mixer[kColorRefPrimary35] = {SkColorSetRGB(0x3e, 0x37, 0xd4)};
  mixer[kColorRefPrimary40] = {SkColorSetRGB(0x4a, 0x46, 0xe0)};
  mixer[kColorRefPrimary50] = {SkColorSetRGB(0x62, 0x61, 0xff)};
  mixer[kColorRefPrimary60] = {SkColorSetRGB(0x82, 0x83, 0xff)};
  mixer[kColorRefPrimary70] = {SkColorSetRGB(0xa1, 0xa2, 0xff)};
  mixer[kColorRefPrimary80] = {SkColorSetRGB(0xc1, 0xc4, 0xff)};
  mixer[kColorRefPrimary90] = {SkColorSetRGB(0xdf, 0xe1, 0xff)};
  mixer[kColorRefPrimary95] = {SkColorSetRGB(0xef, 0xef, 0xff)};
  mixer[kColorRefPrimary98] = {SkColorSetRGB(0xf8, 0xf8, 0xff)};
  mixer[kColorRefPrimary99] = {SkColorSetRGB(0xfb, 0xfb, 0xff)};
  mixer[kColorRefPrimary100] = {SkColorSetRGB(0xff, 0xff, 0xff)};

  mixer[kColorRefSecondary0] = {SkColorSetRGB(0x00, 0x00, 0x00)};
  mixer[kColorRefSecondary5] = {SkColorSetRGB(0x26, 0x06, 0x00)};
  mixer[kColorRefSecondary10] = {SkColorSetRGB(0x38, 0x0c, 0x00)};
  mixer[kColorRefSecondary12] = {SkColorSetRGB(0x17, 0x20, 0x30)};
  mixer[kColorRefSecondary15] = {SkColorSetRGB(0x4a, 0x13, 0x00)};
  mixer[kColorRefSecondary20] = {SkColorSetRGB(0x5c, 0x1a, 0x00)};
  mixer[kColorRefSecondary25] = {SkColorSetRGB(0x6f, 0x20, 0x00)};
  mixer[kColorRefSecondary30] = {SkColorSetRGB(0x82, 0x28, 0x00)};
  mixer[kColorRefSecondary35] = {SkColorSetRGB(0x96, 0x2f, 0x00)};
  mixer[kColorRefSecondary40] = {SkColorSetRGB(0xaa, 0x37, 0x00)};
  mixer[kColorRefSecondary50] = {SkColorSetRGB(0xd4, 0x46, 0x00)};
  mixer[kColorRefSecondary60] = {SkColorSetRGB(0xff, 0x57, 0x05)};
  mixer[kColorRefSecondary70] = {SkColorSetRGB(0xff, 0x8b, 0x62)};
  mixer[kColorRefSecondary80] = {SkColorSetRGB(0xff, 0xb5, 0x9c)};
  mixer[kColorRefSecondary90] = {SkColorSetRGB(0xff, 0xdb, 0xcf)};
  mixer[kColorRefSecondary95] = {SkColorSetRGB(0xff, 0xed, 0xe8)};
  mixer[kColorRefSecondary98] = {SkColorSetRGB(0xff, 0xf8, 0xf6)};
  mixer[kColorRefSecondary99] = {SkColorSetRGB(0xff, 0xfb, 0xff)};
  mixer[kColorRefSecondary100] = {SkColorSetRGB(0xff, 0xff, 0xff)};

  mixer[kColorRefTertiary0] = {SkColorSetRGB(0x00, 0x00, 0x00)};
  mixer[kColorRefTertiary5] = {SkColorSetRGB(0x2b, 0x00, 0x14)};
  mixer[kColorRefTertiary10] = {SkColorSetRGB(0x3e, 0x00, 0x1f)};
  mixer[kColorRefTertiary15] = {SkColorSetRGB(0x51, 0x00, 0x2a)};
  mixer[kColorRefTertiary20] = {SkColorSetRGB(0x64, 0x00, 0x36)};
  mixer[kColorRefTertiary25] = {SkColorSetRGB(0x78, 0x00, 0x42)};
  mixer[kColorRefTertiary30] = {SkColorSetRGB(0x8d, 0x00, 0x4e)};
  mixer[kColorRefTertiary35] = {SkColorSetRGB(0xa3, 0x00, 0x5b)};
  mixer[kColorRefTertiary40] = {SkColorSetRGB(0xb9, 0x00, 0x68)};
  mixer[kColorRefTertiary50] = {SkColorSetRGB(0xe6, 0x00, 0x83)};
  mixer[kColorRefTertiary60] = {SkColorSetRGB(0xff, 0x47, 0x9c)};
  mixer[kColorRefTertiary70] = {SkColorSetRGB(0xff, 0x83, 0xb2)};
  mixer[kColorRefTertiary80] = {SkColorSetRGB(0xff, 0xb0, 0xca)};
  mixer[kColorRefTertiary90] = {SkColorSetRGB(0xff, 0xd9, 0xe3)};
  mixer[kColorRefTertiary95] = {SkColorSetRGB(0xff, 0xec, 0xf0)};
  mixer[kColorRefTertiary98] = {SkColorSetRGB(0xff, 0xf8, 0xf8)};
  mixer[kColorRefTertiary99] = {SkColorSetRGB(0xff, 0xfb, 0xff)};
  mixer[kColorRefTertiary100] = {SkColorSetRGB(0xff, 0xff, 0xff)};
}

void AddGenerated(ColorProvider* provider,
                  SkColor seed_color,
                  ColorProviderKey::SchemeVariant variant) {
  auto palette = GeneratePalette(seed_color, variant);

  auto& mixer = provider->AddMixer();

  mixer[kColorRefNeutral5] = {palette->neutral().get(5)};
  mixer[kColorRefNeutral35] = {palette->neutral().get(35)};

  mixer[kColorRefNeutralVariant5] = {palette->neutral_variant().get(5)};
  mixer[kColorRefNeutralVariant25] = {palette->neutral_variant().get(25)};
  mixer[kColorRefNeutralVariant35] = {palette->neutral_variant().get(35)};
  mixer[kColorRefNeutralVariant98] = {palette->neutral_variant().get(98)};

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

void AddBraveRefColorMixer(ColorProvider* provider,
                           const ColorProviderKey& key) {
  // Note: The logic for determining whether to use the baseline or generated
  // color is taken from AddRefMixer in
  // ui/color/ref_color_mixer.cc
  if (!key.user_color.has_value() ||
      key.user_color_source == ColorProviderKey::UserColorSource::kBaseline ||
      key.user_color_source == ColorProviderKey::UserColorSource::kGrayscale) {
    AddBaseline(provider);
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
