// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/main/browser_agent_util.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#include "ios/chrome/browser/send_tab_to_self/send_tab_to_self_browser_agent.h"
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
