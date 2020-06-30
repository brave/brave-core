/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/frequency_capping/exclusion_rules/conversion_frequency_cap.h"

#include "bat/ads/internal/ads_impl.h"
#include "bat/ads/internal/creative_ad_info.h"

#include "base/strings/stringprintf.h"

namespace ads {

ConversionFrequencyCap::ConversionFrequencyCap(
    const AdsImpl* const ads)
    : ads_(ads) {
  DCHECK(ads_);
}

ConversionFrequencyCap::~ConversionFrequencyCap() = default;

bool ConversionFrequencyCap::ShouldExclude(
    const CreativeAdInfo& ad) {
  if (!ShouldAllow(ad)) {
    last_message_ = base::StringPrintf("creativeSetId %s excluded due to ad "
        " conversion tracking being disabled", ad.creative_set_id.c_str());

    return true;
  }

  const std::map<std::string, std::deque<uint64_t>> history =
      ads_->get_client()->GetAdConversionHistory();

  const std::deque<uint64_t> filtered_history =
      FilterHistory(history, ad.creative_set_id);

  if (!DoesRespectCap(filtered_history, ad)) {
    last_message_ = base::StringPrintf("creativeSetId %s has exceeded the "
        "frequency capping for conversions", ad.creative_set_id.c_str());

    return true;
  }

  return false;
}

std::string ConversionFrequencyCap::get_last_message() const {
  return last_message_;
}

bool ConversionFrequencyCap::ShouldAllow(
    const CreativeAdInfo& ad) {
  if (!ads_->get_ads_client()->ShouldAllowAdConversionTracking() &&
      ad.conversion) {
    return false;
  }

  return true;
}

bool ConversionFrequencyCap::DoesRespectCap(
      const std::deque<uint64_t>& history,
      const CreativeAdInfo& ad) const {
  if (history.size() >= 1) {
    return false;
  }

  return true;
}

std::deque<uint64_t> ConversionFrequencyCap::FilterHistory(
    const std::map<std::string, std::deque<uint64_t>>& history,
    const std::string& creative_set_id) {
  std::deque<uint64_t> filtered_history;

  if (history.find(creative_set_id) != history.end()) {
    filtered_history = history.at(creative_set_id);
  }

  return filtered_history;
}

}  // namespace ads
