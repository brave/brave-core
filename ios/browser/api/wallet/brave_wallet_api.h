/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_WALLET_BRAVE_WALLET_API_H_
#define BRAVE_IOS_BROWSER_API_WALLET_BRAVE_WALLET_API_H_

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@class KeyringControllerIOS;

OBJC_EXPORT
@interface BraveWalletAPI : NSObject

@property(nonatomic, readonly) KeyringControllerIOS* keyringController;

- (instancetype)init NS_UNAVAILABLE;

@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_WALLET_BRAVE_WALLET_API_H_
