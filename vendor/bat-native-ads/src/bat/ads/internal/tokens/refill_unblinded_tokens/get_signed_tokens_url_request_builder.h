/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_TOKENS_REFILL_UNBLINDED_TOKENS_GET_SIGNED_TOKENS_URL_REQUEST_BUILDER_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_TOKENS_REFILL_UNBLINDED_TOKENS_GET_SIGNED_TOKENS_URL_REQUEST_BUILDER_H_

#include <string>

#include "bat/ads/internal/account/wallet/wallet_info.h"
#include "bat/ads/internal/server/url_request_builder.h"
#include "bat/ads/public/interfaces/ads.mojom.h"

namespace ads {

class GetSignedTokensUrlRequestBuilder : UrlRequestBuilder {
 public:
  GetSignedTokensUrlRequestBuilder(const WalletInfo& wallet,
                                   const std::string& nonce);

  ~GetSignedTokensUrlRequestBuilder() override;

  mojom::UrlRequestPtr Build() override;

 private:
  WalletInfo wallet_;
  std::string nonce_;

  std::string BuildUrl() const;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_TOKENS_REFILL_UNBLINDED_TOKENS_GET_SIGNED_TOKENS_URL_REQUEST_BUILDER_H_
