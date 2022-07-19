/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/color/color_palette.h"
#include "brave/browser/ui/views/tabs/brave_tab_strip.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gfx/color_utils.h"

TEST(ColorPaletteTest, LightThemeMinimumContrast) {
  // Re-visit kBraveMinimumContrastRatioForOutlines when contrast ratio between
  // kLightToolbar and kLightFrame has lowered.
  EXPECT_GT(color_utils::GetContrastRatio(kLightToolbar, kLightFrame),
            BraveTabStrip::kBraveMinimumContrastRatioForOutlines);
}
