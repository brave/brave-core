/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_server/get_catalog_url_request_builder.h"

#include "base/strings/stringprintf.h"
#include "bat/ads/internal/catalog/catalog.h"
#include "bat/ads/internal/catalog/catalog_version.h"
#include "bat/ads/internal/server/ads_server_util.h"

namespace ads {

GetCatalogUrlRequestBuilder::GetCatalogUrlRequestBuilder() = default;

GetCatalogUrlRequestBuilder::~GetCatalogUrlRequestBuilder() = default;

// GET /v#/catalog

mojom::UrlRequestPtr GetCatalogUrlRequestBuilder::Build() {
  mojom::UrlRequestPtr url_request = mojom::UrlRequest::New();
  url_request->url = BuildUrl();
  url_request->method = mojom::UrlRequestMethod::kGet;

  return url_request;
}

///////////////////////////////////////////////////////////////////////////////

std::string GetCatalogUrlRequestBuilder::BuildUrl() const {
  return base::StringPrintf("%s/v%d/catalog", server::GetHost().c_str(),
         kCurrentCatalogVersion);
}

}  // namespace ads
