/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_UTILITY_REFILL_UNBLINDED_TOKENS_REQUEST_SIGNED_TOKENS_URL_REQUEST_BUILDER_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_UTILITY_REFILL_UNBLINDED_TOKENS_REQUEST_SIGNED_TOKENS_URL_REQUEST_BUILDER_H_

#include <string>
#include <vector>

#include "bat/ads/internal/account/wallet/wallet_info.h"
#include "bat/ads/internal/privacy/challenge_bypass_ristretto/blinded_token.h"
#include "bat/ads/internal/server/url/url_request_builder_interface.h"
#include "bat/ads/public/interfaces/ads.mojom-forward.h"

class GURL;

namespace ads {

class RequestSignedTokensUrlRequestBuilder final
    : public UrlRequestBuilderInterface {
 public:
  RequestSignedTokensUrlRequestBuilder(
      WalletInfo wallet,
      std::vector<privacy::cbr::BlindedToken> tokens);

  RequestSignedTokensUrlRequestBuilder(
      const RequestSignedTokensUrlRequestBuilder& other) = delete;
  RequestSignedTokensUrlRequestBuilder& operator=(
      const RequestSignedTokensUrlRequestBuilder& other) = delete;

  RequestSignedTokensUrlRequestBuilder(
      RequestSignedTokensUrlRequestBuilder&& other) noexcept = delete;
  RequestSignedTokensUrlRequestBuilder& operator=(
      RequestSignedTokensUrlRequestBuilder&& other) noexcept = delete;

  ~RequestSignedTokensUrlRequestBuilder() override;

  mojom::UrlRequestInfoPtr Build() override;

 private:
  GURL BuildUrl() const;

  std::vector<std::string> BuildHeaders(const std::string& body) const;
  std::string BuildSignatureHeaderValue(const std::string& body) const;

  std::string BuildBody() const;

  const WalletInfo wallet_;
  const std::vector<privacy::cbr::BlindedToken> blinded_tokens_;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_UTILITY_REFILL_UNBLINDED_TOKENS_REQUEST_SIGNED_TOKENS_URL_REQUEST_BUILDER_H_
