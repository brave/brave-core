/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/history/filters/confirmation_history_filter.h"

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

std::map<std::string, HistoryItemInfo> BuildBuckets(
    const base::circular_deque<HistoryItemInfo>& history) {
  std::map<std::string, HistoryItemInfo> buckets;

  for (const auto& item : history) {
    const ConfirmationType confirmation_type =
        item.ad_content.confirmation_type;
    if (ShouldFilterConfirmationType(confirmation_type)) {
      continue;
    }

    const std::string placement_id = item.ad_content.placement_id;
    const auto iter = buckets.find(placement_id);
    if (iter == buckets.end()) {
      buckets.insert({placement_id, item});
    } else {
      const HistoryItemInfo& current_history_item = iter->second;
      if (current_history_item.ad_content.confirmation_type.value() >
          confirmation_type.value()) {
        buckets[placement_id] = item;
      }
    }
  }

  return buckets;
}

}  // namespace

ConfirmationHistoryFilter::ConfirmationHistoryFilter() = default;

ConfirmationHistoryFilter::~ConfirmationHistoryFilter() = default;

base::circular_deque<HistoryItemInfo> ConfirmationHistoryFilter::Apply(
    const base::circular_deque<HistoryItemInfo>& history) const {
  const std::map<std::string, HistoryItemInfo> buckets = BuildBuckets(history);

  base::circular_deque<HistoryItemInfo> filtered_history;
  for (const auto& bucket : buckets) {
    const HistoryItemInfo& history_item = bucket.second;
    filtered_history.push_back(history_item);
  }

  return filtered_history;
}

}  // namespace ads
