// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_API_BRAVE_SHIELDS_ADBLOCK_SERVICE_H_
#define BRAVE_IOS_BROWSER_API_BRAVE_SHIELDS_ADBLOCK_SERVICE_H_

#import <Foundation/Foundation.h>

@class AdblockFilterListCatalogEntry;

NS_ASSUME_NONNULL_BEGIN

OBJC_EXPORT
@interface AdblockService : NSObject

/// Returns the filter lists that are available for the current platform
@property(nonatomic, readonly)
    NSArray<AdblockFilterListCatalogEntry*>* filterListCatalogEntries;
/// Returns the path to the resources file if it is available
@property(nonatomic, readonly, nullable) NSURL* resourcesPath;

/// Enable or disable a filter list given by its UUID
- (void)enableFilterListForUUID:(NSString*)uuid isEnabled:(bool)isEnabled;
/// Returns whether a filter list is available for the given UUID
- (bool)isFilterListAvailableForUUID:(NSString*)uuid;
/// Returns whether a filter list is enabled for the given UUID
- (bool)isFilterListEnabledForUUID:(NSString*)uuid;
/// Returns the install path for a filter list given by its UUID
- (nullable NSURL*)installPathForFilterListUUID:(NSString*)uuid;

/// Listen to downloaded version changes of filter lists
- (void)registerFilterListChanges:(void (^)(bool isDefaultEngine))callback;

/// Listen to downloaded version changes of the filter list catalog
- (void)registerCatalogChanges:(void (^)())callback;

/// Listen to downloaded version changes of resources
- (void)registerResourcesChanges:(void (^)(NSString* resourcesJSON))callback;

/// Update the filter lists
- (void)updateFilterLists:(void (^)(bool))callback;

- (instancetype)init NS_UNAVAILABLE;

@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_BRAVE_SHIELDS_ADBLOCK_SERVICE_H_
