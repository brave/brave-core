/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_BRAVE_SHIELDS_ADBLOCK_SERVICE_H_
#define BRAVE_IOS_BROWSER_API_BRAVE_SHIELDS_ADBLOCK_SERVICE_H_

#import <Foundation/Foundation.h>

@class AdblockFilterListCatalogEntry;

NS_ASSUME_NONNULL_BEGIN

OBJC_EXPORT
@interface AdblockService : NSObject

/// Registers the filter list catalog component and calls `componentReady` each
/// time the component is updated
- (void)registerFilterListCatalogComponent:
    (void (^)(NSArray<AdblockFilterListCatalogEntry*>* filterLists))
        componentReady;

/// Registers the resources component and calls `componentReady`
/// each time the component is updated
- (void)registerResourceComponent:
    (void (^)(NSString* _Nullable installPath))componentReady;

/// Registers a filter list with the component updater and calls
/// `componentReady` each time the component is updated
- (void)registerFilterListComponent:(AdblockFilterListCatalogEntry*)entry
                     componentReady:(void (^)(NSString* _Nullable installPath))
                                        componentReady;

/// Unregisters a filter list with the component updater
- (void)unregisterFilterListComponent:(AdblockFilterListCatalogEntry*)entry;

- (instancetype)init NS_UNAVAILABLE;

@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_BRAVE_SHIELDS_ADBLOCK_SERVICE_H_
