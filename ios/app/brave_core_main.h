/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_APP_BRAVE_CORE_MAIN_H_
#define BRAVE_IOS_APP_BRAVE_CORE_MAIN_H_

#import <Foundation/Foundation.h>

@class BraveBookmarksAPI;
@class BraveHistoryAPI;
@class BraveSyncProfileServiceIOS;
@protocol BraveWalletKeyringController;
@protocol BraveWalletAssetRatioController;
@protocol BraveWalletEthJsonRpcController;

NS_ASSUME_NONNULL_BEGIN

typedef bool (^BraveCoreLogHandler)(int severity,
                                    NSString* file,
                                    int line,
                                    size_t messageStart,
                                    NSString* formattedMessage);

OBJC_EXPORT
@interface BraveCoreMain : NSObject

@property(nullable, nonatomic, readonly) BraveBookmarksAPI* bookmarksAPI;

@property(nullable, nonatomic, readonly) BraveHistoryAPI* historyAPI;

@property(nullable, nonatomic, readonly)
    BraveSyncProfileServiceIOS* syncProfileService;

+ (void)setLogHandler:(nullable BraveCoreLogHandler)logHandler;

- (instancetype)init;

- (instancetype)initWithSyncServiceURL:(NSString*)syncServiceURL;

- (void)scheduleLowPriorityStartupTasks;

- (void)setUserAgent:(NSString*)userAgent;

@property(nonatomic, readonly) id<BraveWalletKeyringController>
    keyringController;

@property(nonatomic, readonly) id<BraveWalletAssetRatioController>
    assetRatioController;

@property(nonatomic, readonly) id<BraveWalletEthJsonRpcController>
    ethJsonRpcController;

@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_APP_BRAVE_CORE_MAIN_H_
