/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/confirmations/refill_unblinded_tokens/get_signed_tokens_url_request_builder.h"

#include <utility>

#include "base/check.h"
#include "base/strings/strcat.h"
#include "brave/components/brave_ads/common/interfaces/brave_ads.mojom.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/refill_unblinded_tokens/get_signed_tokens_url_request_builder_util.h"
#include "brave/components/brave_ads/core/internal/common/url/request_builder/host/url_host_util.h"
#include "url/gurl.h"

namespace brave_ads {

GetSignedTokensUrlRequestBuilder::GetSignedTokensUrlRequestBuilder(
    WalletInfo wallet,
    std::string nonce)
    : wallet_(std::move(wallet)), nonce_(std::move(nonce)) {
  CHECK(wallet_.IsValid());
  CHECK(!nonce_.empty());
}

mojom::UrlRequestInfoPtr GetSignedTokensUrlRequestBuilder::Build() {
  mojom::UrlRequestInfoPtr url_request = mojom::UrlRequestInfo::New();
  url_request->url = BuildUrl();
  url_request->method = mojom::UrlRequestMethodType::kGet;

  return url_request;
}

///////////////////////////////////////////////////////////////////////////////

GURL GetSignedTokensUrlRequestBuilder::BuildUrl() const {
  const std::string spec =
      base::StrCat({GetNonAnonymousUrlHost(),
                    BuildGetSignedTokensUrlPath(wallet_.payment_id, nonce_)});
  return GURL(spec);
}

}  // namespace brave_ads
