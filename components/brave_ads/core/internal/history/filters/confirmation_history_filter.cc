/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/history/filters/confirmation_history_filter.h"

#include <map>
#include <string>

#include "base/notreached.h"
#include "base/types/cxx23_to_underlying.h"

namespace brave_ads {

namespace {

bool ShouldFilterConfirmationType(ConfirmationType confirmation_type) {
  switch (confirmation_type) {
    case ConfirmationType::kViewed:
    case ConfirmationType::kClicked:
    case ConfirmationType::kDismissed: {
      return false;
    }

    case ConfirmationType::kServed:
    case ConfirmationType::kLanded:
    case ConfirmationType::kSaved:
    case ConfirmationType::kFlagged:
    case ConfirmationType::kUpvoted:
    case ConfirmationType::kDownvoted:
    case ConfirmationType::kConversion: {
      return true;
    }

    case ConfirmationType::kUndefined: {
      NOTREACHED_NORETURN();
    }
  }

  NOTREACHED_NORETURN() << "Unexpected value for ConfirmationType: "
                        << base::to_underlying(confirmation_type);
}

std::map</*placement_id=*/std::string, HistoryItemInfo> BuildBuckets(
    const HistoryItemList& history_items) {
  std::map<std::string, HistoryItemInfo> buckets;

  for (const auto& history_item : history_items) {
    const ConfirmationType confirmation_type =
        history_item.ad_content.confirmation_type;
    if (ShouldFilterConfirmationType(confirmation_type)) {
      continue;
    }

    const std::string& placement_id = history_item.ad_content.placement_id;
    const auto iter = buckets.find(placement_id);
    if (iter == buckets.cend()) {
      buckets.insert({placement_id, history_item});
    } else {
      const HistoryItemInfo& current_history_item = iter->second;
      if (current_history_item.ad_content.confirmation_type >
          confirmation_type) {
        buckets[placement_id] = history_item;
      }
    }
  }

  return buckets;
}

}  // namespace

HistoryItemList ConfirmationHistoryFilter::Apply(
    const HistoryItemList& history) const {
  const std::map<std::string, HistoryItemInfo> buckets = BuildBuckets(history);

  HistoryItemList filtered_history;
  for (const auto& [placement_id, history_item] : buckets) {
    filtered_history.push_back(history_item);
  }

  return filtered_history;
}

}  // namespace brave_ads
