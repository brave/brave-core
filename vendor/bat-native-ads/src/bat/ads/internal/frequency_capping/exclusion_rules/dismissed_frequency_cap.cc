/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/frequency_capping/exclusion_rules/dismissed_frequency_cap.h"

#include <stdint.h>

#include "base/strings/stringprintf.h"
#include "bat/ads/internal/ads_impl.h"
#include "bat/ads/internal/sorts/ads_history/ads_history_sort_factory.h"
#include "bat/ads/internal/time_util.h"

namespace ads {

DismissedFrequencyCap::DismissedFrequencyCap(
    const AdsImpl* const ads)
    : ads_(ads) {
  DCHECK(ads_);
}

DismissedFrequencyCap::~DismissedFrequencyCap() = default;

bool DismissedFrequencyCap::ShouldExclude(
    const CreativeAdInfo& ad) {
  const std::deque<AdHistory> history = ads_->get_client()->GetAdsHistory();

  const std::deque<AdHistory> filtered_history =
      FilterHistory(history, ad.campaign_id);

  if (!DoesRespectCap(filtered_history, ad)) {
    last_message_ = base::StringPrintf("campaignId %s has exceeded the "
        "frequency capping for dismissed", ad.campaign_id.c_str());
    return true;
  }

  return false;
}

std::string DismissedFrequencyCap::get_last_message() const {
  return last_message_;
}

bool DismissedFrequencyCap::DoesRespectCap(
    const std::deque<AdHistory>& history,
    const CreativeAdInfo& ad) const {
  int count = 0;
  for (const auto& ad : history) {
    if (ad.ad_content.ad_action == ConfirmationType::kClicked) {
      count = 0;
    } else if (ad.ad_content.ad_action == ConfirmationType::kDismissed) {
      count++;
    }
  }

  if (count >= 2) {
    // An ad was dismissed two or more times in a row without being clicked, so
    // do not show another ad from the same campaign for 48 hours
    return false;
  }

  return true;
}

std::deque<AdHistory> DismissedFrequencyCap::FilterHistory(
    const std::deque<AdHistory>& history,
    const std::string& campaign_id) {
  std::deque<AdHistory> filtered_history;

  const uint64_t time_constraint =
      2 * base::Time::kSecondsPerHour * base::Time::kHoursPerDay;

  const uint64_t now_in_seconds = base::Time::Now().ToDoubleT();

  for (const auto& ad : history) {
    if (ad.ad_content.campaign_id != campaign_id ||
        now_in_seconds - ad.timestamp_in_seconds >= time_constraint) {
      continue;
    }

    filtered_history.push_back(ad);
  }

  const auto sort = AdsHistorySortFactory::Build(
      AdsHistory::SortType::kAscendingOrder);
  DCHECK(sort);

  filtered_history = sort->Apply(filtered_history);

  return filtered_history;
}

}  // namespace ads
