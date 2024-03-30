/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/tor/onion_location_navigation_throttle.h"

#include <string>
#include <utility>

#include "base/functional/bind.h"
#include "brave/components/tor/onion_location_tab_helper.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_observer.h"
#include "net/base/url_util.h"
#include "net/http/http_response_headers.h"

namespace tor {

namespace {

bool GetOnionLocation(const net::HttpResponseHeaders* headers,
                      std::string* onion_location) {
  DCHECK(onion_location);

  onion_location->clear();
  constexpr const char kHeaderName[] = "onion-location";

  if (!headers ||
      !headers->EnumerateHeader(nullptr, kHeaderName, onion_location)) {
    return false;
  }
  return true;
}

}  // namespace

// static
std::unique_ptr<OnionLocationNavigationThrottle>
OnionLocationNavigationThrottle::MaybeCreateThrottleFor(
    content::NavigationHandle* navigation_handle,
    bool is_tor_disabled,
    bool is_tor_profile) {
  if (is_tor_disabled || !navigation_handle->IsInMainFrame()) {
    return nullptr;
  }
  return std::make_unique<OnionLocationNavigationThrottle>(navigation_handle,
                                                           is_tor_profile);
}

OnionLocationNavigationThrottle::OnionLocationNavigationThrottle(
    content::NavigationHandle* navigation_handle,
    bool is_tor_profile)
    : content::NavigationThrottle(navigation_handle),
      is_tor_profile_(is_tor_profile) {}

OnionLocationNavigationThrottle::~OnionLocationNavigationThrottle() = default;

content::NavigationThrottle::ThrottleCheckResult
OnionLocationNavigationThrottle::WillProcessResponse() {
  auto* headers = navigation_handle()->GetResponseHeaders();
  std::string onion_location;
  // The webpage defining the Onion-Location header must not be an onionsite.
  // https://gitlab.torproject.org/tpo/applications/tor-browser-spec/-/raw/HEAD/proposals/100-onion-location-header.txt
  if (headers && GetOnionLocation(headers, &onion_location) &&
      !net::IsOnion(navigation_handle()->GetURL()) &&
      // The webpage defining the Onion-Location header must be served over
      // HTTPS.
      navigation_handle()->GetURL().SchemeIs(url::kHttpsScheme)) {
    GURL url(onion_location);
    // The Onion-Location value must be a valid URL with http: or https:
    // protocol and a .onion hostname.
    if (!url.SchemeIsHTTPOrHTTPS() || !net::IsOnion(url)) {
      return content::NavigationThrottle::PROCEED;
    }
    // Process only 'tabs' web contents and don't touch other.
    if (!OnionLocationTabHelper::FromWebContents(
            navigation_handle()->GetWebContents())) {
      return content::NavigationThrottle::PROCEED;
    }
    OnionLocationTabHelper::SetOnionLocation(
        navigation_handle()->GetWebContents(), url);
  } else {
    OnionLocationTabHelper::SetOnionLocation(
        navigation_handle()->GetWebContents(), GURL());
  }
  return content::NavigationThrottle::PROCEED;
}

content::NavigationThrottle::ThrottleCheckResult
OnionLocationNavigationThrottle::WillStartRequest() {
  // Clear onion location.
  OnionLocationTabHelper::SetOnionLocation(
      navigation_handle()->GetWebContents(), GURL());

  // If a user enters .onion address in non-Tor window, we block the request and
  // offer "Open in Tor" button or automatically opening it in Tor window.
  if (!is_tor_profile_) {
    GURL url = navigation_handle()->GetURL();
    if (url.SchemeIsHTTPOrHTTPS() && net::IsOnion(url)) {
      OnionLocationTabHelper::SetOnionLocation(
          navigation_handle()->GetWebContents(), url);
      return content::NavigationThrottle::BLOCK_REQUEST;
    }
  }
  return content::NavigationThrottle::PROCEED;
}

const char* OnionLocationNavigationThrottle::GetNameForLogging() {
  return "OnionLocationNavigationThrottle";
}

}  // namespace tor
