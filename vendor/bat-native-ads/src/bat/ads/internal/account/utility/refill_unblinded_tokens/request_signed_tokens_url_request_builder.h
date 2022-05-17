/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_UTILITY_REFILL_UNBLINDED_TOKENS_REQUEST_SIGNED_TOKENS_URL_REQUEST_BUILDER_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_UTILITY_REFILL_UNBLINDED_TOKENS_REQUEST_SIGNED_TOKENS_URL_REQUEST_BUILDER_H_

#include <string>
#include <vector>

#include "bat/ads/internal/account/wallet/wallet_info.h"
#include "bat/ads/internal/server/url/url_request_builder_interface.h"
#include "bat/ads/public/interfaces/ads.mojom.h"
#include "url/gurl.h"

namespace ads {

namespace privacy {
namespace cbr {
class BlindedToken;
}  // namespace cbr
}  // namespace privacy

class RequestSignedTokensUrlRequestBuilder final : UrlRequestBuilderInterface {
 public:
  RequestSignedTokensUrlRequestBuilder(
      const WalletInfo& wallet,
      const std::vector<privacy::cbr::BlindedToken>& tokens);
  ~RequestSignedTokensUrlRequestBuilder() override;

  mojom::UrlRequestPtr Build() override;

 private:
  GURL BuildUrl() const;

  std::vector<std::string> BuildHeaders(const std::string& body) const;
  std::string BuildDigestHeaderValue(const std::string& body) const;
  std::string BuildSignatureHeaderValue(const std::string& body) const;

  std::string BuildBody() const;

  WalletInfo wallet_;
  std::vector<privacy::cbr::BlindedToken> blinded_tokens_;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_UTILITY_REFILL_UNBLINDED_TOKENS_REQUEST_SIGNED_TOKENS_URL_REQUEST_BUILDER_H_
