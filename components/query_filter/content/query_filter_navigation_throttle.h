// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_QUERY_FILTER_CONTENT_QUERY_FILTER_NAVIGATION_THROTTLE_H_
#define BRAVE_COMPONENTS_QUERY_FILTER_CONTENT_QUERY_FILTER_NAVIGATION_THROTTLE_H_

#include "base/functional/callback.h"
#include "base/memory/raw_ptr.h"
#include "content/public/browser/navigation_throttle.h"
#include "content/public/browser/navigation_throttle_registry.h"
#include "url/gurl.h"

namespace brave_shields {
class BraveShieldsSettingsService;
}  // namespace brave_shields

namespace query_filter {

// Strips tracking query parameters from a navigation before it can be
// satisfied without reaching the network stack. The network delegate only
// sees requests that hit the network, so a service worker can intercept the
// request and serve a cached response that still contains the original query
// string. See https://github.com/brave/brave-browser/issues/23742.
class QueryFilterNavigationThrottle : public content::NavigationThrottle {
 public:
  explicit QueryFilterNavigationThrottle(
      content::NavigationThrottleRegistry& registry,
      brave_shields::BraveShieldsSettingsService* shields_settings_service);
  ~QueryFilterNavigationThrottle() override;

  QueryFilterNavigationThrottle(const QueryFilterNavigationThrottle&) = delete;
  QueryFilterNavigationThrottle& operator=(
      const QueryFilterNavigationThrottle&) = delete;

  static void MaybeCreateAndAdd(
      content::NavigationThrottleRegistry& registry,
      brave_shields::BraveShieldsSettingsService* shields_settings_service);

  // `content::NavigationThrottle`:
  // Observe the first navigation and strip out any flagged parameters. If
  // stripped creates a "fresh" new internal redirect navigation.
  ThrottleCheckResult WillStartRequest() override;
  const char* GetNameForLogging() override;

  // When set, a filtered navigation invokes this with the stripped URL instead
  // of restarting the navigation. Tests use it to observe the redirect.
  void SetRedirectCallbackForTesting(
      base::RepeatingCallback<void(const GURL&)> callback);

 private:
  raw_ptr<brave_shields::BraveShieldsSettingsService>
      shields_settings_service_ = nullptr;
  base::RepeatingCallback<void(const GURL&)> redirect_callback_for_testing_;
};
}  // namespace query_filter
#endif  // BRAVE_COMPONENTS_QUERY_FILTER_CONTENT_QUERY_FILTER_NAVIGATION_THROTTLE_H_
