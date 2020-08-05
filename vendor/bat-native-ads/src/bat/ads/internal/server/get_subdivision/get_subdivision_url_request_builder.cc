/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/server/get_subdivision/get_subdivision_url_request_builder.h"

#include "base/strings/stringprintf.h"
#include "bat/ads/internal/server/ads_server_util.h"

namespace ads {

GetSubdivisionUrlRequestBuilder::GetSubdivisionUrlRequestBuilder() = default;

GetSubdivisionUrlRequestBuilder::~GetSubdivisionUrlRequestBuilder() = default;

// GET /v5/getstate

UrlRequestPtr GetSubdivisionUrlRequestBuilder::Build() {
  UrlRequestPtr url_request = UrlRequest::New();
  url_request->url = BuildUrl();
  url_request->method = UrlRequestMethod::GET;

  return url_request;
}

///////////////////////////////////////////////////////////////////////////////

std::string GetSubdivisionUrlRequestBuilder::BuildUrl() const {
  return base::StringPrintf("%s/v5/getstate", server::GetDomain().c_str());
}

}  // namespace ads
