/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/api/opentabs/brave_opentabs_api.h"

#include "base/bind.h"
#include "base/strings/sys_string_conversions.h"

#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#include "ios/chrome/browser/ui/recent_tabs/synced_sessions.h"

#include "ios/web/public/thread/web_thread.h"
#include "net/base/mac/url_conversions.h"
#include "url/gurl.h"

// #include "brave/ios/browser/api/opentabs/brave_opentabs_observer.h"
// #include "brave/ios/browser/api/opentabs/opentabs_session_service_listener.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

#pragma mark - IOSOpenTabNode

@implementation IOSOpenTabNode

- (instancetype)initWithURL:(NSURL*)url
                      title:(nullable NSString*)title
                      tabId:(NSUInteger)tabId
                 sessionTag:(nullable NSString*)sessionTag {
  if ((self = [super init])) {
    self.url = url;
    self.title = title;
    self.tabId = tabId;
    self.sessionTag = sessionTag;
  }

  return self;
}

- (id)copyWithZone:(NSZone*)zone {
  IOSOpenTabNode* openTabNodeCopy = [[[self class] allocWithZone:zone] init];

  if (openTabNodeCopy) {
    openTabNodeCopy.url = self.url;
    openTabNodeCopy.title = self.title;
    openTabNodeCopy.tabId = self.tabId;
    openTabNodeCopy.sessionTag = self.sessionTag;
  }

  return openTabNodeCopy;
}

- (void)updateOpenTabNode:(NSURL*)url
                    title:(NSString*)title {
  [self setUrl:url];

  if ([title length] != 0) {
    [self setTitle:title];
  }
}

@end

#pragma mark - BraveOpenTabsAPI

@interface BraveOpenTabsAPI () {
  ChromeBrowserState* _chromeBrowserState;
}
@end

@implementation BraveOpenTabsAPI

- (instancetype)initWithBrowserState:(ChromeBrowserState*)mainBrowserState {
  if ((self = [super init])) {
    _chromeBrowserState = mainBrowserState;
  }
  return self;
}

- (void)dealloc {
  _chromeBrowserState = NULL;
}

@end