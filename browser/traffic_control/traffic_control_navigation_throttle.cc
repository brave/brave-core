// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/traffic_control/traffic_control_navigation_throttle.h"

#include <utility>

#include "base/functional/bind.h"
#include "base/task/sequenced_task_runner.h"
#include "base/types/optional_ref.h"
#include "brave/browser/traffic_control/traffic_control_apply.h"
#include "brave/browser/traffic_control/traffic_control_tab_utils.h"
#include "brave/components/containers/content/browser/preserve_container_destination.h"
#include "brave/components/traffic_control/core/browser/traffic_control_service.h"
#include "brave/components/traffic_control/core/mojom/traffic_control.mojom.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/navigation_throttle_registry.h"
#include "content/public/browser/site_instance.h"
#include "content/public/browser/web_contents.h"
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

  // Explicit container was chosen by the user.
  if (containers::PreserveContainerDestination::Consume(web_contents)) {
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

  // Same-site navigations stay put unless this is an omnibox navigation.
  if (starting_instance && !IsOmniboxNavigation(handle->GetPageTransition()) &&
      starting_instance->IsSameSiteWithURL(url)) {
    return PROCEED;
  }

  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE,
      base::BindOnce(&TrafficControlApplier::Apply, web_contents->GetWeakPtr(),
                     url, rule->target->Clone(), handle->GetInitiatorOrigin(),
                     handle->GetPageTransition()));

  return CANCEL;
}

const char* TrafficControlNavigationThrottle::GetNameForLogging() {
  return "TrafficControlNavigationThrottle";
}

}  // namespace traffic_control
