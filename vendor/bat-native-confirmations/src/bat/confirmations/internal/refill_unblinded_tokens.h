/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_CONFIRMATIONS_INTERNAL_REFILL_UNBLINDED_TOKENS_H_
#define BAT_CONFIRMATIONS_INTERNAL_REFILL_UNBLINDED_TOKENS_H_

#include <memory>
#include <string>
#include <vector>
#include <map>

#include "bat/confirmations/confirmations_client.h"
#include "bat/confirmations/wallet_info.h"
#include "bat/confirmations/internal/refill_unblinded_tokens_delegate.h"
#include "bat/confirmations/internal/retry_timer.h"

#include "wrapper.hpp"  // NOLINT

namespace confirmations {

class ConfirmationsImpl;
class UnblindedTokens;

using challenge_bypass_ristretto::Token;
using challenge_bypass_ristretto::BlindedToken;

class RefillUnblindedTokens {
 public:
  RefillUnblindedTokens(
      ConfirmationsImpl* confirmations,
      UnblindedTokens* unblinded_tokens);

  ~RefillUnblindedTokens();

  void set_delegate(
      RefillUnblindedTokensDelegate* delegate);

  void Refill(
      const WalletInfo& wallet_info,
      const std::string& public_key);

 private:
  WalletInfo wallet_info_;

  std::string public_key_;

  std::string nonce_;

  std::vector<Token> tokens_;
  std::vector<BlindedToken> blinded_tokens_;

  void RequestSignedTokens();
  void OnRequestSignedTokens(
      const UrlResponse& url_response);

  void GetSignedTokens();
  void OnGetSignedTokens(
      const UrlResponse& url_response);

  void OnRefill(
      const Result result,
      const bool should_retry = true);

  RetryTimer retry_timer_;
  void OnRetry();

  bool ShouldRefillUnblindedTokens() const;
  int CalculateAmountOfTokensToRefill() const;

  void GenerateAndBlindTokens(const int count);

  ConfirmationsImpl* confirmations_;  // NOT OWNED
  UnblindedTokens* unblinded_tokens_;  // NOT OWNED

  RefillUnblindedTokensDelegate* delegate_ = nullptr;
};

}  // namespace confirmations

#endif  // BAT_CONFIRMATIONS_INTERNAL_REFILL_UNBLINDED_TOKENS_H_
