// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_VIEW_COUNTER_MODEL_H_
#define BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_VIEW_COUNTER_MODEL_H_

#include "base/gtest_prod_util.h"

namespace ntp_background_images {

class ViewCounterModel {
 public:
  ViewCounterModel();
  ~ViewCounterModel();

  ViewCounterModel(const ViewCounterModel&) = delete;
  ViewCounterModel& operator=(const ViewCounterModel&) = delete;

  int current_branded_wallpaper_image_index() const {
    return current_branded_wallpaper_image_index_;
  }

  void set_total_branded_image_count(int count) {
    total_branded_image_count_ = count;
  }

  int current_wallpaper_image_index() const {
    return current_wallpaper_image_index_;
  }

  void set_total_image_count(int count) { total_image_count_ = count; }

  void set_always_show_branded_wallpaper(bool show) {
    always_show_branded_wallpaper_ = show;
  }

  void set_show_branded_wallpaper(bool show) { show_branded_wallpaper_ = show; }
  void set_show_wallpaper(bool show) { show_wallpaper_ = show; }

  bool ShouldShowBrandedWallpaper() const;
  void RegisterPageView();
  void Reset();

 private:
  static const int kInitialCountToBrandedWallpaper = 1;
  static const int kRegularCountToBrandedWallpaper = 3;

  FRIEND_TEST_ALL_PREFIXES(ViewCounterModelTest, NTPSponsoredImagesTest);
  FRIEND_TEST_ALL_PREFIXES(ViewCounterModelTest, NTPBackgroundImagesTest);
  FRIEND_TEST_ALL_PREFIXES(ViewCounterModelTest, NTPBackgroundImagesOnlyTest);
  FRIEND_TEST_ALL_PREFIXES(NTPBackgroundImagesViewCounterTest, ModelTest);
  FRIEND_TEST_ALL_PREFIXES(NTPBackgroundImagesViewCounterTest,
                           PrefsWithModelTest);

  void RegisterPageViewForBrandedImages();

  void RegisterPageViewForBackgroundImages();

  int current_wallpaper_image_index_ = 0;
  int total_image_count_ = 0;
  int current_branded_wallpaper_image_index_ = 0;
  int count_to_branded_wallpaper_ = 0;
  int total_branded_image_count_ = 0;
  bool always_show_branded_wallpaper_ = false;
  bool show_branded_wallpaper_ = true;
  // For NTP BI.
  bool show_wallpaper_ = true;
};

}  // namespace ntp_background_images

#endif  // BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_VIEW_COUNTER_MODEL_H_
