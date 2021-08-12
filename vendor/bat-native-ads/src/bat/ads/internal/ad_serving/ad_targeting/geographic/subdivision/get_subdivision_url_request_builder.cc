/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_serving/ad_targeting/geographic/subdivision/get_subdivision_url_request_builder.h"

#include "base/strings/stringprintf.h"
#include "bat/ads/internal/server/ads_serve_server_util.h"

namespace ads {
namespace ad_targeting {
namespace geographic {

GetSubdivisionUrlRequestBuilder::GetSubdivisionUrlRequestBuilder() = default;

GetSubdivisionUrlRequestBuilder::~GetSubdivisionUrlRequestBuilder() = default;

// GET /v1/getstate

mojom::UrlRequestPtr GetSubdivisionUrlRequestBuilder::Build() {
  mojom::UrlRequestPtr url_request = mojom::UrlRequest::New();
  url_request->url = BuildUrl();
  url_request->method = mojom::UrlRequestMethod::kGet;

  return url_request;
}

///////////////////////////////////////////////////////////////////////////////

std::string GetSubdivisionUrlRequestBuilder::BuildUrl() const {
  return base::StringPrintf("%s/v1/getstate", serve::server::GetHost().c_str());
}

}  // namespace geographic
}  // namespace ad_targeting
}  // namespace ads
