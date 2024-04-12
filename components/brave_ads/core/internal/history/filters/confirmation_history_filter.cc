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

bool ShouldFilterConfirmationType(const ConfirmationType confirmation_type) {
  switch (confirmation_type) {
    case ConfirmationType::kViewedImpression:
    case ConfirmationType::kClicked:
    case ConfirmationType::kDismissed: {
      return false;
    }

    case ConfirmationType::kServedImpression:
    case ConfirmationType::kLanded:
    case ConfirmationType::kSavedAd:
    case ConfirmationType::kMarkAdAsInappropriate:
    case ConfirmationType::kLikedAd:
    case ConfirmationType::kDislikedAd:
    case ConfirmationType::kConversion:
    case ConfirmationType::kMediaPlay:
    case ConfirmationType::kMedia25:
    case ConfirmationType::kMedia100: {
      return true;
    }

    case ConfirmationType::kUndefined: {
      break;
    }
  }

  NOTREACHED_NORETURN() << "Unexpected value for ConfirmationType: "
                        << base::to_underlying(confirmation_type);
}

std::map</*placement_id*/ std::string, HistoryItemInfo> BuildBuckets(
    const HistoryItemList& history_items) {
  std::map<std::string, HistoryItemInfo> buckets;

  for (const auto& history_item : history_items) {
    const ConfirmationType confirmation_type =
        history_item.ad_content.confirmation_type;
    if (ShouldFilterConfirmationType(confirmation_type)) {
      continue;
    }

    const std::string& placement_id = history_item.ad_content.placement_id;
    auto [iter, inserted] = buckets.try_emplace(placement_id, history_item);
    if (!inserted) {
      const HistoryItemInfo& current_history_item = iter->second;
      if (current_history_item.ad_content.confirmation_type >
          confirmation_type) {
        iter->second = history_item;
      }
    }
  }

  return buckets;
}

}  // namespace

void ConfirmationHistoryFilter::Apply(HistoryItemList& history) const {
  const std::map<std::string, HistoryItemInfo> buckets = BuildBuckets(history);

  HistoryItemList filtered_history;
  filtered_history.reserve(buckets.size());

  for (const auto& [_, history_item] : buckets) {
    filtered_history.push_back(history_item);
  }

  history = filtered_history;
}

}  // namespace brave_ads
