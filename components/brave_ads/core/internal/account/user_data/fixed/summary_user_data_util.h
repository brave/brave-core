/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_USER_DATA_FIXED_SUMMARY_USER_DATA_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_USER_DATA_FIXED_SUMMARY_USER_DATA_UTIL_H_

#include <string>

#include "base/containers/flat_map.h"
#include "brave/components/brave_ads/core/internal/account/tokens/payment_tokens/payment_token_info.h"

namespace brave_ads {

using ConfirmationTypeBucketMap =
    base::flat_map</*confirmation_type=*/std::string, /*count=*/int>;
using AdTypeBucketMap =
    base::flat_map</*ad_type=*/std::string, ConfirmationTypeBucketMap>;

AdTypeBucketMap BuildBuckets(const PaymentTokenList& payment_tokens);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_USER_DATA_FIXED_SUMMARY_USER_DATA_UTIL_H_
