// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/components/ntp_background_images/browser/view_counter_model.h"

#include "base/check_op.h"
#include "base/logging.h"

namespace ntp_background_images {

ViewCounterModel::ViewCounterModel() {
  count_to_branded_wallpaper_ = kInitialCountToBrandedWallpaper;
}

ViewCounterModel::~ViewCounterModel() = default;

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
  // NTP BI component is not ready.
  if (total_branded_image_count_ == 0)
    return;

  // In SR mode, SR image is always visible regardless of
  if (always_show_branded_wallpaper_) {
    current_branded_wallpaper_image_index_++;
    current_branded_wallpaper_image_index_ %= total_branded_image_count_;
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
    // Reset count and increse image index for next time.
    count_to_branded_wallpaper_ = kRegularCountToBrandedWallpaper;
    current_branded_wallpaper_image_index_++;
    current_branded_wallpaper_image_index_ %= total_branded_image_count_;
    return;
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

void ViewCounterModel::Reset() {
  current_wallpaper_image_index_ = 0;
  total_image_count_ = 0;
  current_branded_wallpaper_image_index_ = 0;
  total_branded_image_count_ = 0;
  always_show_branded_wallpaper_ = false;
}

}  // namespace ntp_background_images
