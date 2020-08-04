/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/server/redeem_unblinded_token/fetch_payment_token_url_request_builder.h"

#include "base/strings/stringprintf.h"
#include "bat/ads/internal/confirmations/confirmation_info.h"
#include "bat/ads/internal/logging.h"
#include "bat/ads/internal/server/ads_server_util.h"
#include "bat/ads/mojom.h"

namespace ads {

FetchPaymentTokenUrlRequestBuilder::FetchPaymentTokenUrlRequestBuilder(
    const ConfirmationInfo& confirmation)
    : confirmation_(confirmation) {
  DCHECK(confirmation_.IsValid());
}

FetchPaymentTokenUrlRequestBuilder::
~FetchPaymentTokenUrlRequestBuilder() = default;

// GET /v1/confirmation/{confirmation_id}/paymentToken

UrlRequestPtr FetchPaymentTokenUrlRequestBuilder::Build() {
  UrlRequestPtr url_request = UrlRequest::New();
  url_request->url = BuildUrl();
  url_request->method = UrlRequestMethod::GET;

  return url_request;
}

///////////////////////////////////////////////////////////////////////////////

std::string FetchPaymentTokenUrlRequestBuilder::BuildUrl() const {
  return base::StringPrintf("%s/v1/confirmation/%s/paymentToken",
      server::GetDomain().c_str(), confirmation_.id.c_str());
}

}  // namespace ads
