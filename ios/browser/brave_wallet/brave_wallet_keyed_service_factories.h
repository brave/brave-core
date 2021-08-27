/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_BRAVE_WALLET_BRAVE_WALLET_KEYED_SERVICE_FACTORIES_H_
#define BRAVE_IOS_BROWSER_BRAVE_WALLET_BRAVE_WALLET_KEYED_SERVICE_FACTORIES_H_

#import <Foundation/Foundation.h>
#include "keyed_service_factory_wrapper.h"

@protocol BraveWalletAssetRatioController;

OBJC_EXPORT
@interface AssetRatioControllerFactory
    : KeyedServiceFactoryWrapper < id <BraveWalletAssetRatioController>
> @end

#endif  // BRAVE_IOS_BROWSER_BRAVE_WALLET_BRAVE_WALLET_KEYED_SERVICE_FACTORIES_H_
