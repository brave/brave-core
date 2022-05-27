/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/net/brave_service_key_network_delegate_helper.h"

#include <string>
#include <vector>

#include "base/no_destructor.h"
#include "brave/components/constants/brave_services_key.h"
#include "brave/components/constants/network_constants.h"
#include "brave/components/update_client/buildflags.h"
#include "net/base/net_errors.h"
#include "net/http/http_request_headers.h"
#include "url/gurl.h"

namespace brave {

int OnBeforeStartTransaction_BraveServiceKey(
    net::HttpRequestHeaders* headers,
    const ResponseCallback& next_callback,
    std::shared_ptr<BraveRequestInfo> ctx) {
  static const base::NoDestructor<std::vector<std::string>> allowed_domains{
      {kExtensionUpdaterDomain, GURL(BUILDFLAG(UPDATER_DEV_ENDPOINT)).host(),
       GURL(BUILDFLAG(UPDATER_PROD_ENDPOINT)).host()}};

  const GURL& url = ctx->request_url;

  if (url.SchemeIs(url::kHttpsScheme)) {
    if (std::any_of(
            allowed_domains->begin(), allowed_domains->end(),
            [&url](const auto& domain) { return url.DomainIs(domain); })) {
      headers->SetHeader(kBraveServicesKeyHeader,
                         BUILDFLAG(BRAVE_SERVICES_KEY));
    }
  }
  return net::OK;
}

}  // namespace brave
