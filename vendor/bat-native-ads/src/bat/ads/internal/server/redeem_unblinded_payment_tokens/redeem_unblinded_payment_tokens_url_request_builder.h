/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_SERVER_REDEEM_UNBLINDED_PAYMENT_TOKENS_REDEEM_UNBLINDED_PAYMENT_TOKENS_URL_REQUEST_BUILDER_H_  // NOLINT
#define BAT_ADS_INTERNAL_SERVER_REDEEM_UNBLINDED_PAYMENT_TOKENS_REDEEM_UNBLINDED_PAYMENT_TOKENS_URL_REQUEST_BUILDER_H_  // NOLINT

#include <string>
#include <vector>

#include "base/values.h"
#include "bat/ads/internal/privacy/unblinded_tokens/unblinded_token_info.h"
#include "bat/ads/internal/server/url_request_builder.h"
#include "bat/ads/internal/wallet/wallet_info.h"

namespace ads {

class RedeemUnblindedPaymentTokensUrlRequestBuilder : UrlRequestBuilder {
 public:
  RedeemUnblindedPaymentTokensUrlRequestBuilder(
      const WalletInfo& wallet,
      const privacy::UnblindedTokenList& unblinded_tokens);

  ~RedeemUnblindedPaymentTokensUrlRequestBuilder() override;

  UrlRequestPtr Build() override;

 private:
  WalletInfo wallet_;
  privacy::UnblindedTokenList unblinded_tokens_;

  std::string BuildUrl() const;

  std::vector<std::string> BuildHeaders() const;

  std::string BuildBody(
      const std::string& payload) const;

  std::string CreatePayload() const;

  base::Value CreatePaymentRequestDTO(
      const std::string& payload) const;

  base::Value CreateCredential(
      const privacy::UnblindedTokenInfo& unblinded_token,
      const std::string& payload) const;
};

}  // namespace ads

#endif  // BAT_ADS_INTERNAL_SERVER_REDEEM_UNBLINDED_PAYMENT_TOKENS_REDEEM_UNBLINDED_PAYMENT_TOKENS_URL_REQUEST_BUILDER_H_  // NOLINT
