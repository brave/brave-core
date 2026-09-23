/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#import <Foundation/Foundation.h>

#ifndef BRAVE_IOS_BROWSER_API_OPENTABS_OPENTABS_SESSION_LISTENER_IOS_H_
#define BRAVE_IOS_BROWSER_API_OPENTABS_OPENTABS_SESSION_LISTENER_IOS_H_

#include "base/memory/raw_ptr.h"
#include "brave/ios/browser/api/opentabs/brave_opentabs_observer.h"
#include "components/sync/service/sync_service.h"
#include "components/sync/service/sync_service_observer.h"

@interface OpenTabsSessionListenerImpl : NSObject <OpenTabsSessionStateListener>
- (instancetype)init:(id<OpenTabsSessionStateObserver>)observer
         syncService:(void*)service;
@end

namespace brave {
namespace ios {

class OpenTabsSessionListenerIOS : public syncer::SyncServiceObserver {
 public:
  explicit OpenTabsSessionListenerIOS(id<OpenTabsSessionStateObserver> observer,
                                      syncer::SyncService* service);
  ~OpenTabsSessionListenerIOS() override;

 private:
  // OpenTabsSessionListener implementation.
  void OnStateChanged(syncer::SyncService* sync) override;
  void OnSyncCycleCompleted(syncer::SyncService* sync) override;
  void OnSyncShutdown(syncer::SyncService* sync) override;

  id<OpenTabsSessionStateObserver> observer_;
  raw_ptr<syncer::SyncService> service_;
};

}  // namespace ios
}  // namespace brave

#endif  // BRAVE_IOS_BROWSER_API_OPENTABS_OPENTABS_SESSION_LISTENER_IOS_H_
