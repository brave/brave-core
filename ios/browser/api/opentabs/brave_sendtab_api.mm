/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/api/opentabs/brave_sendtab_api.h"
// #include "brave/ios/browser/api/opentabs/brave_sendtab_observer.h"
// #include "brave/ios/browser/api/opentabs/sendtab_session_listener_ios.h"

#include "base/bind.h"
#include "base/strings/sys_string_conversions.h"

#include "components/send_tab_to_self/send_tab_to_self_model.h"
#include "components/send_tab_to_self/send_tab_to_self_sync_service.h"

#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#include "ios/chrome/browser/ui/recent_tabs/synced_sessions.h"
#include "ios/chrome/browser/sync/send_tab_to_self_sync_service_factory.h"

#include "ios/web/public/thread/web_thread.h"
#include "net/base/mac/url_conversions.h"
#include "url/gurl.h"

#pragma mark - BraveSendTabAPI

@interface BraveSendTabAPI () {
  // SendTab Sync Service is needed in order to send session data to different devices
  // And receive device information
  send_tab_to_self::SendTabToSelfSyncService* sendtab_sync_service_;
}
@end

@implementation BraveSendTabAPI

- (instancetype)initWithSyncService:(send_tab_to_self::SendTabToSelfSyncService*)syncService {
  if ((self = [super init])) {
    sendtab_sync_service_ = syncService;
  }
  return self;
}

- (void)dealloc {
  sendtab_sync_service_ = nullptr;
}

// - (id<SendTabSessionStateListener>)addObserver:(id<SendTabSessionStateObserver>)observer {
//   return [[<SendTabSessionListenerImpl alloc] init:observer
//                                        syncService:sync_service_];
// }

// - (void)removeObserver:(id<SendTabSessionStateListener>)observer {
//   [observer destroy];
// }

- (void)getListOfSyncedDevices {


}

@end
