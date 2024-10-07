// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/components/ntp_background_images/browser/view_counter_model.h"

#include <algorithm>

#include "base/check.h"
#include "base/rand_util.h"
#include "brave/components/ntp_background_images/browser/features.h"
#include "components/prefs/pref_service.h"

namespace ntp_background_images {

ViewCounterModel::ViewCounterModel(PrefService* prefs) : prefs_(prefs) {
  CHECK(prefs);

  // When browser is restarted we reset to "initial" count. This will also get
  // set again in the Reset() function, called e.g. when component is updated.
  count_to_branded_wallpaper_ =
      features::kInitialCountToBrandedWallpaper.Get() - 1;

  // We also reset when a specific amount of time is elapsed when in SI mode
  timer_counts_reset_.Start(FROM_HERE, features::kResetCounterAfter.Get(), this,
                            &ViewCounterModel::OnTimerCountsResetExpired);
}

ViewCounterModel::~ViewCounterModel() = default;

void ViewCounterModel::SetCampaignsTotalBrandedImageCount(
    const std::vector<size_t>& campaigns_total_image_count) {
  campaigns_total_branded_image_count_ = campaigns_total_image_count;
  total_campaign_count_ = campaigns_total_branded_image_count_.size();

  // Pick the first image index for each campaign randomly for SI
  for (size_t i = 0; i < total_campaign_count_; ++i) {
    const int index =
        always_show_branded_wallpaper_
            ? 0
            : base::RandInt(0, campaigns_total_branded_image_count_[i] - 1);
    campaigns_current_branded_image_index_.push_back(index);
  }

  // Pick the first campaign index randomly.
  current_campaign_index_ = base::RandInt(0, total_campaign_count_ - 1);
}

std::tuple<size_t, size_t> ViewCounterModel::GetCurrentBrandedImageIndex()
    const {
  return {current_campaign_index_,
          campaigns_current_branded_image_index_[current_campaign_index_]};
}

bool ViewCounterModel::ShouldShowBrandedWallpaper() const {
  if (always_show_branded_wallpaper_)
    return true;

  if (!show_branded_wallpaper_)
    return false;

  return count_to_branded_wallpaper_ == 0;
}

void ViewCounterModel::RegisterPageView() {
  // Call BG images first to know this calling is after showing
  // branded image or not. If this calling is from branded image
  // showing, background image index should not be changed.
  RegisterPageViewForBackgroundImages();
  RegisterPageViewForBrandedImages();
}

void ViewCounterModel::RegisterPageViewForBrandedImages() {
  // NTP SI/SR component is not ready.
  if (total_campaign_count_ == 0)
    return;

  // In SR mode, SR image is always visible regardless of
  if (always_show_branded_wallpaper_) {
    // SR uses only one campaign.
    DCHECK_EQ(1UL, total_campaign_count_);
    campaigns_current_branded_image_index_[0]++;
    campaigns_current_branded_image_index_[0] %=
        campaigns_total_branded_image_count_[0];
    return;
  }

  // User turned off "Show Sponsored Images" option.
  if (!show_branded_wallpaper_)
    return;

  // When count is `0` then UI is free to show
  // the branded wallpaper, until the next time `RegisterPageView`
  // is called.
  // We select the appropriate image index for the scheduled
  // view of the branded wallpaper.
  count_to_branded_wallpaper_--;
  if (count_to_branded_wallpaper_ < 0) {
    // Reset count and randomize image index for next time.
    count_to_branded_wallpaper_ = features::kCountToBrandedWallpaper.Get() - 1;

    // Randomize SI campaign branded image index for next time.
    campaigns_current_branded_image_index_[current_campaign_index_] =
        base::RandInt(
            0,
            campaigns_total_branded_image_count_[current_campaign_index_] - 1);

    // Randomize campaign index for next time.
    current_campaign_index_ = base::RandInt(0, total_campaign_count_ - 1);
  }
}

void ViewCounterModel::RegisterPageViewForBackgroundImages() {
  // We don't show NTP BI in SR mode.
  if (always_show_branded_wallpaper_)
    return;

  // Don't count when SI will be visible.
  if (show_branded_wallpaper_ && total_campaign_count_ != 0 &&
      count_to_branded_wallpaper_ == 0) {
    return;
  }

  RotateBackgroundWallpaperImageIndex();
}

void ViewCounterModel::RotateBackgroundWallpaperImageIndex() {
  // NTP BI component is not ready.
  if (total_image_count_ == 0)
    return;

  if (!show_wallpaper_)
    return;

  current_wallpaper_image_index_++;
  current_wallpaper_image_index_ %= total_image_count_;
}

void ViewCounterModel::NextBrandedImage() {
  campaigns_current_branded_image_index_[current_campaign_index_]++;
  if (campaigns_current_branded_image_index_[current_campaign_index_] >=
      campaigns_total_branded_image_count_[current_campaign_index_]) {
    campaigns_current_branded_image_index_[current_campaign_index_] = 0;

    current_campaign_index_++;
    if (current_campaign_index_ >= total_campaign_count_) {
      current_campaign_index_ = 0;
      campaigns_current_branded_image_index_[current_campaign_index_] = 0;
    }
  }
}

void ViewCounterModel::MaybeResetBrandedWallpaperCount() {
  // Set count so that user is more likely to see new branded data at least once
  // Only reset count for SI images
  if (!always_show_branded_wallpaper_ && show_branded_wallpaper_) {
    count_to_branded_wallpaper_ =
        std::min(count_to_branded_wallpaper_,
                 features::kInitialCountToBrandedWallpaper.Get() - 1);
  }
}

void ViewCounterModel::Reset() {
  current_wallpaper_image_index_ = 0;
  total_image_count_ = 0;
  always_show_branded_wallpaper_ = false;
  current_campaign_index_ = 0;
  total_campaign_count_ = 0;
  campaigns_total_branded_image_count_.clear();
  campaigns_current_branded_image_index_.clear();
  MaybeResetBrandedWallpaperCount();
  // Restart timer with same parameters as set during this class' constructor
  timer_counts_reset_.Reset();
}

void ViewCounterModel::OnTimerCountsResetExpired() {
  MaybeResetBrandedWallpaperCount();
}

}  // namespace ntp_background_images
