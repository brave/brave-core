/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/user_data/fixed/top_segment_user_data.h"

#include <string>
#include <utility>

#include "brave/components/brave_ads/core/internal/account/transactions/transaction_info.h"
#include "brave/components/brave_ads/core/internal/serving/targeting/segments/top_segments.h"
#include "brave/components/brave_ads/core/internal/serving/targeting/user_model/interest/interest_segments.h"
#include "brave/components/brave_ads/core/internal/settings/settings.h"
#include "brave/components/brave_ads/core/public/account/confirmations/confirmation_type.h"

namespace brave_ads {

namespace {

constexpr char kTopSegmentKey[] = "topSegment";
constexpr char kInterestSegmentKey[] = "interest";

}  // namespace

base::Value::Dict BuildTopSegmentUserData(const TransactionInfo& transaction) {
  base::Value::Dict user_data;

  if (!UserHasJoinedBraveRewards()) {
    return user_data;
  }

  if (transaction.confirmation_type != ConfirmationType::kViewed) {
    return user_data;
  }

  base::Value::List list;

  if (const absl::optional<std::string> top_segment =
          GetTopSegment(BuildInterestSegments(), /*parent_only=*/false)) {
    list.Append(base::Value::Dict().Set(kInterestSegmentKey, *top_segment));
  }

  user_data.Set(kTopSegmentKey, std::move(list));

  return user_data;
}

}  // namespace brave_ads
