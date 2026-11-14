// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/query_filter/content/query_filter_throttle.h"

#include <memory>
#include <optional>

#include "base/feature_list.h"
#include "base/memory/ptr_util.h"
#include "brave/components/brave_shields/core/browser/brave_shields_settings_service.h"
#include "brave/components/query_filter/browser/utils.h"
#include "brave/components/query_filter/common/features.h"
#include "brave/components/query_filter/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "services/network/public/cpp/resource_request.h"
#include "url/gurl.h"

namespace query_filter {

QueryFilterThrottle::QueryFilterThrottle() = default;

QueryFilterThrottle::~QueryFilterThrottle() = default;

// static
std::unique_ptr<blink::URLLoaderThrottle>
QueryFilterThrottle::MaybeCreateThrottle(
    const PrefService* prefs,
    brave_shields::BraveShieldsSettingsService* shields_settings_service,
    const GURL& url) {
  if (!shields_settings_service) {
    return nullptr;
  }
  if (!base::FeatureList::IsEnabled(features::kQueryFilterComponent)) {
    return nullptr;
  }
  if (!prefs->GetBoolean(kTrackingQueryParametersFilteringEnabled)) {
    return nullptr;
  }
  if (!shields_settings_service->IsQueryFilteringEnabled(url)) {
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

}  // namespace query_filter
