/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/user_data/summary_user_data.h"

#include <utility>

#include "brave/components/brave_ads/core/internal/account/user_data/summary_user_data_util.h"

namespace brave_ads {

namespace {

constexpr char kSummaryKey[] = "totals";
constexpr char kAdFormatKey[] = "ad_format";

}  // namespace

base::Value::Dict BuildSummaryUserData(
    const privacy::UnblindedPaymentTokenList& unblinded_payment_tokens) {
  const AdTypeBucketMap buckets = BuildBuckets(unblinded_payment_tokens);

  base::Value::List list;
  for (const auto& [ad_format, confirmations] : buckets) {
    base::Value::Dict dict;

    dict.Set(kAdFormatKey, ad_format);

    for (const auto& [confirmation_type, count] : confirmations) {
      dict.Set(confirmation_type, count);
    }

    list.Append(std::move(dict));
  }

  base::Value::Dict user_data;
  user_data.Set(kSummaryKey, std::move(list));
  return user_data;
}

}  // namespace brave_ads
