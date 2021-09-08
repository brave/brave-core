/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/tokens/refill_unblinded_tokens/get_signed_tokens_url_request_builder.h"

#include "base/check.h"
#include "base/strings/stringprintf.h"
#include "bat/ads/internal/server/confirmations_server_util.h"

namespace ads {

GetSignedTokensUrlRequestBuilder::GetSignedTokensUrlRequestBuilder(
    const WalletInfo& wallet,
    const std::string& nonce)
    : wallet_(wallet), nonce_(nonce) {
  DCHECK(wallet_.IsValid());
  DCHECK(!nonce_.empty());
}

GetSignedTokensUrlRequestBuilder::~GetSignedTokensUrlRequestBuilder() = default;

// GET /v1/confirmation/token/{payment_id}?nonce={nonce}

mojom::UrlRequestPtr GetSignedTokensUrlRequestBuilder::Build() {
  mojom::UrlRequestPtr url_request = mojom::UrlRequest::New();
  url_request->url = BuildUrl();
  url_request->method = mojom::UrlRequestMethod::kGet;

  return url_request;
}

///////////////////////////////////////////////////////////////////////////////

std::string GetSignedTokensUrlRequestBuilder::BuildUrl() const {
  return base::StringPrintf("%s/v1/confirmation/token/%s?nonce=%s",
                            confirmations::server::GetHost().c_str(),
                            wallet_.id.c_str(), nonce_.c_str());
}

}  // namespace ads
