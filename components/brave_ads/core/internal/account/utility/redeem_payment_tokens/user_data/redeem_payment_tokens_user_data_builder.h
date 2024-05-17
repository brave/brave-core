/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_UTILITY_REDEEM_PAYMENT_TOKENS_USER_DATA_REDEEM_PAYMENT_TOKENS_USER_DATA_BUILDER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_UTILITY_REDEEM_PAYMENT_TOKENS_USER_DATA_REDEEM_PAYMENT_TOKENS_USER_DATA_BUILDER_H_

#include "base/values.h"
#include "brave/components/brave_ads/core/internal/account/tokens/payment_tokens/payment_token_info.h"

namespace brave_ads {

base::Value::Dict BuildRedeemPaymentTokensUserData(
    const PaymentTokenList& payment_tokens);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_UTILITY_REDEEM_PAYMENT_TOKENS_USER_DATA_REDEEM_PAYMENT_TOKENS_USER_DATA_BUILDER_H_
