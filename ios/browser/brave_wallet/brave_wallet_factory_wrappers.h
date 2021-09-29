/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_BRAVE_WALLET_BRAVE_WALLET_FACTORY_WRAPPERS_H_
#define BRAVE_IOS_BROWSER_BRAVE_WALLET_BRAVE_WALLET_FACTORY_WRAPPERS_H_

#import <Foundation/Foundation.h>
#include "keyed_service_factory_wrapper.h"  // NOLINT

@protocol BraveWalletAssetRatioController
, BraveWalletBraveWalletService, BraveWalletEthJsonRpcController,
    BraveWalletEthTxController, BraveWalletKeyringController,
    BraveWalletSwapController;

OBJC_EXPORT
NS_SWIFT_NAME(BraveWallet.AssetRatioControllerFactory)
@interface BraveWalletAssetRatioControllerFactory
    : KeyedServiceFactoryWrapper < id <BraveWalletAssetRatioController>
> @end

OBJC_EXPORT
NS_SWIFT_NAME(BraveWallet.EthJsonRpcControllerFactory)
@interface BraveWalletEthJsonRpcControllerFactory
    : KeyedServiceFactoryWrapper < id <BraveWalletEthJsonRpcController>
> @end

OBJC_EXPORT
NS_SWIFT_NAME(BraveWallet.EthTxControllerFactory)
@interface BraveWalletEthTxControllerFactory
    : KeyedServiceFactoryWrapper < id <BraveWalletEthTxController>
> @end

OBJC_EXPORT
NS_SWIFT_NAME(BraveWallet.KeyringControllerFactory)
@interface BraveWalletKeyringControllerFactory
    : KeyedServiceFactoryWrapper < id <BraveWalletKeyringController>
> @end

OBJC_EXPORT
NS_SWIFT_NAME(BraveWallet.ServiceFactory)
@interface BraveWalletServiceFactory
    : KeyedServiceFactoryWrapper < id <BraveWalletBraveWalletService>
> @end

OBJC_EXPORT
NS_SWIFT_NAME(BraveWallet.SwapControllerFactory)
@interface BraveWalletSwapControllerFactory
    : KeyedServiceFactoryWrapper < id <BraveWalletSwapController>
> @end

#endif  // BRAVE_IOS_BROWSER_BRAVE_WALLET_BRAVE_WALLET_FACTORY_WRAPPERS_H_
