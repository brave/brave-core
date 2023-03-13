/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/issuers/issuers_url_request_builder.h"

#include <string>

#include "base/strings/stringprintf.h"
#include "brave/components/brave_ads/common/interfaces/ads.mojom.h"
#include "brave/components/brave_ads/core/internal/server/url/hosts/server_host_util.h"
#include "url/gurl.h"

namespace brave_ads {

namespace {

GURL BuildUrl() {
  const std::string spec =
      base::StringPrintf("%s/v3/issuers/", server::GetStaticHost().c_str());
  return GURL(spec);
}

}  // namespace

// GET /v3/issuers/

mojom::UrlRequestInfoPtr IssuersUrlRequestBuilder::Build() {
  mojom::UrlRequestInfoPtr url_request = mojom::UrlRequestInfo::New();
  url_request->url = BuildUrl();
  url_request->method = mojom::UrlRequestMethodType::kGet;

  return url_request;
}

}  // namespace brave_ads
