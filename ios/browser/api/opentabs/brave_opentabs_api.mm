/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/api/opentabs/brave_opentabs_api.h"

#include "base/memory/raw_ptr.h"
#include "base/strings/sys_string_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/ios/browser/api/opentabs/brave_opentabs_observer.h"
#include "brave/ios/browser/api/opentabs/opentabs_session_listener_ios.h"
#include "components/sync/service/sync_service.h"
#include "components/sync_device_info/device_info.h"
#include "components/sync_sessions/open_tabs_ui_delegate.h"
#include "components/sync_sessions/session_sync_service.h"
#include "ios/chrome/browser/synced_sessions/model/synced_sessions.h"
#include "net/base/apple/url_conversions.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

SyncDeviceFormFactor const SyncDeviceFormFactorUnknown =
    static_cast<NSInteger>(syncer::DeviceInfo::FormFactor::kUnknown);
SyncDeviceFormFactor const SyncDeviceFormFactorDesktop =
    static_cast<NSInteger>(syncer::DeviceInfo::FormFactor::kDesktop);
SyncDeviceFormFactor const SyncDeviceFormFactorPhone =
    static_cast<NSInteger>(syncer::DeviceInfo::FormFactor::kPhone);
SyncDeviceFormFactor const SyncDeviceFormFactorTablet =
    static_cast<NSInteger>(syncer::DeviceInfo::FormFactor::kTablet);

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
  IOSOpenDistantTab* openDistantTabCopy =
      [[[self class] allocWithZone:zone] init];

  if (openDistantTabCopy) {
    openDistantTabCopy.url = self.url;
    openDistantTabCopy.title = self.title;
    openDistantTabCopy.tabId = self.tabId;
    openDistantTabCopy.sessionTag = self.sessionTag;
  }

  return openDistantTabCopy;
}

- (void)updateOpenDistantTab:(NSURL*)url title:(NSString*)title {
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
            deviceFormFactor:(SyncDeviceFormFactor)deviceFormFactor {
  if ((self = [super init])) {
    self.name = name;
    self.sessionTag = sessionTag;
    self.modifiedTime = modifiedTime;
    self.deviceFormFactor = deviceFormFactor;
    self.tabs = [[NSMutableArray alloc] init];
  }

  return self;
}

- (id)copyWithZone:(NSZone*)zone {
  IOSOpenDistantSession* openDistantSession =
      [[[self class] allocWithZone:zone] init];

  if (openDistantSession) {
    openDistantSession.name = [self.name copy];
    openDistantSession.sessionTag = [self.sessionTag copy];
    openDistantSession.modifiedTime = [self.modifiedTime copy];
    openDistantSession.deviceFormFactor = self.deviceFormFactor;
    openDistantSession.tabs = [self.tabs copy];
  }

  return openDistantSession;
}

- (void)updateOpenDistantSessionModified:(NSDate*)modifiedTime {
  [self setModifiedTime:modifiedTime];
}

// Helper to extract the relevant content from a SessionTab and add it to a
// DistantSession.
- (void)addTab:(const sessions::SessionTab&)session_tab
    sessionTag:(const std::string&)session_tag {
  if (session_tab.navigations.size() > 0) {
    // Retrieve tab index
    int index = session_tab.current_navigation_index;
    if (index < 0) {
      index = 0;
    }

    if (index > static_cast<int>(session_tab.navigations.size()) - 1) {
      index = session_tab.navigations.size() - 1;
    }

    // Retrieve tab title and url
    const sessions::SerializedNavigationEntry* navigation =
        &session_tab.navigations[index];
    std::string tab_title = base::UTF16ToUTF8(navigation->title());
    GURL virtual_url = navigation->virtual_url();

    if (tab_title.empty()) {
      tab_title = navigation->virtual_url().spec();
    }

    // Store the tab
    IOSOpenDistantTab* distant_tab = [[IOSOpenDistantTab alloc]
        initWithURL:net::NSURLWithGURL(virtual_url)
              title:base::SysUTF8ToNSString(tab_title)
              tabId:session_tab.tab_id.id()
         sessionTag:base::SysUTF8ToNSString(session_tag)];
    [(NSMutableArray*)_tabs addObject:distant_tab];
  }
}

@end

#pragma mark - BraveOpenTabsAPI

@interface BraveOpenTabsAPI () {
  // SyncService is needed in order to observe sync changes
  raw_ptr<syncer::SyncService> sync_service_;

  // Session Sync Service is needed in order to receive session details from
  // different instances
  raw_ptr<sync_sessions::SessionSyncService> session_sync_service_;
}
@end

@implementation BraveOpenTabsAPI

- (instancetype)initWithSyncService:(syncer::SyncService*)syncService
                 sessionSyncService:
                     (sync_sessions::SessionSyncService*)sessionSyncService {
  if ((self = [super init])) {
    sync_service_ = syncService;
    session_sync_service_ = sessionSyncService;
  }
  return self;
}

- (void)dealloc {
  sync_service_ = nullptr;
  session_sync_service_ = nullptr;
}

- (id<OpenTabsSessionStateListener>)addObserver:
    (id<OpenTabsSessionStateObserver>)observer {
  return [[OpenTabsSessionListenerImpl alloc] init:observer
                                       syncService:sync_service_];
}

- (void)removeObserver:(id<OpenTabsSessionStateListener>)observer {
  [observer destroy];
}

- (NSArray<IOSOpenDistantSession*>*)getSyncedSessions {
  // Taken from: ios/chrome/browser/ui/recent_tabs/synced_sessions.mm
  // but modified to allow us to retrieve `device_type` via
  // GetDeviceFormFactor()

  NSMutableArray<IOSOpenDistantSession*>* sessions_list =
      [[NSMutableArray alloc] init];

  DCHECK(session_sync_service_);
  // Reload Sync open tab sessions.
  sync_sessions::OpenTabsUIDelegate* open_tabs_delegate =
      session_sync_service_->GetOpenTabsUIDelegate();
  if (open_tabs_delegate) {
    // Iterating through all remote sessions, then retrieving the tabs to
    // display to the user.
    std::vector<raw_ptr<const sync_sessions::SyncedSession, VectorExperimental>>
        sessions;
    open_tabs_delegate->GetAllForeignSessions(&sessions);

    for (const sync_sessions::SyncedSession* session : sessions) {
      // Create a distant session
      IOSOpenDistantSession* distant_session = [[IOSOpenDistantSession alloc]
              initWithName:base::SysUTF8ToNSString(session->GetSessionName())
                sessionTag:base::SysUTF8ToNSString(session->GetSessionTag())
               dateCreated:session->GetModifiedTime().ToNSDate()
          deviceFormFactor:static_cast<SyncDeviceFormFactor>(
                               session->GetDeviceFormFactor())];

      // Add tabs to the distant session
      std::vector<const sessions::SessionTab*> open_tabs;
      open_tabs_delegate->GetForeignSessionTabs(session->GetSessionTag(),
                                                &open_tabs);
      for (const sessions::SessionTab* session_tab : open_tabs) {
        [distant_session addTab:*session_tab
                     sessionTag:session->GetSessionTag()];
      }

      // Don't display sessions with no tabs.
      if ([distant_session.tabs count] > 0) {
        [sessions_list addObject:distant_session];
      }
    }

    // Sort Sessions by Time
    [sessions_list sortedArrayUsingComparator:^NSComparisonResult(
                       IOSOpenDistantSession* a, IOSOpenDistantSession* b) {
      return [[a modifiedTime] compare:[b modifiedTime]];
    }];
  }

  return [sessions_list copy];
}

- (void)deleteSyncedSession:(NSString*)sessionTag {
  session_sync_service_->GetOpenTabsUIDelegate()->DeleteForeignSession(
      base::SysNSStringToUTF8(sessionTag));
}

@end
