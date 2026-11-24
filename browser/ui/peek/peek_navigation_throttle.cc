/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/peek/peek_navigation_throttle.h"

#include <memory>

#include "base/feature_list.h"
#include "brave/browser/ui/peek/peek_features.h"
#include "brave/browser/ui/peek/peek_utils.h"
#include "brave/browser/ui/peek/public/peek_controller.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/navigation_throttle_registry.h"

// static
void PeekNavigationThrottle::MaybeCreateAndAdd(
    content::NavigationThrottleRegistry& registry) {
  if (!base::FeatureList::IsEnabled(peek::features::kBravePeek)) {
    return;
  }
  content::NavigationHandle& handle = registry.GetNavigationHandle();
  if (handle.IsInPrimaryMainFrame() &&
      PeekController::FromPeekContents(handle.GetWebContents())) {
    registry.AddThrottle(std::make_unique<PeekNavigationThrottle>(registry));
  }
}

PeekNavigationThrottle::PeekNavigationThrottle(
    content::NavigationThrottleRegistry& registry)
    : content::NavigationThrottle(registry) {}

PeekNavigationThrottle::~PeekNavigationThrottle() = default;

content::NavigationThrottle::ThrottleCheckResult
PeekNavigationThrottle::WillStartRequest() {
  return CheckURL();
}

content::NavigationThrottle::ThrottleCheckResult
PeekNavigationThrottle::WillRedirectRequest() {
  return CheckURL();
}

content::NavigationThrottle::ThrottleCheckResult
PeekNavigationThrottle::WillCommitWithoutUrlLoader() {
  return CheckURL();
}

const char* PeekNavigationThrottle::GetNameForLogging() {
  return "PeekNavigationThrottle";
}

content::NavigationThrottle::ThrottleCheckResult
PeekNavigationThrottle::CheckURL() {
  // The contents may have been moved into a tab since the navigation started.
  if (!PeekController::FromPeekContents(
          navigation_handle()->GetWebContents()) ||
      peek::IsPeekableURL(navigation_handle()->GetURL())) {
    return PROCEED;
  }
  return CANCEL_AND_IGNORE;
}
