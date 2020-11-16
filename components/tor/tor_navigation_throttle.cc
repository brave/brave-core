/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/tor/tor_navigation_throttle.h"

#include <utility>

#include "brave/components/tor/tor_profile_service.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/url_constants.h"
#include "extensions/common/constants.h"

namespace tor {

// static
std::unique_ptr<TorNavigationThrottle>
TorNavigationThrottle::MaybeCreateThrottleFor(
    content::NavigationHandle* navigation_handle,
    TorProfileService* service,
    bool is_tor_profile) {
  if (!is_tor_profile || !service)
    return nullptr;
  return std::make_unique<TorNavigationThrottle>(navigation_handle, service);
}

TorNavigationThrottle::TorNavigationThrottle(
    content::NavigationHandle* navigation_handle,
    TorProfileService* service)
    : content::NavigationThrottle(navigation_handle),
      tor_profile_service_(service) {
  DCHECK(tor_profile_service_);
  tor_profile_service_->AddObserver(this);
}

TorNavigationThrottle::~TorNavigationThrottle() {
  tor_profile_service_->RemoveObserver(this);
}

content::NavigationThrottle::ThrottleCheckResult
TorNavigationThrottle::WillStartRequest() {
  GURL url = navigation_handle()->GetURL();
  if (url.SchemeIsHTTPOrHTTPS() || url.SchemeIs(content::kChromeUIScheme) ||
      url.SchemeIs(extensions::kExtensionScheme) ||
      url.SchemeIs(content::kChromeDevToolsScheme)) {
    if (!tor_profile_service_->IsTorConnected() &&
        !url.SchemeIs(content::kChromeUIScheme)) {
      resume_pending_ = true;
      return content::NavigationThrottle::DEFER;
    }
    return content::NavigationThrottle::PROCEED;
  }
  return content::NavigationThrottle::BLOCK_REQUEST;
}

const char* TorNavigationThrottle::GetNameForLogging() {
  return "TorNavigationThrottle";
}

void TorNavigationThrottle::OnTorCircuitEstablished(bool result) {
  if (result && resume_pending_) {
    resume_pending_ = false;
    Resume();
  }
}

}  // namespace tor
