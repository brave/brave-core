/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/confirmations/redeem_unblinded_payment_tokens/redeem_unblinded_payment_tokens_user_data_builder.h"

#include <utility>

#include "base/functional/callback.h"
#include "base/values.h"
#include "brave/components/brave_ads/core/internal/account/user_data/platform_user_data.h"
#include "brave/components/brave_ads/core/internal/account/user_data/summary_user_data.h"

namespace brave_ads {

RedeemUnblindedPaymentTokensUserDataBuilder::
    RedeemUnblindedPaymentTokensUserDataBuilder(
        privacy::UnblindedPaymentTokenList unblinded_payment_tokens)
    : unblinded_payment_tokens_(std::move(unblinded_payment_tokens)) {}

RedeemUnblindedPaymentTokensUserDataBuilder::
    ~RedeemUnblindedPaymentTokensUserDataBuilder() = default;

void RedeemUnblindedPaymentTokensUserDataBuilder::Build(
    UserDataBuilderCallback callback) const {
  base::Value::Dict user_data;
  user_data.Merge(BuildPlatformUserData());
  user_data.Merge(BuildSummaryUserData(unblinded_payment_tokens_));

  std::move(callback).Run(std::move(user_data));
}

}  // namespace brave_ads
