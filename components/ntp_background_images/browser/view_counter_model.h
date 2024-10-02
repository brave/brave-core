// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_VIEW_COUNTER_MODEL_H_
#define BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_VIEW_COUNTER_MODEL_H_

#include <tuple>
#include <vector>

#include "base/gtest_prod_util.h"
#include "base/memory/raw_ptr.h"
#include "base/timer/timer.h"

class PrefService;

namespace ntp_background_images {

class ViewCounterModel {
 public:
  explicit ViewCounterModel(PrefService* prefs);
  ~ViewCounterModel();

  ViewCounterModel(const ViewCounterModel&) = delete;
  ViewCounterModel& operator=(const ViewCounterModel&) = delete;

  // Set each campaigns total image count.
  void SetCampaignsTotalBrandedImageCount(
      const std::vector<size_t>& campaigns_total_image_count);

  // Returns current campaign index and its bg index.
  std::tuple<size_t, size_t> GetCurrentBrandedImageIndex() const;

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
  void MaybeResetBrandedWallpaperCount();
  void NextBrandedImage();
  void Reset();
  void RotateBackgroundWallpaperImageIndex();

 private:
  friend class NTPBackgroundImagesViewCounterTest;
  FRIEND_TEST_ALL_PREFIXES(ViewCounterModelTest, NTPSponsoredImagesTest);
  FRIEND_TEST_ALL_PREFIXES(ViewCounterModelTest,
                           NTPSponsoredImagesCountResetTest);
  FRIEND_TEST_ALL_PREFIXES(ViewCounterModelTest,
                           NTPSponsoredImagesCountResetMinTest);
  FRIEND_TEST_ALL_PREFIXES(ViewCounterModelTest,
                           NTPSponsoredImagesCountResetTimerTest);
  FRIEND_TEST_ALL_PREFIXES(ViewCounterModelTest,
                           NTPSponsoredImagesCountToBrandedWallpaperTest);
  FRIEND_TEST_ALL_PREFIXES(ViewCounterModelTest, NTPBackgroundImagesTest);
  FRIEND_TEST_ALL_PREFIXES(ViewCounterModelTest,
                           NTPBackgroundImagesWithSIDisabledTest);
  FRIEND_TEST_ALL_PREFIXES(ViewCounterModelTest,
                           NTPBackgroundImagesWithEmptyCampaignTest);
  FRIEND_TEST_ALL_PREFIXES(ViewCounterModelTest,
                           NTPFailedToLoadSponsoredImagesTest);
  FRIEND_TEST_ALL_PREFIXES(NTPBackgroundImagesViewCounterTest, ModelTest);
  FRIEND_TEST_ALL_PREFIXES(NTPBackgroundImagesViewCounterTest,
                           PrefsWithModelTest);

  void RegisterPageViewForBrandedImages();

  void RegisterPageViewForBackgroundImages();

  void OnTimerCountsResetExpired();

  // For NTP SI.
  raw_ptr<PrefService> prefs_ = nullptr;
  int count_to_branded_wallpaper_ = 0;
  bool always_show_branded_wallpaper_ = false;
  bool show_branded_wallpaper_ = true;
  size_t current_campaign_index_ = 0;
  size_t total_campaign_count_ = 0;
  std::vector<size_t> campaigns_total_branded_image_count_;
  std::vector<size_t> campaigns_current_branded_image_index_;
  base::RepeatingTimer timer_counts_reset_;

  // For NTP BI.
  int current_wallpaper_image_index_ = 0;
  int total_image_count_ = 0;
  bool show_wallpaper_ = true;
};

}  // namespace ntp_background_images

#endif  // BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_VIEW_COUNTER_MODEL_H_
