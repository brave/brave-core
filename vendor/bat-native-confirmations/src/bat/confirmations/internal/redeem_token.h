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
#include "bat/confirmations/confirmation_type.h"

#include "wrapper.hpp"  // NOLINT

using challenge_bypass_ristretto::Token;
using challenge_bypass_ristretto::BlindedToken;

namespace confirmations {

class ConfirmationsImpl;
class UnblindedTokens;
struct AdInfo;
struct ConfirmationInfo;
struct TokenInfo;

class RedeemToken {
 public:
  RedeemToken(
      ConfirmationsImpl* confirmations,
      ConfirmationsClient* confirmations_client,
      UnblindedTokens* unblinded_tokens,
      UnblindedTokens* unblinded_payment_tokens);

  ~RedeemToken();

  void Redeem(
      const AdInfo& info,
      const ConfirmationType confirmation_type);
  void Redeem(
      const std::string& creative_instance_id,
      const std::string& creative_set_id,
      const ConfirmationType confirmation_type);
  void Redeem(
      const ConfirmationInfo& confirmation);

 private:
  void CreateConfirmation(
      const ConfirmationInfo& confirmation);
  void CreateConfirmation(
      const AdInfo& ad_info,
      const ConfirmationType confirmation_type,
      const TokenInfo& token_info);
  void OnCreateConfirmation(
      const std::string& url,
      const int response_status_code,
      const std::string& response,
      const std::map<std::string, std::string>& headers,
      const ConfirmationInfo& confirmation);

  void FetchPaymentToken(
      const ConfirmationInfo& confirmation);
  void OnFetchPaymentToken(
      const std::string& url,
      const int response_status_code,
      const std::string& response,
      const std::map<std::string, std::string>& headers,
      const ConfirmationInfo& confirmation);

  void OnRedeem(
      const Result result,
      const ConfirmationInfo& confirmation,
      const bool should_retry = true);

  bool Verify(
     const ConfirmationInfo& confirmation) const;

  ConfirmationsImpl* confirmations_;  // NOT OWNED
  ConfirmationsClient* confirmations_client_;  // NOT OWNED
  UnblindedTokens* unblinded_tokens_;  // NOT OWNED
  UnblindedTokens* unblinded_payment_tokens_;  // NOT OWNED
};

}  // namespace confirmations

#endif  // BAT_CONFIRMATIONS_INTERNAL_REDEEM_TOKEN_H_
