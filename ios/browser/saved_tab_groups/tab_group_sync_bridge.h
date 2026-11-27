// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_SAVED_TAB_GROUPS_TAB_GROUP_SYNC_BRIDGE_H_
#define BRAVE_IOS_BROWSER_SAVED_TAB_GROUPS_TAB_GROUP_SYNC_BRIDGE_H_

#import <Foundation/Foundation.h>

@protocol BraveTabGroupSyncLocalModel;

namespace tab_groups {
class TabGroupSyncService;
}  // namespace tab_groups

namespace brave {

// Observes `service` so local groups are added before the sync delegate's
// startup close. Call this before `SetTabGroupSyncDelegate`: the service
// notifies observers in registration order.
void InstallTabGroupSyncBridge(tab_groups::TabGroupSyncService* service,
                               id<BraveTabGroupSyncLocalModel> model);

}  // namespace brave

#endif  // BRAVE_IOS_BROWSER_SAVED_TAB_GROUPS_TAB_GROUP_SYNC_BRIDGE_H_
