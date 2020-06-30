/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/frequency_capping/exclusion_rules/total_max_frequency_cap.h"

#include "bat/ads/internal/ads_impl.h"
#include "bat/ads/internal/creative_ad_info.h"

#include "base/logging.h"
#include "base/strings/stringprintf.h"

namespace ads {

TotalMaxFrequencyCap::TotalMaxFrequencyCap(
    const AdsImpl* const ads)
    : ads_(ads) {
  DCHECK(ads_);
}

TotalMaxFrequencyCap::~TotalMaxFrequencyCap() = default;

bool TotalMaxFrequencyCap::ShouldExclude(
    const CreativeAdInfo& ad) {
  const std::map<std::string, std::deque<uint64_t>> history =
      ads_->get_client()->GetCreativeSetHistory();

  const std::deque<uint64_t> filtered_history =
      FilterHistory(history, ad.creative_set_id);

  if (!DoesRespectCap(filtered_history, ad)) {
    last_message_ = base::StringPrintf("creativeSetId %s has exceeded the "
        "frequency capping for totalMax", ad.creative_set_id.c_str());

    return true;
  }

  return false;
}

std::string TotalMaxFrequencyCap::get_last_message() const {
  return last_message_;
}

bool TotalMaxFrequencyCap::DoesRespectCap(
    const std::deque<uint64_t>& history,
    const CreativeAdInfo& ad) const {
  if (history.size() >= ad.total_max) {
    return false;
  }

  return true;
}

std::deque<uint64_t> TotalMaxFrequencyCap::FilterHistory(
    const std::map<std::string, std::deque<uint64_t>>& history,
    const std::string& creative_set_id) {
  std::deque<uint64_t> filtered_history;

  if (history.find(creative_set_id) != history.end()) {
    filtered_history = history.at(creative_set_id);
  }

  return filtered_history;
}

}  // namespace ads
