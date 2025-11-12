/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/user_data/fixed/segment_user_data.h"

#include <string_view>

#include "brave/components/brave_ads/core/internal/account/transactions/transaction_info.h"
#include "brave/components/brave_ads/core/internal/settings/settings.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"

namespace brave_ads {

namespace {
constexpr std::string_view kSegmentKey = "segment";
}  // namespace

base::Value::Dict BuildSegmentUserData(const TransactionInfo& transaction) {
  if (!UserHasJoinedBraveRewards()) {
    return {};
  }

  if (transaction.ad_type == mojom::AdType::kSearchResultAd) {
    // Unsupported ad type.
    return {};
  }

  if (transaction.segment.empty()) {
    // Invalid segment.
    return {};
  }

  return base::Value::Dict().Set(kSegmentKey, transaction.segment);
}

}  // namespace brave_ads
