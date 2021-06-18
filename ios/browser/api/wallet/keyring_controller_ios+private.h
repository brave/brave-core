/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_WALLET_KEYRING_CONTROLLER_IOS_PRIVATE_H_
#define BRAVE_IOS_BROWSER_API_WALLET_KEYRING_CONTROLLER_IOS_PRIVATE_H_

#import <Foundation/Foundation.h>

#import "brave/ios/browser/api/wallet/keyring_controller_ios.h"

NS_ASSUME_NONNULL_BEGIN

namespace brave_wallet {
class KeyringController;
}

@interface KeyringControllerIOS (Private)
- (instancetype)initWithController:(brave_wallet::KeyringController*)controller;
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_WALLET_KEYRING_CONTROLLER_IOS_PRIVATE_H_
