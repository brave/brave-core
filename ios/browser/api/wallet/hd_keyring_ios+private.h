/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_WALLET_HD_KEYRING_IOS_PRIVATE_H_
#define BRAVE_IOS_BROWSER_API_WALLET_HD_KEYRING_IOS_PRIVATE_H_

#import <Foundation/Foundation.h>

#import "brave/ios/browser/api/wallet/hd_keyring_ios.h"

NS_ASSUME_NONNULL_BEGIN

namespace brave_wallet {
class HDKeyring;
}

@interface HDKeyringIOS (Private)
- (instancetype)initWithKeyring:(brave_wallet::HDKeyring*)keyring;
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_WALLET_HD_KEYRING_IOS_PRIVATE_H_
