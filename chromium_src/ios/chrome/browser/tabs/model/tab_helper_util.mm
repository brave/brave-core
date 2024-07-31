/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#import "ios/chrome/browser/tabs/model/tab_helper_util.h"

#include "ios/chrome/browser/complex_tasks/model/ios_task_tab_helper.h"
#include "ios/chrome/browser/sessions/model/ios_chrome_session_tab_helper.h"
#import "ios/chrome/browser/sessions/model/web_session_state_tab_helper.h"
#include "ios/chrome/browser/tabs/model/ios_chrome_synced_tab_delegate.h"

void AttachTabHelpers(web::WebState* web_state, TabHelperFilter filter_flags) {
  IOSChromeSessionTabHelper::CreateForWebState(web_state);
  IOSChromeSyncedTabDelegate::CreateForWebState(web_state);
  WebSessionStateTabHelper::CreateForWebState(web_state);
  IOSTaskTabHelper::CreateForWebState(web_state);
}
