/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/net/brave_system_request_handler.h"

#include "brave/browser/net/brave_block_safebrowsing_urls.h"
#include "brave/browser/net/brave_common_static_redirect_network_delegate_helper.h"
#include "brave/browser/net/brave_static_redirect_network_delegate_helper.h"
#include "brave/common/network_constants.h"
#include "extensions/common/url_pattern.h"
#include "services/network/public/cpp/resource_request.h"
#include "url/gurl.h"

#if !defined(BRAVE_SERVICES_KEY)
// Refer to the keyed API spec for more details about the Brave Services Key
#define BRAVE_SERVICES_KEY "dummytoken"
#endif

const char kBraveServicesKeyHeader[] = "BraveServiceKey";

namespace brave {

void AddBraveServicesKeyHeader(network::ResourceRequest* url_request) {
  static URLPattern proxy_pattern(URLPattern::SCHEME_HTTPS,
                                  kBraveProxyPattern);
  if (proxy_pattern.MatchesURL(url_request->url)) {
      url_request->headers.SetHeaderIfMissing(kBraveServicesKeyHeader,
                                              BRAVE_SERVICES_KEY);
  }
  return;
}

network::ResourceRequest OnBeforeSystemRequest(
    const network::ResourceRequest& url_request) {
  GURL new_url;
  brave::OnBeforeURLRequest_BlockSafeBrowsingReportingURLs(url_request.url,
                                                           &new_url);
  brave::OnBeforeURLRequest_StaticRedirectWorkForGURL(url_request.url,
                                                      &new_url);
  brave::OnBeforeURLRequest_CommonStaticRedirectWorkForGURL(url_request.url,
                                                            &new_url);
  network::ResourceRequest patched_request = url_request;
  if (!new_url.is_empty()) {
    patched_request.url = new_url;
  }
  AddBraveServicesKeyHeader(&patched_request);
  return patched_request;
}

}  // namespace brave
