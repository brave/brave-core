// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/tabs/tab_helper_util.h"

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
}
