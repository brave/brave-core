// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ui/color/brave_sys_color_mixer.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "ui/color/color_id.h"
#include "ui/color/color_provider.h"
#include "ui/color/color_provider_key.h"
#include "ui/color/ref_color_mixer.h"
#include "ui/color/sys_color_mixer.h"

namespace ui {

TEST(BraveSysColorMixer, DarkModeGrayscaleOverridesAreApplied) {
  ui::ColorProviderKey key;
  key.color_mode = ColorProviderKey::ColorMode::kDark;
  key.user_color_source = ColorProviderKey::UserColorSource::kGrayscale;

  ColorProvider provider;
  AddRefColorMixer(&provider, key);
  AddSysColorMixer(&provider, key);

  // Sanity check these two colors are overriden.
  EXPECT_EQ(provider.GetColor(kColorRefNeutral5),
            provider.GetColor(kColorSysHeader));
  EXPECT_EQ(provider.GetColor(kColorRefNeutral15),
            provider.GetColor(kColorSysBase));
}

TEST(BraveSysColorMixer, LightModeGrayscaleOverridesAreNotApplied) {
  ui::ColorProviderKey key;
  key.color_mode = ColorProviderKey::ColorMode::kLight;
  key.user_color_source = ColorProviderKey::UserColorSource::kGrayscale;

  ColorProvider provider;
  AddRefColorMixer(&provider, key);
  AddSysColorMixer(&provider, key);

  // Sanity check these two colors are not set to our custom dark grayscale
  // theme.
  EXPECT_NE(provider.GetColor(kColorRefNeutral5),
            provider.GetColor(kColorSysHeader));
  EXPECT_NE(provider.GetColor(kColorRefNeutral15),
            provider.GetColor(kColorSysBase));
}

TEST(BraveSysColorMixer, OverridesAreNotAppliedInNonGrayscale) {
  ui::ColorProviderKey key;
  key.color_mode = ColorProviderKey::ColorMode::kDark;
  key.user_color_source = ColorProviderKey::UserColorSource::kBaseline;

  ColorProvider provider_dark;
  AddRefColorMixer(&provider_dark, key);
  AddSysColorMixer(&provider_dark, key);

  // Sanity check these two colors are not set to our custom dark grayscale
  // theme.
  EXPECT_NE(provider_dark.GetColor(kColorRefNeutral5),
            provider_dark.GetColor(kColorSysHeader));
  EXPECT_NE(provider_dark.GetColor(kColorRefNeutral15),
            provider_dark.GetColor(kColorSysBase));

  key.color_mode = ColorProviderKey::ColorMode::kLight;
  ColorProvider provider_light;
  AddRefColorMixer(&provider_light, key);
  AddSysColorMixer(&provider_light, key);

  // Sanity check these two colors are not set to our custom dark grayscale
  // theme.
  EXPECT_NE(provider_light.GetColor(kColorRefNeutral5),
            provider_light.GetColor(kColorSysHeader));
  EXPECT_NE(provider_light.GetColor(kColorRefNeutral15),
            provider_light.GetColor(kColorSysBase));
}

}  // namespace ui
