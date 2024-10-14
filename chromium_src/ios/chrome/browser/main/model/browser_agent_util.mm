/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#import "ios/chrome/browser/main/model/browser_agent_util.h"

#include "ios/chrome/browser/send_tab_to_self/model/send_tab_to_self_browser_agent.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"
#include "ios/chrome/browser/tabs/model/synced_window_delegate_browser_agent.h"

void AttachBrowserAgents(Browser* browser) {
  SyncedWindowDelegateBrowserAgent::CreateForBrowser(browser);

  // Send Tab To Self is non-OTR only.
  if (!browser->GetProfile()->IsOffTheRecord()) {
    SendTabToSelfBrowserAgent::CreateForBrowser(browser);
  }
}
