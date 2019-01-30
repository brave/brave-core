/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_CONFIRMATIONS_REFILL_TOKENS_H_
#define BAT_CONFIRMATIONS_REFILL_TOKENS_H_

#include <string>
#include <vector>
#include <memory>

#include "confirmations_impl.h"
#include "bat/confirmations/confirmations_client.h"
#include "bat/confirmations/wallet_info.h"

#include "brave/vendor/challenge_bypass_ristretto_ffi/src/wrapper.hpp"

namespace confirmations {

using namespace challenge_bypass_ristretto;

class ConfirmationsImpl;

class RefillTokens {
 public:
  RefillTokens(
      ConfirmationsImpl* confirmations,
      ConfirmationsClient* confirmations_client);

  ~RefillTokens();

  void Refill(const WalletInfo& wallet_info, const std::string& public_key);

  void RetryGettingSignedTokens();

 private:
  std::string payment_id_;
  std::vector<uint8_t> secret_key_;

  PublicKey public_key_;

  std::string last_fetch_tokens_ads_serve_url_;

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

  void OnRefill(const Result result);

  void AppendUnblindedTokens(
      const std::vector<UnblindedToken>& unblinded_tokens);

  ConfirmationsImpl* confirmations_;  // NOT OWNED
  ConfirmationsClient* confirmations_client_;  // NOT OWNED
};

}  // namespace confirmations

#endif  // BAT_CONFIRMATIONS_REFILL_TOKENS_H_
