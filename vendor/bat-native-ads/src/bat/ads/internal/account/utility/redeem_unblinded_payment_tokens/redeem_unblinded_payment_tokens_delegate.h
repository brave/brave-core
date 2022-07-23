/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_UTILITY_REDEEM_UNBLINDED_PAYMENT_TOKENS_REDEEM_UNBLINDED_PAYMENT_TOKENS_DELEGATE_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_UTILITY_REDEEM_UNBLINDED_PAYMENT_TOKENS_REDEEM_UNBLINDED_PAYMENT_TOKENS_DELEGATE_H_

#include "base/time/time.h"
#include "bat/ads/internal/privacy/tokens/unblinded_payment_tokens/unblinded_payment_token_info.h"

namespace ads {

class RedeemUnblindedPaymentTokensDelegate {
 public:
  virtual ~RedeemUnblindedPaymentTokensDelegate() = default;

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
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_UTILITY_REDEEM_UNBLINDED_PAYMENT_TOKENS_REDEEM_UNBLINDED_PAYMENT_TOKENS_DELEGATE_H_
