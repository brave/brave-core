/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_search/backup_results_navigation_throttle.h"

#include "brave/browser/brave_search/backup_results_service_factory.h"
#include "brave/components/brave_search/browser/backup_results_service.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents.h"

namespace brave_search {

// static
void BackupResultsNavigationThrottle::MaybeCreateAndAdd(
    content::NavigationThrottleRegistry& registry) {
  auto* context =
      registry.GetNavigationHandle().GetWebContents()->GetBrowserContext();
  auto* profile = Profile::FromBrowserContext(context);
  if (!profile->IsOffTheRecord() ||
      !profile->GetOTRProfileID().IsSearchBackupResults()) {
    return;
  }

  registry.AddThrottle(
      std::make_unique<BackupResultsNavigationThrottle>(registry));
}

BackupResultsNavigationThrottle::BackupResultsNavigationThrottle(
    content::NavigationThrottleRegistry& registry)
    : NavigationThrottle(registry) {}
BackupResultsNavigationThrottle::~BackupResultsNavigationThrottle() = default;

content::NavigationThrottle::ThrottleCheckResult
BackupResultsNavigationThrottle::WillStartRequest() {
  if (!navigation_handle()->IsInPrimaryMainFrame()) {
    return content::NavigationThrottle::CANCEL;
  }

  auto* web_contents = navigation_handle()->GetWebContents();
  auto* backup_results_service =
      BackupResultsServiceFactory::GetForBrowserContext(
          web_contents->GetBrowserContext());
  if (backup_results_service->HandleWebContentsStartRequest(
          web_contents, navigation_handle()->GetURL())) {
    return content::NavigationThrottle::PROCEED;
  }

  return content::NavigationThrottle::CANCEL;
}

content::NavigationThrottle::ThrottleCheckResult
BackupResultsNavigationThrottle::WillRedirectRequest() {
  return WillStartRequest();
}

const char* BackupResultsNavigationThrottle::GetNameForLogging() {
  return "BackupResultsNavigationThrottle";
}

}  // namespace brave_search
