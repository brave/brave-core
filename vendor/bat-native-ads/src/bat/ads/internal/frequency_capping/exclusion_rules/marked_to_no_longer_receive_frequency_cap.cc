/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/frequency_capping/exclusion_rules/marked_to_no_longer_receive_frequency_cap.h"

#include <algorithm>

#include "base/strings/stringprintf.h"
#include "bat/ads/internal/creative_ad_info.h"
#include "bat/ads/internal/ads_impl.h"
#include "bat/ads/internal/filtered_ad.h"

namespace ads {

MarkedToNoLongerReceiveFrequencyCap::MarkedToNoLongerReceiveFrequencyCap(
    const AdsImpl* const ads)
    : ads_(ads) {
  DCHECK(ads_);
}

MarkedToNoLongerReceiveFrequencyCap::
~MarkedToNoLongerReceiveFrequencyCap() = default;

bool MarkedToNoLongerReceiveFrequencyCap::ShouldExclude(
    const CreativeAdInfo& ad) {
  if (!DoesRespectCap(ad)) {
    last_message_ = base::StringPrintf("creativeSetId %s excluded due to being "
        "marked to no longer receive ads", ad.creative_set_id.c_str());

    return true;
  }

  return false;
}

std::string MarkedToNoLongerReceiveFrequencyCap::get_last_message() const {
  return last_message_;
}

bool MarkedToNoLongerReceiveFrequencyCap::DoesRespectCap(
      const CreativeAdInfo& ad) const {
  const FilteredAdsList filtered_ads = ads_->get_client()->get_filtered_ads();
  if (filtered_ads.empty()) {
    return true;
  }

  const auto iter = std::find_if(filtered_ads.begin(), filtered_ads.end(),
      [&ad](const FilteredAd& filtered_ad) {
    return filtered_ad.creative_set_id == ad.creative_set_id;
  });

  if (iter == filtered_ads.end()) {
    return true;
  }

  return false;
}

}  // namespace ads
