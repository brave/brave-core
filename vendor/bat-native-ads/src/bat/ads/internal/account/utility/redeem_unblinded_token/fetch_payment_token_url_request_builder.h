/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_UTILITY_REDEEM_UNBLINDED_TOKEN_FETCH_PAYMENT_TOKEN_URL_REQUEST_BUILDER_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_UTILITY_REDEEM_UNBLINDED_TOKEN_FETCH_PAYMENT_TOKEN_URL_REQUEST_BUILDER_H_

#include "bat/ads/internal/account/confirmations/confirmation_info.h"
#include "bat/ads/internal/server/url/url_request_builder_interface.h"
#include "bat/ads/public/interfaces/ads.mojom-forward.h"

class GURL;

namespace ads {

class FetchPaymentTokenUrlRequestBuilder final
    : public UrlRequestBuilderInterface {
 public:
  explicit FetchPaymentTokenUrlRequestBuilder(ConfirmationInfo confirmation);

  mojom::UrlRequestInfoPtr Build() override;

 private:
  GURL BuildUrl() const;

  ConfirmationInfo confirmation_;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_UTILITY_REDEEM_UNBLINDED_TOKEN_FETCH_PAYMENT_TOKEN_URL_REQUEST_BUILDER_H_
