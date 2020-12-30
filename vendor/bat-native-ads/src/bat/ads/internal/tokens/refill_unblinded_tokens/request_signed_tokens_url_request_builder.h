/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_TOKENS_REFILL_UNBLINDED_TOKENS_REQUEST_SIGNED_TOKENS_URL_REQUEST_BUILDER_H_  // NOLINT
#define BAT_ADS_INTERNAL_TOKENS_REFILL_UNBLINDED_TOKENS_REQUEST_SIGNED_TOKENS_URL_REQUEST_BUILDER_H_  // NOLINT

#include <string>
#include <vector>

#include "wrapper.hpp"
#include "bat/ads/internal/account/wallet/wallet_info.h"
#include "bat/ads/internal/server/url_request_builder.h"
#include "bat/ads/mojom.h"

namespace ads {

using challenge_bypass_ristretto::BlindedToken;

class RequestSignedTokensUrlRequestBuilder : UrlRequestBuilder {
 public:
  RequestSignedTokensUrlRequestBuilder(
      const WalletInfo& wallet,
      const std::vector<BlindedToken>& tokens);

  ~RequestSignedTokensUrlRequestBuilder() override;

  UrlRequestPtr Build() override;

 private:
  WalletInfo wallet_;
  std::vector<BlindedToken> blinded_tokens_;

  std::string BuildUrl() const;

  std::vector<std::string> BuildHeaders(
      const std::string& body) const;
  std::string BuildDigestHeaderValue(
      const std::string& body) const;
  std::string BuildSignatureHeaderValue(
      const std::string& body) const;
  std::string BuildViaHeader() const;

  std::string BuildBody() const;
};

}  // namespace ads

#endif  // BAT_ADS_INTERNAL_TOKENS_REFILL_UNBLINDED_TOKENS_REQUEST_SIGNED_TOKENS_URL_REQUEST_BUILDER_H_  // NOLINT
