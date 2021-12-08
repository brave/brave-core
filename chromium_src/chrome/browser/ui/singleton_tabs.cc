/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#define ShowSingletonTab ShowSingletonTab_ChromiumImpl
#include "src/chrome/browser/ui/singleton_tabs.cc"
#undef ShowSingletonTab

void ShowSingletonTab(Browser* browser, const GURL& url) {
  GURL new_url = url.DomainIs("google.com") ?
    GURL("https://community.brave.com/") : url;

  ShowSingletonTab_ChromiumImpl(browser, new_url);
}
