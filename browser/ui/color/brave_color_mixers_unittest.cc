/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <cmath>

#include "chrome/browser/ui/color/chrome_color_id.h"
#include "chrome/browser/ui/color/chrome_color_mixers.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/color/color_provider.h"
#include "ui/color/color_provider_key.h"

class BraveColorMixersTest : public testing::Test {
 public:
  BraveColorMixersTest() = default;

  ui::ColorProvider& color_provider() { return color_provider_; }

  void AddColorMixers() {
    // AddChromeColorMixers() calls all our mixers.
    AddChromeColorMixers(&color_provider_, color_provider_key_);
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
}
