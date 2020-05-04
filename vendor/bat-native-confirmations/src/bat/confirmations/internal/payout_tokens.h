/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_CONFIRMATIONS_INTERNAL_PAYOUT_TOKENS_H_
#define BAT_CONFIRMATIONS_INTERNAL_PAYOUT_TOKENS_H_

#include <stdint.h>
#include <string>
#include <map>

#include "bat/confirmations/confirmations_client.h"
#include "bat/confirmations/wallet_info.h"
#include "bat/confirmations/internal/retry_timer.h"
#include "bat/confirmations/internal/timer.h"

namespace confirmations {

class ConfirmationsImpl;
class UnblindedTokens;

class PayoutTokens {
 public:
  PayoutTokens(
      ConfirmationsImpl* confirmations,
      UnblindedTokens* unblinded_payment_tokens);

  ~PayoutTokens();

  void PayoutAfterDelay(
      const WalletInfo& wallet_info);

  uint64_t get_token_redemption_timestamp_in_seconds() const;
  void set_token_redemption_timestamp_in_seconds(
      const uint64_t timestamp_in_seconds);

 private:
  WalletInfo wallet_info_;

  Timer timer_;

  void RedeemPaymentTokens();
  void OnRedeemPaymentTokens(
      const UrlResponse& url_response);

  void OnPayout(const Result result);

  void ScheduleNextPayout();
  uint64_t token_redemption_timestamp_in_seconds_ = 0;

  RetryTimer retry_timer_;
  void OnRetry();

  uint64_t CalculatePayoutDelay();
  void UpdateNextTokenRedemptionDate();

  ConfirmationsImpl* confirmations_;  // NOT OWNED
  UnblindedTokens* unblinded_payment_tokens_;  // NOT OWNED
};

}  // namespace confirmations

#endif  // BAT_CONFIRMATIONS_INTERNAL_PAYOUT_TOKENS_H_
