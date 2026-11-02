// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/traffic_control/traffic_control_navigation_throttle.h"

#include <string>

#include "base/functional/bind.h"
#include "base/task/sequenced_task_runner.h"
#include "base/types/optional_ref.h"
#include "brave/browser/containers/containers_service_factory.h"
#include "brave/browser/traffic_control/traffic_control_apply.h"
#include "brave/browser/traffic_control/traffic_control_tab_utils.h"
#include "brave/components/containers/content/browser/storage_partition_utils.h"
#include "brave/components/containers/core/browser/containers_service.h"
#include "brave/components/traffic_control/core/browser/traffic_control_service.h"
#include "brave/components/traffic_control/core/mojom/traffic_control.mojom.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/navigation_throttle_registry.h"
#include "content/public/browser/site_instance.h"
#include "content/public/browser/web_contents.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace traffic_control {

namespace {

bool IsInConfinedContainer(content::WebContents* web_contents) {
  // Null when the Containers feature is disabled, in which case nothing can be
  // in a container.
  containers::ContainersService* containers_service =
      ContainersServiceFactory::GetForProfile(
          Profile::FromBrowserContext(web_contents->GetBrowserContext()));
  if (!containers_service) {
    return false;
  }
  const std::string container_id =
      containers::GetContainerIdForWebContents(web_contents);
  return !container_id.empty() &&
         containers_service->IsContainerConfined(container_id);
}

}  // namespace

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

  // A confined container's tabs stay in it, whatever the rules say.
  if (IsInConfinedContainer(web_contents)) {
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
                     url, rule->target->Clone(), handle->GetInitiatorOrigin(),
                     handle->GetPageTransition()));

  return CANCEL;
}

const char* TrafficControlNavigationThrottle::GetNameForLogging() {
  return "TrafficControlNavigationThrottle";
}

}  // namespace traffic_control
