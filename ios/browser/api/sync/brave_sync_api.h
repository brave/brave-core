/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_SYNC_BRAVE_SYNC_API_H_
#define BRAVE_IOS_BROWSER_API_SYNC_BRAVE_SYNC_API_H_

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

@interface BraveSyncAPI : NSObject
- (instancetype)init;

- (void)setSyncEnabled:(bool)enabled;

- (NSString *)getSyncCode;

- (bool)setSyncCode:(NSString *)sync_code;

- (UIImage *)getQRCodeImage:(CGSize)size;

- (NSString *)getDeviceListJSON;

@end

#endif  // BRAVE_IOS_BROWSER_API_SYNC_BRAVE_SYNC_API_H_
