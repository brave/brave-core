// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/traffic_control/traffic_control_navigation_throttle.h"

#include <vector>

#include "base/functional/bind.h"
#include "base/task/sequenced_task_runner.h"
#include "base/types/optional_ref.h"
#include "brave/browser/traffic_control/traffic_control_apply.h"
#include "brave/browser/traffic_control/traffic_control_tab_utils.h"
#include "brave/components/traffic_control/core/browser/traffic_control_service.h"
#include "brave/components/traffic_control/core/mojom/traffic_control.mojom.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/navigation_throttle_registry.h"
#include "content/public/browser/site_instance.h"
#include "content/public/browser/web_contents.h"
#include "net/http/http_request_headers.h"
#include "ui/base/page_transition_types.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace traffic_control {

// static
void TrafficControlNavigationThrottle::MaybeCreateAndAdd(
    content::NavigationThrottleRegistry& registry,
    TrafficControlService* service) {
  if (!service || !service->IsEnabled()) {
    return;
  }

  registry.AddThrottle(
      std::make_unique<TrafficControlNavigationThrottle>(registry, *service));
}

TrafficControlNavigationThrottle::TrafficControlNavigationThrottle(
    content::NavigationThrottleRegistry& registry,
    TrafficControlService& service)
    : content::NavigationThrottle(registry), service_(service) {}

TrafficControlNavigationThrottle::~TrafficControlNavigationThrottle() = default;

content::NavigationThrottle::ThrottleCheckResult
TrafficControlNavigationThrottle::WillStartRequest() {
  return MaybeReroute();
}

content::NavigationThrottle::ThrottleCheckResult
TrafficControlNavigationThrottle::WillRedirectRequest() {
  return MaybeReroute();
}

content::NavigationThrottle::ThrottleCheckResult
TrafficControlNavigationThrottle::MaybeReroute() {
  content::NavigationHandle* handle = navigation_handle();
  content::WebContents* web_contents = handle->GetWebContents();
  if (!web_contents || !handle->IsInPrimaryMainFrame()) {
    return PROCEED;
  }
  if (handle->IsSameDocument() || handle->IsDownload()) {
    return PROCEED;
  }

  const GURL& url = handle->GetURL();
  base::optional_ref<const mojom::TrafficRule> rule =
      service_->FindMatchingRule(url);
  if (!rule || !rule->target) {
    return PROCEED;
  }

  if (TrafficControlApplier::AlreadyAtTarget(web_contents, *rule->target)) {
    return PROCEED;
  }

  content::SiteInstance* starting_instance = handle->GetStartingSiteInstance();

  // Same-site navigations stay put unless this is an omnibox navigation. This
  // keeps a tab the user deliberately opened elsewhere (e.g. via open-in-
  // container) in its container while browsing within the same site. A brand
  // new tab has no such history of its own and only inherits the opener's
  // container, so rules take priority there.
  if (starting_instance && !IsNewTabNavigation(web_contents) &&
      !IsOmniboxNavigation(handle->GetPageTransition()) &&
      starting_instance->IsSameSiteWithURL(url)) {
    return PROCEED;
  }

  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE,
      base::BindOnce(&TrafficControlApplier::Apply, web_contents->GetWeakPtr(),
                     GetRerouteUrl(*rule->target), rule->target->Clone(),
                     handle->GetInitiatorOrigin(),
                     handle->GetPageTransition()));

  return CANCEL;
}

GURL TrafficControlNavigationThrottle::GetRerouteUrl(
    const mojom::Target& target) {
  content::NavigationHandle* handle = navigation_handle();
  if (!handle->WasServerRedirect()) {
    return handle->GetURL();
  }

  // Reopening the start of a form submission would submit it again.
  if (handle->GetRequestMethod() != net::HttpRequestHeaders::kGetMethod) {
    return handle->GetURL();
  }

  // A client-side redirect puts the redirecting page ahead of the requested
  // URL in the chain.
  const std::vector<GURL>& chain = handle->GetRedirectChain();
  const size_t start_index =
      handle->GetPageTransition() & ui::PAGE_TRANSITION_CLIENT_REDIRECT ? 1 : 0;
  if (start_index >= chain.size()) {
    return handle->GetURL();
  }
  const GURL& start_url = chain[start_index];

  // Once reopened at |target|, the start URL must stay there; a rule sending
  // it elsewhere would bounce the navigation between tabs.
  base::optional_ref<const mojom::TrafficRule> start_rule =
      service_->FindMatchingRule(start_url);
  if (start_rule && start_rule->target && !start_rule->target->Equals(target)) {
    return handle->GetURL();
  }

  return start_url;
}

const char* TrafficControlNavigationThrottle::GetNameForLogging() {
  return "TrafficControlNavigationThrottle";
}

}  // namespace traffic_control
