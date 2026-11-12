// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/query_filter/content/query_filter_throttle.h"

#include <memory>
#include <optional>

#include "base/memory/ptr_util.h"
#include "brave/components/brave_shields/core/browser/brave_shields_settings_service.h"
#include "brave/components/query_filter/browser/utils.h"
#include "brave/components/query_filter/common/features.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"
#include "services/network/public/cpp/resource_request.h"
#include "url/gurl.h"

namespace {

bool IsQueryFilteringEnabled(content::BrowserContext* browser_context) {
  if (!browser_context) {
    return false;
  }
  if (!base::FeatureList::IsEnabled(
          query_filter::features::kQueryFilterComponent)) {
    return false;
  }
  return !query_filter::IsQueryFilteringDisabledByPolicy(
      user_prefs::UserPrefs::Get(browser_context));
}

}  // namespace

namespace query_filter {

QueryFilterThrottle::QueryFilterThrottle(
    brave_shields::BraveShieldsSettingsService* shields_settings_service)
    : shields_settings_service_(shields_settings_service) {}

QueryFilterThrottle::~QueryFilterThrottle() = default;

// static
std::unique_ptr<blink::URLLoaderThrottle>
QueryFilterThrottle::MaybeCreateThrottle(
    content::BrowserContext* browser_context,
    brave_shields::BraveShieldsSettingsService* shields_settings_service) {
  if (!IsQueryFilteringEnabled(browser_context)) {
    return nullptr;
  }
  return base::WrapUnique(new QueryFilterThrottle(shields_settings_service));
}

void QueryFilterThrottle::WillStartRequest(network::ResourceRequest* request,
                                           bool* /*defer*/) {
  if (shields_settings_service_ &&
      !shields_settings_service_->IsBraveShieldsEnabled(request->url)) {
    return;
  }
  const std::optional<GURL> filtered_url = MaybeApplyQueryStringFilter(
      request->request_initiator, GURL(), request->url, request->method,
      /*internal_redirect=*/false);
  if (!filtered_url.has_value()) {
    return;
  }
  request->url = *filtered_url;
}

}  // namespace query_filter
