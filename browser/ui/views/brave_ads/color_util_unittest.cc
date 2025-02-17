/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/brave_ads/color_util.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/skia/include/core/SkColor.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsColorUtilTest, RgbStringToSkColor) {
  SkColor color;
  EXPECT_TRUE(RgbStringToSkColor("42fe4c", &color));
}

TEST(BraveAdsColorUtilTest, DoNotConvertInvalidRgbStringToSkColor) {
  SkColor color;
  EXPECT_FALSE(RgbStringToSkColor("42fe4", &color));
}

TEST(BraveAdsColorUtilTest, DoNotConvertEmptyRgbStringToSkColor) {
  SkColor color;
  EXPECT_FALSE(RgbStringToSkColor("", &color));
}

TEST(BraveAdsColorUtilTest, DoNotConvertNonHexadecimalRgbStringToSkColor) {
  SkColor color;
  EXPECT_FALSE(RgbStringToSkColor("xxxxxx", &color));
}

}  // namespace brave_ads
