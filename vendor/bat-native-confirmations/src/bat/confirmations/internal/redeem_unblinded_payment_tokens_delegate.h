/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_CONFIRMATIONS_INTERNAL_REDEEM_UNBLINDED_PAYMENT_TOKENS_DELEGATE_H_
#define BAT_CONFIRMATIONS_INTERNAL_REDEEM_UNBLINDED_PAYMENT_TOKENS_DELEGATE_H_

namespace confirmations {

class RedeemUnblindedPaymentTokensDelegate {
 public:
  RedeemUnblindedPaymentTokensDelegate() = default;
  virtual ~RedeemUnblindedPaymentTokensDelegate() = default;

  virtual void OnDidRedeemUnblindedPaymentTokens() = 0;
  virtual void OnFailedToRedeemUnblindedPaymentTokens() = 0;
  virtual void OnDidRetryRedeemingUnblindedPaymentTokens() = 0;
};

}  // namespace confirmations

#endif  // BAT_CONFIRMATIONS_INTERNAL_REDEEM_UNBLINDED_PAYMENT_TOKENS_DELEGATE_H_  // NOLINT
