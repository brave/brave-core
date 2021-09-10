// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/views/brave_ads/color_util.h"
#include "base/strings/string_piece.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/skia/include/core/SkColor.h"

// npm run test -- brave_unit_tests --filter=ColorUtilTest.*

TEST(ColorUtilTest, CheckRgbStringToSkColor) {
  SkColor color;
  EXPECT_TRUE(brave_ads::RgbStringToSkColor("42fe4c", &color));
  EXPECT_EQ(SkColorSetRGB(0x42, 0xfe, 0x4c), color);

  EXPECT_FALSE(brave_ads::RgbStringToSkColor("", &color));
  EXPECT_FALSE(brave_ads::RgbStringToSkColor("42fe4", &color));
  EXPECT_FALSE(brave_ads::RgbStringToSkColor("h2fe4c", &color));
}
