/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_CONFIRMATIONS_REDEEM_CONFIRMATION_URL_REQUEST_BUILDERS_FETCH_PAYMENT_TOKEN_URL_REQUEST_BUILDER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_CONFIRMATIONS_REDEEM_CONFIRMATION_URL_REQUEST_BUILDERS_FETCH_PAYMENT_TOKEN_URL_REQUEST_BUILDER_H_

#include "brave/components/brave_ads/common/interfaces/brave_ads.mojom-forward.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/confirmation_info.h"
#include "brave/components/brave_ads/core/internal/common/url/request_builder/url_request_builder_interface.h"

class GURL;

namespace brave_ads {

class FetchPaymentTokenUrlRequestBuilder final
    : public UrlRequestBuilderInterface {
 public:
  explicit FetchPaymentTokenUrlRequestBuilder(ConfirmationInfo confirmation);

  mojom::UrlRequestInfoPtr Build() override;

 private:
  GURL BuildUrl() const;

  ConfirmationInfo confirmation_;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_CONFIRMATIONS_REDEEM_CONFIRMATION_URL_REQUEST_BUILDERS_FETCH_PAYMENT_TOKEN_URL_REQUEST_BUILDER_H_
