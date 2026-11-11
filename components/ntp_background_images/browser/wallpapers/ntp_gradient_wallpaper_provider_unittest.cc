/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ntp_background_images/browser/wallpapers/ntp_gradient_wallpaper_provider.h"

#include <string>

#include "base/test/test_future.h"
#include "base/test/values_test_util.h"
#include "brave/components/ntp_background_images/browser/wallpapers/ntp_wallpaper_provider_constants.h"
#include "brave/components/ntp_background_images/browser/wallpapers/test/fake_ntp_custom_background_delegate.h"
#include "brave/components/ntp_background_images/browser/url_constants.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ntp_background_images {

namespace {

// Enables the color background on `delegate` and sets `color` as its
// stored value.
void EnableColorBackground(test::FakeNTPCustomBackgroundDelegate& delegate,
                            const std::string& color) {
  delegate.set_is_color_background_enabled(true);
  delegate.set_color(color);
}

}  // namespace

TEST(NTPGradientWallpaperProviderTest, NotEligibleByDefault) {
  test::FakeNTPCustomBackgroundDelegate custom_background_delegate;
  NTPGradientWallpaperProvider wallpaper_provider(custom_background_delegate);

  EXPECT_FALSE(wallpaper_provider.IsEligible());
}

TEST(NTPGradientWallpaperProviderTest,
     NotEligibleWhenColorBackgroundIsDisabledEvenIfColorIsAGradient) {
  test::FakeNTPCustomBackgroundDelegate custom_background_delegate;
  custom_background_delegate.set_is_color_background_enabled(false);
  custom_background_delegate.set_color(
      "linear-gradient(0deg, #ff0000, #0000ff)");
  NTPGradientWallpaperProvider wallpaper_provider(custom_background_delegate);

  EXPECT_FALSE(wallpaper_provider.IsEligible());
}

TEST(NTPGradientWallpaperProviderTest, NotEligibleWhenColorIsSolid) {
  test::FakeNTPCustomBackgroundDelegate custom_background_delegate;
  EnableColorBackground(custom_background_delegate, "#ff0000");
  NTPGradientWallpaperProvider wallpaper_provider(custom_background_delegate);

  EXPECT_FALSE(wallpaper_provider.IsEligible());
}

TEST(NTPGradientWallpaperProviderTest, EligibleWhenColorIsALinearGradient) {
  test::FakeNTPCustomBackgroundDelegate custom_background_delegate;
  EnableColorBackground(custom_background_delegate,
                         "linear-gradient(0deg, #ff0000, #0000ff)");
  NTPGradientWallpaperProvider wallpaper_provider(custom_background_delegate);

  EXPECT_TRUE(wallpaper_provider.IsEligible());
  base::test::TestFuture<std::optional<base::DictValue>> test_future;
  wallpaper_provider.MaybeGetWallpaper(test_future.GetCallback());
  EXPECT_THAT(test_future.Take(),
              ::testing::Optional(base::test::IsSupersetOfValue(
                  base::DictValue()
                      .Set(kWallpaperTypeKey, kWallpaperTypeColor)
                      .Set(kWallpaperColorKey,
                           "linear-gradient(0deg, #ff0000, #0000ff)"))));
}

TEST(NTPGradientWallpaperProviderTest, EligibleWhenColorIsARadialGradient) {
  test::FakeNTPCustomBackgroundDelegate custom_background_delegate;
  EnableColorBackground(custom_background_delegate,
                         "radial-gradient(circle, #ff0000, #0000ff)");
  NTPGradientWallpaperProvider wallpaper_provider(custom_background_delegate);

  EXPECT_TRUE(wallpaper_provider.IsEligible());
}

}  // namespace ntp_background_images
