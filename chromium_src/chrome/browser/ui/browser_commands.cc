/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/browser_commands.h"

#include "base/check.h"
#include "brave/components/commander/common/buildflags/buildflags.h"
#include "brave/components/tor/buildflags/buildflags.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/common/webui_url_constants.h"

class ReadingListModel;

namespace chrome {

// Upstream function, renamed by rewrite rule.
void ReloadBypassingCache_ChromiumImpl(BrowserWindowInterface* browser,
                                       WindowOpenDisposition disposition);

// We override upstream's ReloadBypassingCache_ChromiumImpl with this function.
// This is needed to handle the case where the profile is a Tor profile,
// in which case we want to trigger a new Tor connection instead of a regular
// hard reload.
void ReloadBypassingCache(BrowserWindowInterface* browser,
                          WindowOpenDisposition disposition) {
#if BUILDFLAG(ENABLE_TOR)
  Profile* profile = browser->GetProfile();
  DCHECK(profile);
  // NewTorConnectionForSite will do hard reload after obtaining new identity
  if (profile->IsTor()) {
    brave::NewTorConnectionForSite(browser);
    return;
  }
#endif
  ReloadBypassingCache_ChromiumImpl(browser, disposition);
}

}  // namespace chrome

#include <chrome/browser/ui/browser_commands.cc>
