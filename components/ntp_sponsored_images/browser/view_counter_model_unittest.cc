// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/components/ntp_sponsored_images/browser/view_counter_model.h"
#include "testing/gtest/include/gtest/gtest.h"

using ntp_sponsored_images::ViewCounterModel;

TEST(ViewCounterModelTest, BasicTest) {
  ViewCounterModel model;
  model.set_total_image_count(3);

  // First loading.
  EXPECT_FALSE(model.ShouldShowBrandedWallpaper());
  model.RegisterPageView();

  // Second loading.
  // Image at index 0 should be displayed now.
  EXPECT_TRUE(model.ShouldShowBrandedWallpaper());
  EXPECT_EQ(0, model.current_wallpaper_image_index());
  model.RegisterPageView();

  // Loading 3 times.
  for (int i = 0; i < 3; ++i) {
    EXPECT_FALSE(model.ShouldShowBrandedWallpaper());
    EXPECT_EQ(1, model.current_wallpaper_image_index());
    model.RegisterPageView();
  }

  // Image at index 1 should be displayed now.
  EXPECT_TRUE(model.ShouldShowBrandedWallpaper());
  EXPECT_EQ(1, model.current_wallpaper_image_index());
  model.RegisterPageView();

  // Loading 3 times again.
  for (int i = 0; i < 3; ++i) {
    EXPECT_FALSE(model.ShouldShowBrandedWallpaper());
    EXPECT_EQ(2, model.current_wallpaper_image_index());
    model.RegisterPageView();
  }

  // Image at index 2 should be displayed now.
  EXPECT_TRUE(model.ShouldShowBrandedWallpaper());
  EXPECT_EQ(2, model.current_wallpaper_image_index());
  model.RegisterPageView();

  // Loading 3 times again.
  for (int i = 0; i < 3; ++i) {
    EXPECT_FALSE(model.ShouldShowBrandedWallpaper());
    EXPECT_EQ(0, model.current_wallpaper_image_index());
    model.RegisterPageView();
  }

  // Image at index 0 should be displayed now.
  EXPECT_TRUE(model.ShouldShowBrandedWallpaper());
  EXPECT_EQ(0, model.current_wallpaper_image_index());
  model.RegisterPageView();
}
