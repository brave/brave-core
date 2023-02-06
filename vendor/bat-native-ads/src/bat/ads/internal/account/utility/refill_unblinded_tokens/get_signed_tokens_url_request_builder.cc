/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/utility/refill_unblinded_tokens/get_signed_tokens_url_request_builder.h"

#include <utility>

#include "base/check.h"
#include "base/strings/stringprintf.h"
#include "bat/ads/internal/server/url/hosts/server_host_util.h"
#include "bat/ads/public/interfaces/ads.mojom.h"
#include "url/gurl.h"

namespace ads {

GetSignedTokensUrlRequestBuilder::GetSignedTokensUrlRequestBuilder(
    WalletInfo wallet,
    std::string nonce)
    : wallet_(std::move(wallet)), nonce_(std::move(nonce)) {
  DCHECK(wallet_.IsValid());
  DCHECK(!nonce_.empty());
}

// GET /v3/confirmation/token/{paymentId}?nonce={nonce}

mojom::UrlRequestInfoPtr GetSignedTokensUrlRequestBuilder::Build() {
  mojom::UrlRequestInfoPtr url_request = mojom::UrlRequestInfo::New();
  url_request->url = BuildUrl();
  url_request->method = mojom::UrlRequestMethodType::kGet;

  return url_request;
}

///////////////////////////////////////////////////////////////////////////////

GURL GetSignedTokensUrlRequestBuilder::BuildUrl() const {
  const std::string spec =
      base::StringPrintf("%s/v3/confirmation/token/%s?nonce=%s",
                         server::GetNonAnonymousHost().c_str(),
                         wallet_.payment_id.c_str(), nonce_.c_str());
  return GURL(spec);
}

}  // namespace ads
