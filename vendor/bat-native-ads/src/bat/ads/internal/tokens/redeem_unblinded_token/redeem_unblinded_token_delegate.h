/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_TOKENS_REDEEM_UNBLINDED_TOKEN_REDEEM_UNBLINDED_TOKEN_DELEGATE_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_TOKENS_REDEEM_UNBLINDED_TOKEN_REDEEM_UNBLINDED_TOKEN_DELEGATE_H_

#include "bat/ads/internal/account/confirmations/confirmation_info.h"
#include "bat/ads/internal/privacy/unblinded_tokens/unblinded_token_info.h"

namespace ads {

class RedeemUnblindedTokenDelegate {
 public:
  virtual ~RedeemUnblindedTokenDelegate() = default;

  // Invoked to tell the delegate a |confirmation| was sent
  virtual void OnDidSendConfirmation(const ConfirmationInfo& confirmation) {}

  // Invoked to tell the delegate an unblinded token was redeemed for the
  // corresponding |confirmation| and |unblinded_payment_token|
  virtual void OnDidRedeemUnblindedToken(
      const ConfirmationInfo& confirmation,
      const privacy::UnblindedTokenInfo& unblinded_payment_token) {}

  // Invoked to tell the delegate unblinded token redemption failed for the
  // corresponding |confirmation| and whether we should retry
  virtual void OnFailedToRedeemUnblindedToken(
      const ConfirmationInfo& confirmation,
      const bool should_retry) {}
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_TOKENS_REDEEM_UNBLINDED_TOKEN_REDEEM_UNBLINDED_TOKEN_DELEGATE_H_
