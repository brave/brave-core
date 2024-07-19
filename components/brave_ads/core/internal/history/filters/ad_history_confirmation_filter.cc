/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/history/filters/ad_history_confirmation_filter.h"

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

std::map</*placement_id*/ std::string, AdHistoryItemInfo> BuildBuckets(
    const AdHistoryList& ad_history) {
  std::map<std::string, AdHistoryItemInfo> buckets;

  for (const auto& ad_history_item : ad_history) {
    const ConfirmationType confirmation_type =
        ad_history_item.confirmation_type;
    if (ShouldFilterConfirmationType(confirmation_type)) {
      continue;
    }

    auto [iter, inserted] =
        buckets.try_emplace(ad_history_item.placement_id, ad_history_item);
    if (!inserted) {
      const AdHistoryItemInfo& current_ad_history = iter->second;
      if (current_ad_history.confirmation_type > confirmation_type) {
        iter->second = ad_history_item;
      }
    }
  }

  return buckets;
}

}  // namespace

void AdHistoryConfirmationFilter::Apply(AdHistoryList& ad_history) const {
  const std::map<std::string, AdHistoryItemInfo> buckets =
      BuildBuckets(ad_history);

  AdHistoryList filtered_ad_history;
  filtered_ad_history.reserve(buckets.size());

  for (const auto& [_, ad_history_item] : buckets) {
    filtered_ad_history.push_back(ad_history_item);
  }

  ad_history = filtered_ad_history;
}

}  // namespace brave_ads
