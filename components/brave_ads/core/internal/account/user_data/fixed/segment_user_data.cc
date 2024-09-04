/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/user_data/fixed/segment_user_data.h"

#include "brave/components/brave_ads/core/internal/account/transactions/transaction_info.h"
#include "brave/components/brave_ads/core/internal/settings/settings.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"

namespace brave_ads {

namespace {
constexpr char kSegmentKey[] = "segment";
}  // namespace

base::Value::Dict BuildSegmentUserData(const TransactionInfo& transaction) {
  if (!UserHasJoinedBraveRewards()) {
    return {};
  }

  base::Value::Dict user_data;

  if (transaction.ad_type != mojom::AdType::kSearchResultAd &&
      !transaction.segment.empty()) {
    user_data.Set(kSegmentKey, transaction.segment);
  }

  return user_data;
}

}  // namespace brave_ads
