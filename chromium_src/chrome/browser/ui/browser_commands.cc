/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/browser_commands.h"

#include "brave/browser/profiles/profile_util.h"
#include "chrome/browser/ui/browser_commands.h"
#define ReloadBypassingCache ReloadBypassingCache_ChromiumImpl
#include "../../../../../chrome/browser/ui/browser_commands.cc"  // NOLINT
#undef ReloadBypassingCache

namespace chrome {

void ReloadBypassingCache(Browser* browser, WindowOpenDisposition disposition) {
  Profile* profile = browser->profile();
  DCHECK(profile);
  // NewTorConnectionForSite will do hard reload after obtaining new identity
  if (brave::IsTorProfile(profile))
    brave::NewTorConnectionForSite(browser);
  else
    ReloadBypassingCache_ChromiumImpl(browser, disposition);
}

}  // namespace chrome
