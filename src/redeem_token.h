/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_CONFIRMATIONS_REDEEM_TOKEN_H_
#define BAT_CONFIRMATIONS_REDEEM_TOKEN_H_

#include <string>
#include <vector>

#include "bat/confirmations/confirmations_client.h"

#include "wrapper.hpp"

using challenge_bypass_ristretto::Token;
using challenge_bypass_ristretto::BlindedToken;
using challenge_bypass_ristretto::UnblindedToken;

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
      const std::string& creative_instance_id);

 private:
  void CreateConfirmation(
      const std::string& creative_instance_id,
      const UnblindedToken& unblinded_token);
  void OnCreateConfirmation(
      const std::string& url,
      const int response_status_code,
      const std::string& response,
      const std::map<std::string, std::string>& headers,
      const std::string& confirmation_id,
      const Token& payment_token,
      const BlindedToken& blinded_payment_token,
      const UnblindedToken& unblinded_token);

  void FetchPaymentToken(
      const std::string& confirmation_id,
      const Token& payment_token,
      const BlindedToken& blinded_payment_token,
      const UnblindedToken& unblinded_token);
  void OnFetchPaymentToken(
      const std::string& url,
      const int response_status_code,
      const std::string& response,
      const std::map<std::string, std::string>& headers,
      const Token& payment_token,
      const BlindedToken& blinded_payment_token,
      const UnblindedToken& unblinded_token);

  void OnRedeem(
      const Result result,
      const UnblindedToken& unblinded_token);

  ConfirmationsImpl* confirmations_;  // NOT OWNED
  ConfirmationsClient* confirmations_client_;  // NOT OWNED
  UnblindedTokens* unblinded_tokens_;  // NOT OWNED
  UnblindedTokens* unblinded_payment_tokens_;  // NOT OWNED
};

}  // namespace confirmations

#endif  // BAT_CONFIRMATIONS_REDEEM_TOKEN_H_
