/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_BRAVE_WALLET_BRAVE_WALLET_API_PRIVATE_H_
#define BRAVE_IOS_BROWSER_API_BRAVE_WALLET_BRAVE_WALLET_API_PRIVATE_H_

#import <Foundation/Foundation.h>

#include "brave/ios/browser/api/brave_wallet/brave_wallet_api.h"

NS_ASSUME_NONNULL_BEGIN

class ProfileIOS;

@interface BraveWalletAPI (Private)
- (instancetype)initWithBrowserState:(ProfileIOS*)profile;
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_BRAVE_WALLET_BRAVE_WALLET_API_PRIVATE_H_
