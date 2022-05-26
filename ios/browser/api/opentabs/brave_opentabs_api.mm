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

#pragma mark - IOSOpenDistantTab

@implementation IOSOpenDistantTab

- (instancetype)initWithURL:(NSURL*)url
                      title:(nullable NSString*)title
                      tabId:(NSInteger)tabId
                 sessionTag:(NSString*)sessionTag {
  if ((self = [super init])) {
    self.url = url;
    self.title = title;
    self.tabId = tabId;
    self.sessionTag = sessionTag;
  }

  return self;
}

- (id)copyWithZone:(NSZone*)zone {
  IOSOpenDistantTab* openDistantTabCopy = [[[self class] allocWithZone:zone] init];

  if (openDistantTabCopy) {
    openDistantTabCopy.url = self.url;
    openDistantTabCopy.title = self.title;
    openDistantTabCopy.tabId = self.tabId;
    openDistantTabCopy.sessionTag = self.sessionTag;
  }

  return openDistantTabCopy;
}

- (void)updateOpenDistantTab:(NSURL*)url
                       title:(NSString*)title {
  [self setUrl:url];

  if ([title length] != 0) {
    [self setTitle:title];
  }
}

@end

#pragma mark - IOSOpenDistantSession

@implementation IOSOpenDistantSession

- (instancetype)initWithName:(nullable NSString*)name
                  sessionTag:(NSString*)sessionTag
                 dateCreated:(nullable NSDate*)modifiedTime
                  deviceType:(SyncDeviceType)deviceType
                        tabs:(NSArray<IOSOpenDistantTab*>*)tabs {
  if ((self = [super init])) {
    self.name = name;
    self.sessionTag = sessionTag;
    self.modifiedTime = modifiedTime;
    self.deviceType = deviceType;
    self.tabs = tabs;
  }

  return self;
}

- (id)copyWithZone:(NSZone*)zone {
  IOSOpenDistantSession* openDistantSession = [[[self class] allocWithZone:zone] init];

  if (openDistantSession) {
    openDistantSession.name = self.name;
    openDistantSession.sessionTag = self.sessionTag;
    openDistantSession.modifiedTime = self.modifiedTime;
    openDistantSession.deviceType = self.deviceType;
    openDistantSession.tabs = self.tabs;
  }

  return openDistantSession;
}

- (void)updateOpenDistantSessionModified:(NSDate*)modifiedTime {
    [self setModifiedTime:modifiedTime];
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


- (void)getSyncedSessions:(void (^)(NSArray<IOSOpenDistantSession*>*))completion {
  // Getting SessionSyncService from BrowserState
  sync_sessions::SessionSyncService* syncService =
      SessionSyncServiceFactory::GetForBrowserState(_chromeBrowserState);

  // Getting SyncedSessions from SessionSyncService
  auto syncedSessions =
      std::make_unique<synced_sessions::SyncedSessions>(syncService);

  // Getting DistantTabSet from SyncSessions
  std::vector<synced_sessions::DistantTabsSet> displayedTabs;

  NSMutableArray<IOSOpenDistantSession*>* distantSessionList = [[NSMutableArray alloc] init];

  for (size_t s = 0; s < syncedSessions->GetSessionCount(); s++) {
    const synced_sessions::DistantSession* session =
        syncedSessions->GetSession(s);

    // TODO: Fill up the session detail
    synced_sessions::DistantTabsSet distant_tabs;
    distant_tabs.session_tag = session->tag;
    displayedTabs.push_back(distant_tabs);
  }

  completion([distantSessionList copy]);
}

@end