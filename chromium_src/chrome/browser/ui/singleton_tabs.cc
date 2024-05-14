/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#define ShowSingletonTab ShowSingletonTab_ChromiumImpl
#include "src/chrome/browser/ui/singleton_tabs.cc"
#undef ShowSingletonTab

// ShowSingletonTab functions (for Browser and Profile) are used to display
// various help pages both local (such as chrome://password-manager/settings)
// and remote (on https://www.google.com and https://support.google.com).
// For remote URLs going to Google we want to point users to our community site
// instead.
void ShowSingletonTab(Browser* browser, const GURL& url) {
  GURL new_url = url.DomainIs("google.com") ?
    GURL("https://community.brave.com/") : url;

  ShowSingletonTab_ChromiumImpl(browser, new_url);
}

void ShowSingletonTab(Profile* profile, const GURL& url) {
  GURL new_url =
      url.DomainIs("google.com") ? GURL("https://community.brave.com/") : url;

  ShowSingletonTab_ChromiumImpl(profile, new_url);
}
