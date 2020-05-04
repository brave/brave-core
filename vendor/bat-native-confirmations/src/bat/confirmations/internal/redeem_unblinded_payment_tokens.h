/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_CONFIRMATIONS_INTERNAL_REDEEM_UNBLINDED_PAYMENT_TOKENS_H_
#define BAT_CONFIRMATIONS_INTERNAL_REDEEM_UNBLINDED_PAYMENT_TOKENS_H_

#include <stdint.h>
#include <string>
#include <map>

#include "bat/confirmations/confirmations_client.h"
#include "bat/confirmations/wallet_info.h"
#include "bat/confirmations/internal/redeem_unblinded_payment_tokens_delegate.h"
#include "bat/confirmations/internal/retry_timer.h"
#include "bat/confirmations/internal/timer.h"

namespace confirmations {

class ConfirmationsImpl;
class UnblindedTokens;

class RedeemUnblindedPaymentTokens {
 public:
  RedeemUnblindedPaymentTokens(
      ConfirmationsImpl* confirmations,
      UnblindedTokens* unblinded_payment_tokens);

  ~RedeemUnblindedPaymentTokens();

  void set_delegate(
      RedeemUnblindedPaymentTokensDelegate* delegate);

  void RedeemAfterDelay(
      const WalletInfo& wallet_info);

  uint64_t get_token_redemption_timestamp() const;
  void set_token_redemption_timestamp(
      const uint64_t timestamp_in_seconds);

 private:
  WalletInfo wallet_info_;

  Timer timer_;

  void Redeem();
  void OnRedeem(
      const UrlResponse& url_response);

  void OnRedeemUnblindedPaymentTokens(
      const Result result);

  void ScheduleNextTokenRedemption();
  uint64_t token_redemption_timestamp_in_seconds_ = 0;

  RetryTimer retry_timer_;
  void OnRetry();

  uint64_t CalculateTokenRedemptionDelay();
  void UpdateNextTokenRedemptionDate();

  ConfirmationsImpl* confirmations_;  // NOT OWNED
  UnblindedTokens* unblinded_payment_tokens_;  // NOT OWNED

  RedeemUnblindedPaymentTokensDelegate* delegate_ = nullptr;
};

}  // namespace confirmations

#endif  // BAT_CONFIRMATIONS_INTERNAL_REDEEM_UNBLINDED_PAYMENT_TOKENS_H_
