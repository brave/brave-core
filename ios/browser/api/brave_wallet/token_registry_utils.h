/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_BRAVE_WALLET_TOKEN_REGISTRY_UTILS_H_
#define BRAVE_IOS_BROWSER_API_BRAVE_WALLET_TOKEN_REGISTRY_UTILS_H_

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

OBJC_EXPORT
NS_SWIFT_NAME(BraveWallet.TokenRegistryUtils)
@interface BraveWalletTokenRegistryUtils : NSObject

/// The file path that ERC20Token logos will be downloaded to. If the component
/// has not been downloaded yet, this will be `nil`.
@property(nonatomic, class, nullable, readonly) NSURL* tokenLogoBaseURL;

- (instancetype)init NS_UNAVAILABLE;

@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_BRAVE_WALLET_TOKEN_REGISTRY_UTILS_H_
