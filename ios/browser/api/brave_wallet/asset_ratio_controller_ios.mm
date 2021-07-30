/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#import "brave/ios/browser/api/brave_wallet/asset_ratio_controller_ios.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

#include "base/strings/sys_string_conversions.h"
#include "brave/build/ios/mojom/cpp_transformations.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/ios/browser/api/brave_wallet/brave_wallet.mojom.objc+private.h"

@interface BraveWalletAssetRatioController () {
  brave_wallet::mojom::AssetRatioController* _controller;  // NOT OWNED
}
@end

@implementation BraveWalletAssetRatioController

- (instancetype)initWithController:
    (brave_wallet::mojom::AssetRatioController*)controller {
  if ((self = [super init])) {
    _controller = controller;
  }
  return self;
}

- (void)priceFromAssets:(NSArray<NSString*>*)fromAssets
               toAssets:(NSArray<NSString*>*)toAssets
             completion:
                 (void (^)(bool, NSArray<BraveWalletAssetPrice*>*))completion {
  auto callback = ^(bool success,
                    std::vector<brave_wallet::mojom::AssetPricePtr> prices) {
    NSMutableArray* objs = [[NSMutableArray alloc] init];
    for (const auto& price : prices) {
      if (price.get() == nullptr) {
        continue;
      }
      [objs
          addObject:[[BraveWalletAssetPrice alloc] initWithAssetPrice:*price]];
    }
    completion(success, [objs copy]);
  };
  _controller->GetPrice(VectorFromNSArray(fromAssets),
                        VectorFromNSArray(toAssets), base::BindOnce(callback));
}

- (void)priceHistoryForAsset:(NSString*)asset
                   timeframe:(BraveWalletAssetPriceTimeframe)timeframe
                  completion:
                      (void (^)(bool success,
                                NSArray<BraveWalletAssetTimePrice*>* values))
                          completion {
  auto callback =
      ^(bool success,
        std::vector<brave_wallet::mojom::AssetTimePricePtr> prices) {
        NSMutableArray* objs = [[NSMutableArray alloc] init];
        for (const auto& price : prices) {
          if (price.get() == nullptr) {
            continue;
          }
          [objs addObject:[[BraveWalletAssetTimePrice alloc]
                              initWithAssetTimePrice:*price]];
        }
        completion(success, [objs copy]);
      };
  _controller->GetPriceHistory(
      base::SysNSStringToUTF8(asset),
      static_cast<brave_wallet::mojom::AssetPriceTimeframe>(timeframe),
      base::BindOnce(callback));
}

@end
