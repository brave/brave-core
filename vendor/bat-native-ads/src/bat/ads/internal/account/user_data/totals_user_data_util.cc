/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/user_data/totals_user_data_util.h"

#include <string>

namespace ads {
namespace user_data {

AdTypeBucketMap BuildBuckets(
    const privacy::UnblindedPaymentTokenList& unblinded_payment_tokens) {
  AdTypeBucketMap buckets;

  for (const auto& unblinded_payment_token : unblinded_payment_tokens) {
    const std::string ad_type = unblinded_payment_token.ad_type.ToString();
    const std::string confirmation_type =
        unblinded_payment_token.confirmation_type.ToString();

    buckets[ad_type][confirmation_type]++;
  }

  return buckets;
}

}  // namespace user_data
}  // namespace ads
