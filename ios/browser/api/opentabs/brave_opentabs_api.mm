/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/api/opentabs/brave_opentabs_api.h"
#include "brave/ios/browser/api/opentabs/brave_opentabs_observer.h"
#include "brave/ios/browser/api/opentabs/opentabs_session_listener_ios.h"

#include "base/strings/sys_string_conversions.h"
#include "components/sync/driver/sync_service.h"
#include "components/sync_sessions/session_sync_service.h"
#include "ios/chrome/browser/ui/recent_tabs/synced_sessions.h"
#include "net/base/mac/url_conversions.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

SyncDeviceType const SyncDeviceTypeUnset =
    static_cast<NSInteger>(sync_pb::SyncEnums::DeviceType::SyncEnums_DeviceType_TYPE_UNSET);
SyncDeviceType const SyncDeviceTypeWin =
    static_cast<NSInteger>(sync_pb::SyncEnums::DeviceType::SyncEnums_DeviceType_TYPE_WIN);
SyncDeviceType const SyncDeviceTypeMac =
    static_cast<NSInteger>(sync_pb::SyncEnums::DeviceType::SyncEnums_DeviceType_TYPE_MAC);
SyncDeviceType const SyncDeviceTypeLinux =
    static_cast<NSInteger>(sync_pb::SyncEnums::DeviceType::SyncEnums_DeviceType_TYPE_LINUX);
SyncDeviceType const SyncDeviceTypeCros =
    static_cast<NSInteger>(sync_pb::SyncEnums::DeviceType::SyncEnums_DeviceType_TYPE_CROS);
SyncDeviceType const SyncDeviceTypeOther =
    static_cast<NSInteger>(sync_pb::SyncEnums::DeviceType::SyncEnums_DeviceType_TYPE_OTHER);
SyncDeviceType const SyncDeviceTypePhone =
    static_cast<NSInteger>(sync_pb::SyncEnums::DeviceType::SyncEnums_DeviceType_TYPE_PHONE);
SyncDeviceType const SyncDeviceTypeTablet =
    static_cast<NSInteger>(sync_pb::SyncEnums::DeviceType::SyncEnums_DeviceType_TYPE_TABLET);

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
  IOSOpenDistantSession* openDistantSession =
      [[[self class] allocWithZone:zone] init];

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
  // SyncService is needed in order to observe sync changes
  syncer::SyncService* sync_service_;

  // Session Sync Service is needed in order to receive session details from
  // different instances
  sync_sessions::SessionSyncService* session_sync_service_;
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
  // Getting SyncedSessions from SessionSyncService
  auto syncedSessions =
      std::make_unique<synced_sessions::SyncedSessions>(session_sync_service_);

  NSMutableArray<IOSOpenDistantSession*>* distantSessionList =
      [[NSMutableArray alloc] init];

  // Traversing Sync Sessions to fetch list of Distant Sessions
  for (size_t sessionIndex = 0;
       sessionIndex < syncedSessions->GetSessionCount(); sessionIndex++) {
    const synced_sessions::DistantSession* session =
        syncedSessions->GetSession(sessionIndex);

    // Create a DistantTab List information  for each Distant Session
    NSArray<IOSOpenDistantTab*>* distantTabs =
        [self onDistantTabResults:session->tabs];

    IOSOpenDistantSession* distantSession = [[IOSOpenDistantSession alloc]
        initWithName:base::SysUTF8ToNSString(session->name)
          sessionTag:base::SysUTF8ToNSString(session->tag)
         dateCreated:session->modified_time.ToNSDate()
          deviceType:static_cast<SyncDeviceType>(session->device_type)
                tabs:distantTabs];
    [distantSessionList addObject:distantSession];
  }

  return [distantSessionList copy];
}

- (NSArray<IOSOpenDistantTab*>*)onDistantTabResults:
    (const std::vector<std::unique_ptr<synced_sessions::DistantTab>>&)
        distantTabList {
  NSMutableArray<IOSOpenDistantTab*>* distantTabs =
      [[NSMutableArray alloc] init];

  for (const auto& tab : distantTabList) {
    IOSOpenDistantTab* distantTab = [[IOSOpenDistantTab alloc]
        initWithURL:net::NSURLWithGURL(tab->virtual_url)
              title:base::SysUTF16ToNSString(tab->title)
              tabId:tab->tab_id.id()
         sessionTag:base::SysUTF8ToNSString(tab->session_tag)];

    [distantTabs addObject:distantTab];
  }

  return [distantTabs copy];
}

@end
