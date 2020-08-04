/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_SERVER_AD_REWARDS_AD_GRANTS_AD_GRANTS_URL_REQUEST_BUILDER_H_  // NOLINT
#define BAT_ADS_INTERNAL_SERVER_AD_REWARDS_AD_GRANTS_AD_GRANTS_URL_REQUEST_BUILDER_H_  // NOLINT

#include <string>

#include "bat/ads/internal/server/url_request_builder.h"
#include "bat/ads/internal/wallet/wallet_info.h"

namespace ads {

class AdGrantsUrlRequestBuilder : UrlRequestBuilder {
 public:
  AdGrantsUrlRequestBuilder(
      const WalletInfo& wallet);

  ~AdGrantsUrlRequestBuilder() override;

  UrlRequestPtr Build() override;

 private:
  WalletInfo wallet_;

  std::string BuildUrl() const;
};

}  // namespace ads

#endif  // BAT_ADS_INTERNAL_SERVER_AD_REWARDS_AD_GRANTS_AD_GRANTS_URL_REQUEST_BUILDER_H_  // NOLINT
