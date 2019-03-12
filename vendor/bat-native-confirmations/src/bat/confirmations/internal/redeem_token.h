/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_CONFIRMATIONS_INTERNAL_REDEEM_TOKEN_H_
#define BAT_CONFIRMATIONS_INTERNAL_REDEEM_TOKEN_H_

#include <string>
#include <vector>
#include <map>

#include "bat/confirmations/confirmations_client.h"
#include "bat/confirmations/internal/token_info.h"
#include "bat/confirmations/confirmation_type.h"

#include "wrapper.hpp"  // NOLINT

using challenge_bypass_ristretto::Token;
using challenge_bypass_ristretto::BlindedToken;

namespace confirmations {

class ConfirmationsImpl;
class UnblindedTokens;

class RedeemToken {
 public:
  RedeemToken(
      ConfirmationsImpl* confirmations,
      ConfirmationsClient* confirmations_client,
      UnblindedTokens* unblinded_tokens,
      UnblindedTokens* unblinded_payment_tokens);

  ~RedeemToken();

  void Redeem(
      const std::string& creative_instance_id,
      const ConfirmationType confirmation_type);

 private:
  void CreateConfirmation(
      const std::string& creative_instance_id,
      const TokenInfo& token_info,
      const ConfirmationType confirmation_type);
  void OnCreateConfirmation(
      const std::string& url,
      const int response_status_code,
      const std::string& response,
      const std::map<std::string, std::string>& headers,
      const ConfirmationType confirmation_type,
      const std::string& confirmation_id,
      const Token& payment_token,
      const BlindedToken& blinded_payment_token,
      const TokenInfo& token_info);

  void FetchPaymentToken(
      const ConfirmationType confirmation_type,
      const std::string& confirmation_id,
      const Token& payment_token,
      const BlindedToken& blinded_payment_token,
      const TokenInfo& token_info);
  void OnFetchPaymentToken(
      const std::string& url,
      const int response_status_code,
      const std::string& response,
      const std::map<std::string, std::string>& headers,
      const ConfirmationType confirmation_type,
      const Token& payment_token,
      const BlindedToken& blinded_payment_token,
      const TokenInfo& token_info);

  void OnRedeem(
      const Result result,
      const TokenInfo& token_info);

  ConfirmationsImpl* confirmations_;  // NOT OWNED
  ConfirmationsClient* confirmations_client_;  // NOT OWNED
  UnblindedTokens* unblinded_tokens_;  // NOT OWNED
  UnblindedTokens* unblinded_payment_tokens_;  // NOT OWNED
};

}  // namespace confirmations

#endif  // BAT_CONFIRMATIONS_INTERNAL_REDEEM_TOKEN_H_
