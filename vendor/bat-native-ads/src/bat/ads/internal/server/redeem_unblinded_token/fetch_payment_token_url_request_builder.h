/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_SERVER_REDEEM_UNBLINDED_TOKEN_FETCH_PAYMENT_TOKEN_URL_REQUEST_BUILDER_H_  // NOLINT
#define BAT_ADS_INTERNAL_SERVER_REDEEM_UNBLINDED_TOKEN_FETCH_PAYMENT_TOKEN_URL_REQUEST_BUILDER_H_  // NOLINT

#include <string>

#include "bat/ads/internal/confirmations/confirmation_info.h"
#include "bat/ads/internal/server/url_request_builder.h"

namespace ads {

class FetchPaymentTokenUrlRequestBuilder : UrlRequestBuilder {
 public:
  FetchPaymentTokenUrlRequestBuilder(
      const ConfirmationInfo& confirmation);

  ~FetchPaymentTokenUrlRequestBuilder() override;

  UrlRequestPtr Build() override;

 private:
  ConfirmationInfo confirmation_;

  std::string BuildUrl() const;
};

}  // namespace ads

#endif  // BAT_ADS_INTERNAL_SERVER_REDEEM_UNBLINDED_TOKEN_FETCH_PAYMENT_TOKEN_URL_REQUEST_BUILDER_H_  // NOLINT
