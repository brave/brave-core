/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_CONFIRMATIONS_REDEEM_UNBLINDED_PAYMENT_TOKENS_REDEEM_UNBLINDED_PAYMENT_TOKENS_DELEGATE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_CONFIRMATIONS_REDEEM_UNBLINDED_PAYMENT_TOKENS_REDEEM_UNBLINDED_PAYMENT_TOKENS_DELEGATE_H_

#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/privacy/tokens/unblinded_payment_tokens/unblinded_payment_token_info.h"

namespace brave_ads {

class RedeemUnblindedPaymentTokensDelegate {
 public:
  // Invoked to tell the delegate that the |unblinded_payment_tokens| were
  // successfully redeemed.
  virtual void OnDidRedeemUnblindedPaymentTokens(
      const privacy::UnblindedPaymentTokenList& unblinded_payment_tokens) {}

  // Invoked to tell the delegate that the unblinded payment tokens failed to
  // redeem.
  virtual void OnFailedToRedeemUnblindedPaymentTokens() {}

  // Invoked to tell the delegate that unblinded payment token redemption is
  // scheduled for |redeem_at|.
  virtual void OnDidScheduleNextUnblindedPaymentTokensRedemption(
      const base::Time redeem_at) {}

  // Invoked to tell the delegate that we will retry redeeming unblinded payment
  // tokens at |retry_at|.
  virtual void OnWillRetryRedeemingUnblindedPaymentTokens(
      const base::Time retry_at) {}

  // Invoked to tell the delegate that we retried redeeming unblinded payment
  // tokens.
  virtual void OnDidRetryRedeemingUnblindedPaymentTokens() {}

 protected:
  virtual ~RedeemUnblindedPaymentTokensDelegate() = default;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_CONFIRMATIONS_REDEEM_UNBLINDED_PAYMENT_TOKENS_REDEEM_UNBLINDED_PAYMENT_TOKENS_DELEGATE_H_
