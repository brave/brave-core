/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_CONFIRMATIONS_INTERNAL_REFILL_TOKENS_H_
#define BAT_CONFIRMATIONS_INTERNAL_REFILL_TOKENS_H_

#include <string>
#include <vector>
#include <map>

#include "bat/confirmations/confirmations_client.h"
#include "bat/confirmations/wallet_info.h"

#include "wrapper.hpp"

using challenge_bypass_ristretto::Token;
using challenge_bypass_ristretto::BlindedToken;

namespace confirmations {

class ConfirmationsImpl;
class UnblindedTokens;

class RefillTokens {
 public:
  RefillTokens(
      ConfirmationsImpl* confirmations,
      ConfirmationsClient* confirmations_client,
      UnblindedTokens* unblinded_tokens);

  ~RefillTokens();

  void Refill(const WalletInfo& wallet_info, const std::string& public_key);

  void RetryGettingSignedTokens();

 private:
  WalletInfo wallet_info_;

  std::string public_key_;

  std::string nonce_;

  std::vector<Token> tokens_;
  std::vector<BlindedToken> blinded_tokens_;

  void RequestSignedTokens();
  void OnRequestSignedTokens(
      const std::string& url,
      const int response_status_code,
      const std::string& response,
      const std::map<std::string, std::string>& headers);

  void GetSignedTokens();
  void OnGetSignedTokens(
      const std::string& url,
      const int response_status_code,
      const std::string& response,
      const std::map<std::string, std::string>& headers);

  void OnRefill(
      const Result result,
      const bool should_retry = true);

  bool ShouldRefillTokens() const;
  int CalculateAmountOfTokensToRefill() const;

  void GenerateAndBlindTokens(const int count);

  ConfirmationsImpl* confirmations_;  // NOT OWNED
  ConfirmationsClient* confirmations_client_;  // NOT OWNED
  UnblindedTokens* unblinded_tokens_;  // NOT OWNED
};

}  // namespace confirmations

#endif  // BAT_CONFIRMATIONS_INTERNAL_REFILL_TOKENS_H_
