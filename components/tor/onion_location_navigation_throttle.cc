/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/tor/onion_location_navigation_throttle.h"

#include <string>
#include <utility>

#include "base/bind.h"
#include "brave/components/tor/onion_location_tab_helper.h"
#include "brave/components/tor/pref_names.h"
#include "components/prefs/pref_service.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents.h"

namespace tor {

namespace {

bool GetOnionLocation(const net::HttpResponseHeaders* headers,
                      std::string* onion_location) {
  DCHECK(onion_location);

  onion_location->clear();
  std::string name = "onion-location";

  if (!headers || !headers->EnumerateHeader(nullptr, name, onion_location))
    return false;
  return true;
}

}  // namespace

// static
std::unique_ptr<OnionLocationNavigationThrottle>
OnionLocationNavigationThrottle::MaybeCreateThrottleFor(
    content::NavigationHandle* navigation_handle,
    bool is_tor_disabled,
    std::unique_ptr<Delegate> delegate,
    bool is_tor_profile) {
  if (is_tor_disabled || !navigation_handle->IsInMainFrame())
    return nullptr;
  return std::make_unique<OnionLocationNavigationThrottle>(
      navigation_handle, std::move(delegate), is_tor_profile);
}

OnionLocationNavigationThrottle::OnionLocationNavigationThrottle(
    content::NavigationHandle* navigation_handle,
    std::unique_ptr<Delegate> delegate,
    bool is_tor_profile)
    : content::NavigationThrottle(navigation_handle),
      is_tor_profile_(is_tor_profile),
      delegate_(std::move(delegate)) {
  content::BrowserContext* context =
      navigation_handle->GetWebContents()->GetBrowserContext();
  pref_service_ = user_prefs::UserPrefs::Get(context);
}

OnionLocationNavigationThrottle::~OnionLocationNavigationThrottle() {}

content::NavigationThrottle::ThrottleCheckResult
OnionLocationNavigationThrottle::WillProcessResponse() {
  auto* headers = navigation_handle()->GetResponseHeaders();
  std::string onion_location;
  // The webpage defining the Onion-Location header must not be an onionsite.
  // https://gitweb.torproject.org/tor-browser-spec.git/plain/proposals/100-onion-location-header.txt
  if (headers && GetOnionLocation(headers, &onion_location) &&
      !navigation_handle()->GetURL().DomainIs("onion") &&
      // The webpage defining the Onion-Location header must be served over
      // HTTPS.
      navigation_handle()->GetURL().SchemeIs(url::kHttpsScheme)) {
    GURL url(onion_location);
    // The Onion-Location value must be a valid URL with http: or https:
    // protocol and a .onion hostname.
    if (!url.SchemeIsHTTPOrHTTPS() || !url.DomainIs("onion"))
      return content::NavigationThrottle::PROCEED;
    // If user prefers opening it automatically
    if (pref_service_->GetBoolean(prefs::kAutoOnionRedirect)) {
      delegate_->OpenInTorWindow(navigation_handle()->GetWebContents(), url);
    } else {
      OnionLocationTabHelper::SetOnionLocation(
          navigation_handle()->GetWebContents(), url);
    }
  } else {
    OnionLocationTabHelper::SetOnionLocation(
        navigation_handle()->GetWebContents(), GURL());
  }
  return content::NavigationThrottle::PROCEED;
}

content::NavigationThrottle::ThrottleCheckResult
OnionLocationNavigationThrottle::WillStartRequest() {
  // Open .onion site in Tor window
  if (!is_tor_profile_) {
    GURL url = navigation_handle()->GetURL();
    if (url.SchemeIsHTTPOrHTTPS() && url.DomainIs("onion") &&
        pref_service_->GetBoolean(prefs::kAutoOnionRedirect)) {
      delegate_->OpenInTorWindow(navigation_handle()->GetWebContents(),
                                 std::move(url));
    }
  }
  return content::NavigationThrottle::PROCEED;
}

const char* OnionLocationNavigationThrottle::GetNameForLogging() {
  return "OnionLocationNavigationThrottle";
}

}  // namespace tor
