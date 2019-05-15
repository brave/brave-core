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

namespace confirmations {

class ConfirmationsImpl;
class UnblindedTokens;

class PayoutTokens {
 public:
  PayoutTokens(
      ConfirmationsImpl* confirmations,
      ConfirmationsClient* confirmations_client,
      UnblindedTokens* unblinded_payment_tokens);

  ~PayoutTokens();

  void Payout(const WalletInfo& wallet_info);

 private:
  WalletInfo wallet_info_;

  void RedeemPaymentTokens();
  void OnRedeemPaymentTokens(
      const std::string& url,
      const int response_status_code,
      const std::string& response,
      const std::map<std::string, std::string>& headers);

  void OnPayout(const Result result);

  void ScheduleNextPayout() const;
  uint64_t next_retry_start_timer_in_;
  unsigned backoff_count_;
  void RetryNextPayout();

  ConfirmationsImpl* confirmations_;  // NOT OWNED
  ConfirmationsClient* confirmations_client_;  // NOT OWNED
  UnblindedTokens* unblinded_payment_tokens_;  // NOT OWNED
};

}  // namespace confirmations

#endif  // BAT_CONFIRMATIONS_INTERNAL_PAYOUT_TOKENS_H_
