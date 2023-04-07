/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_BRAVE_WALLET_BRAVE_WALLET_FACTORY_WRAPPERS_H_
#define BRAVE_IOS_BROWSER_BRAVE_WALLET_BRAVE_WALLET_FACTORY_WRAPPERS_H_

#import <Foundation/Foundation.h>
#include "keyed_service_factory_wrapper.h"  // NOLINT

@protocol BraveWalletAssetRatioService
, BraveWalletBraveWalletService, BraveWalletJsonRpcService,
    BraveWalletEthTxManagerProxy, BraveWalletSolanaTxManagerProxy,
    BraveWalletTxService, BraveWalletKeyringService, BraveWalletSwapService,
    BraveWalletIpfsService;

OBJC_EXPORT
NS_SWIFT_NAME(BraveWallet.AssetRatioServiceFactory)
@interface BraveWalletAssetRatioServiceFactory
    : KeyedServiceFactoryWrapper < id <BraveWalletAssetRatioService>
> @end

OBJC_EXPORT
NS_SWIFT_NAME(BraveWallet.JsonRpcServiceFactory)
@interface BraveWalletJsonRpcServiceFactory
    : KeyedServiceFactoryWrapper < id <BraveWalletJsonRpcService>
> @end

OBJC_EXPORT
NS_SWIFT_NAME(BraveWallet.TxServiceFactory)
@interface BraveWalletTxServiceFactory
    : KeyedServiceFactoryWrapper < id <BraveWalletTxService>
> @end

OBJC_EXPORT
NS_SWIFT_NAME(BraveWallet.EthTxManagerProxyFactory)
@interface BraveWalletEthTxManagerProxyFactory
    : KeyedServiceFactoryWrapper < id <BraveWalletEthTxManagerProxy>
> @end

OBJC_EXPORT
NS_SWIFT_NAME(BraveWallet.SolanaTxManagerProxyFactory)
@interface BraveWalletSolanaTxManagerProxyFactory
    : KeyedServiceFactoryWrapper < id <BraveWalletSolanaTxManagerProxy>
> @end

OBJC_EXPORT
NS_SWIFT_NAME(BraveWallet.KeyringServiceFactory)
@interface BraveWalletKeyringServiceFactory
    : KeyedServiceFactoryWrapper < id <BraveWalletKeyringService>
> @end

OBJC_EXPORT
NS_SWIFT_NAME(BraveWallet.ServiceFactory)
@interface BraveWalletServiceFactory
    : KeyedServiceFactoryWrapper < id <BraveWalletBraveWalletService>
> @end

OBJC_EXPORT
NS_SWIFT_NAME(BraveWallet.SwapServiceFactory)
@interface BraveWalletSwapServiceFactory
    : KeyedServiceFactoryWrapper < id <BraveWalletSwapService>
> @end

OBJC_EXPORT
NS_SWIFT_NAME(BraveWallet.IpfsServiceFactory)
@interface BraveWalletIpfsServiceFactory
    : KeyedServiceFactoryWrapper <id <BraveWalletIpfsService>>
@end

#endif  // BRAVE_IOS_BROWSER_BRAVE_WALLET_BRAVE_WALLET_FACTORY_WRAPPERS_H_
