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
@interface BraveSyncAPI : NSObject
- (instancetype)init;

- (bool)setSyncEnabled:(bool)enabled;

- (bool)resetSync;

- (NSString *)getOrCreateSyncCode;

// returns false is sync is already configured or if the sync code is invalid
- (bool)setSyncCode:(NSString *)syncCode;

- (nullable UIImage *)getQRCodeImage:(CGSize)size;

- (nullable NSString *)getDeviceListJSON;

// has the user enabled sync
- (bool)isSyncEnabled;

// is sync configured and running
- (bool)isSyncFeatureActive;
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_SYNC_BRAVE_SYNC_API_H_
