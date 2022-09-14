// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/components/ntp_background_images/browser/view_counter_model.h"

#include "base/check_op.h"
#include "base/logging.h"
#include "base/rand_util.h"

namespace ntp_background_images {

ViewCounterModel::ViewCounterModel() {
  count_to_branded_wallpaper_ = kInitialCountToBrandedWallpaper;
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
  RegisterPageViewForBrandedImages();
  RegisterPageViewForBackgroundImages();
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
    count_to_branded_wallpaper_ = kRegularCountToBrandedWallpaper;

    if (always_show_branded_wallpaper_) {
      // Reset count and increse image index for next time.
      count_to_branded_wallpaper_ = kRegularCountToBrandedWallpaper;
      campaigns_current_branded_image_index_[current_campaign_index_]++;
      campaigns_current_branded_image_index_[current_campaign_index_] %=
          campaigns_total_branded_image_count_[current_campaign_index_];

      // Increse campaign index for next time.
      current_campaign_index_++;
      current_campaign_index_ %= total_campaign_count_;
    } else {
      // Randomize SI campaign branded image index for next time.
      campaigns_current_branded_image_index_[current_campaign_index_] =
          base::RandInt(
              0, campaigns_total_branded_image_count_[current_campaign_index_] -
                     1);

      // Randomize campaign index for next time.
      current_campaign_index_ = base::RandInt(0, total_campaign_count_ - 1);
    }
  }
}

void ViewCounterModel::RegisterPageViewForBackgroundImages() {
  // NTP BI component is not ready.
  if (total_image_count_ == 0)
    return;

  if (!show_wallpaper_)
    return;

  // We don't show NTP BI in SR mode.
  if (always_show_branded_wallpaper_)
    return;

  // Don't count when SI will be visible.
  if (show_branded_wallpaper_ && count_to_branded_wallpaper_ == 0)
    return;

  // Increase background image index
  current_wallpaper_image_index_++;
  current_wallpaper_image_index_ %= total_image_count_;
}

void ViewCounterModel::IncreaseBackgroundWallpaperImageIndex() {
  // NTP BI component is not ready.
  if (total_image_count_ == 0)
    return;

  if (!show_wallpaper_)
    return;

  // Increase background image index
  current_wallpaper_image_index_++;
  current_wallpaper_image_index_ %= total_image_count_;
}

void ViewCounterModel::Reset() {
  current_wallpaper_image_index_ = 0;
  total_image_count_ = 0;
  always_show_branded_wallpaper_ = false;
  current_campaign_index_ = 0;
  campaigns_total_branded_image_count_.clear();
  campaigns_current_branded_image_index_.clear();
}

}  // namespace ntp_background_images
