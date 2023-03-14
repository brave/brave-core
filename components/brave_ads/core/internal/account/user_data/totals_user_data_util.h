/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_USER_DATA_TOTALS_USER_DATA_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_USER_DATA_TOTALS_USER_DATA_UTIL_H_

#include "brave/components/brave_ads/core/internal/privacy/tokens/unblinded_payment_tokens/unblinded_payment_token_info.h"

#include "brave/components/brave_ads/core/internal/account/user_data/totals_user_data_alias.h"

namespace ads::user_data {

AdTypeBucketMap BuildBuckets(
    const privacy::UnblindedPaymentTokenList& unblinded_payment_tokens);

}  // namespace ads::user_data

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_USER_DATA_TOTALS_USER_DATA_UTIL_H_
