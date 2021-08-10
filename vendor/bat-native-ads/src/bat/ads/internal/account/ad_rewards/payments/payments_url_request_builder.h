/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_AD_REWARDS_PAYMENTS_PAYMENTS_URL_REQUEST_BUILDER_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_AD_REWARDS_PAYMENTS_PAYMENTS_URL_REQUEST_BUILDER_H_

#include <string>
#include <vector>

#include "bat/ads/internal/account/wallet/wallet_info.h"
#include "bat/ads/internal/server/url_request_builder.h"
#include "bat/ads/public/interfaces/ads.mojom.h"

namespace ads {

class PaymentsUrlRequestBuilder : UrlRequestBuilder {
 public:
  explicit PaymentsUrlRequestBuilder(const WalletInfo& wallet);

  ~PaymentsUrlRequestBuilder() override;

  mojom::UrlRequestPtr Build() override;

 private:
  WalletInfo wallet_;

  std::string BuildUrl() const;

  std::vector<std::string> BuildHeaders(const std::string& body) const;
  std::string BuildDigestHeaderValue(const std::string& body) const;
  std::string BuildSignatureHeaderValue(const std::string& body) const;

  std::string BuildBody() const;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_AD_REWARDS_PAYMENTS_PAYMENTS_URL_REQUEST_BUILDER_H_
