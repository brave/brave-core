/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_SERVER_REFILL_UNBLINDED_TOKENS_GET_SIGNED_TOKENS_URL_REQUEST_BUILDER_H_  // NOLINT
#define BAT_ADS_INTERNAL_SERVER_REFILL_UNBLINDED_TOKENS_GET_SIGNED_TOKENS_URL_REQUEST_BUILDER_H_  // NOLINT

#include <string>

#include "bat/ads/internal/server/url_request_builder.h"
#include "bat/ads/internal/wallet/wallet_info.h"
#include "bat/ads/mojom.h"

namespace ads {

class GetSignedTokensUrlRequestBuilder : UrlRequestBuilder {
 public:
  GetSignedTokensUrlRequestBuilder(
      const WalletInfo& wallet,
      const std::string& nonce);

  ~GetSignedTokensUrlRequestBuilder() override;

  UrlRequestPtr Build() override;

 private:
  WalletInfo wallet_;
  std::string nonce_;

  std::string BuildUrl() const;
};

}  // namespace ads

#endif  // BAT_ADS_INTERNAL_SERVER_REFILL_UNBLINDED_TOKENS_GET_SIGNED_TOKENS_URL_REQUEST_BUILDER_H_  // NOLINT
