/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/server/ad_rewards/ad_grants/ad_grants_url_request_builder.h"

#include "base/strings/stringprintf.h"
#include "bat/ads/internal/logging.h"
#include "bat/ads/internal/server/rewards_server_util.h"

namespace ads {

AdGrantsUrlRequestBuilder::AdGrantsUrlRequestBuilder(
    const WalletInfo& wallet)
    : wallet_(wallet) {
  DCHECK(wallet_.IsValid());
}

AdGrantsUrlRequestBuilder::~AdGrantsUrlRequestBuilder() = default;

// GET /v1/promotions/ads/grants/summary?paymentId={payment_id}

UrlRequestPtr AdGrantsUrlRequestBuilder::Build() {
  UrlRequestPtr url_request = UrlRequest::New();
  url_request->url = BuildUrl();
  url_request->method = UrlRequestMethod::GET;

  return url_request;
}

///////////////////////////////////////////////////////////////////////////////

std::string AdGrantsUrlRequestBuilder::BuildUrl() const {
  return base::StringPrintf("%s/v1/promotions/ads/grants/summary?paymentId=%s",
      rewards::server::GetDomain().c_str(), wallet_.payment_id.c_str());
}

}  // namespace ads
