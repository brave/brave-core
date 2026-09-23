/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_UTILITY_REDEEM_PAYMENT_TOKENS_REDEEM_PAYMENT_TOKENS_DELEGATE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_UTILITY_REDEEM_PAYMENT_TOKENS_REDEEM_PAYMENT_TOKENS_DELEGATE_H_

#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/account/tokens/payment_tokens/payment_token_info.h"

namespace brave_ads {

class RedeemPaymentTokensDelegate {
 public:
  // Invoked to tell the delegate that the `payment_tokens` were successfully
  // redeemed.
  virtual void OnDidRedeemPaymentTokens(
      const PaymentTokenList& payment_tokens) {}

  // Invoked to tell the delegate that the payment tokens failed to redeem.
  virtual void OnFailedToRedeemPaymentTokens() {}

  // Invoked to tell the delegate that payment token redemption is scheduled for
  // `redeem_at`.
  virtual void OnDidScheduleNextPaymentTokenRedemption(base::Time redeem_at) {}

  // Invoked to tell the delegate that we will retry redeeming payment tokens at
  // `retry_at`.
  virtual void OnWillRetryRedeemingPaymentTokens(base::Time retry_at) {}

  // Invoked to tell the delegate that we retried redeeming payment tokens.
  virtual void OnDidRetryRedeemingPaymentTokens() {}

 protected:
  virtual ~RedeemPaymentTokensDelegate() = default;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_UTILITY_REDEEM_PAYMENT_TOKENS_REDEEM_PAYMENT_TOKENS_DELEGATE_H_
