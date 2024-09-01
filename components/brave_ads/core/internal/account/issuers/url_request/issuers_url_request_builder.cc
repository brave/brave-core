/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/issuers/url_request/issuers_url_request_builder.h"

#include <string>

#include "base/strings/strcat.h"
#include "brave/components/brave_ads/core/internal/account/issuers/url_request/issuers_url_request_builder_util.h"
#include "brave/components/brave_ads/core/internal/common/url/request_builder/host/url_host_util.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "url/gurl.h"

namespace brave_ads {

namespace {

GURL BuildUrl() {
  const std::string spec =
      base::StrCat({GetStaticUrlHost(), BuildIssuersUrlPath()});
  return GURL(spec);
}

}  // namespace

mojom::UrlRequestInfoPtr IssuersUrlRequestBuilder::Build() {
  mojom::UrlRequestInfoPtr mojom_url_request = mojom::UrlRequestInfo::New();
  mojom_url_request->url = BuildUrl();
  mojom_url_request->method = mojom::UrlRequestMethodType::kGet;

  return mojom_url_request;
}

}  // namespace brave_ads
