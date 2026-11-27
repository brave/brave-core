// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_SAVED_TAB_GROUPS_BRAVE_TAB_GROUP_ID_H_
#define BRAVE_IOS_BROWSER_SAVED_TAB_GROUPS_BRAVE_TAB_GROUP_ID_H_

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

// Sync ids for a tab group. The token words are the group's UUID bytes: the
// first 8 bytes are `high` as a big-endian uint64, and the last 8 are `low`.
NS_SWIFT_NAME(TabGroupSyncID)
OBJC_EXPORT
@interface BraveTabGroupID : NSObject

// A new `LocalTabID`. Never zero.
+ (int32_t)makeSyncTabID;

// Writes the `TabGroupId` token words for `groupID` to `high` and `low` and
// returns YES. Returns NO and leaves the outputs untouched if any argument is
// nil.
+ (BOOL)tokenForGroupID:(NSUUID*)groupID
                   high:(uint64_t*)high
                    low:(uint64_t*)low;

// The UUID whose bytes are the token words, big-endian.
+ (NSUUID*)groupIDForTokenHigh:(uint64_t)high low:(uint64_t)low;

- (instancetype)init NS_UNAVAILABLE;

@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_SAVED_TAB_GROUPS_BRAVE_TAB_GROUP_ID_H_
