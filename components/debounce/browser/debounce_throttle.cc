/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/debounce/browser/debounce_throttle.h"

#include <memory>
#include <string>
#include <vector>

#include "brave/components/brave_shields/browser/brave_shields_util.h"
#include "brave/components/debounce/browser/debounce_service.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "services/network/public/cpp/request_mode.h"
#include "services/network/public/cpp/resource_request.h"
#include "url/gurl.h"

namespace debounce {

// static
std::unique_ptr<DebounceThrottle> DebounceThrottle::MaybeCreateThrottleFor(
    DebounceService* debounce_service,
    HostContentSettingsMap* host_content_settings_map) {
  // If debouncing is disabled in brave://flags, debounce service will
  // never be created (will be null) so we won't create the throttle
  // either. Caller must nullcheck this.
  if (!debounce_service)
    return nullptr;
  return std::make_unique<DebounceThrottle>(debounce_service,
                                            host_content_settings_map);
}

DebounceThrottle::DebounceThrottle(
    DebounceService* debounce_service,
    HostContentSettingsMap* host_content_settings_map)
    : debounce_service_(debounce_service),
      host_content_settings_map_(host_content_settings_map) {
  DCHECK(debounce_service_);
  DCHECK(host_content_settings_map_);
}

DebounceThrottle::~DebounceThrottle() = default;

void DebounceThrottle::WillStartRequest(network::ResourceRequest* request,
                                        bool* defer) {
  // Never debounce opaque URLs (e.g. third-party iframes)
  if (request->site_for_cookies.site().opaque())
    return;

  VLOG(1) << request->site_for_cookies.ToDebugString();
  GURL debounced_url;
  // Call debounce service to try to debounce this URL based on available rules.
  // Returns false if no rules apply.
  if (!brave_shields::ShouldDoDebouncing(host_content_settings_map_,
                                         request->url) ||
      !debounce_service_->Debounce(request->url, &debounced_url))
    return;

  VLOG(1) << "Debouncing rule applied: " << request->url << " -> "
          << debounced_url;
  url::Origin original_origin = url::Origin::Create(request->url);
  url::Origin debounced_origin = url::Origin::Create(debounced_url);
  request->url = debounced_url;

  // If we're debouncing to a different site, we need to reinitialize
  // the trusted params for the new origin and restart the request.
  if (!original_origin.IsSameOriginWith(debounced_origin)) {
    request->site_for_cookies =
        net::SiteForCookies::FromOrigin(debounced_origin);
    request->request_initiator = debounced_origin;
    request->trusted_params = network::ResourceRequest::TrustedParams();
    request->trusted_params->isolation_info = net::IsolationInfo::Create(
        net::IsolationInfo::RequestType::kOther, debounced_origin,
        debounced_origin, net::SiteForCookies::FromOrigin(debounced_origin));
    VLOG(1) << request->trusted_params->isolation_info.site_for_cookies()
                   .ToDebugString();
  }
  delegate_->RestartWithFlags(/* additional_load_flags */ 0);
}

void DebounceThrottle::WillRedirectRequest(
    net::RedirectInfo* redirect_info,
    const network::mojom::URLResponseHead& response_head,
    bool* defer,
    std::vector<std::string>* to_be_removed_request_headers,
    net::HttpRequestHeaders* modified_request_headers,
    net::HttpRequestHeaders* modified_cors_exempt_request_headers) {
  GURL debounced_url;
  if (!brave_shields::ShouldDoDebouncing(host_content_settings_map_,
                                         redirect_info->new_url) ||
      !debounce_service_->Debounce(redirect_info->new_url, &debounced_url))
    return;

  // Debouncing on redirect is actually easier than debouncing at the start
  // of a request because our callback is called before the caller has set
  // up the isolation info for the new URL, so all we have to do is modify
  // |redirect_info| to point to the debounced URL instead of the one we
  // were originally going to redirect to.
  VLOG(1) << "Debouncing rule applied: " << redirect_info->new_url << " -> "
          << debounced_url;
  redirect_info->new_url = debounced_url;
}

}  // namespace debounce
