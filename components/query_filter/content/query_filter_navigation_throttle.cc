// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/query_filter/content/query_filter_navigation_throttle.h"

#include <memory>
#include <optional>
#include <string>
#include <utility>

#include "brave/components/brave_shields/content/browser/restart_navigation.h"
#include "brave/components/brave_shields/core/browser/brave_shields_settings_service.h"
#include "brave/components/query_filter/browser/utils.h"
#include "brave/components/query_filter/common/features.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/page_transition_types.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace {

bool ShouldInterceptNavigation(
    content::BrowserContext* browser_context,
    brave_shields::BraveShieldsSettingsService* shields_settings_service,
    const std::optional<GURL>& url) {
  if (!browser_context) {
    return false;
  }
  // We ignore interception for the following casees:
  // 1. Feature flag is disabled.
  if (!base::FeatureList::IsEnabled(
          query_filter::features::kQueryFilterComponent)) {
    return false;
  }

  // 2. Interception disabled by policy.
  if (query_filter::IsQueryFilteringDisabledByPolicy(
          user_prefs::UserPrefs::Get(browser_context))) {
    return false;
  }

  // 3. Brave shields is disabled.
  if (shields_settings_service && url.has_value() &&
      !shields_settings_service->IsBraveShieldsEnabled(*url)) {
    return false;
  }

  return true;
}

}  // namespace

namespace query_filter {
QueryFilterNavigationThrottle::QueryFilterNavigationThrottle(
    content::NavigationThrottleRegistry& registry,
    brave_shields::BraveShieldsSettingsService* shields_settings_service)
    : content::NavigationThrottle(registry),
      shields_settings_service_(shields_settings_service) {}

QueryFilterNavigationThrottle::~QueryFilterNavigationThrottle() = default;

// static
void QueryFilterNavigationThrottle::MaybeCreateAndAdd(
    content::NavigationThrottleRegistry& registry,
    brave_shields::BraveShieldsSettingsService* shields_settings_service) {
  if (!ShouldInterceptNavigation(
          registry.GetNavigationHandle().GetWebContents()->GetBrowserContext(),
          shields_settings_service, /*url=*/std::nullopt)) {
    return;
  }

  registry.AddThrottle(std::make_unique<QueryFilterNavigationThrottle>(
      registry, shields_settings_service));
}

content::NavigationThrottle::ThrottleCheckResult
QueryFilterNavigationThrottle::WillStartRequest() {
  if (!navigation_handle() || !navigation_handle()->GetWebContents()) {
    return NavigationThrottle::PROCEED;
  }

  // Check if interception is allowed.
  const GURL request_url = navigation_handle()->GetURL();
  if (!ShouldInterceptNavigation(
          navigation_handle()->GetWebContents()->GetBrowserContext(),
          shields_settings_service_, request_url)) {
    return NavigationThrottle::PROCEED;
  }

  // Perform query filtering.
  const std::optional<url::Origin> request_initiator =
      navigation_handle()->GetInitiatorOrigin();
  const std::string request_method = navigation_handle()->GetRequestMethod();
  const std::optional<GURL> filtered_url = MaybeApplyQueryStringFilter(
      request_initiator, GURL(), request_url, request_method, false);
  if (!filtered_url.has_value()) {
    return NavigationThrottle::PROCEED;
  }

  if (redirect_callback_for_testing_) {
    redirect_callback_for_testing_.Run(*filtered_url);  // IN-TEST
    return NavigationThrottle::CANCEL;
  }

  // Redirect internally with the |new_url|.
  if (brave_shields::RestartNavigationWithURL(
          *navigation_handle(), *filtered_url,
          ui::PAGE_TRANSITION_CLIENT_REDIRECT)) {
    return NavigationThrottle::CANCEL;
  }

  return NavigationThrottle::PROCEED;
}

const char* QueryFilterNavigationThrottle::GetNameForLogging() {
  return "QueryFilterNavigationThrottle";
}

void QueryFilterNavigationThrottle::SetRedirectCallbackForTesting(  // IN-TEST
    base::RepeatingCallback<void(const GURL&)> callback) {
  redirect_callback_for_testing_ = std::move(callback);
}
}  // namespace query_filter
