/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_CONFIRMATIONS_INTERNAL_REDEEM_PAYMENT_TOKENS_REQUEST_H_
#define BAT_CONFIRMATIONS_INTERNAL_REDEEM_PAYMENT_TOKENS_REQUEST_H_

#include <string>
#include <vector>

#include "bat/confirmations/confirmations_client.h"
#include "bat/confirmations/wallet_info.h"
#include "bat/confirmations/internal/token_info.h"

#include "base/values.h"

namespace confirmations {

class RedeemPaymentTokensRequest {
 public:
  RedeemPaymentTokensRequest();
  ~RedeemPaymentTokensRequest();

  std::string BuildUrl(
      const WalletInfo& wallet_info) const;

  URLRequestMethod GetMethod() const;

  std::string BuildBody(
      const TokenList& tokens,
      const std::string& payload) const;

  std::string CreatePayload(
      const WalletInfo& wallet_info) const;

  std::vector<std::string> BuildHeaders() const;
  std::string GetAcceptHeaderValue() const;

  std::string GetContentType() const;

 private:
  base::Value CreatePaymentRequestDTO(
      const TokenList& tokens,
      const std::string& payload) const;

  base::Value CreateCredential(
      const TokenInfo& token_info,
      const std::string& payload) const;
};

}  // namespace confirmations

#endif  // BAT_CONFIRMATIONS_INTERNAL_REDEEM_PAYMENT_TOKENS_REQUEST_H_
