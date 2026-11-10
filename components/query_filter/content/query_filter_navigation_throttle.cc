// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/query_filter/content/query_filter_navigation_throttle.h"

#include <memory>
#include <optional>
#include <string>

#include "brave/components/brave_shields/content/browser/restart_navigation.h"
#include "brave/components/query_filter/browser/utils.h"
#include "brave/components/query_filter/common/features.h"
#include "brave/components/query_filter/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/page_transition_types.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace query_filter {
QueryFilterNavigationThrottle::QueryFilterNavigationThrottle(
    content::NavigationThrottleRegistry& registry)
    : content::NavigationThrottle(registry) {}

QueryFilterNavigationThrottle::~QueryFilterNavigationThrottle() = default;

// static
void QueryFilterNavigationThrottle::MaybeCreateAndAdd(
    content::NavigationThrottleRegistry& registry) {
  if (!base::FeatureList::IsEnabled(
          query_filter::features::kQueryFilterComponent)) {
    return;
  }

  // Default to enabled unless controlled by policy.
  PrefService* prefs = user_prefs::UserPrefs::Get(
      registry.GetNavigationHandle().GetWebContents()->GetBrowserContext());
  if (prefs->IsManagedPreference(kTrackingQueryParametersFilteringEnabled) &&
      !prefs->GetBoolean(kTrackingQueryParametersFilteringEnabled)) {
    return;
  }

  registry.AddThrottle(
      std::make_unique<QueryFilterNavigationThrottle>(registry));
}

content::NavigationThrottle::ThrottleCheckResult
QueryFilterNavigationThrottle::WillStartRequest() {
  const std::optional<url::Origin> request_initiator =
      navigation_handle()->GetInitiatorOrigin();
  const GURL request_url = navigation_handle()->GetURL();
  const std::string request_method = navigation_handle()->GetRequestMethod();
  const std::optional<GURL> new_url = MaybeApplyQueryStringFilter(
      request_initiator, GURL(), request_url, request_method, false);
  if (!new_url.has_value()) {
    return NavigationThrottle::PROCEED;
  }

  if (!navigation_handle()->GetWebContents()) {
    return NavigationThrottle::PROCEED;
  }

  // Below we will initiate a new navigation with |new_url| and cancel the
  // previous one as internal redirect.
  brave_shields::RestartNavigationWithURL(*navigation_handle(), *new_url,
                                          ui::PAGE_TRANSITION_CLIENT_REDIRECT);
  return NavigationThrottle::CANCEL;
}

const char* QueryFilterNavigationThrottle::GetNameForLogging() {
  return "QueryFilterNavigationThrottle";
}
}  // namespace query_filter
