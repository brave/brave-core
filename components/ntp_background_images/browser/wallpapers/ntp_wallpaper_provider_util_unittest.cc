/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ntp_background_images/browser/wallpapers/ntp_wallpaper_provider_util.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace ntp_background_images {

TEST(NTPWallpaperProviderUtilTest, IsGradientColorForLinearGradient) {
  EXPECT_TRUE(IsGradientColor("linear-gradient(0deg, #ff0000, #0000ff)"));
}

TEST(NTPWallpaperProviderUtilTest, IsGradientColorForRadialGradient) {
  EXPECT_TRUE(IsGradientColor("radial-gradient(circle, #ff0000, #0000ff)"));
}

TEST(NTPWallpaperProviderUtilTest, IsGradientColorFalseForSolidColor) {
  EXPECT_FALSE(IsGradientColor("#ff0000"));
}

TEST(NTPWallpaperProviderUtilTest, IsGradientColorFalseForEmptyString) {
  EXPECT_FALSE(IsGradientColor(""));
}

TEST(NTPWallpaperProviderUtilTest,
     IsGradientColorFalseWhenPrefixIsNotAtTheStart) {
  EXPECT_FALSE(IsGradientColor("not-linear-gradient(0deg, #ff0000, #0000ff)"));
}

TEST(NTPWallpaperProviderUtilTest, IsGradientColorTrueForBarePrefix) {
  EXPECT_TRUE(IsGradientColor("linear-gradient("));
}

}  // namespace ntp_background_images
