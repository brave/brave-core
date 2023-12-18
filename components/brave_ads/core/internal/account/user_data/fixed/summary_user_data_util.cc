/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/user_data/fixed/summary_user_data_util.h"

namespace brave_ads {

AdTypeBucketMap BuildAdTypeBuckets(const PaymentTokenList& payment_tokens) {
  AdTypeBucketMap buckets;

  for (const auto& payment_token : payment_tokens) {
    ++buckets[payment_token.ad_type][payment_token.confirmation_type];
  }

  return buckets;
}

}  // namespace brave_ads
