/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/geographic/subdivision_targeting/get_subdivision_url_request_builder.h"

#include <string>

#include "base/strings/strcat.h"
#include "brave/components/brave_ads/common/interfaces/brave_ads.mojom.h"
#include "brave/components/brave_ads/core/internal/common/url/request_builder/host/url_host_util.h"
#include "brave/components/brave_ads/core/internal/geographic/subdivision_targeting/get_subdivision_url_request_builder_util.h"
#include "url/gurl.h"

namespace brave_ads {

namespace {

GURL BuildUrl() {
  const std::string spec =
      base::StrCat({GetGeoUrlHost(), BuildSubdivisionTargetingUrlPath()});
  return GURL(spec);
}

}  // namespace

mojom::UrlRequestInfoPtr GetSubdivisionUrlRequestBuilder::Build() {
  mojom::UrlRequestInfoPtr url_request = mojom::UrlRequestInfo::New();
  url_request->url = BuildUrl();
  url_request->method = mojom::UrlRequestMethodType::kGet;

  return url_request;
}

}  // namespace brave_ads
