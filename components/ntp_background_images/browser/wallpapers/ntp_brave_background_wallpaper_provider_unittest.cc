/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ntp_background_images/browser/wallpapers/ntp_brave_background_wallpaper_provider.h"

#include <memory>
#include <utility>

#include "base/files/file_path.h"
#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "base/test/values_test_util.h"
#include "brave/components/ntp_background_images/browser/ntp_background_images_data.h"
#include "brave/components/ntp_background_images/browser/test/fake_ntp_background_images_service.h"
#include "brave/components/ntp_background_images/browser/wallpapers/test/fake_ntp_custom_background_delegate.h"
#include "brave/components/ntp_background_images/browser/url_constants.h"
#include "brave/components/ntp_background_images/browser/view_counter_model.h"
#include "brave/components/ntp_background_images/common/view_counter_pref_registry.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ntp_background_images {

class NTPBraveBackgroundWallpaperProviderTest : public testing::Test {
 public:
  NTPBraveBackgroundWallpaperProviderTest() = default;

  void SetUp() override {
    RegisterProfilePrefs(prefs_.registry());
    background_images_service_ =
        std::make_unique<FakeNTPBackgroundImagesService>(
            /*variations_service=*/nullptr,
            /*component_updater_service=*/nullptr, &prefs_);
    view_counter_model_ = std::make_unique<ViewCounterModel>(&prefs_);
  }

  std::unique_ptr<NTPBraveBackgroundWallpaperProvider>
  CreateWallpaperProvider() {
    return std::make_unique<NTPBraveBackgroundWallpaperProvider>(
        custom_background_delegate_, *background_images_service_,
        *view_counter_model_);
  }

  void MockBackgroundImagesData() {
    auto background_images_data = std::make_unique<NTPBackgroundImagesData>();
    background_images_data->backgrounds = {
        {base::FilePath(FILE_PATH_LITERAL("wallpaper.jpg")),
         /*author=*/"Brave", /*link=*/"https://brave.com/"}};
    background_images_service_->SetBackgroundImagesData(
        std::move(background_images_data));
    view_counter_model_->set_total_image_count(1);
  }

 protected:
  base::test::TaskEnvironment task_environment_;
  sync_preferences::TestingPrefServiceSyncable prefs_;
  test::FakeNTPCustomBackgroundDelegate custom_background_delegate_;
  std::unique_ptr<FakeNTPBackgroundImagesService> background_images_service_;
  std::unique_ptr<ViewCounterModel> view_counter_model_;
};

TEST_F(NTPBraveBackgroundWallpaperProviderTest, AlwaysEligible) {
  auto wallpaper_provider = CreateWallpaperProvider();

  EXPECT_TRUE(wallpaper_provider->IsEligible());
}

TEST_F(NTPBraveBackgroundWallpaperProviderTest, PrefersThePinnedPhoto) {
  base::DictValue background;
  background.Set("wallpaperImageUrl", "chrome://background-wallpaper/1.jpg");
  custom_background_delegate_.SetPreferredBraveBackground(
      /*has_preferred=*/true, std::move(background));
  auto wallpaper_provider = CreateWallpaperProvider();

  base::test::TestFuture<std::optional<base::DictValue>> test_future;
  wallpaper_provider->MaybeGetWallpaper(test_future.GetCallback());
  EXPECT_THAT(test_future.Take(),
              ::testing::Optional(base::test::IsSupersetOfValue(
                  base::DictValue()
                      .Set(kWallpaperRandomKey, false)
                      .Set("wallpaperImageUrl",
                           "chrome://background-wallpaper/1.jpg"))));
}

TEST_F(NTPBraveBackgroundWallpaperProviderTest,
       FallsBackToRandomWhenPinnedPhotoIsEmpty) {
  custom_background_delegate_.SetPreferredBraveBackground(
      /*has_preferred=*/true, base::DictValue());
  MockBackgroundImagesData();
  auto wallpaper_provider = CreateWallpaperProvider();

  base::test::TestFuture<std::optional<base::DictValue>> test_future;
  wallpaper_provider->MaybeGetWallpaper(test_future.GetCallback());
  EXPECT_THAT(test_future.Take(),
              ::testing::Optional(base::test::IsSupersetOfValue(
                  base::DictValue().Set(kWallpaperRandomKey, true))));
}

TEST_F(NTPBraveBackgroundWallpaperProviderTest,
       FallsBackToRandomWhenNotPinned) {
  MockBackgroundImagesData();
  auto wallpaper_provider = CreateWallpaperProvider();

  base::test::TestFuture<std::optional<base::DictValue>> test_future;
  wallpaper_provider->MaybeGetWallpaper(test_future.GetCallback());
  EXPECT_THAT(test_future.Take(),
              ::testing::Optional(base::test::IsSupersetOfValue(
                  base::DictValue().Set(kWallpaperRandomKey, true))));
}

TEST_F(NTPBraveBackgroundWallpaperProviderTest,
       NoWallpaperWhenIndexIsOutOfRangeForCurrentBackgrounds) {
  MockBackgroundImagesData();
  // Simulate the component data shrinking after the index was chosen: the
  // model believes there are 2 backgrounds and rotates to index 1, but only
  // 1 is actually available now.
  view_counter_model_->set_rand_int_inclusive_callback_for_testing(
      base::BindLambdaForTesting(
          [](int /*min*/, int /*max*/) { return 1; }));
  view_counter_model_->set_total_image_count(2);
  view_counter_model_->RotateBackgroundWallpaperImageIndex();
  ASSERT_EQ(view_counter_model_->current_wallpaper_image_index(), 1);
  auto wallpaper_provider = CreateWallpaperProvider();

  base::test::TestFuture<std::optional<base::DictValue>> test_future;
  wallpaper_provider->MaybeGetWallpaper(test_future.GetCallback());
  EXPECT_FALSE(test_future.Take());
}

TEST_F(NTPBraveBackgroundWallpaperProviderTest,
       NoWallpaperWhenDataUnavailable) {
  auto wallpaper_provider = CreateWallpaperProvider();

  base::test::TestFuture<std::optional<base::DictValue>> test_future;
  wallpaper_provider->MaybeGetWallpaper(test_future.GetCallback());
  EXPECT_FALSE(test_future.Take());
}

}  // namespace ntp_background_images
