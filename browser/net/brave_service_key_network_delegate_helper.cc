/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/net/brave_service_key_network_delegate_helper.h"

#include <memory>
#include <string>
#include <vector>

#include "brave/common/network_constants.h"
#include "net/base/net_errors.h"
#include "net/http/http_request_headers.h"
#include "url/gurl.h"

namespace brave {

int OnBeforeStartTransaction_BraveServiceKey(
    net::HttpRequestHeaders* headers,
    const ResponseCallback& next_callback,
    std::shared_ptr<BraveRequestInfo> ctx) {
  const std::vector<std::string> whitelisted_domains = {
      kExtensionUpdaterDomain, GURL(UPDATER_DEV_ENDPOINT).host(),
      GURL(UPDATER_PROD_ENDPOINT).host()};

  const GURL url = ctx->request_url;

  if (url.SchemeIs(url::kHttpsScheme)) {
    if (std::any_of(
            whitelisted_domains.begin(), whitelisted_domains.end(),
            [&url](std::string domain) { return url.DomainIs(domain); })) {
      headers->SetHeader(kBraveServicesKeyHeader, BRAVE_SERVICES_KEY);
    }
  }
  return net::OK;
}

}  // namespace brave
