/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <cmath>

#include "brave/browser/ui/color/brave_color_id.h"
#include "brave/ui/color/nala/nala_color_id.h"
#include "chrome/browser/ui/color/chrome_color_id.h"
#include "chrome/browser/ui/color/chrome_color_mixers.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/color/color_mixers.h"
#include "ui/color/color_provider.h"
#include "ui/color/color_provider_key.h"

class BraveColorMixersTest : public testing::Test {
 public:
  BraveColorMixersTest() = default;

  ui::ColorProvider& color_provider() { return color_provider_; }
  ui::ColorProviderKey& color_provider_key() { return color_provider_key_; }

  void AddColorMixers() {
    // AddChromeColorMixers() calls all our mixers.
    AddChromeColorMixers(&color_provider_, color_provider_key_);
  }

  // Nala colors are defined by the ui/ mixers, which run before the Chrome
  // ones.
  void AddUiAndChromeColorMixers() {
    ui::AddColorMixers(&color_provider_, color_provider_key_);
    AddColorMixers();
  }

 private:
  ui::ColorProvider color_provider_;
  ui::ColorProviderKey color_provider_key_;
};

TEST_F(BraveColorMixersTest, ColorOverrideTest) {
  AddColorMixers();

  EXPECT_EQ(color_provider().GetColor(kColorToolbar),
            color_provider().GetColor(kColorInfoBarBackground));
  EXPECT_EQ(color_provider().GetColor(kColorOmniboxIconHover),
            SkColorSetA(color_provider().GetColor(kColorOmniboxText),
                        std::ceil(0.10f * 255.0f)));
  EXPECT_EQ(color_provider().GetColor(kColorOmniboxSecurityChipText),
            color_provider().GetColor(kColorOmniboxSecurityChipDangerous));
}

// Tab colors come straight from the generated Nala palette. Shifting them in
// HSL on top of that amplifies the hue that 8-bit rounding leaves in these
// near-grey colors, which tinted the neutral themes ("Grey" red, "Cool grey"
// pink).
class BraveTabPaletteColorMixersTest
    : public BraveColorMixersTest,
      public testing::WithParamInterface<SkColor> {};

INSTANTIATE_TEST_SUITE_P(All,
                         BraveTabPaletteColorMixersTest,
                         testing::Values(SkColorSetRGB(0x88, 0x88, 0x88),
                                         SkColorSetRGB(0x8C, 0xAB, 0xE4)));

TEST_P(BraveTabPaletteColorMixersTest, AccentColorLeavesTabColorsUntintedTest) {
  color_provider_key().user_color = GetParam();
  color_provider_key().user_color_source =
      ui::ColorProviderKey::UserColorSource::kAccent;
  color_provider_key().scheme_variant =
      ui::ColorProviderKey::SchemeVariant::kNeutral;
  AddUiAndChromeColorMixers();

  EXPECT_EQ(color_provider().GetColor(kColorBraveVerticalTabActiveBackground),
            color_provider().GetColor(
                nala::kColorDesktopbrowserTabbarActiveTabVertical));
  EXPECT_EQ(color_provider().GetColor(kColorBraveVerticalTabHoveredBackground),
            color_provider().GetColor(
                nala::kColorDesktopbrowserTabbarHoverTabVertical));
  EXPECT_EQ(
      color_provider().GetColor(kColorTabBackgroundInactiveHoverFrameActive),
      color_provider().GetColor(
          nala::kColorDesktopbrowserTabbarHoverTabHorizontal));
  EXPECT_EQ(
      color_provider().GetColor(kColorBraveSplitViewTileBackgroundHorizontal),
      color_provider().GetColor(
          nala::kColorDesktopbrowserTabbarSplitViewBackgroundHorizontal));
  EXPECT_EQ(
      color_provider().GetColor(kColorBraveSplitViewTileBackgroundVertical),
      color_provider().GetColor(
          nala::kColorDesktopbrowserTabbarSplitViewBackgroundVertical));
}
