/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_SYNC_BRAVE_SYNC_API_H_
#define BRAVE_IOS_BROWSER_API_SYNC_BRAVE_SYNC_API_H_

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

NS_ASSUME_NONNULL_BEGIN

OBJC_EXPORT
@interface BraveSyncDeviceObserver: NSObject
- (instancetype)initWithCallback:(void(^)())onDeviceInfoChanged;
@end

OBJC_EXPORT
@interface BraveSyncServiceObserver: NSObject
- (instancetype)initWithCallback:(void(^)())onSyncServiceStateChanged;
@end

OBJC_EXPORT
@interface BraveSyncAPI : NSObject

@property(class, readonly, strong) BraveSyncAPI *sharedSyncAPI NS_SWIFT_NAME(shared);
@property(nonatomic) bool syncEnabled;
@property(nonatomic, readonly) bool isSyncFeatureActive;

- (bool)resetSync;

- (bool)isValidSyncCode:(NSString *)syncCode;

- (NSString *)getSyncCode;

// returns false is sync is already configured or if the sync code is invalid
- (bool)setSyncCode:(NSString *)syncCode;

- (NSString *)syncCodeFromHexSeed:(NSString *)hexSeed;

- (nullable UIImage *)getQRCodeImage:(CGSize)size;

- (nullable NSString *)getDeviceListJSON;
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_SYNC_BRAVE_SYNC_API_H_
