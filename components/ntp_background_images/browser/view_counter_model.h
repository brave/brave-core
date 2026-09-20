// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_VIEW_COUNTER_MODEL_H_
#define BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_VIEW_COUNTER_MODEL_H_

#include <vector>

#include "base/functional/callback.h"
#include "base/gtest_prod_util.h"
#include "base/memory/raw_ptr.h"
#include "base/timer/wall_clock_timer.h"

class PrefService;

namespace ntp_background_images {

class ViewCounterModel {
 public:
  explicit ViewCounterModel(PrefService* prefs);
  ~ViewCounterModel();

  ViewCounterModel(const ViewCounterModel&) = delete;
  ViewCounterModel& operator=(const ViewCounterModel&) = delete;

  void SetCampaignsTotalNewTabTakeoverCreativeCount(
      const std::vector<size_t>& campaigns_total_creative_count);

  int current_wallpaper_image_index() const {
    return current_wallpaper_image_index_;
  }

  void set_total_image_count(int count) { total_image_count_ = count; }

  void set_show_new_tab_takeover_wallpaper(bool show) {
    show_new_tab_takeover_wallpaper_ = show;
  }
  void set_show_wallpaper(bool show) { show_wallpaper_ = show; }

  bool ShouldShowSponsoredImages() const;
  void RegisterPageView();
  void MaybeResetNewTabTakeoverCount();
  void Reset();
  void RotateBackgroundWallpaperImageIndex();

  using RandIntInclusiveCallback =
      base::RepeatingCallback<int(int min, int max)>;
  void set_rand_int_inclusive_callback_for_testing(
      RandIntInclusiveCallback callback) {
    rand_int_inclusive_callback_ = std::move(callback);
  }

 private:
  friend class ViewCounterServiceTest;
  FRIEND_TEST_ALL_PREFIXES(ViewCounterModelTest, NTPSponsoredImagesTest);
  FRIEND_TEST_ALL_PREFIXES(ViewCounterModelTest,
                           NTPSponsoredImagesCountResetTest);
  FRIEND_TEST_ALL_PREFIXES(ViewCounterModelTest,
                           NTPSponsoredImagesCountResetMinTest);
  FRIEND_TEST_ALL_PREFIXES(ViewCounterModelTest,
                           NTPSponsoredImagesCountResetTimerTest);
  FRIEND_TEST_ALL_PREFIXES(ViewCounterModelTest,
                           NTPSponsoredImagesCountToNewTabTakeoverTest);
  FRIEND_TEST_ALL_PREFIXES(ViewCounterModelTest, NTPBackgroundImagesTest);
  FRIEND_TEST_ALL_PREFIXES(ViewCounterModelTest,
                           NTPBackgroundImagesWithSIDisabledTest);
  FRIEND_TEST_ALL_PREFIXES(ViewCounterModelTest,
                           NTPBackgroundImagesWithEmptyCampaignTest);
  FRIEND_TEST_ALL_PREFIXES(ViewCounterModelTest,
                           NTPFailedToLoadSponsoredImagesTest);
  FRIEND_TEST_ALL_PREFIXES(ViewCounterServiceTest, ModelTest);
  FRIEND_TEST_ALL_PREFIXES(ViewCounterServiceTest, PrefsWithModelTest);

  void RegisterPageViewForNewTabTakeoverCreatives();

  void RegisterPageViewForBackgroundImages();

  void ScheduleNextNewTabTakeoverCountReset();
  void ResetNewTabTakeoverCountAndScheduleNextCountReset();

  // For sponsored content.
  raw_ptr<PrefService> prefs_ = nullptr;
  int count_to_new_tab_takeover_wallpaper_ = 0;
  bool show_new_tab_takeover_wallpaper_ = true;
  size_t total_campaign_count_ = 0;
  base::WallClockTimer counts_reset_timer_;

  // For sponsored backgrounds.
  int current_wallpaper_image_index_ = 0;
  int total_image_count_ = 0;
  bool show_wallpaper_ = true;

  // Random number generator used to pick a background image index. Indirected
  // through a callback so tests can make the selection deterministic.
  RandIntInclusiveCallback rand_int_inclusive_callback_;
};

}  // namespace ntp_background_images

#endif  // BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_VIEW_COUNTER_MODEL_H_
