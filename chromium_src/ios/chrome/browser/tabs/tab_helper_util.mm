/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#import "ios/chrome/browser/tabs/tab_helper_util.h"

#include "ios/chrome/browser/complex_tasks/ios_task_tab_helper.h"
#include "ios/chrome/browser/sessions/ios_chrome_session_tab_helper.h"
#include "ios/chrome/browser/sync/ios_chrome_synced_tab_delegate.h"
#include "ios/chrome/browser/tabs/tab_helper_util.h"
#include "ios/chrome/browser/web/session_state/web_session_state_tab_helper.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

void AttachTabHelpers(web::WebState* web_state, bool for_prerender) {
  IOSChromeSessionTabHelper::CreateForWebState(web_state);
  IOSChromeSyncedTabDelegate::CreateForWebState(web_state);
  WebSessionStateTabHelper::CreateForWebState(web_state);
  IOSTaskTabHelper::CreateForWebState(web_state);
}
