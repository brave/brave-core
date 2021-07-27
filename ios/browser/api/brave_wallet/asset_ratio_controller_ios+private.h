/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_BRAVE_WALLET_ASSET_RATIO_CONTROLLER_IOS_PRIVATE_H_
#define BRAVE_IOS_BROWSER_API_BRAVE_WALLET_ASSET_RATIO_CONTROLLER_IOS_PRIVATE_H_

#import <Foundation/Foundation.h>

#import "brave/ios/browser/api/brave_wallet/asset_ratio_controller_ios.h"

NS_ASSUME_NONNULL_BEGIN

namespace brave_wallet {
namespace mojom {
class AssetRatioController;
}
}

@interface BraveWalletAssetRatioController (Private)
- (instancetype)initWithController:
    (brave_wallet::mojom::AssetRatioController*)controller;
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_BRAVE_WALLET_ASSET_RATIO_CONTROLLER_IOS_PRIVATE_H_
