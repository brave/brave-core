/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/tor/onion_location_navigation_throttle.h"

#include <string>
#include <utility>

#include "base/bind.h"
#include "brave/browser/profiles/profile_util.h"
#include "brave/browser/tor/onion_location_tab_helper.h"
#include "brave/browser/tor/tor_profile_service_factory.h"
#include "brave/components/tor/pref_names.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_window.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
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

void OnTorProfileCreated(GURL onion_location,
                         Profile* profile,
                         Profile::CreateStatus status) {
  if (status != Profile::CreateStatus::CREATE_STATUS_INITIALIZED)
    return;
  Browser* browser = chrome::FindTabbedBrowser(profile, true);
  if (!browser)
    return;
  content::OpenURLParams open_tor(onion_location, content::Referrer(),
                                  WindowOpenDisposition::OFF_THE_RECORD,
                                  ui::PAGE_TRANSITION_TYPED, false);
  browser->OpenURL(open_tor);
}

}  // namespace

// static
std::unique_ptr<OnionLocationNavigationThrottle>
OnionLocationNavigationThrottle::MaybeCreateThrottleFor(
    content::NavigationHandle* navigation_handle) {
  if (TorProfileServiceFactory::IsTorDisabled() ||
      !navigation_handle->IsInMainFrame())
    return nullptr;
  return std::make_unique<OnionLocationNavigationThrottle>(navigation_handle);
}

OnionLocationNavigationThrottle::OnionLocationNavigationThrottle(
    content::NavigationHandle* navigation_handle)
    : content::NavigationThrottle(navigation_handle) {
  profile_ = Profile::FromBrowserContext(
      navigation_handle->GetWebContents()->GetBrowserContext());
}

OnionLocationNavigationThrottle::~OnionLocationNavigationThrottle() {}

content::NavigationThrottle::ThrottleCheckResult
OnionLocationNavigationThrottle::WillProcessResponse() {
  auto* headers = navigation_handle()->GetResponseHeaders();
  std::string onion_location;
  // The webpage defining the Onion-Location header must not be an onionsite.
  // https://gitweb.torproject.org/tor-browser-spec.git/plain/proposals/100-onion-location-header.txt
  if (headers && GetOnionLocation(headers, &onion_location) &&
      !navigation_handle()->GetURL().DomainIs("onion")) {
    // If user prefers opening it automatically
    if (profile_->GetPrefs()->GetBoolean(prefs::kAutoOnionLocation)) {
      profiles::SwitchToTorProfile(
          base::BindRepeating(&OnTorProfileCreated, GURL(onion_location)));
      // We do not close last tab of the window
      Browser* browser = chrome::FindBrowserWithProfile(profile_);
      if (browser && browser->tab_strip_model()->count() > 1)
        navigation_handle()->GetWebContents()->ClosePage();
    } else {
      OnionLocationTabHelper::SetOnionLocation(
          navigation_handle()->GetWebContents(), GURL(onion_location));
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
  if (!brave::IsTorProfile(profile_)) {
    GURL url = navigation_handle()->GetURL();
    if (url.SchemeIsHTTPOrHTTPS() && url.DomainIs("onion")) {
      profiles::SwitchToTorProfile(
          base::BindRepeating(&OnTorProfileCreated, std::move(url)));
      return content::NavigationThrottle::CANCEL_AND_IGNORE;
    }
  }
  return content::NavigationThrottle::PROCEED;
}

const char* OnionLocationNavigationThrottle::GetNameForLogging() {
  return "OnionLocationNavigationThrottle";
}

}  // namespace tor
