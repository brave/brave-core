/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_TOKENS_REDEEM_PAYMENT_TOKENS_USER_DATA_REDEEM_PAYMENT_TOKENS_USER_DATA_BUILDER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_TOKENS_REDEEM_PAYMENT_TOKENS_USER_DATA_REDEEM_PAYMENT_TOKENS_USER_DATA_BUILDER_H_

#include "brave/components/brave_ads/core/internal/account/user_data/build_user_data_callback.h"
#include "brave/components/brave_ads/core/internal/privacy/tokens/payment_tokens/payment_token_info.h"

namespace brave_ads {

void BuildRedeemPaymentTokensUserData(
    const privacy::PaymentTokenList& payment_tokens,
    BuildUserDataCallback callback);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_TOKENS_REDEEM_PAYMENT_TOKENS_USER_DATA_REDEEM_PAYMENT_TOKENS_USER_DATA_BUILDER_H_
