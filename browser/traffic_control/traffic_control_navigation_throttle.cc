// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/traffic_control/traffic_control_navigation_throttle.h"

#include "base/functional/bind.h"
#include "base/task/sequenced_task_runner.h"
#include "base/types/optional_ref.h"
#include "brave/browser/traffic_control/traffic_control_applier.h"
#include "brave/browser/traffic_control/traffic_control_tab_utils.h"
#include "brave/browser/traffic_control/traffic_control_types.h"
#include "brave/components/traffic_control/core/browser/traffic_control_service.h"
#include "brave/components/traffic_control/core/mojom/traffic_control.mojom.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/navigation_throttle_registry.h"
#include "content/public/browser/site_instance.h"
#include "content/public/browser/web_contents.h"
#include "net/http/http_request_headers.h"
#include "services/network/public/cpp/web_sandbox_flags.h"
#include "services/network/public/mojom/web_sandbox_flags.mojom.h"
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

bool TrafficControlNavigationThrottle::IsSandboxedNavigationAllowed() {
  using network::mojom::WebSandboxFlags;
  const WebSandboxFlags initiator_sandbox_flags =
      navigation_handle()->SandboxFlagsInitiator();
  // Rerouting opens a new tab, so it must obey the initiator's popup ban.
  if ((initiator_sandbox_flags & WebSandboxFlags::kPopups) !=
      WebSandboxFlags::kNone) {
    return false;
  }
  // Without the top-navigation restriction, no user gesture is required.
  if ((initiator_sandbox_flags & WebSandboxFlags::kTopNavigation) ==
      WebSandboxFlags::kNone) {
    return true;
  }
  // Both top-navigation flags set means top navigation is always forbidden.
  if ((initiator_sandbox_flags &
       WebSandboxFlags::kTopNavigationByUserActivation) !=
      WebSandboxFlags::kNone) {
    return false;
  }
  // With only user-activation allowance, a gesture is required.
  return navigation_handle()->HasUserGesture();
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
  // Partition rerouting only supports network navigations with an HTTP origin.
  if (!url.SchemeIsHTTPOrHTTPS()) {
    return PROCEED;
  }
  // Reopening non-GET requests would discard bodies or replay submissions.
  if (handle->GetRequestMethod() != net::HttpRequestHeaders::kGetMethod ||
      handle->IsPost() || handle->IsFormSubmission()) {
    return PROCEED;
  }
  // Preserve user-controlled reload, restore, activation, and history flows.
  if (handle->GetReloadType() != content::ReloadType::NONE ||
      handle->GetRestoreType() != content::RestoreType::kNotRestored ||
      handle->IsPageActivation() ||
      (handle->GetPageTransition() & ui::PAGE_TRANSITION_FORWARD_BACK)) {
    return PROCEED;
  }
  // Check if the navigation is allowed under the current sandbox flags.
  if (!IsSandboxedNavigationAllowed()) {
    return PROCEED;
  }

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
                     NavigationIntent(*handle, rule->target->Clone())));

  return CANCEL;
}

const char* TrafficControlNavigationThrottle::GetNameForLogging() {
  return "TrafficControlNavigationThrottle";
}

}  // namespace traffic_control
