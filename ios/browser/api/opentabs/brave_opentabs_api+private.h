/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_OPENTABS_BRAVE_OPENTABS_API_PRIVATE_H_
#define BRAVE_IOS_BROWSER_API_OPENTABS_BRAVE_OPENTABS_API_PRIVATE_H_

#import <Foundation/Foundation.h>

#include "brave/ios/browser/api/opentabs/brave_opentabs_api.h"

NS_ASSUME_NONNULL_BEGIN

namespace syncer {
class SyncService;
}  // namespace syncer

namespace sync_sessions {
class SessionSyncService;
}  // namespace sync_sessions

@interface BraveOpenTabsAPI (Private)
- (instancetype)initWithSyncService:(syncer::SyncService*)syncService
                 sessionSyncService:
                     (sync_sessions::SessionSyncService*)sessionSyncService;
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_OPENTABS_BRAVE_OPENTABS_API_PRIVATE_H_
