/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/tor/onion_location_navigation_throttle_delegate.h"

#include <utility>

#include "brave/browser/tor/tor_profile_manager.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "content/public/browser/web_contents.h"

namespace tor {

namespace {

void OpenURLInTor(Browser* browser, const GURL& onion_location) {
  if (!browser) {
    return;
  }

  // New tab.
  content::OpenURLParams open_tor(onion_location, content::Referrer(),
                                  WindowOpenDisposition::SWITCH_TO_TAB,
                                  ui::PAGE_TRANSITION_TYPED, false);
  browser->OpenURL(open_tor);
}

}  // namespace

OnionLocationNavigationThrottleDelegate::
    OnionLocationNavigationThrottleDelegate() = default;

OnionLocationNavigationThrottleDelegate::
    ~OnionLocationNavigationThrottleDelegate() = default;

void OnionLocationNavigationThrottleDelegate::OpenInTorWindow(
    content::WebContents* web_contents,
    const GURL& onion_location) {
  Profile* profile =
      Profile::FromBrowserContext(web_contents->GetBrowserContext());
  Browser* tor_browser = TorProfileManager::SwitchToTorProfile(profile);
  OpenURLInTor(tor_browser, onion_location);
}

}  // namespace tor
