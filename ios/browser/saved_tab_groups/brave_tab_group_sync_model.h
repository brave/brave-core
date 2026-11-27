// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_SAVED_TAB_GROUPS_BRAVE_TAB_GROUP_SYNC_MODEL_H_
#define BRAVE_IOS_BROWSER_SAVED_TAB_GROUPS_BRAVE_TAB_GROUP_SYNC_MODEL_H_

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

// Local tab groups the sync delegate reads. Swift installs one before the
// profile is created. It does not ask for the sync service.
NS_SWIFT_NAME(TabGroupSyncLocalModel)
@protocol BraveTabGroupSyncLocalModel <NSObject>

// Every saved group, in display order.
- (NSArray<NSUUID*>*)localGroupIDs;

- (NSString*)titleForGroupID:(NSUUID*)groupID;
// Chromium `TabGroupColorId`. Unknown values are grey.
- (int16_t)colorForGroupID:(NSUUID*)groupID;
- (BOOL)isCollapsedForGroupID:(NSUUID*)groupID;

// Ordered `LocalTabID`s. `0` is omitted.
- (NSArray<NSNumber*>*)syncTabIDsForGroupID:(NSUUID*)groupID;
- (NSString*)titleForSyncTabID:(int32_t)syncTabID;
- (nullable NSURL*)URLForSyncTabID:(int32_t)syncTabID;

// Selected regular tabs across windows. `0` is omitted.
- (NSArray<NSNumber*>*)selectedSyncTabIDs;

// `YES` enters a pause and `NO` leaves one. Pauses nest.
- (void)setLocalObservationPaused:(BOOL)paused
    NS_SWIFT_NAME(setLocalObservationPaused(_:));
- (BOOL)isLocalObservationPaused NS_SWIFT_NAME(isLocalObservationPaused());

@end

NS_SWIFT_NAME(TabGroupSyncLocalModels)
@interface BraveTabGroupSyncLocalModels : NSObject

+ (void)setSharedModel:(nullable id<BraveTabGroupSyncLocalModel>)model;
+ (nullable id<BraveTabGroupSyncLocalModel>)sharedModel;

@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_SAVED_TAB_GROUPS_BRAVE_TAB_GROUP_SYNC_MODEL_H_
