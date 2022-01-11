/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/tokens/issuers/issuers_url_request_builder.h"

#include "base/strings/stringprintf.h"
#include "bat/ads/internal/server/confirmations_server_util.h"

namespace ads {

IssuersUrlRequestBuilder::IssuersUrlRequestBuilder() = default;

IssuersUrlRequestBuilder::~IssuersUrlRequestBuilder() = default;

// GET /v1/issuers/

mojom::UrlRequestPtr IssuersUrlRequestBuilder::Build() {
  mojom::UrlRequestPtr url_request = mojom::UrlRequest::New();
  url_request->url = BuildUrl();
  url_request->method = mojom::UrlRequestMethod::kGet;

  return url_request;
}

///////////////////////////////////////////////////////////////////////////////

std::string IssuersUrlRequestBuilder::BuildUrl() const {
  return base::StringPrintf("%s/v1/issuers/",
                            confirmations::server::GetHost().c_str());
}

}  // namespace ads
