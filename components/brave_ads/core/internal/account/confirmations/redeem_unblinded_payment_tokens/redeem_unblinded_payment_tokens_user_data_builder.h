/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_CONFIRMATIONS_REDEEM_UNBLINDED_PAYMENT_TOKENS_REDEEM_UNBLINDED_PAYMENT_TOKENS_USER_DATA_BUILDER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_CONFIRMATIONS_REDEEM_UNBLINDED_PAYMENT_TOKENS_REDEEM_UNBLINDED_PAYMENT_TOKENS_USER_DATA_BUILDER_H_

#include "brave/components/brave_ads/core/internal/account/user_data/user_data_builder_interface.h"
#include "brave/components/brave_ads/core/internal/privacy/tokens/unblinded_payment_tokens/unblinded_payment_token_info.h"

namespace brave_ads {

class RedeemUnblindedPaymentTokensUserDataBuilder final
    : public UserDataBuilderInterface {
 public:
  explicit RedeemUnblindedPaymentTokensUserDataBuilder(
      privacy::UnblindedPaymentTokenList unblinded_payment_tokens);

  RedeemUnblindedPaymentTokensUserDataBuilder(
      const RedeemUnblindedPaymentTokensUserDataBuilder&) = delete;
  RedeemUnblindedPaymentTokensUserDataBuilder& operator=(
      const RedeemUnblindedPaymentTokensUserDataBuilder&) = delete;

  RedeemUnblindedPaymentTokensUserDataBuilder(
      RedeemUnblindedPaymentTokensUserDataBuilder&&) noexcept = delete;
  RedeemUnblindedPaymentTokensUserDataBuilder& operator=(
      RedeemUnblindedPaymentTokensUserDataBuilder&&) noexcept = delete;

  ~RedeemUnblindedPaymentTokensUserDataBuilder() override;

  void Build(UserDataBuilderCallback callback) const override;

 private:
  privacy::UnblindedPaymentTokenList unblinded_payment_tokens_;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_CONFIRMATIONS_REDEEM_UNBLINDED_PAYMENT_TOKENS_REDEEM_UNBLINDED_PAYMENT_TOKENS_USER_DATA_BUILDER_H_
