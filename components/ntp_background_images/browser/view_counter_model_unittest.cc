// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/components/ntp_background_images/browser/view_counter_model.h"
#include "base/test/scoped_feature_list.h"
#include "base/time/time.h"
#include "brave/components/ntp_background_images/browser/features.h"
#include "brave/components/ntp_background_images/browser/view_counter_service.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {
const size_t kTestImageCount = 3;
const std::vector<size_t> kTestCampaignsTotalImageCount = {3, 2, 3};
}  // namespace

namespace ntp_background_images {

class ViewCounterModelTest : public testing::Test {
 public:
  ViewCounterModelTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}
  ~ViewCounterModelTest() override = default;

  void SetUp() override {
    auto* registry = prefs()->registry();
    ViewCounterService::RegisterProfilePrefs(registry);

    base::FieldTrialParams parameters;
    std::vector<base::test::FeatureRefAndParams> enabled_features;
    parameters[features::kInitialCountToBrandedWallpaper.name] = "2";
    parameters[features::kCountToBrandedWallpaper.name] = "4";
    enabled_features.emplace_back(features::kBraveNTPBrandedWallpaper,
                                  parameters);
    feature_list_.InitWithFeaturesAndParameters(enabled_features, {});
  }

  sync_preferences::TestingPrefServiceSyncable* prefs() { return &prefs_; }

 protected:
  content::BrowserTaskEnvironment task_environment_;
  sync_preferences::TestingPrefServiceSyncable prefs_;
  base::test::ScopedFeatureList feature_list_;
};

TEST_F(ViewCounterModelTest, NTPSponsoredImagesTest) {
  ViewCounterModel model(prefs());

  model.SetCampaignsTotalBrandedImageCount(kTestCampaignsTotalImageCount);

  // Check randomly picked campaign index.
  EXPECT_TRUE(model.current_campaign_index_ >= 0 &&
              model.current_campaign_index_ <
                  kTestCampaignsTotalImageCount.size());

  // Set current campaign index explicitely to test easily.
  model.current_campaign_index_ = 1;

  EXPECT_FALSE(model.always_show_branded_wallpaper_);

  // Loading initial count times.
  for (int i = 0; i < features::kInitialCountToBrandedWallpaper.Get() - 1;
       ++i) {
    EXPECT_FALSE(model.ShouldShowBrandedWallpaper());
    model.RegisterPageView();
  }

  for (int i = 0; i < 30; i++) {
    // Random image should be displayed now after loading initial count.
    EXPECT_TRUE(model.ShouldShowBrandedWallpaper());

    const std::tuple</*campaign index*/ size_t, /*image index*/ size_t>
        current_index = model.GetCurrentBrandedImageIndex();
    const size_t campaign_index = std::get<0>(current_index);
    const size_t image_index = std::get<1>(current_index);

    EXPECT_TRUE(campaign_index >= 0 &&
                campaign_index < kTestCampaignsTotalImageCount.size());
    EXPECT_TRUE(image_index >= 0 &&
                image_index < kTestCampaignsTotalImageCount.at(campaign_index));
    model.RegisterPageView();

    // Loading regular-count times.
    for (int j = 0; j < features::kCountToBrandedWallpaper.Get() - 1; ++j) {
      EXPECT_FALSE(model.ShouldShowBrandedWallpaper());
      model.RegisterPageView();
    }
  }
}

TEST_F(ViewCounterModelTest, NTPSponsoredImagesCountToBrandedWallpaperTest) {
  ViewCounterModel model(prefs());
  model.count_to_branded_wallpaper_ = 1;

  model.SetCampaignsTotalBrandedImageCount(kTestCampaignsTotalImageCount);

  // Check randomly picked campaign index.
  EXPECT_TRUE(model.current_campaign_index_ >= 0 &&
              model.current_campaign_index_ <
                  kTestCampaignsTotalImageCount.size());

  // Set current campaign index explicitely to test easily.
  model.current_campaign_index_ = 1;

  EXPECT_FALSE(model.always_show_branded_wallpaper_);

  // Count is 1 so we should not show branded wallpaper.
  EXPECT_FALSE(model.ShouldShowBrandedWallpaper());
  model.RegisterPageView();

  // Count is 0 so we should show branded wallpaper.
  EXPECT_TRUE(model.ShouldShowBrandedWallpaper());
  model.RegisterPageView();

  // Loading regular-count times from kCountToBrandedWallpaper to 0 and do not
  // show branded wallpaper.
  for (int i = 0; i < features::kCountToBrandedWallpaper.Get() - 1; ++i) {
    EXPECT_FALSE(model.ShouldShowBrandedWallpaper());
    model.RegisterPageView();
  }

  // Count is 0 so we should show branded wallpaper.
  EXPECT_TRUE(model.ShouldShowBrandedWallpaper());
  model.RegisterPageView();
}

TEST_F(ViewCounterModelTest, NTPSponsoredImagesCountResetTest) {
  ViewCounterModel model(prefs());
  model.SetCampaignsTotalBrandedImageCount(kTestCampaignsTotalImageCount);

  // Verify param value for initial count was used
  EXPECT_EQ(model.count_to_branded_wallpaper_, 1);
  model.RegisterPageView();
  EXPECT_TRUE(model.ShouldShowBrandedWallpaper());
  model.RegisterPageView();
  EXPECT_FALSE(model.ShouldShowBrandedWallpaper());
  EXPECT_EQ(model.count_to_branded_wallpaper_, 3);

  // We expect to be reset to initial count when source data updates (which
  // calls Reset).
  model.Reset();
  EXPECT_EQ(model.count_to_branded_wallpaper_, 1);
}

TEST_F(ViewCounterModelTest, NTPSponsoredImagesCountResetMinTest) {
  ViewCounterModel model(prefs());
  model.SetCampaignsTotalBrandedImageCount(kTestCampaignsTotalImageCount);

  // Verify param value for initial count was used
  EXPECT_EQ(model.count_to_branded_wallpaper_, 1);
  model.RegisterPageView();
  EXPECT_TRUE(model.ShouldShowBrandedWallpaper());
  EXPECT_EQ(model.count_to_branded_wallpaper_, 0);

  // We expect to be reset to initial count only if count_to_branded_wallpaper_
  // is higher than initial count.
  model.Reset();
  EXPECT_TRUE(model.ShouldShowBrandedWallpaper());
  EXPECT_EQ(model.count_to_branded_wallpaper_, 0);
}

TEST_F(ViewCounterModelTest, NTPSponsoredImagesCountResetTimerTest) {
  ViewCounterModel model(prefs());
  model.SetCampaignsTotalBrandedImageCount(kTestCampaignsTotalImageCount);

  // Verify param value for initial count was used
  EXPECT_EQ(model.count_to_branded_wallpaper_, 1);
  model.RegisterPageView();
  EXPECT_TRUE(model.ShouldShowBrandedWallpaper());
  model.RegisterPageView();
  EXPECT_FALSE(model.ShouldShowBrandedWallpaper());
  EXPECT_EQ(model.count_to_branded_wallpaper_, 3);
  task_environment_.FastForwardBy(features::kResetCounterAfter.Get());
  EXPECT_EQ(model.count_to_branded_wallpaper_, 1);
}

TEST_F(ViewCounterModelTest, NTPBackgroundImagesTest) {
  ViewCounterModel model(prefs());

  model.SetCampaignsTotalBrandedImageCount(kTestCampaignsTotalImageCount);
  model.set_total_image_count(kTestImageCount);

  // Loading initial count times.
  for (int i = 0; i < features::kInitialCountToBrandedWallpaper.Get() - 1;
       ++i) {
    EXPECT_EQ(i, model.current_wallpaper_image_index());
    model.RegisterPageView();
  }

  // Skip next sponsored image
  model.RegisterPageView();

  // Loading regular-count times.
  int expected_wallpaper_index;
  for (int i = 0; i < features::kCountToBrandedWallpaper.Get() - 1; ++i) {
    expected_wallpaper_index =
        (i + (features::kInitialCountToBrandedWallpaper.Get() - 1)) %
        model.total_image_count_;
    EXPECT_EQ(expected_wallpaper_index, model.current_wallpaper_image_index());
    model.RegisterPageView();
  }

  // It's time for sponsored image.
  EXPECT_EQ(0, model.count_to_branded_wallpaper_);
  const auto bg_index = model.current_wallpaper_image_index();
  model.RegisterPageView();

  // Check bg image index is not changed if sponsored image is shown.
  // Only |count_to_branded_wallpapaer_| is reset.
  EXPECT_NE(0, model.count_to_branded_wallpaper_);
  EXPECT_EQ(bg_index, model.current_wallpaper_image_index());
}

// Test for background images only case (SI option is disabled)
TEST_F(ViewCounterModelTest, NTPBackgroundImagesWithSIDisabledTest) {
  ViewCounterModel model(prefs());

  model.set_total_image_count(kTestImageCount);
  model.SetCampaignsTotalBrandedImageCount(kTestCampaignsTotalImageCount);

  // Check branded wallpaper index is not modified when only BI is used.
  model.set_show_branded_wallpaper(false);
  const auto initial_branded_wallpaper_index =
      model.GetCurrentBrandedImageIndex();

  int expected_wallpaper_index;
  constexpr int kTestPageViewCount = 30;
  for (int i = 0; i < kTestPageViewCount; ++i) {
    expected_wallpaper_index = i % model.total_image_count_;
    EXPECT_EQ(expected_wallpaper_index, model.current_wallpaper_image_index());
    model.RegisterPageView();

    // Check branded wallpaper is not changed.
    EXPECT_EQ(initial_branded_wallpaper_index,
              model.GetCurrentBrandedImageIndex());
  }

  // Disable background image and check its count is not changed.
  const int latest_wallpaper_index = model.current_wallpaper_image_index();
  model.set_show_wallpaper(false);
  for (int i = 0; i < kTestPageViewCount; ++i) {
    EXPECT_EQ(latest_wallpaper_index, model.current_wallpaper_image_index());
    model.RegisterPageView();
  }
}

// Test for background images only case (SI option is enabled but no campaign)
TEST_F(ViewCounterModelTest, NTPBackgroundImagesWithEmptyCampaignTest) {
  ViewCounterModel model(prefs());

  // Check background wallpaper index is properly updated when SI option is
  // enabled but there is no campaign.
  model.Reset();
  model.set_total_image_count(kTestImageCount);
  model.set_show_branded_wallpaper(true);
  model.count_to_branded_wallpaper_ = 0;

  constexpr int kTestPageViewCount = 30;
  for (int i = 0; i < kTestPageViewCount; ++i) {
    EXPECT_EQ(i % model.total_image_count_,
              model.current_wallpaper_image_index());
    model.RegisterPageView();
  }
}

TEST_F(ViewCounterModelTest, NTPSuperReferralTest) {
  ViewCounterModel model(prefs());

  model.set_always_show_branded_wallpaper(true);
  model.SetCampaignsTotalBrandedImageCount({kTestImageCount});
  model.set_total_image_count(kTestImageCount);
  const int initial_wallpaper_index = model.current_wallpaper_image_index();

  // Loading any number of times and check branded wallpaper is visible always
  // with proper index from the start.
  for (int i = 0; i < 10; ++i) {
    EXPECT_TRUE(model.ShouldShowBrandedWallpaper());
    const auto current_index = model.GetCurrentBrandedImageIndex();
    // Always first campaign is used in SR mode.
    EXPECT_EQ(0UL, std::get<0>(current_index));
    EXPECT_EQ(i % kTestImageCount, std::get<1>(current_index));
    model.RegisterPageView();

    // Background wallpaper index is not changed in NTP SR mode.
    EXPECT_EQ(initial_wallpaper_index, model.current_wallpaper_image_index());
  }
}

TEST_F(ViewCounterModelTest, NTPFailedToLoadSponsoredImagesTest) {
  ViewCounterModel model(prefs());

  model.SetCampaignsTotalBrandedImageCount(kTestCampaignsTotalImageCount);
  model.set_total_image_count(kTestImageCount);

  // Loading initial count model.
  for (int i = 0; i < features::kInitialCountToBrandedWallpaper.Get() - 1;
       ++i) {
    EXPECT_EQ(i, model.current_wallpaper_image_index());
    model.RegisterPageView();
  }
  EXPECT_TRUE(model.ShouldShowBrandedWallpaper());
  const int initial_wallpaper_image_index =
      model.current_wallpaper_image_index();

  // Simulate that sponsored image ad was frequency capped by ads service.
  // If |count_to_branded_wallpaper_| is zero when RegisterPageView() is called,
  // only |count_to_branded_wallpaper_| is reset and background image index is
  // not changed because it's time to show branded image. So, need to increase
  // background image explicitely when background image is shown as ads was
  // frequency capped. Client(ViewCounterService) calls this increase method
  // when it's capped.
  model.RotateBackgroundWallpaperImageIndex();
  model.RegisterPageView();

  // Check background index is increased properly.
  int expected_image_index =
      (initial_wallpaper_image_index + 1) % model.total_image_count_;
  EXPECT_EQ(expected_image_index, model.current_wallpaper_image_index());

  // Process register page view for sponsored image.
  model.RegisterPageView();

  expected_image_index =
      (initial_wallpaper_image_index + 2) % model.total_image_count_;
  EXPECT_EQ(expected_image_index, model.current_wallpaper_image_index());
}

}  // namespace ntp_background_images
