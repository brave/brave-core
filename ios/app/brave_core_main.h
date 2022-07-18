/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_APP_BRAVE_CORE_MAIN_H_
#define BRAVE_IOS_APP_BRAVE_CORE_MAIN_H_

#import <Foundation/Foundation.h>

@class BraveBookmarksAPI;
@class BraveHistoryAPI;
@class BravePasswordAPI;
@class BraveSyncAPI;
@class BraveSyncProfileServiceIOS;
@class BraveStats;
@class AdblockFilterList;

@protocol BraveWalletBlockchainRegistry;
@protocol BraveWalletEthereumProvider;
@protocol BraveWalletProviderDelegate;
@protocol BraveWalletSolanaProvider;

typedef NS_ENUM(NSInteger, BraveWalletCoinType);

NS_ASSUME_NONNULL_BEGIN

typedef NSString* BraveCoreSwitch NS_STRING_ENUM;
/// Overrides the component updater source. Defaults to the CI provided value
///
/// Expected value: url-source={url}
OBJC_EXPORT const BraveCoreSwitch BraveCoreSwitchComponentUpdater;
/// Overrides Chromium VLOG verbosity. Defaults to only printing from folders
/// existing within a `brave` subfolder up to level 5.
///
/// Expected value: {folder-expression}={level}
OBJC_EXPORT const BraveCoreSwitch BraveCoreSwitchVModule;
/// Overrides the sync service base URL. Defaults to the CI provided value
///
/// Expected value: A URL string
OBJC_EXPORT const BraveCoreSwitch BraveCoreSwitchSyncURL;

typedef bool (^BraveCoreLogHandler)(int severity,
                                    NSString* file,
                                    int line,
                                    size_t messageStart,
                                    NSString* formattedMessage);

OBJC_EXPORT
@interface BraveCoreMain : NSObject

@property(nonatomic, readonly) BraveBookmarksAPI* bookmarksAPI;

@property(nonatomic, readonly) BraveHistoryAPI* historyAPI;

@property(nonatomic, readonly) BraveSyncAPI* syncAPI;

@property(nonatomic, readonly) BraveSyncProfileServiceIOS* syncProfileService;

@property(nonatomic, readonly) BravePasswordAPI* passwordAPI;

+ (void)setLogHandler:(nullable BraveCoreLogHandler)logHandler;

- (instancetype)init NS_UNAVAILABLE;

- (instancetype)initWithUserAgent:(NSString*)userAgent;

- (instancetype)initWithUserAgent:(NSString*)userAgent
               additionalSwitches:(NSDictionary<BraveCoreSwitch, NSString*>*)
                                      additionalSwitches;

- (void)scheduleLowPriorityStartupTasks;

@property(class, readonly) id<BraveWalletBlockchainRegistry> blockchainRegistry;

- (nullable id<BraveWalletEthereumProvider>)
    ethereumProviderWithDelegate:(id<BraveWalletProviderDelegate>)delegate
               isPrivateBrowsing:(bool)isPrivateBrowsing;

- (nullable id<BraveWalletSolanaProvider>)
    solanaProviderWithDelegate:(id<BraveWalletProviderDelegate>)delegate
             isPrivateBrowsing:(bool)isPrivateBrowsing;

- (NSString*)providerScriptForCoinType:(BraveWalletCoinType)coinType;

@property(readonly) BraveStats* braveStats;

/// The main shields file install path (KVO compiliant)
@property(readonly, nullable) NSString* shieldsInstallPath;

/// Regional filter lists
@property(readonly, nullable) NSArray<AdblockFilterList*>* regionalFilterLists;

/// Executed each time the main shields component is updated
@property(nonatomic, copy, nullable) void (^shieldsComponentReady)
    (NSString* _Nullable installPath);

/// Registers a filter list with the component updater and calls
/// `componentReady` each time the component is updated
- (void)registerFilterListComponent:(AdblockFilterList*)filterList
                     componentReady:(void (^)(AdblockFilterList* filterList,
                                              NSString* _Nullable installPath))
                                        componentReady;

/// Unregisters a filter list with the component updater
- (void)unregisterFilterListComponent:(AdblockFilterList*)filterList;

@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_APP_BRAVE_CORE_MAIN_H_
