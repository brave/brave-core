/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/filters/ads_history_confirmation_filter.h"

#include <map>
#include <string>

#include "bat/ads/ads_history.h"

namespace ads {

AdsHistoryConfirmationFilter::AdsHistoryConfirmationFilter() = default;

AdsHistoryConfirmationFilter::~AdsHistoryConfirmationFilter() = default;

std::deque<AdHistory> AdsHistoryConfirmationFilter::Apply(
    const std::deque<AdHistory>& history) const {
  std::map<std::string, AdHistory> filtered_ads_history_map;

  for (const auto& ad : history) {
    const ConfirmationType ad_action = ad.ad_content.ad_action;
    if (ShouldFilterAction(ad_action)) {
      continue;
    }

    std::string uuid = ad.parent_uuid;

    const auto it = filtered_ads_history_map.find(uuid);
    if (it == filtered_ads_history_map.end()) {
      filtered_ads_history_map.insert({uuid, ad});
    } else {
      AdHistory filtered_ad = it->second;
      if (filtered_ad.ad_content.ad_action.value() > ad_action.value()) {
        filtered_ads_history_map[uuid] = ad;
      }
    }
  }

  std::deque<AdHistory> filtered_ads_history;
  for (const auto& filtered_ad : filtered_ads_history_map) {
    const AdHistory ad = filtered_ad.second;
    filtered_ads_history.push_back(ad);
  }

  return filtered_ads_history;
}

bool AdsHistoryConfirmationFilter::ShouldFilterAction(
    const ConfirmationType& confirmation_type) const {
  bool should_filter;

  switch (confirmation_type.value()) {
    case ConfirmationType::kClicked:
    case ConfirmationType::kViewed:
    case ConfirmationType::kDismissed: {
      should_filter = false;
      break;
    }
    case ConfirmationType::kNone:
    case ConfirmationType::kLanded:
    case ConfirmationType::kFlagged:
    case ConfirmationType::kUpvoted:
    case ConfirmationType::kDownvoted:
    case ConfirmationType::kConversion: {
      should_filter = true;
      break;
    }
  }

  return should_filter;
}

}  // namespace ads
