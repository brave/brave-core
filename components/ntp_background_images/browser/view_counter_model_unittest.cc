// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/components/ntp_background_images/browser/view_counter_model.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {
const size_t kTestImageCount = 3;
const std::vector<size_t> kTestCampaignsTotalImageCount = {3, 2, 3};
}  // namespace

namespace ntp_background_images {

TEST(ViewCounterModelTest, NTPSponsoredImagesTest) {
  ViewCounterModel model;
  model.SetCampaignsTotalBrandedImageCount(kTestCampaignsTotalImageCount);

  // Check randomly picked campaign index.
  EXPECT_TRUE(model.current_campaign_index_ >= 0 &&
              model.current_campaign_index_ <
                  kTestCampaignsTotalImageCount.size());

  // Set current campaign index explicitely to test easily.
  model.current_campaign_index_ = 1;

  EXPECT_FALSE(model.always_show_branded_wallpaper_);

  // Loading initial count times.
  for (int i = 0; i < ViewCounterModel::kInitialCountToBrandedWallpaper; ++i) {
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
    for (int j = 0; j < ViewCounterModel::kRegularCountToBrandedWallpaper;
         ++j) {
      EXPECT_FALSE(model.ShouldShowBrandedWallpaper());
      model.RegisterPageView();
    }
  }
}

TEST(ViewCounterModelTest, NTPBackgroundImagesTest) {
  ViewCounterModel model;
  model.SetCampaignsTotalBrandedImageCount(kTestCampaignsTotalImageCount);
  model.set_total_image_count(kTestImageCount);

  // Loading initial count times.
  for (int i = 0; i < ViewCounterModel::kInitialCountToBrandedWallpaper; ++i) {
    EXPECT_EQ(i, model.current_wallpaper_image_index());
    model.RegisterPageView();
  }

  // Skip next sponsored image
  model.RegisterPageView();

  // Loading regular-count times.
  int expected_wallpaper_index;
  for (int i = 0; i < ViewCounterModel::kRegularCountToBrandedWallpaper; ++i) {
    expected_wallpaper_index =
        (i + ViewCounterModel::kInitialCountToBrandedWallpaper) %
        model.total_image_count_;
    EXPECT_EQ(expected_wallpaper_index, model.current_wallpaper_image_index());
    model.RegisterPageView();
  }
}

// Test for background images only case (SI not active)
TEST(ViewCounterModelTest, NTPBackgroundImagesOnlyTest) {
  ViewCounterModel model;
  model.set_total_image_count(kTestImageCount);
  model.SetCampaignsTotalBrandedImageCount(kTestCampaignsTotalImageCount);

  // Check branded wallpaper index is not modified when only BI is used.
  model.set_show_branded_wallpaper(false);
  const auto initial_branded_wallpaper_index =
      model.GetCurrentBrandedImageIndex();

  int expected_wallpaper_index;
  const int kTestPageViewCount = 30;
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

TEST(ViewCounterModelTest, NTPSuperReferralTest) {
  ViewCounterModel model;
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

TEST(ViewCounterModelTest, NTPFailedToLoadSponsoredImagesTest) {
  ViewCounterModel model;
  model.SetCampaignsTotalBrandedImageCount(kTestCampaignsTotalImageCount);
  model.set_total_image_count(kTestImageCount);

  // Loading initial count times.
  for (int i = 0; i < ViewCounterModel::kInitialCountToBrandedWallpaper; ++i) {
    EXPECT_EQ(i, model.current_wallpaper_image_index());
    model.RegisterPageView();
  }
  EXPECT_TRUE(model.ShouldShowBrandedWallpaper());
  const int initial_wallpaper_image_index =
      model.current_wallpaper_image_index();

  // Simulate that sponsored image ad was frequency capped by ads service. In
  // this case next background wallpaper will be shown.
  model.IncreaseBackgroundWallpaperImageIndex();

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
