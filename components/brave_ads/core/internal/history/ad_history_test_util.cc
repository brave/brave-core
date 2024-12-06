/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/history/ad_history_test_util.h"

#include <string>

#include "base/uuid.h"
#include "brave/components/brave_ads/core/internal/ad_units/ad_test_constants.h"
#include "brave/components/brave_ads/core/internal/ad_units/ad_test_util.h"
#include "brave/components/brave_ads/core/internal/history/ad_history_builder_util.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "brave/components/brave_ads/core/public/ad_units/ad_info.h"
#include "brave/components/brave_ads/core/public/history/ad_history_item_info.h"

namespace brave_ads::test {

AdHistoryItemInfo BuildAdHistoryItem(
    mojom::AdType mojom_ad_type,
    mojom::ConfirmationType mojom_confirmation_type,
    bool should_generate_random_uuids) {
  const AdInfo ad = BuildAd(mojom_ad_type, should_generate_random_uuids);
  return BuildAdHistoryItem(ad, mojom_confirmation_type, kTitle, kDescription);
}

AdHistoryList BuildAdHistory(
    mojom::AdType mojom_ad_type,
    const std::vector<mojom::ConfirmationType>& mojom_confirmation_types,
    bool should_generate_random_uuids) {
  AdHistoryList ad_history;

  for (const auto& mojom_confirmation_type : mojom_confirmation_types) {
    const AdHistoryItemInfo ad_history_item = BuildAdHistoryItem(
        mojom_ad_type, mojom_confirmation_type, should_generate_random_uuids);
    ad_history.push_back(ad_history_item);
  }

  return ad_history;
}

AdHistoryList BuildAdHistoryForSamePlacement(
    mojom::AdType mojom_ad_type,
    const std::vector<mojom::ConfirmationType>& mojom_confirmation_types,
    bool should_generate_random_uuids) {
  AdHistoryList ad_history = BuildAdHistory(
      mojom_ad_type, mojom_confirmation_types, should_generate_random_uuids);

  // Assign the same placement id to all ad history items.
  const std::string placement_id =
      base::Uuid::GenerateRandomV4().AsLowercaseString();
  for (auto& ad_history_item : ad_history) {
    ad_history_item.placement_id = placement_id;
  }

  return ad_history;
}

}  // namespace brave_ads::test
