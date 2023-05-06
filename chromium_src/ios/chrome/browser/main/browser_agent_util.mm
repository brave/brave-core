/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#import "ios/chrome/browser/main/browser_agent_util.h"
#include "ios/chrome/browser/send_tab_to_self/send_tab_to_self_browser_agent.h"
#include "ios/chrome/browser/shared/model/browser_state/chrome_browser_state.h"
#include "ios/chrome/browser/tabs/synced_window_delegate_browser_agent.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

void AttachBrowserAgents(Browser* browser) {
  SyncedWindowDelegateBrowserAgent::CreateForBrowser(browser);

  // Send Tab To Self is non-OTR only.
  if (!browser->GetBrowserState()->IsOffTheRecord()) {
    SendTabToSelfBrowserAgent::CreateForBrowser(browser);
  }
}
