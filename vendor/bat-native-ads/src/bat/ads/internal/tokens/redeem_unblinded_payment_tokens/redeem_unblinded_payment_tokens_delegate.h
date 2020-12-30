/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_TOKENS_REDEEM_UNBLINDED_PAYMENT_TOKENS_REDEEM_UNBLINDED_PAYMENT_TOKENS_DELEGATE_H_  // NOLINT
#define BAT_ADS_INTERNAL_TOKENS_REDEEM_UNBLINDED_PAYMENT_TOKENS_REDEEM_UNBLINDED_PAYMENT_TOKENS_DELEGATE_H_  // NOLINT

#include "base/time/time.h"

namespace ads {

class RedeemUnblindedPaymentTokensDelegate {
 public:
  virtual ~RedeemUnblindedPaymentTokensDelegate() = default;

  // Invoked to tell the delegate unblinded payment tokens were redeemed
  virtual void OnDidRedeemUnblindedPaymentTokens() {}

  // Invoked to tell the delegate unblinded payment token redemption failed
  virtual void OnFailedToRedeemUnblindedPaymentTokens() {}

  // Invoked to tell the delegate that we have scheduled the unblinded payment
  // token redemption |time|
  virtual void OnDidScheduleNextUnblindedPaymentTokensRedemption(
      const base::Time& time) {}

  // Invoked to tell the delegate that we will retry to redeem unblinded
  // payment tokens
  virtual void OnWillRetryRedeemingUnblindedPaymentTokens() {}

  // Invoked to tell the delegate that we did retry to redeem unblinded
  // payment tokens
  virtual void OnDidRetryRedeemingUnblindedPaymentTokens() {}
};

}  // namespace ads

#endif  // BAT_ADS_INTERNAL_TOKENS_REDEEM_UNBLINDED_PAYMENT_TOKENS_REDEEM_UNBLINDED_PAYMENT_TOKENS_DELEGATE_H_  // NOLINT
