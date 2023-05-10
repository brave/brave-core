/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_CONFIRMATIONS_REDEEM_CONFIRMATION_REDEEM_CONFIRMATION_DELEGATE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_CONFIRMATIONS_REDEEM_CONFIRMATION_REDEEM_CONFIRMATION_DELEGATE_H_

#include "brave/components/brave_ads/core/internal/privacy/tokens/unblinded_payment_tokens/unblinded_payment_token_info.h"

namespace brave_ads {

namespace privacy {
struct UnblindedPaymentTokenInfo;
}  // namespace privacy

struct ConfirmationInfo;

class RedeemConfirmationDelegate {
 public:
  // Invoked to tell the delegate that the |confirmation| was successfully
  // redeemed and an |unblinded_payment_token| was provided for an opted-in
  // user.
  virtual void OnDidRedeemOptedInConfirmation(
      const ConfirmationInfo& confirmation,
      const privacy::UnblindedPaymentTokenInfo& unblinded_payment_token) {}

  // Invoked to tell the delegate that the |confirmation| was successfully
  // redeemed for an opted-out user.
  virtual void OnDidRedeemOptedOutConfirmation(
      const ConfirmationInfo& confirmation) {}

  // Invoked to tell the delegate that |confirmation| redemption failed and
  // whether we |should_retry| and |should_backoff| for subsequent failures.
  virtual void OnFailedToRedeemConfirmation(
      const ConfirmationInfo& confirmation,
      const bool should_retry,
      const bool should_backoff) {}

 protected:
  virtual ~RedeemConfirmationDelegate() = default;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_CONFIRMATIONS_REDEEM_CONFIRMATION_REDEEM_CONFIRMATION_DELEGATE_H_
