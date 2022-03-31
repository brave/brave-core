/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_SYNC_DRIVER_BRAVE_SYNC_PROFILE_SERVICE_H_
#define BRAVE_IOS_BROWSER_API_SYNC_DRIVER_BRAVE_SYNC_PROFILE_SERVICE_H_

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

typedef NS_OPTIONS(NSUInteger, BraveSyncUserSelectableTypes) {
  BraveSyncUserSelectableTypes_NONE = 1ULL << 0,
  BraveSyncUserSelectableTypes_BOOKMARKS = 1ULL << 1,
  BraveSyncUserSelectableTypes_PREFERENCES = 1ULL << 2,
  BraveSyncUserSelectableTypes_PASSWORDS = 1ULL << 3,
  BraveSyncUserSelectableTypes_AUTOFILL = 1ULL << 4,
  BraveSyncUserSelectableTypes_THEMES = 1ULL << 5,
  BraveSyncUserSelectableTypes_HISTORY = 1ULL << 6,
  BraveSyncUserSelectableTypes_EXTENSIONS = 1ULL << 7,
  BraveSyncUserSelectableTypes_APPS = 1ULL << 8,
  BraveSyncUserSelectableTypes_READING_LIST = 1ULL << 9,
  BraveSyncUserSelectableTypes_TABS = 1ULL << 10,
  BraveSyncUserSelectableTypes_WIFI_CONFIGURATIONS = 1ULL << 11,
};

OBJC_EXPORT
@interface BraveSyncProfileServiceIOS : NSObject

- (instancetype)init NS_UNAVAILABLE;

// Whether all conditions are satisfied for Sync to start
// Does not imply that Sync is actually running
@property(nonatomic, readonly) bool isSyncFeatureActive;

/// Selectable Types for the Sync User
/// Used for opting in/out on iOS side
@property(nonatomic, assign) BraveSyncUserSelectableTypes userSelectedTypes;
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_SYNC_DRIVER_BRAVE_SYNC_PROFILE_SERVICE_H_
