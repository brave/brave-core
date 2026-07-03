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

  // Baseline primitive palette from @brave/leo tokens (styles.json). These feed
  // nala::kColorPrimitive* via ui::kColorRef* in nala_color_mixer.cc.
  mixer[kColorRefNeutral0] = {SkColorSetRGB(0x00, 0x00, 0x00)};
  mixer[kColorRefNeutral4] = {SkColorSetRGB(0x0f, 0x0f, 0x0f)};
  mixer[kColorRefNeutral5] = {SkColorSetRGB(0x14, 0x14, 0x15)};
  mixer[kColorRefNeutral6] = {SkColorSetRGB(0x14, 0x14, 0x14)};
  mixer[kColorRefNeutral8] = {SkColorSetRGB(0x18, 0x18, 0x19)};
  mixer[kColorRefNeutral10] = {SkColorSetRGB(0x1c, 0x1c, 0x1d)};
  mixer[kColorRefNeutral12] = {SkColorSetRGB(0x1f, 0x1f, 0x23)};
  mixer[kColorRefNeutral15] = {SkColorSetRGB(0x25, 0x25, 0x27)};
  mixer[kColorRefNeutral17] = {SkColorSetRGB(0x29, 0x29, 0x2d)};
  mixer[kColorRefNeutral20] = {SkColorSetRGB(0x30, 0x30, 0x32)};
  mixer[kColorRefNeutral22] = {SkColorSetRGB(0x34, 0x34, 0x38)};
  mixer[kColorRefNeutral24] = {SkColorSetRGB(0x38, 0x39, 0x3d)};
  mixer[kColorRefNeutral25] = {SkColorSetRGB(0x39, 0x39, 0x3b)};
  mixer[kColorRefNeutral30] = {SkColorSetRGB(0x46, 0x46, 0x49)};
  mixer[kColorRefNeutral35] = {SkColorSetRGB(0x52, 0x52, 0x56)};
  mixer[kColorRefNeutral40] = {SkColorSetRGB(0x5e, 0x5e, 0x62)};
  mixer[kColorRefNeutral50] = {SkColorSetRGB(0x78, 0x78, 0x7c)};
  mixer[kColorRefNeutral60] = {SkColorSetRGB(0x90, 0x90, 0x93)};
  mixer[kColorRefNeutral70] = {SkColorSetRGB(0xaa, 0xaa, 0xad)};
  mixer[kColorRefNeutral80] = {SkColorSetRGB(0xc9, 0xc9, 0xca)};
  mixer[kColorRefNeutral87] = {SkColorSetRGB(0xd9, 0xd9, 0xde)};
  mixer[kColorRefNeutral90] = {SkColorSetRGB(0xe4, 0xe4, 0xe5)};
  mixer[kColorRefNeutral92] = {SkColorSetRGB(0xe8, 0xe8, 0xeb)};
  mixer[kColorRefNeutral94] = {SkColorSetRGB(0xee, 0xee, 0xf1)};
  mixer[kColorRefNeutral95] = {SkColorSetRGB(0xf2, 0xf2, 0xf3)};
  mixer[kColorRefNeutral96] = {SkColorSetRGB(0xf4, 0xf4, 0xf6)};
  mixer[kColorRefNeutral98] = {SkColorSetRGB(0xfa, 0xfa, 0xfb)};
  mixer[kColorRefNeutral99] = {SkColorSetRGB(0xfc, 0xfc, 0xfe)};
  mixer[kColorRefNeutral100] = {SkColorSetRGB(0xff, 0xff, 0xff)};

  mixer[kColorRefNeutralVariant0] = {SkColorSetRGB(0x00, 0x00, 0x00)};
  mixer[kColorRefNeutralVariant5] = {SkColorSetRGB(0x14, 0x13, 0x17)};
  mixer[kColorRefNeutralVariant10] = {SkColorSetRGB(0x1c, 0x1b, 0x20)};
  mixer[kColorRefNeutralVariant15] = {SkColorSetRGB(0x25, 0x24, 0x2b)};
  mixer[kColorRefNeutralVariant20] = {SkColorSetRGB(0x31, 0x2f, 0x37)};
  mixer[kColorRefNeutralVariant25] = {SkColorSetRGB(0x39, 0x38, 0x40)};
  mixer[kColorRefNeutralVariant30] = {SkColorSetRGB(0x47, 0x45, 0x4f)};
  mixer[kColorRefNeutralVariant35] = {SkColorSetRGB(0x53, 0x51, 0x5c)};
  mixer[kColorRefNeutralVariant40] = {SkColorSetRGB(0x5f, 0x5d, 0x6a)};
  mixer[kColorRefNeutralVariant50] = {SkColorSetRGB(0x79, 0x77, 0x84)};
  mixer[kColorRefNeutralVariant60] = {SkColorSetRGB(0x91, 0x8f, 0x9a)};
  mixer[kColorRefNeutralVariant70] = {SkColorSetRGB(0xab, 0xa9, 0xb2)};
  mixer[kColorRefNeutralVariant80] = {SkColorSetRGB(0xc9, 0xc8, 0xcd)};
  mixer[kColorRefNeutralVariant90] = {SkColorSetRGB(0xe4, 0xe4, 0xe6)};
  mixer[kColorRefNeutralVariant95] = {SkColorSetRGB(0xf4, 0xf4, 0xf5)};
  mixer[kColorRefNeutralVariant98] = {SkColorSetRGB(0xfa, 0xfa, 0xf9)};
  mixer[kColorRefNeutralVariant99] = {SkColorSetRGB(0xfb, 0xfc, 0xfb)};
  mixer[kColorRefNeutralVariant100] = {SkColorSetRGB(0xff, 0xff, 0xff)};

  mixer[kColorRefPrimary0] = {SkColorSetRGB(0x00, 0x00, 0x00)};
  mixer[kColorRefPrimary5] = {SkColorSetRGB(0x10, 0x13, 0x22)};
  mixer[kColorRefPrimary10] = {SkColorSetRGB(0x15, 0x19, 0x2d)};
  mixer[kColorRefPrimary15] = {SkColorSetRGB(0x1a, 0x1f, 0x38)};
  mixer[kColorRefPrimary20] = {SkColorSetRGB(0x21, 0x28, 0x48)};
  mixer[kColorRefPrimary25] = {SkColorSetRGB(0x27, 0x2f, 0x55)};
  mixer[kColorRefPrimary30] = {SkColorSetRGB(0x30, 0x3a, 0x68)};
  mixer[kColorRefPrimary35] = {SkColorSetRGB(0x3a, 0x2e, 0xcd)};
  mixer[kColorRefPrimary40] = {SkColorSetRGB(0x43, 0x4f, 0xcf)};
  mixer[kColorRefPrimary50] = {SkColorSetRGB(0x5b, 0x67, 0xe8)};
  mixer[kColorRefPrimary60] = {SkColorSetRGB(0x76, 0x86, 0xec)};
  mixer[kColorRefPrimary70] = {SkColorSetRGB(0x96, 0xa5, 0xf1)};
  mixer[kColorRefPrimary80] = {SkColorSetRGB(0xbc, 0xc6, 0xf3)};
  mixer[kColorRefPrimary90] = {SkColorSetRGB(0xdf, 0xe4, 0xf6)};
  mixer[kColorRefPrimary95] = {SkColorSetRGB(0xee, 0xf2, 0xff)};
  mixer[kColorRefPrimary98] = {SkColorSetRGB(0xf9, 0xfa, 0xfd)};
  mixer[kColorRefPrimary99] = {SkColorSetRGB(0xfb, 0xfb, 0xfd)};
  mixer[kColorRefPrimary100] = {SkColorSetRGB(0xff, 0xff, 0xff)};

  mixer[kColorRefSecondary0] = {SkColorSetRGB(0x00, 0x00, 0x00)};
  mixer[kColorRefSecondary5] = {SkColorSetRGB(0x12, 0x12, 0x22)};
  mixer[kColorRefSecondary10] = {SkColorSetRGB(0x18, 0x18, 0x2c)};
  mixer[kColorRefSecondary12] = {SkColorSetRGB(0x1e, 0x1e, 0x30)};
  mixer[kColorRefSecondary15] = {SkColorSetRGB(0x1e, 0x1f, 0x38)};
  mixer[kColorRefSecondary20] = {SkColorSetRGB(0x2d, 0x2e, 0x42)};
  mixer[kColorRefSecondary25] = {SkColorSetRGB(0x2d, 0x2e, 0x54)};
  mixer[kColorRefSecondary30] = {SkColorSetRGB(0x37, 0x39, 0x67)};
  mixer[kColorRefSecondary35] = {SkColorSetRGB(0x41, 0x43, 0x79)};
  mixer[kColorRefSecondary40] = {SkColorSetRGB(0x4b, 0x4d, 0x8d)};
  mixer[kColorRefSecondary50] = {SkColorSetRGB(0x61, 0x64, 0xb7)};
  mixer[kColorRefSecondary60] = {SkColorSetRGB(0x76, 0x7a, 0xdf)};
  mixer[kColorRefSecondary70] = {SkColorSetRGB(0x92, 0x97, 0xff)};
  mixer[kColorRefSecondary80] = {SkColorSetRGB(0xbb, 0xc1, 0xff)};
  mixer[kColorRefSecondary90] = {SkColorSetRGB(0xd7, 0xdb, 0xff)};
  mixer[kColorRefSecondary95] = {SkColorSetRGB(0xef, 0xf1, 0xff)};
  mixer[kColorRefSecondary98] = {SkColorSetRGB(0xfa, 0xfa, 0xfb)};
  mixer[kColorRefSecondary99] = {SkColorSetRGB(0xfb, 0xfb, 0xfc)};
  mixer[kColorRefSecondary100] = {SkColorSetRGB(0xff, 0xff, 0xff)};

  mixer[kColorRefTertiary0] = {SkColorSetRGB(0x00, 0x00, 0x00)};
  mixer[kColorRefTertiary5] = {SkColorSetRGB(0x1e, 0x0f, 0x19)};
  mixer[kColorRefTertiary10] = {SkColorSetRGB(0x28, 0x14, 0x21)};
  mixer[kColorRefTertiary15] = {SkColorSetRGB(0x32, 0x18, 0x2a)};
  mixer[kColorRefTertiary20] = {SkColorSetRGB(0x40, 0x20, 0x36)};
  mixer[kColorRefTertiary25] = {SkColorSetRGB(0x4b, 0x25, 0x3f)};
  mixer[kColorRefTertiary30] = {SkColorSetRGB(0x5d, 0x2e, 0x4e)};
  mixer[kColorRefTertiary35] = {SkColorSetRGB(0x6c, 0x35, 0x5b)};
  mixer[kColorRefTertiary40] = {SkColorSetRGB(0x7e, 0x3d, 0x69)};
  mixer[kColorRefTertiary50] = {SkColorSetRGB(0xa5, 0x51, 0x8a)};
  mixer[kColorRefTertiary60] = {SkColorSetRGB(0xc8, 0x62, 0xa8)};
  mixer[kColorRefTertiary70] = {SkColorSetRGB(0xe8, 0x7e, 0xc6)};
  mixer[kColorRefTertiary80] = {SkColorSetRGB(0xf2, 0xb1, 0xdb)};
  mixer[kColorRefTertiary90] = {SkColorSetRGB(0xf8, 0xd2, 0xea)};
  mixer[kColorRefTertiary95] = {SkColorSetRGB(0xfd, 0xee, 0xf7)};
  mixer[kColorRefTertiary98] = {SkColorSetRGB(0xfb, 0xf9, 0xfa)};
  mixer[kColorRefTertiary99] = {SkColorSetRGB(0xfb, 0xf9, 0xfa)};
  mixer[kColorRefTertiary100] = {SkColorSetRGB(0xfc, 0xfb, 0xfc)};
}

void AddGenerated(ColorProvider* provider,
                  SkColor seed_color,
                  ColorProviderKey::SchemeVariant variant) {
  auto palette = GeneratePalette(seed_color, variant);

  auto& mixer = provider->AddMixer();

  // Keep in sync with ui::AddGeneratedPalette() so Nala primitives that map to
  // ui::kColorRef* pick up accent theme colors.
  mixer[kColorRefPrimary0] = {palette->primary().get(0)};
  mixer[kColorRefPrimary5] = {palette->primary().get(5)};
  mixer[kColorRefPrimary10] = {palette->primary().get(10)};
  mixer[kColorRefPrimary15] = {palette->primary().get(15)};
  mixer[kColorRefPrimary20] = {palette->primary().get(20)};
  mixer[kColorRefPrimary25] = {palette->primary().get(25)};
  mixer[kColorRefPrimary30] = {palette->primary().get(30)};
  mixer[kColorRefPrimary35] = {palette->primary().get(35)};
  mixer[kColorRefPrimary40] = {palette->primary().get(40)};
  mixer[kColorRefPrimary50] = {palette->primary().get(50)};
  mixer[kColorRefPrimary60] = {palette->primary().get(60)};
  mixer[kColorRefPrimary70] = {palette->primary().get(70)};
  mixer[kColorRefPrimary80] = {palette->primary().get(80)};
  mixer[kColorRefPrimary90] = {palette->primary().get(90)};
  mixer[kColorRefPrimary95] = {palette->primary().get(95)};
  mixer[kColorRefPrimary98] = {palette->primary().get(98)};
  mixer[kColorRefPrimary99] = {palette->primary().get(99)};
  mixer[kColorRefPrimary100] = {palette->primary().get(100)};

  mixer[kColorRefSecondary0] = {palette->secondary().get(0)};
  mixer[kColorRefSecondary5] = {palette->secondary().get(5)};
  mixer[kColorRefSecondary10] = {palette->secondary().get(10)};
  mixer[kColorRefSecondary12] = {palette->secondary().get(12)};
  mixer[kColorRefSecondary15] = {palette->secondary().get(15)};
  mixer[kColorRefSecondary20] = {palette->secondary().get(20)};
  mixer[kColorRefSecondary25] = {palette->secondary().get(25)};
  mixer[kColorRefSecondary30] = {palette->secondary().get(30)};
  mixer[kColorRefSecondary35] = {palette->secondary().get(35)};
  mixer[kColorRefSecondary40] = {palette->secondary().get(40)};
  mixer[kColorRefSecondary50] = {palette->secondary().get(50)};
  mixer[kColorRefSecondary60] = {palette->secondary().get(60)};
  mixer[kColorRefSecondary70] = {palette->secondary().get(70)};
  mixer[kColorRefSecondary80] = {palette->secondary().get(80)};
  mixer[kColorRefSecondary90] = {palette->secondary().get(90)};
  mixer[kColorRefSecondary95] = {palette->secondary().get(95)};
  mixer[kColorRefSecondary98] = {palette->secondary().get(98)};
  mixer[kColorRefSecondary99] = {palette->secondary().get(99)};
  mixer[kColorRefSecondary100] = {palette->secondary().get(100)};

  mixer[kColorRefTertiary0] = {palette->tertiary().get(0)};
  mixer[kColorRefTertiary5] = {palette->tertiary().get(5)};
  mixer[kColorRefTertiary10] = {palette->tertiary().get(10)};
  mixer[kColorRefTertiary15] = {palette->tertiary().get(15)};
  mixer[kColorRefTertiary20] = {palette->tertiary().get(20)};
  mixer[kColorRefTertiary25] = {palette->tertiary().get(25)};
  mixer[kColorRefTertiary30] = {palette->tertiary().get(30)};
  mixer[kColorRefTertiary35] = {palette->tertiary().get(35)};
  mixer[kColorRefTertiary40] = {palette->tertiary().get(40)};
  mixer[kColorRefTertiary50] = {palette->tertiary().get(50)};
  mixer[kColorRefTertiary60] = {palette->tertiary().get(60)};
  mixer[kColorRefTertiary70] = {palette->tertiary().get(70)};
  mixer[kColorRefTertiary80] = {palette->tertiary().get(80)};
  mixer[kColorRefTertiary90] = {palette->tertiary().get(90)};
  mixer[kColorRefTertiary95] = {palette->tertiary().get(95)};
  mixer[kColorRefTertiary98] = {palette->tertiary().get(98)};
  mixer[kColorRefTertiary99] = {palette->tertiary().get(99)};
  mixer[kColorRefTertiary100] = {palette->tertiary().get(100)};

  mixer[kColorRefNeutral0] = {palette->neutral().get(0)};
  mixer[kColorRefNeutral4] = {palette->neutral().get(4)};
  mixer[kColorRefNeutral5] = {palette->neutral().get(5)};
  mixer[kColorRefNeutral6] = {palette->neutral().get(6)};
  mixer[kColorRefNeutral8] = {palette->neutral().get(8)};
  mixer[kColorRefNeutral10] = {palette->neutral().get(10)};
  mixer[kColorRefNeutral12] = {palette->neutral().get(12)};
  mixer[kColorRefNeutral15] = {palette->neutral().get(15)};
  mixer[kColorRefNeutral17] = {palette->neutral().get(17)};
  mixer[kColorRefNeutral20] = {palette->neutral().get(20)};
  mixer[kColorRefNeutral22] = {palette->neutral().get(22)};
  mixer[kColorRefNeutral24] = {palette->neutral().get(24)};
  mixer[kColorRefNeutral25] = {palette->neutral().get(25)};
  mixer[kColorRefNeutral30] = {palette->neutral().get(30)};
  mixer[kColorRefNeutral35] = {palette->neutral().get(35)};
  mixer[kColorRefNeutral40] = {palette->neutral().get(40)};
  mixer[kColorRefNeutral50] = {palette->neutral().get(50)};
  mixer[kColorRefNeutral60] = {palette->neutral().get(60)};
  mixer[kColorRefNeutral70] = {palette->neutral().get(70)};
  mixer[kColorRefNeutral80] = {palette->neutral().get(80)};
  mixer[kColorRefNeutral87] = {palette->neutral().get(87)};
  mixer[kColorRefNeutral90] = {palette->neutral().get(90)};
  mixer[kColorRefNeutral92] = {palette->neutral().get(92)};
  mixer[kColorRefNeutral94] = {palette->neutral().get(94)};
  mixer[kColorRefNeutral95] = {palette->neutral().get(95)};
  mixer[kColorRefNeutral96] = {palette->neutral().get(96)};
  mixer[kColorRefNeutral98] = {palette->neutral().get(98)};
  mixer[kColorRefNeutral99] = {palette->neutral().get(99)};
  mixer[kColorRefNeutral100] = {palette->neutral().get(100)};

  mixer[kColorRefNeutralVariant0] = {palette->neutral_variant().get(0)};
  mixer[kColorRefNeutralVariant5] = {palette->neutral_variant().get(5)};
  mixer[kColorRefNeutralVariant10] = {palette->neutral_variant().get(10)};
  mixer[kColorRefNeutralVariant15] = {palette->neutral_variant().get(15)};
  mixer[kColorRefNeutralVariant20] = {palette->neutral_variant().get(20)};
  mixer[kColorRefNeutralVariant25] = {palette->neutral_variant().get(25)};
  mixer[kColorRefNeutralVariant30] = {palette->neutral_variant().get(30)};
  mixer[kColorRefNeutralVariant35] = {palette->neutral_variant().get(35)};
  mixer[kColorRefNeutralVariant40] = {palette->neutral_variant().get(40)};
  mixer[kColorRefNeutralVariant50] = {palette->neutral_variant().get(50)};
  mixer[kColorRefNeutralVariant60] = {palette->neutral_variant().get(60)};
  mixer[kColorRefNeutralVariant70] = {palette->neutral_variant().get(70)};
  mixer[kColorRefNeutralVariant80] = {palette->neutral_variant().get(80)};
  mixer[kColorRefNeutralVariant90] = {palette->neutral_variant().get(90)};
  mixer[kColorRefNeutralVariant95] = {palette->neutral_variant().get(95)};
  mixer[kColorRefNeutralVariant98] = {palette->neutral_variant().get(98)};
  mixer[kColorRefNeutralVariant99] = {palette->neutral_variant().get(99)};
  mixer[kColorRefNeutralVariant100] = {palette->neutral_variant().get(100)};
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
