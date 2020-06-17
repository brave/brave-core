/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/frequency_capping/exclusion_rules/marked_as_inappropriate_frequency_cap.h"

#include <algorithm>

#include "base/strings/stringprintf.h"
#include "bat/ads/internal/creative_ad_info.h"
#include "bat/ads/internal/ads_impl.h"
#include "bat/ads/internal/flagged_ad.h"

namespace ads {

MarkedAsInappropriateFrequencyCap::MarkedAsInappropriateFrequencyCap(
    const AdsImpl* const ads)
    : ads_(ads) {
  DCHECK(ads_);
}

MarkedAsInappropriateFrequencyCap::
~MarkedAsInappropriateFrequencyCap() = default;

bool MarkedAsInappropriateFrequencyCap::ShouldExclude(
    const CreativeAdInfo& ad) {
  if (!DoesRespectCap(ad)) {
    last_message_ = base::StringPrintf("creativeSetId %s excluded due to being "
        "marked as inappropriate", ad.creative_set_id.c_str());

    return true;
  }

  return false;
}

std::string MarkedAsInappropriateFrequencyCap::get_last_message() const {
  return last_message_;
}

bool MarkedAsInappropriateFrequencyCap::DoesRespectCap(
      const CreativeAdInfo& ad) const {
  const FlaggedAdsList flagged_ads = ads_->get_client()->get_flagged_ads();
  if (flagged_ads.empty()) {
    return true;
  }

  const auto iter = std::find_if(flagged_ads.begin(), flagged_ads.end(),
      [&ad](const FlaggedAd& flagged_ad) {
    return flagged_ad.creative_set_id == ad.creative_set_id;
  });

  if (iter == flagged_ads.end()) {
    return true;
  }

  return false;
}

}  // namespace ads
