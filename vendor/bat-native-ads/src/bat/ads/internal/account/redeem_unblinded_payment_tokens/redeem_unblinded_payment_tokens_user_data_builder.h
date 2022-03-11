/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_REDEEM_UNBLINDED_PAYMENT_TOKENS_REDEEM_UNBLINDED_PAYMENT_TOKENS_USER_DATA_BUILDER_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_REDEEM_UNBLINDED_PAYMENT_TOKENS_REDEEM_UNBLINDED_PAYMENT_TOKENS_USER_DATA_BUILDER_H_

#include "bat/ads/internal/account/user_data/user_data_builder.h"
#include "bat/ads/internal/privacy/unblinded_payment_tokens/unblinded_payment_token_info_aliases.h"

namespace ads {

class RedeemUnblindedPaymentTokensUserDataBuilder final
    : public UserDataBuilder {
 public:
  RedeemUnblindedPaymentTokensUserDataBuilder(
      const privacy::UnblindedPaymentTokenList& unblinded_payment_tokens);
  ~RedeemUnblindedPaymentTokensUserDataBuilder() override;

  void Build(UserDataBuilderCallback callback) const override;

 private:
  privacy::UnblindedPaymentTokenList unblinded_payment_tokens_;

  RedeemUnblindedPaymentTokensUserDataBuilder(
      const RedeemUnblindedPaymentTokensUserDataBuilder&) = delete;
  RedeemUnblindedPaymentTokensUserDataBuilder& operator=(
      const RedeemUnblindedPaymentTokensUserDataBuilder&) = delete;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_REDEEM_UNBLINDED_PAYMENT_TOKENS_REDEEM_UNBLINDED_PAYMENT_TOKENS_USER_DATA_BUILDER_H_
