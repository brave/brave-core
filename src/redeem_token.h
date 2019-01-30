/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_CONFIRMATIONS_REDEEM_TOKEN_H_
#define BAT_CONFIRMATIONS_REDEEM_TOKEN_H_

#include "confirmations_impl.h"
#include "bat/confirmations/confirmations_client.h"

#include "brave/vendor/challenge_bypass_ristretto_ffi/src/wrapper.hpp"

namespace confirmations {

using namespace challenge_bypass_ristretto;

class ConfirmationsImpl;

class RedeemToken {
 public:
  RedeemToken(
      ConfirmationsImpl* confirmations,
      ConfirmationsClient* confirmations_client);

  ~RedeemToken();

  void Redeem(
      const std::string& creative_instance_id,
      const std::string& public_key);

 private:
  PublicKey public_key_;

  void CreateConfirmation(
      const std::string& creative_instance_id,
      const std::string& unblinded_token_base64);
  void OnCreateConfirmation(
      const std::string& url,
      const int response_status_code,
      const std::string& response,
      const std::map<std::string, std::string>& headers,
      const std::string& confirmation_id,
      const std::string& token_base64,
      const std::string& blinded_payment_token_base64);

  void FetchPaymentToken(
      const std::string& confirmation_id,
      const std::string& token_base64,
      const std::string& blinded_payment_token_base64);
  void OnFetchPaymentToken(
      const std::string& url,
      const int response_status_code,
      const std::string& response,
      const std::map<std::string, std::string>& headers,
      const std::string& token_base64,
      const std::string& blinded_payment_token_base64);

  void OnRedeem(const Result result);

  void RemoveUnblindedToken();

  void AppendUnblindedPaymentTokens(
      const std::vector<UnblindedToken>& unblinded_paymemt_tokens);

  ConfirmationsImpl* confirmations_;  // NOT OWNED
  ConfirmationsClient* confirmations_client_;  // NOT OWNED
};

}  // namespace confirmations

#endif  // BAT_CONFIRMATIONS_REDEEM_TOKEN_H_
