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

// The "Grey" theme's user color is achromatic, so tab colors must not be
// tinted - HSLShift() would read its hue as 0 and turn them red.
TEST_F(BraveColorMixersTest, AchromaticUserColorLeavesTabColorsUntintedTest) {
  color_provider_key().user_color = SkColorSetRGB(0x88, 0x88, 0x88);
  color_provider_key().user_color_source =
      ui::ColorProviderKey::UserColorSource::kAccent;
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
}
