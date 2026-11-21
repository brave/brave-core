// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/query_filter/browser/query_filter_throttle.h"

#include <memory>
#include <optional>

#include "base/feature_list.h"
#include "base/memory/ptr_util.h"
#include "brave/components/brave_shields/core/browser/brave_shields_settings_service.h"
#include "brave/components/query_filter/browser/utils.h"
#include "brave/components/query_filter/common/features.h"
#include "net/base/isolation_info.h"
#include "services/network/public/cpp/resource_request.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace query_filter {

namespace {

// Mirrors how BraveRequestInfo::MakeCTX picks the Shields origin for the
// network delegate. Mainly ctx->set_allow_brave_shields call.
// TODO(https://github.com/brave/brave-browser/issues/23742): Make this shared
// with BraveRequestInfo::MakeCTX.
GURL GetShieldsSettingsURL(const network::ResourceRequest& request) {
  if (request.is_outermost_main_frame) {
    return request.url;
  }
  if (request.trusted_params) {
    const std::optional<url::Origin>& top_frame_origin =
        request.trusted_params->isolation_info.top_frame_origin();
    if (top_frame_origin && !top_frame_origin->opaque()) {
      return top_frame_origin->GetURL();
    }
  }
  if (request.request_initiator && !request.request_initiator->opaque()) {
    return request.request_initiator->GetURL();
  }
  return request.url;
}

}  // namespace

QueryFilterThrottle::QueryFilterThrottle() = default;

QueryFilterThrottle::~QueryFilterThrottle() = default;

// static
std::unique_ptr<blink::URLLoaderThrottle>
QueryFilterThrottle::MaybeCreateThrottle(
    brave_shields::BraveShieldsSettingsService* shields_settings_service,
    const network::ResourceRequest& request) {
  // Kill-switch in-case throttling creates any issues.
  if (!base::FeatureList::IsEnabled(
          query_filter::features::kQueryFilterBrowserThrottleEnabled)) {
    return nullptr;
  }

  if (!shields_settings_service ||
      !shields_settings_service->IsQueryFilteringEnabled(
          GetShieldsSettingsURL(request))) {
    return nullptr;
  }

  return base::WrapUnique(new QueryFilterThrottle());
}

void QueryFilterThrottle::WillStartRequest(network::ResourceRequest* request,
                                           bool* /*defer*/) {
  const std::optional<GURL> filtered_url = MaybeApplyQueryStringFilter(
      request->request_initiator, GURL(), request->url, request->method,
      /*internal_redirect=*/false);
  if (filtered_url.has_value()) {
    request->url = *filtered_url;
  }
}

void QueryFilterThrottle::WillRedirectRequest(
    net::RedirectInfo* redirect_info,
    const network::mojom::URLResponseHead& /*response_head*/,
    bool* /*defer*/,
    network::HttpRequestHeadersUpdateParams* /*headers_update_params*/) {}

}  // namespace query_filter
