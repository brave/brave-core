/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_UTILITY_REFILL_CONFIRMATION_TOKENS_URL_REQUESTS_REQUEST_SIGNED_TOKENS_REQUEST_SIGNED_TOKENS_URL_REQUEST_BUILDER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_UTILITY_REFILL_CONFIRMATION_TOKENS_URL_REQUESTS_REQUEST_SIGNED_TOKENS_REQUEST_SIGNED_TOKENS_URL_REQUEST_BUILDER_H_

#include <string>
#include <vector>

#include "brave/components/brave_ads/core/internal/account/wallet/wallet_info.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/blinded_token.h"
#include "brave/components/brave_ads/core/internal/common/url/request_builder/url_request_builder_interface.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-forward.h"

class GURL;

namespace brave_ads {

class RequestSignedTokensUrlRequestBuilder final
    : public UrlRequestBuilderInterface {
 public:
  RequestSignedTokensUrlRequestBuilder(WalletInfo wallet,
                                       cbr::BlindedTokenList tokens);

  RequestSignedTokensUrlRequestBuilder(
      const RequestSignedTokensUrlRequestBuilder&) = delete;
  RequestSignedTokensUrlRequestBuilder& operator=(
      const RequestSignedTokensUrlRequestBuilder&) = delete;

  RequestSignedTokensUrlRequestBuilder(
      RequestSignedTokensUrlRequestBuilder&&) noexcept = delete;
  RequestSignedTokensUrlRequestBuilder& operator=(
      RequestSignedTokensUrlRequestBuilder&&) noexcept = delete;

  ~RequestSignedTokensUrlRequestBuilder() override;

  mojom::UrlRequestInfoPtr Build() override;

 private:
  GURL BuildUrl() const;

  std::vector<std::string> BuildHeaders(const std::string& body) const;
  std::string BuildSignatureHeaderValue(const std::string& body) const;

  std::string BuildBody() const;

  const WalletInfo wallet_;
  const cbr::BlindedTokenList blinded_tokens_;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_UTILITY_REFILL_CONFIRMATION_TOKENS_URL_REQUESTS_REQUEST_SIGNED_TOKENS_REQUEST_SIGNED_TOKENS_URL_REQUEST_BUILDER_H_
