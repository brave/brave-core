// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/components/ntp_sponsored_images/browser/referral_view_counter_model.h"
#include "brave/components/ntp_sponsored_images/browser/sponsored_view_counter_model.h"
#include "testing/gtest/include/gtest/gtest.h"

using ntp_sponsored_images::SponsoredViewCounterModel;
using ntp_sponsored_images::ReferralViewCounterModel;

TEST(ViewCounterModelTest, SponsoredModelTest) {
  SponsoredViewCounterModel model;
  model.set_total_image_count(3);

  // First loading.
  EXPECT_FALSE(model.ShouldShowWallpaper());
  model.RegisterPageView();

  // Second loading.
  // Image at index 0 should be displayed now.
  EXPECT_TRUE(model.ShouldShowWallpaper());
  EXPECT_EQ(0, model.current_wallpaper_image_index());
  model.RegisterPageView();

  // Loading 3 times.
  for (int i = 0; i < 3; ++i) {
    EXPECT_FALSE(model.ShouldShowWallpaper());
    EXPECT_EQ(1, model.current_wallpaper_image_index());
    model.RegisterPageView();
  }

  // Image at index 1 should be displayed now.
  EXPECT_TRUE(model.ShouldShowWallpaper());
  EXPECT_EQ(1, model.current_wallpaper_image_index());
  model.RegisterPageView();

  // Loading 3 times again.
  for (int i = 0; i < 3; ++i) {
    EXPECT_FALSE(model.ShouldShowWallpaper());
    EXPECT_EQ(2, model.current_wallpaper_image_index());
    model.RegisterPageView();
  }

  // Image at index 2 should be displayed now.
  EXPECT_TRUE(model.ShouldShowWallpaper());
  EXPECT_EQ(2, model.current_wallpaper_image_index());
  model.RegisterPageView();

  // Loading 3 times again.
  for (int i = 0; i < 3; ++i) {
    EXPECT_FALSE(model.ShouldShowWallpaper());
    EXPECT_EQ(0, model.current_wallpaper_image_index());
    model.RegisterPageView();
  }

  // Image at index 0 should be displayed now.
  EXPECT_TRUE(model.ShouldShowWallpaper());
  EXPECT_EQ(0, model.current_wallpaper_image_index());
  model.RegisterPageView();
}

TEST(ViewCounterModelTest, ReferralModelTest) {
  // Start with three background images.
  const int total_image_count = 3;
  ReferralViewCounterModel model;
  model.set_total_image_count(total_image_count);

  // Load tab 8 times and check wallpaper is shown always.
  for (int i = 0; i < 8; ++i) {
    EXPECT_TRUE(model.ShouldShowWallpaper());
    EXPECT_EQ(i % total_image_count, model.current_wallpaper_image_index());
    model.RegisterPageView();
  }
}
