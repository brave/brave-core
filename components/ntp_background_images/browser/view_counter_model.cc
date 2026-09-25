// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/components/ntp_background_images/browser/view_counter_model.h"

#include <algorithm>

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/rand_util.h"
#include "brave/components/ntp_background_images/browser/features.h"
#include "components/prefs/pref_service.h"

namespace ntp_background_images {

ViewCounterModel::ViewCounterModel(PrefService* prefs)
    : prefs_(prefs),
      rand_int_inclusive_callback_(
          base::BindRepeating(&base::RandIntInclusive)) {
  CHECK(prefs);

  // When browser is restarted we reset to "initial" count. This will also get
  // set again in the Reset() function, called e.g. when component is updated.
  count_to_new_tab_takeover_wallpaper_ =
      features::kInitialCountToBrandedWallpaper.Get() - 1;

  // We also reset when a specific amount of time is elapsed while sponsored
  // content is enabled.
  ScheduleNextNewTabTakeoverCountReset();
}

ViewCounterModel::~ViewCounterModel() = default;

void ViewCounterModel::SetCampaignsTotalNewTabTakeoverCreativeCount(
    const std::vector<size_t>& campaigns_total_creative_count) {
  total_campaign_count_ = campaigns_total_creative_count.size();
}

bool ViewCounterModel::ShouldShowNewTabTakeover() const {
  if (!show_new_tab_takeover_wallpaper_) {
    return false;
  }

  return count_to_new_tab_takeover_wallpaper_ == 0;
}

void ViewCounterModel::RegisterPageView() {
  // Call BG images first to know this calling is after showing
  // a New Tab Takeover creative or not. If this calling is from a New Tab
  // Takeover creative showing, background image index should not be changed.
  RegisterPageViewForBackgroundImages();
  RegisterPageViewForNewTabTakeoverCreatives();
}

void ViewCounterModel::RegisterPageViewForNewTabTakeoverCreatives() {
  // The sponsored content component is not ready.
  if (total_campaign_count_ == 0) {
    return;
  }

  // User turned off "Show Sponsored Images" option.
  if (!show_new_tab_takeover_wallpaper_) {
    return;
  }

  // When count is `0` then UI is free to show
  // the New Tab Takeover wallpaper, until the next time `RegisterPageView`
  // is called.
  count_to_new_tab_takeover_wallpaper_--;
  if (count_to_new_tab_takeover_wallpaper_ < 0) {
    // Reset count for next time.
    count_to_new_tab_takeover_wallpaper_ =
        features::kCountToBrandedWallpaper.Get() - 1;
  }
}

void ViewCounterModel::RegisterPageViewForBackgroundImages() {
  // Don't count when sponsored content will be visible.
  if (show_new_tab_takeover_wallpaper_ && total_campaign_count_ != 0 &&
      count_to_new_tab_takeover_wallpaper_ == 0) {
    return;
  }

  RotateBackgroundWallpaperImageIndex();
}

void ViewCounterModel::RotateBackgroundWallpaperImageIndex() {
  // The sponsored backgrounds component is not ready.
  if (total_image_count_ == 0) {
    return;
  }

  if (!show_wallpaper_) {
    return;
  }

  // Select a background image at random rather than rotating sequentially.
  current_wallpaper_image_index_ =
      rand_int_inclusive_callback_.Run(0, total_image_count_ - 1);
}

void ViewCounterModel::MaybeResetNewTabTakeoverCount() {
  // Set count so that user is more likely to see new New Tab Takeover data at
  // least once. Only reset count for sponsored content.
  if (show_new_tab_takeover_wallpaper_) {
    count_to_new_tab_takeover_wallpaper_ =
        std::min(count_to_new_tab_takeover_wallpaper_,
                 features::kInitialCountToBrandedWallpaper.Get() - 1);
  }
}

void ViewCounterModel::Reset() {
  current_wallpaper_image_index_ = 0;
  total_image_count_ = 0;
  total_campaign_count_ = 0;
  MaybeResetNewTabTakeoverCount();
  ScheduleNextNewTabTakeoverCountReset();
}

void ViewCounterModel::ScheduleNextNewTabTakeoverCountReset() {
  const base::Time next_counts_reset_time =
      base::Time::Now() + features::kResetCounterAfter.Get();
  counts_reset_timer_.Start(
      FROM_HERE, next_counts_reset_time,
      base::BindOnce(&ViewCounterModel::
                         ResetNewTabTakeoverCountAndScheduleNextCountReset,
                     base::Unretained(this)));
}

void ViewCounterModel::ResetNewTabTakeoverCountAndScheduleNextCountReset() {
  MaybeResetNewTabTakeoverCount();
  ScheduleNextNewTabTakeoverCountReset();
}

}  // namespace ntp_background_images
