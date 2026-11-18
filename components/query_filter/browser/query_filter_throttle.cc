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
#include "brave/components/query_filter/common/features.h"
#include "net/base/isolation_info.h"
#include "services/network/public/cpp/resource_request.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace query_filter {

namespace {

// Mirrors how BraveRequestInfo::MakeCTX picks the Shields origin for the
// network delegate: top-frame origin first, then the request initiator.
// TODO(https://github.com/brave/brave-browser/issues/23742): Make this shared
// with BraveRequestInfo::MakeCTX.
GURL GetShieldsSettingsURL(const network::ResourceRequest& request) {
  // TODO(https://github.com/brave/brave-browser/issues/23742): Add logic to
  // handle cases where the request is initiated inside a subframe.
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
  // TODO(https://github.com/brave/brave-browser/issues/23742): Strip tracking
  // query parameters from GetShieldsSettingsURL(`request->url`).
}

void QueryFilterThrottle::WillRedirectRequest(
    net::RedirectInfo* redirect_info,
    const network::mojom::URLResponseHead& /*response_head*/,
    bool* /*defer*/,
    network::HttpRequestHeadersUpdateParams* /*headers_update_params*/) {
  // TODO(https://github.com/brave/brave-browser/issues/23742): Redirect needs
  // more invetigation to be sure it doesn't affect debouncing.
}

}  // namespace query_filter
