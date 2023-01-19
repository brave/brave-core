/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_UTILITY_REDEEM_UNBLINDED_TOKEN_REDEEM_UNBLINDED_TOKEN_DELEGATE_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_UTILITY_REDEEM_UNBLINDED_TOKEN_REDEEM_UNBLINDED_TOKEN_DELEGATE_H_

namespace ads {

namespace privacy {
struct UnblindedPaymentTokenInfo;
}  // namespace privacy

struct ConfirmationInfo;

class RedeemUnblindedTokenDelegate {
 public:
  // Invoked to tell the delegate the |confirmation| was successfully sent.
  virtual void OnDidSendConfirmation(const ConfirmationInfo& confirmation) {}

  // Invoked to tell the delegate the |confirmation| failed to send.
  // |should_retry| is |true| if we should retry otherwise |false|.
  virtual void OnFailedToSendConfirmation(const ConfirmationInfo& confirmation,
                                          const bool should_retry) {}

  // Invoked to tell the delegate an unblinded token was redeemed for the
  // corresponding |confirmation| and |unblinded_payment_token|.
  virtual void OnDidRedeemUnblindedToken(
      const ConfirmationInfo& confirmation,
      const privacy::UnblindedPaymentTokenInfo& unblinded_payment_token) {}

  // Invoked to tell the delegate an unblinded token redemption failed for the
  // corresponding |confirmation| and whether we |should_retry| and
  // |should_backoff| for subsequent failures.
  virtual void OnFailedToRedeemUnblindedToken(
      const ConfirmationInfo& confirmation,
      const bool should_retry,
      const bool should_backoff) {}

 protected:
  virtual ~RedeemUnblindedTokenDelegate() = default;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_UTILITY_REDEEM_UNBLINDED_TOKEN_REDEEM_UNBLINDED_TOKEN_DELEGATE_H_
