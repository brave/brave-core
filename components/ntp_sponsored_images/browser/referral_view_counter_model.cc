// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/components/ntp_sponsored_images/browser/referral_view_counter_model.h"

namespace ntp_sponsored_images {

ReferralViewCounterModel::ReferralViewCounterModel() = default;
ReferralViewCounterModel::~ReferralViewCounterModel() = default;

bool ReferralViewCounterModel::ShouldShowWallpaper() const {
  return true;
}

void ReferralViewCounterModel::RegisterPageView() {
  current_wallpaper_image_index_++;
  current_wallpaper_image_index_ %= total_image_count_;
}

}  // namespace ntp_sponsored_images
