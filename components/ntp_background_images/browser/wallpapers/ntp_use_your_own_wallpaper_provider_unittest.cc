/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ntp_background_images/browser/wallpapers/ntp_use_your_own_wallpaper_provider.h"

#include "base/test/test_future.h"
#include "base/test/values_test_util.h"
#include "brave/components/ntp_background_images/browser/wallpapers/ntp_wallpaper_provider_constants.h"
#include "brave/components/ntp_background_images/browser/wallpapers/test/fake_ntp_custom_background_delegate.h"
#include "brave/components/ntp_background_images/browser/url_constants.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ntp_background_images {

TEST(NTPUseYourOwnWallpaperProviderTest, NotEligibleByDefault) {
  test::FakeNTPCustomBackgroundDelegate custom_background_delegate;
  NTPUseYourOwnWallpaperProvider wallpaper_provider(custom_background_delegate);

  EXPECT_FALSE(wallpaper_provider.IsEligible());
}

TEST(NTPUseYourOwnWallpaperProviderTest, EligibleWhenCustomImageEnabled) {
  test::FakeNTPCustomBackgroundDelegate custom_background_delegate;
  custom_background_delegate.set_is_custom_image_background_enabled(true);
  custom_background_delegate.set_should_use_random_value(true);
  NTPUseYourOwnWallpaperProvider wallpaper_provider(custom_background_delegate);

  EXPECT_TRUE(wallpaper_provider.IsEligible());
  base::test::TestFuture<std::optional<base::DictValue>> test_future;
  wallpaper_provider.MaybeGetWallpaper(test_future.GetCallback());
  EXPECT_THAT(test_future.Take(),
              ::testing::Optional(base::test::IsSupersetOfValue(
                  base::DictValue()
                      .Set(kWallpaperTypeKey, kWallpaperTypeImage)
                      .Set(kWallpaperURLKey,
                           "chrome://custom-wallpaper/foo.jpg")
                      .Set(kWallpaperRandomKey, true))));
}

TEST(NTPUseYourOwnWallpaperProviderTest, NotRandomWhenRandomValueDisabled) {
  test::FakeNTPCustomBackgroundDelegate custom_background_delegate;
  custom_background_delegate.set_is_custom_image_background_enabled(true);
  custom_background_delegate.set_should_use_random_value(false);
  NTPUseYourOwnWallpaperProvider wallpaper_provider(custom_background_delegate);

  base::test::TestFuture<std::optional<base::DictValue>> test_future;
  wallpaper_provider.MaybeGetWallpaper(test_future.GetCallback());
  EXPECT_THAT(test_future.Take(),
              ::testing::Optional(base::test::IsSupersetOfValue(
                  base::DictValue().Set(kWallpaperRandomKey, false))));
}

}  // namespace ntp_background_images
