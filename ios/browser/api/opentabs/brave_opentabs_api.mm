/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/api/opentabs/brave_opentabs_api.h"

#include "base/bind.h"
#include "base/strings/sys_string_conversions.h"

#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#include "ios/chrome/browser/sync/session_sync_service_factory.h"
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
                      tabId:(NSInteger)tabId
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


- (void)getSyncedTabs:(void (^)(NSArray<IOSOpenTabNode*>* results))completion {
  // Getting SessionSyncService from BrowserState
  sync_sessions::SessionSyncService* syncService =
      SessionSyncServiceFactory::GetForBrowserState(_chromeBrowserState);

  // Getting SyncedSessions from SessionSyncService
  auto syncedSessions =
      std::make_unique<synced_sessions::SyncedSessions>(syncService);

  // Getting DistantTabSet from SyncSessions
  std::vector<synced_sessions::DistantTabsSet> displayedTabs;

  for (size_t s = 0; s < syncedSessions->GetSessionCount(); s++) {
    const synced_sessions::DistantSession* session =
        syncedSessions->GetSession(s);

    synced_sessions::DistantTabsSet distant_tabs;
    distant_tabs.session_tag = session->tag;
    displayedTabs.push_back(distant_tabs);
  }

  NSArray<IOSOpenTabNode*>* tabInfoList = [self onLoginsResult:std::move(displayedTabs)];

  completion(tabInfoList);

  // const synced_sessions::DistantTabsSet* exampleSet = &displayedTabs[0];

  // const synced_sessions::DistantTab* distantTab = exampleSet->filtered_tabs.value()[0];

  // std::cout << "Title of synced Tab " << distantTab->title;

  // Conversion to objc object OpenTabNode
}

- (NSArray<IOSOpenTabNode*>*)onLoginsResult:
      (std::vector<synced_sessions::DistantTabsSet>)distantTabSet {
  NSMutableArray<IOSOpenTabNode*>* tabNodeList = [[NSMutableArray alloc] init];

  int index = 0;

  for (const auto& distantTabSetItem : distantTabSet) {
    synced_sessions::DistantTab* distantTab = 
        distantTabSetItem.filtered_tabs.value()[index];

    IOSOpenTabNode* openTabNode = [[IOSOpenTabNode alloc]
                initWithURL:net::NSURLWithGURL(distantTab->virtual_url)
                      title:base::SysUTF16ToNSString(distantTab->title)
                      tabId: distantTab->tab_id.id()
                 sessionTag:base::SysUTF8ToNSString(distantTab->session_tag)];
    
    [tabNodeList addObject: openTabNode];

    ++index;
  }

  return tabNodeList;
}

@end