/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/user_data/totals_user_data.h"

#include <string>
#include <utility>

#include "base/strings/string_number_conversions.h"
#include "base/values.h"
#include "bat/ads/internal/account/user_data/totals_user_data_util.h"

namespace ads {
namespace user_data {

namespace {

constexpr char kTotalsKey[] = "totals";
constexpr char kAdFormatKey[] = "ad_format";

}  // namespace

base::DictionaryValue GetTotals(
    const privacy::UnblindedPaymentTokenList& unblinded_payment_tokens) {
  const AdTypeBucketMap buckets = BuildBuckets(unblinded_payment_tokens);

  base::ListValue list;
  for (const auto& bucket : buckets) {
    base::DictionaryValue total;

    const std::string& ad_format = bucket.first;
    total.SetStringKey(kAdFormatKey, ad_format);

    const ConfirmationTypeBucketMap& confirmations = bucket.second;
    for (const auto& confirmation : confirmations) {
      const std::string& confirmation_type = confirmation.first;
      const std::string count = base::NumberToString(confirmation.second);

      total.SetStringKey(confirmation_type, count);
    }

    list.Append(std::move(total));
  }

  base::DictionaryValue user_data;
  user_data.SetKey(kTotalsKey, std::move(list));
  return user_data;
}

}  // namespace user_data
}  // namespace ads
