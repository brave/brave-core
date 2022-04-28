/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/history/filters/history_confirmation_filter.h"

#include <map>
#include <string>

#include "base/notreached.h"
#include "bat/ads/history_item_info.h"

namespace ads {

namespace {

bool ShouldFilterConfirmationType(const ConfirmationType& confirmation_type) {
  switch (confirmation_type.value()) {
    case ConfirmationType::kViewed:
    case ConfirmationType::kClicked:
    case ConfirmationType::kDismissed: {
      return false;
    }

    case ConfirmationType::kServed:
    case ConfirmationType::kTransferred:
    case ConfirmationType::kSaved:
    case ConfirmationType::kFlagged:
    case ConfirmationType::kUpvoted:
    case ConfirmationType::kDownvoted:
    case ConfirmationType::kConversion: {
      return true;
    }

    case ConfirmationType::kUndefined: {
      NOTREACHED();
      return true;
    }
  }
}

}  // namespace

HistoryConfirmationFilter::HistoryConfirmationFilter() = default;

HistoryConfirmationFilter::~HistoryConfirmationFilter() = default;

base::circular_deque<HistoryItemInfo> HistoryConfirmationFilter::Apply(
    const base::circular_deque<HistoryItemInfo>& history) const {
  std::map<std::string, HistoryItemInfo> filtered_history_map;

  for (const auto& ad : history) {
    const ConfirmationType confirmation_type = ad.ad_content.confirmation_type;
    if (ShouldFilterConfirmationType(confirmation_type)) {
      continue;
    }

    const std::string uuid = ad.ad_content.uuid;

    const auto it = filtered_history_map.find(uuid);
    if (it == filtered_history_map.end()) {
      filtered_history_map.insert({uuid, ad});
    } else {
      const HistoryItemInfo filtered_ad = it->second;
      if (filtered_ad.ad_content.confirmation_type.value() >
          confirmation_type.value()) {
        filtered_history_map[uuid] = ad;
      }
    }
  }

  base::circular_deque<HistoryItemInfo> filtered_history;
  for (const auto& filtered_ad : filtered_history_map) {
    const HistoryItemInfo ad = filtered_ad.second;
    filtered_history.push_back(ad);
  }

  return filtered_history;
}

}  // namespace ads
