/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/tor/onion_location_navigation_throttle.h"

#include <string>
#include <utility>

#include "base/check.h"
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
void OnionLocationNavigationThrottle::MaybeCreateAndAdd(
    content::NavigationThrottleRegistry& registry,
    bool is_tor_disabled,
    bool is_tor_profile,
    bool onion_only_in_tor_windows) {
  if (is_tor_disabled) {
    return;
  }
  content::NavigationHandle& navigation_handle = registry.GetNavigationHandle();
  registry.AddThrottle(std::make_unique<OnionLocationNavigationThrottle>(
      registry, is_tor_profile, onion_only_in_tor_windows));
}

OnionLocationNavigationThrottle::OnionLocationNavigationThrottle(
    content::NavigationThrottleRegistry& registry,
    bool is_tor_profile,
    bool onion_only_in_tor_windows)
    : content::NavigationThrottle(registry),
      is_tor_profile_(is_tor_profile),
      onion_only_in_tor_windows_(onion_only_in_tor_windows) {}

OnionLocationNavigationThrottle::~OnionLocationNavigationThrottle() = default;

content::NavigationThrottle::ThrottleCheckResult
OnionLocationNavigationThrottle::WillProcessResponse() {
  if (!navigation_handle()->IsInMainFrame()) {
    return content::NavigationThrottle::PROCEED;
  }

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
    OnionLocationTabHelper::SetOnionLocationByThrottle(
        navigation_handle()->GetWebContents(), url);
  } else {
    OnionLocationTabHelper::SetOnionLocationByThrottle(
        navigation_handle()->GetWebContents(), GURL());
  }
  return content::NavigationThrottle::PROCEED;
}

content::NavigationThrottle::ThrottleCheckResult
OnionLocationNavigationThrottle::WillStartRequest() {
  const bool is_main_frame = navigation_handle()->IsInMainFrame();

  if (is_main_frame) {
    // Clear onion location.
    OnionLocationTabHelper::SetOnionLocationByThrottle(
        navigation_handle()->GetWebContents(), GURL());
  }

  // If a user enters .onion address in non-Tor window, we block the request and
  // offer "Open in Tor" button or automatically opening it in Tor window.
  if (!is_tor_profile_ && onion_only_in_tor_windows_) {
    const GURL& url = navigation_handle()->GetURL();
    if (url.SchemeIsHTTPOrHTTPS() && net::IsOnion(url)) {
      if (is_main_frame) {
        OnionLocationTabHelper::SetOnionLocationByThrottle(
            navigation_handle()->GetWebContents(), url,
            navigation_handle()->GetInitiatorOrigin());
        return content::NavigationThrottle::BLOCK_REQUEST;
      }
      // Subframes should use CANCEL_AND_IGNORE. BLOCK_REQUEST commits an error
      // page that still reports the .onion URL as the last committed URL.
      return content::NavigationThrottle::CANCEL_AND_IGNORE;
    }
  }
  return content::NavigationThrottle::PROCEED;
}

content::NavigationThrottle::ThrottleCheckResult
OnionLocationNavigationThrottle::WillRedirectRequest() {
  return WillStartRequest();
}

const char* OnionLocationNavigationThrottle::GetNameForLogging() {
  return "OnionLocationNavigationThrottle";
}

}  // namespace tor
