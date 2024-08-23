/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/net/brave_system_request_handler.h"

#include "brave/browser/net/brave_block_safebrowsing_urls.h"
#include "brave/browser/net/brave_common_static_redirect_network_delegate_helper.h"
#include "brave/browser/net/brave_speech_to_text_network_delegate_helper.h"
#include "brave/browser/net/brave_static_redirect_network_delegate_helper.h"
#include "brave/components/constants/brave_services_key.h"
#include "brave/components/constants/brave_services_key_helper.h"
#include "brave/components/constants/network_constants.h"
#include "extensions/common/url_pattern.h"
#include "services/network/public/cpp/resource_request.h"
#include "url/gurl.h"

namespace brave {

std::string BraveServicesKeyForTesting() {
  return BUILDFLAG(BRAVE_SERVICES_KEY);
}

void AddBraveServicesKeyHeader(network::ResourceRequest* url_request) {
  if (brave::ShouldAddBraveServicesKeyHeader(url_request->url)) {
    url_request->headers.SetHeaderIfMissing(kBraveServicesKeyHeader,
                                            BUILDFLAG(BRAVE_SERVICES_KEY));
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
  stt::OnBeforeURLRequest_SpoofSpeechToText(&patched_request);
  AddBraveServicesKeyHeader(&patched_request);
  return patched_request;
}

}  // namespace brave
