/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/frequency_capping/exclusion_rules/dislike_frequency_cap.h"

#include "base/strings/stringprintf.h"
#include "bat/ads/internal/client/client.h"
#include "bat/ads/internal/client/preferences/filtered_ad_info.h"

namespace ads {

DislikeFrequencyCap::DislikeFrequencyCap() = default;

DislikeFrequencyCap::~DislikeFrequencyCap() = default;

bool DislikeFrequencyCap::ShouldExclude(const CreativeAdInfo& ad) {
  if (!DoesRespectCap(ad)) {
    last_message_ =
        base::StringPrintf("creativeSetId %s excluded due to being disliked",
                           ad.creative_set_id.c_str());

    return true;
  }

  return false;
}

std::string DislikeFrequencyCap::get_last_message() const {
  return last_message_;
}

bool DislikeFrequencyCap::DoesRespectCap(const CreativeAdInfo& ad) {
  const FilteredAdList filtered_ads = Client::Get()->get_filtered_ads();
  if (filtered_ads.empty()) {
    return true;
  }

  const auto iter =
      std::find_if(filtered_ads.begin(), filtered_ads.end(),
                   [&ad](const FilteredAdInfo& filtered_ad) {
                     return filtered_ad.creative_set_id == ad.creative_set_id;
                   });

  if (iter == filtered_ads.end()) {
    return true;
  }

  return false;
}

}  // namespace ads
