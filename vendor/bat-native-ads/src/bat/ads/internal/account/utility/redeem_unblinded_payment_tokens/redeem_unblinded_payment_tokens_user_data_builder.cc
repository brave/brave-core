/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/utility/redeem_unblinded_payment_tokens/redeem_unblinded_payment_tokens_user_data_builder.h"

#include <utility>

#include "base/values.h"
#include "bat/ads/internal/account/user_data/odyssey_user_data.h"
#include "bat/ads/internal/account/user_data/platform_user_data.h"
#include "bat/ads/internal/account/user_data/totals_user_data.h"

namespace ads {

RedeemUnblindedPaymentTokensUserDataBuilder::
    RedeemUnblindedPaymentTokensUserDataBuilder(
        const privacy::UnblindedPaymentTokenList& unblinded_payment_tokens)
    : unblinded_payment_tokens_(unblinded_payment_tokens) {}

RedeemUnblindedPaymentTokensUserDataBuilder::
    ~RedeemUnblindedPaymentTokensUserDataBuilder() = default;

void RedeemUnblindedPaymentTokensUserDataBuilder::Build(
    UserDataBuilderCallback callback) const {
  base::Value::Dict user_data;

  base::Value::Dict odyssey_user_data = user_data::GetOdyssey();
  user_data.Merge(odyssey_user_data);

  base::Value::Dict platform_user_data = user_data::GetPlatform();
  user_data.Merge(platform_user_data);

  base::Value::Dict totals_user_data =
      user_data::GetTotals(unblinded_payment_tokens_);
  user_data.Merge(totals_user_data);

  callback(user_data);
}

}  // namespace ads
