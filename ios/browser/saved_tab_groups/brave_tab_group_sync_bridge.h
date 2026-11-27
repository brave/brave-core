// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_SAVED_TAB_GROUPS_BRAVE_TAB_GROUP_SYNC_BRIDGE_H_
#define BRAVE_IOS_BROWSER_SAVED_TAB_GROUPS_BRAVE_TAB_GROUP_SYNC_BRIDGE_H_

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

// One tab in a group being saved. A `syncTabID` of `0` is ignored.
NS_SWIFT_NAME(TabGroupSyncTab)
OBJC_EXPORT
@interface BraveTabGroupSyncTab : NSObject

@property(nonatomic, readonly) int32_t syncTabID;
@property(nonatomic, readonly, copy) NSString* title;
@property(nonatomic, readonly, copy, nullable) NSURL* URL;
@property(nonatomic, readonly) NSInteger index;

- (instancetype)initWithSyncTabID:(int32_t)syncTabID
                            title:(NSString*)title
                              URL:(nullable NSURL*)URL
                            index:(NSInteger)index NS_DESIGNATED_INITIALIZER;
- (instancetype)init NS_UNAVAILABLE;

@end

// Local tab group edits sent to the sync service.
//
// Does nothing while local observation is paused, or when tab groups are off.
// URLs are normalized before they are stored. `addGroupWithID` is given to the
// service immediately. The other methods wait until the service has
// initialized, because it drops them while it does not know the group.
NS_SWIFT_NAME(TabGroupSyncPush)
OBJC_EXPORT
@interface BraveTabGroupSyncPush : NSObject

+ (void)addGroupWithID:(NSUUID*)groupID
                 title:(NSString*)title
                 color:(int16_t)color
                  tabs:(NSArray<BraveTabGroupSyncTab*>*)tabs;

+ (void)updateVisualDataForGroupID:(NSUUID*)groupID
                             title:(NSString*)title
                             color:(int16_t)color
                         collapsed:(BOOL)collapsed;

+ (void)addTabWithSyncTabID:(int32_t)syncTabID
                    groupID:(NSUUID*)groupID
                      title:(NSString*)title
                        URL:(nullable NSURL*)URL
                      index:(NSInteger)index;

+ (void)removeTabWithSyncTabID:(int32_t)syncTabID groupID:(NSUUID*)groupID;

+ (void)moveTabWithSyncTabID:(int32_t)syncTabID
                     groupID:(NSUUID*)groupID
                     toIndex:(NSInteger)index;

+ (void)navigateTabWithSyncTabID:(int32_t)syncTabID
                         groupID:(NSUUID*)groupID
                           title:(NSString*)title
                             URL:(nullable NSURL*)URL;

// Removes the saved group. The last tab in a group uses this. Removing that
// tab leaves the group in the service.
+ (void)removeGroupWithID:(NSUUID*)groupID;

@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_SAVED_TAB_GROUPS_BRAVE_TAB_GROUP_SYNC_BRIDGE_H_
