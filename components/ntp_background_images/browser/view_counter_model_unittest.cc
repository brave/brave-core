// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/components/ntp_background_images/browser/view_counter_model.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {
const int kTestImageCount = 3;
}  // namespace

namespace ntp_background_images {

TEST(ViewCounterModelTest, NTPSponsoredImagesTest) {
  ViewCounterModel model;
  model.set_total_image_count(kTestImageCount);

  EXPECT_FALSE(model.ignore_count_to_branded_wallpaper_);

  // Loading initial count times.
  for (int i = 0; i < ViewCounterModel::kInitialCountToBrandedWallpaper; ++i) {
    EXPECT_FALSE(model.ShouldShowBrandedWallpaper());
    model.RegisterPageView();
  }

  // Image at index 0 should be displayed now after loading initial count.
  EXPECT_TRUE(model.ShouldShowBrandedWallpaper());
  EXPECT_EQ(0, model.current_wallpaper_image_index());
  model.RegisterPageView();

  // Loading regular-count times.
  for (int i = 0; i < ViewCounterModel::kRegularCountToBrandedWallpaper; ++i) {
    EXPECT_FALSE(model.ShouldShowBrandedWallpaper());
    EXPECT_EQ(1, model.current_wallpaper_image_index());
    model.RegisterPageView();
  }

  // Image at index 1 should be displayed now because
  EXPECT_TRUE(model.ShouldShowBrandedWallpaper());
  EXPECT_EQ(1, model.current_wallpaper_image_index());
  model.RegisterPageView();

  // Loading regular-count times again.
  for (int i = 0; i < ViewCounterModel::kRegularCountToBrandedWallpaper; ++i) {
    EXPECT_FALSE(model.ShouldShowBrandedWallpaper());
    EXPECT_EQ(2, model.current_wallpaper_image_index());
    model.RegisterPageView();
  }

  // Image at index 2 should be displayed now.
  EXPECT_TRUE(model.ShouldShowBrandedWallpaper());
  EXPECT_EQ(2, model.current_wallpaper_image_index());
  model.RegisterPageView();

  // Loading regular-count times again.
  for (int i = 0; i < ViewCounterModel::kRegularCountToBrandedWallpaper; ++i) {
    EXPECT_FALSE(model.ShouldShowBrandedWallpaper());
    EXPECT_EQ(0, model.current_wallpaper_image_index());
    model.RegisterPageView();
  }

  // Image at index 0 should be displayed now.
  EXPECT_TRUE(model.ShouldShowBrandedWallpaper());
  EXPECT_EQ(0, model.current_wallpaper_image_index());
  model.RegisterPageView();
}

TEST(ViewCounterModelTest, NTPSuperReferrerTest) {
  ViewCounterModel model;
  model.set_ignore_count_to_branded_wallpaper(true);
  model.set_total_image_count(kTestImageCount);

  // Loading any number of times and check branded wallpaper is visible always
  // with proper index from the start.
  for (int i = 0; i < 10; ++i) {
    EXPECT_TRUE(model.ShouldShowBrandedWallpaper());
    EXPECT_EQ(i % kTestImageCount, model.current_wallpaper_image_index());
    model.RegisterPageView();
  }
}

}  // namespace ntp_background_images
