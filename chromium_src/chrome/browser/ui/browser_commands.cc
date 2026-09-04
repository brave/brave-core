/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/browser_commands.h"

#include "base/check.h"
#include "brave/browser/ui/tabs/brave_tab_strip_model.h"
#include "brave/components/commander/common/buildflags/buildflags.h"
#include "brave/components/tor/buildflags/buildflags.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/common/webui_url_constants.h"

class ReadingListModel;

namespace {

// Returns true if the reload was handled by Brave (a new Tor connection was
// triggered) and the caller should return early.
bool BraveMaybeHandleReloadBypassingCache(BrowserWindowInterface* browser) {
#if BUILDFLAG(ENABLE_TOR)
  Profile* profile = browser->GetProfile();
  DCHECK(profile);
  // NewTorConnectionForSite will do hard reload after obtaining new identity
  if (profile->IsTor()) {
    brave::NewTorConnectionForSite(browser);
    return true;
  }
#endif  // BUILDFLAG(ENABLE_TOR)
  return false;
}

}  // namespace

#include <chrome/browser/ui/browser_commands.cc>
