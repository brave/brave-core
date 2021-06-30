/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_TOKENS_REFILL_UNBLINDED_TOKENS_REQUEST_SIGNED_TOKENS_URL_REQUEST_BUILDER_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_TOKENS_REFILL_UNBLINDED_TOKENS_REQUEST_SIGNED_TOKENS_URL_REQUEST_BUILDER_H_

#include <string>
#include <vector>

#include "bat/ads/internal/account/wallet/wallet_info.h"
#include "bat/ads/internal/server/url_request_builder.h"
#include "bat/ads/public/interfaces/ads.mojom.h"
#include "wrapper.hpp"

namespace ads {

using challenge_bypass_ristretto::BlindedToken;

class RequestSignedTokensUrlRequestBuilder : UrlRequestBuilder {
 public:
  RequestSignedTokensUrlRequestBuilder(const WalletInfo& wallet,
                                       const std::vector<BlindedToken>& tokens);

  ~RequestSignedTokensUrlRequestBuilder() override;

  mojom::UrlRequestPtr Build() override;

 private:
  WalletInfo wallet_;
  std::vector<BlindedToken> blinded_tokens_;

  std::string BuildUrl() const;

  std::vector<std::string> BuildHeaders(const std::string& body) const;
  std::string BuildDigestHeaderValue(const std::string& body) const;
  std::string BuildSignatureHeaderValue(const std::string& body) const;

  std::string BuildBody() const;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_TOKENS_REFILL_UNBLINDED_TOKENS_REQUEST_SIGNED_TOKENS_URL_REQUEST_BUILDER_H_
