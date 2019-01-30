/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_CONFIRMATIONS_PAYOUT_TOKENS_H_
#define BAT_CONFIRMATIONS_PAYOUT_TOKENS_H_

#include "confirmations_impl.h"
#include "bat/confirmations/confirmations_client.h"
#include "bat/confirmations/wallet_info.h"

#include "brave/vendor/challenge_bypass_ristretto_ffi/src/wrapper.hpp"

namespace confirmations {

using namespace challenge_bypass_ristretto;

class ConfirmationsImpl;

class PayoutTokens {
 public:
  PayoutTokens(
      ConfirmationsImpl* confirmations,
      ConfirmationsClient* confirmations_client);

  ~PayoutTokens();

  void Payout(const WalletInfo& wallet_info, const std::string& public_key);

 private:
  std::string payment_id_;

  PublicKey public_key_;

  void RedeemPaymentTokens();
  void OnRedeemPaymentTokens(
      const std::string& url,
      const int response_status_code,
      const std::string& response,
      const std::map<std::string, std::string>& headers);

  void OnPayout(const Result result);

  void RemoveAllUnblindedPaymentTokens();

  ConfirmationsImpl* confirmations_;  // NOT OWNED
  ConfirmationsClient* confirmations_client_;  // NOT OWNED
};

}  // namespace confirmations

#endif  // BAT_CONFIRMATIONS_PAYOUT_TOKENS_H_
