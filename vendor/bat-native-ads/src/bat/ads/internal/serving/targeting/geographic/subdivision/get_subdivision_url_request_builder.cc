/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/serving/targeting/geographic/subdivision/get_subdivision_url_request_builder.h"

#include <string>

#include "base/strings/stringprintf.h"
#include "bat/ads/internal/server/hosts/server_host_util.h"

namespace ads {
namespace targeting {
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

GURL GetSubdivisionUrlRequestBuilder::BuildUrl() const {
  const std::string spec =
      base::StringPrintf("%s/v1/getstate", server::GetGeoHost().c_str());
  return GURL(spec);
}

}  // namespace geographic
}  // namespace targeting
}  // namespace ads
