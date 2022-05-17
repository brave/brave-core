/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/utility/redeem_unblinded_token/fetch_payment_token_url_request_builder.h"

#include <string>

#include "base/check.h"
#include "base/strings/stringprintf.h"
#include "bat/ads/internal/server/hosts/server_host_util.h"

namespace ads {

FetchPaymentTokenUrlRequestBuilder::FetchPaymentTokenUrlRequestBuilder(
    const ConfirmationInfo& confirmation)
    : confirmation_(confirmation) {
  DCHECK(confirmation_.IsValid());
}

FetchPaymentTokenUrlRequestBuilder::~FetchPaymentTokenUrlRequestBuilder() =
    default;

// GET /v2/confirmation/{confirmation_id}/paymentToken

mojom::UrlRequestPtr FetchPaymentTokenUrlRequestBuilder::Build() {
  mojom::UrlRequestPtr url_request = mojom::UrlRequest::New();
  url_request->url = BuildUrl();
  url_request->method = mojom::UrlRequestMethod::kGet;

  return url_request;
}

///////////////////////////////////////////////////////////////////////////////

GURL FetchPaymentTokenUrlRequestBuilder::BuildUrl() const {
  const std::string spec = base::StringPrintf(
      "%s/v2/confirmation/%s/paymentToken", server::GetAnonymousHost().c_str(),
      confirmation_.id.c_str());
  return GURL(spec);
}

}  // namespace ads
