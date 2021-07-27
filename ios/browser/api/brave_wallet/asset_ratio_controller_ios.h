/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_BRAVE_WALLET_ASSET_RATIO_CONTROLLER_IOS_H_
#define BRAVE_IOS_BROWSER_API_BRAVE_WALLET_ASSET_RATIO_CONTROLLER_IOS_H_

#import <Foundation/Foundation.h>
#import "brave_wallet.mojom.objc.h"

NS_ASSUME_NONNULL_BEGIN

OBJC_EXPORT
NS_SWIFT_NAME(BraveWallet.AssetRatioController)
@interface BraveWalletAssetRatioController : NSObject

- (void)priceFromAssets:(NSArray<NSString*>*)fromAssets
               toAssets:(NSArray<NSString*>*)toAssets
             completion:
                 (void (^)(bool success,
                           NSArray<BraveWalletAssetPrice*>* prices))completion;

- (void)priceHistoryForAsset:(NSString*)asset
                   timeframe:(BraveWalletAssetPriceTimeframe)timeframe
                  completion:
                      (void (^)(bool success,
                                NSArray<BraveWalletAssetTimePrice*>* values))
                          completion;

- (instancetype)init NS_UNAVAILABLE;

@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_BRAVE_WALLET_ASSET_RATIO_CONTROLLER_IOS_H_
