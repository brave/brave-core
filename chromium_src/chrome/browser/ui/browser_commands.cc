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
#include "chrome/common/webui_url_constants.h"

#define ReloadBypassingCache ReloadBypassingCache_ChromiumImpl
#define GetReadingListModel GetReadingListModel_ChromiumImpl
#define kChromeUISplitViewNewTabPageURL kChromeUINewTabURL

// Don't show toast when user tries to close a pinned tab via the keyboard
// accelerator.
#define BRAVE_CLOSE_TAB toast_controller = nullptr;

#include <chrome/browser/ui/browser_commands.cc>

#undef BRAVE_CLOSE_TAB
#undef kChromeUISplitViewNewTabPageURL
#undef ReloadBypassingCache
#undef GetReadingListModel

namespace chrome {

void ReloadBypassingCache(Browser* browser, WindowOpenDisposition disposition) {
#if BUILDFLAG(ENABLE_TOR)
  Profile* profile = browser->profile();
  DCHECK(profile);
  // NewTorConnectionForSite will do hard reload after obtaining new identity
  if (profile->IsTor()) {
    brave::NewTorConnectionForSite(browser);
    return;
  }
#endif
  ReloadBypassingCache_ChromiumImpl(browser, disposition);
}

ReadingListModel* GetReadingListModel(Browser* browser) {
  return nullptr;
}

}  // namespace chrome
