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

/// The main shields file install path (KVO compiliant)
@property(readonly, nullable) NSString* shieldsInstallPath;

/// Regional filter lists
@property(readonly, nullable)
    NSArray<AdblockFilterListCatalogEntry*>* regionalFilterLists;

/// Executed each time the main shields component is updated
@property(nonatomic, copy, nullable) void (^shieldsComponentReady)
    (NSString* _Nullable installPath);

/// Registers a filter list with the component updater and calls
/// `componentReady` each time the component is updated
- (void)registerFilterListComponent:(AdblockFilterListCatalogEntry*)entry
                 useLegacyComponent:(bool)useLegacyComponent
                     componentReady:(void (^)(NSString* _Nullable installPath))
                                        componentReady;

/// Unregisters a filter list with the component updater
- (void)unregisterFilterListComponent:(AdblockFilterListCatalogEntry*)entry
                   useLegacyComponent:(bool)useLegacyComponent;

- (instancetype)init NS_UNAVAILABLE;

@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_BRAVE_SHIELDS_ADBLOCK_SERVICE_H_
