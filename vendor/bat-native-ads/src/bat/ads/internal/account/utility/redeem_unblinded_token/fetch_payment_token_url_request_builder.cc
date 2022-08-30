/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/utility/redeem_unblinded_token/fetch_payment_token_url_request_builder.h"

#include <string>

#include "base/check.h"
#include "base/strings/stringprintf.h"
#include "bat/ads/internal/account/utility/redeem_unblinded_token/redeem_unblinded_token_util.h"
#include "bat/ads/public/interfaces/ads.mojom.h"
#include "url/gurl.h"

namespace ads {

FetchPaymentTokenUrlRequestBuilder::FetchPaymentTokenUrlRequestBuilder(
    const ConfirmationInfo& confirmation)
    : confirmation_(confirmation) {
  DCHECK(confirmation_.IsValid());
}

FetchPaymentTokenUrlRequestBuilder::~FetchPaymentTokenUrlRequestBuilder() =
    default;

// GET /v2/confirmation/{transactionId}/paymentToken

mojom::UrlRequestInfoPtr FetchPaymentTokenUrlRequestBuilder::Build() {
  mojom::UrlRequestInfoPtr url_request = mojom::UrlRequestInfo::New();
  url_request->url = BuildUrl();
  url_request->method = mojom::UrlRequestMethodType::kGet;

  return url_request;
}

///////////////////////////////////////////////////////////////////////////////

GURL FetchPaymentTokenUrlRequestBuilder::BuildUrl() const {
  const std::string spec =
      base::StringPrintf("%s/v2/confirmation/%s/paymentToken",
                         GetAnonymousHost(confirmation_.ad_type).c_str(),
                         confirmation_.transaction_id.c_str());
  return GURL(spec);
}

}  // namespace ads
