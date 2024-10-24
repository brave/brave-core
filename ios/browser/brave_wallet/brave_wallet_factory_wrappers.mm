/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/brave_wallet/brave_wallet_factory_wrappers.h"

#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/tx_service.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/ios/browser/api/brave_wallet/brave_wallet.mojom.objc+private.h"
#include "brave/ios/browser/brave_wallet/asset_ratio_service_factory.h"
#include "brave/ios/browser/brave_wallet/brave_wallet_ipfs_service_factory.h"
#include "brave/ios/browser/brave_wallet/brave_wallet_service_factory.h"
#include "brave/ios/browser/brave_wallet/meld_integration_service_factory.h"
#include "brave/ios/browser/brave_wallet/swap_service_factory.h"
#include "brave/ios/browser/keyed_service/keyed_service_factory_wrapper+private.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@implementation BraveWalletAssetRatioServiceFactory
+ (nullable id)serviceForProfile:(ProfileIOS*)profile {
  auto service =
      brave_wallet::AssetRatioServiceFactory::GetForBrowserState(profile);
  if (!service) {
    return nil;
  }
  return [[BraveWalletAssetRatioServiceMojoImpl alloc]
      initWithAssetRatioService:std::move(service)];
}
@end

@implementation BraveWalletBitcoinWalletServiceFactory
+ (nullable id)serviceForProfile:(ProfileIOS*)profile {
  auto* brave_wallet_service =
      brave_wallet::BraveWalletServiceFactory::GetServiceForState(profile);
  if (!brave_wallet_service) {
    return nil;
  }
  mojo::PendingRemote<brave_wallet::mojom::BitcoinWalletService> pending_remote;
  brave_wallet_service->Bind(pending_remote.InitWithNewPipeAndPassReceiver());
  return [[BraveWalletBitcoinWalletServiceMojoImpl alloc]
      initWithBitcoinWalletService:std::move(pending_remote)];
}

@end

@implementation BraveWalletJsonRpcServiceFactory
+ (nullable id)serviceForProfile:(ProfileIOS*)profile {
  auto* brave_wallet_service =
      brave_wallet::BraveWalletServiceFactory::GetServiceForState(profile);
  if (!brave_wallet_service) {
    return nil;
  }
  mojo::PendingRemote<brave_wallet::mojom::JsonRpcService> pending_remote;
  brave_wallet_service->Bind(pending_remote.InitWithNewPipeAndPassReceiver());
  return [[BraveWalletJsonRpcServiceMojoImpl alloc]
      initWithJsonRpcService:std::move(pending_remote)];
}
@end

@implementation BraveWalletTxServiceFactory
+ (nullable id)serviceForProfile:(ProfileIOS*)profile {
  auto* brave_wallet_service =
      brave_wallet::BraveWalletServiceFactory::GetServiceForState(profile);
  if (!brave_wallet_service) {
    return nil;
  }
  mojo::PendingRemote<brave_wallet::mojom::TxService> pending_remote;
  brave_wallet_service->Bind(pending_remote.InitWithNewPipeAndPassReceiver());
  return [[BraveWalletTxServiceMojoImpl alloc]
      initWithTxService:std::move(pending_remote)];
}
@end

@implementation BraveWalletEthTxManagerProxyFactory
+ (nullable id)serviceForProfile:(ProfileIOS*)profile {
  auto* brave_wallet_service =
      brave_wallet::BraveWalletServiceFactory::GetServiceForState(profile);
  if (!brave_wallet_service) {
    return nil;
  }
  mojo::PendingRemote<brave_wallet::mojom::EthTxManagerProxy> pending_remote;
  brave_wallet_service->Bind(pending_remote.InitWithNewPipeAndPassReceiver());
  return [[BraveWalletEthTxManagerProxyMojoImpl alloc]
      initWithEthTxManagerProxy:std::move(pending_remote)];
}
@end

@implementation BraveWalletSolanaTxManagerProxyFactory
+ (nullable id)serviceForProfile:(ProfileIOS*)profile {
  auto* brave_wallet_service =
      brave_wallet::BraveWalletServiceFactory::GetServiceForState(profile);
  if (!brave_wallet_service) {
    return nil;
  }
  mojo::PendingRemote<brave_wallet::mojom::SolanaTxManagerProxy> pending_remote;
  brave_wallet_service->Bind(pending_remote.InitWithNewPipeAndPassReceiver());
  return [[BraveWalletSolanaTxManagerProxyMojoImpl alloc]
      initWithSolanaTxManagerProxy:std::move(pending_remote)];
}
@end

@implementation BraveWalletKeyringServiceFactory
+ (nullable id)serviceForProfile:(ProfileIOS*)profile {
  auto* brave_wallet_service =
      brave_wallet::BraveWalletServiceFactory::GetServiceForState(profile);
  if (!brave_wallet_service) {
    return nil;
  }
  mojo::PendingRemote<brave_wallet::mojom::KeyringService> pending_remote;
  brave_wallet_service->Bind(pending_remote.InitWithNewPipeAndPassReceiver());
  return [[BraveWalletKeyringServiceMojoImpl alloc]
      initWithKeyringService:std::move(pending_remote)];
}
@end

@implementation BraveWalletMeldIntegrationServiceFactory
+ (nullable id)serviceForProfile:(ProfileIOS*)profile {
  auto service =
      brave_wallet::MeldIntegrationServiceFactory::GetForBrowserState(profile);
  if (!service) {
    return nil;
  }
  return [[BraveWalletMeldIntegrationServiceMojoImpl alloc]
      initWithMeldIntegrationService:std::move(service)];
}
@end

@implementation BraveWalletServiceFactory
+ (nullable id)serviceForProfile:(ProfileIOS*)profile {
  auto* brave_wallet_service =
      brave_wallet::BraveWalletServiceFactory::GetServiceForState(profile);
  if (!brave_wallet_service) {
    return nil;
  }
  mojo::PendingRemote<brave_wallet::mojom::BraveWalletService> pending_remote;
  brave_wallet_service->Bind(pending_remote.InitWithNewPipeAndPassReceiver());
  return [[BraveWalletBraveWalletServiceMojoImpl alloc]
      initWithBraveWalletService:std::move(pending_remote)];
}
@end

@implementation BraveWalletSwapServiceFactory
+ (nullable id)serviceForProfile:(ProfileIOS*)profile {
  auto service = brave_wallet::SwapServiceFactory::GetForBrowserState(profile);
  if (!service) {
    return nil;
  }
  return [[BraveWalletSwapServiceMojoImpl alloc]
      initWithSwapService:std::move(service)];
}
@end

@implementation BraveWalletIpfsServiceFactory
+ (nullable id)serviceForProfile:(ProfileIOS*)profile {
  auto service =
      brave_wallet::BraveWalletIpfsServiceFactory::GetForBrowserState(profile);
  if (!service) {
    return nil;
  }
  return [[BraveWalletIpfsServiceMojoImpl alloc]
      initWithIpfsService:std::move(service)];
}
@end

@implementation BraveWalletZCashWalletServiceFactory
+ (nullable id)serviceForProfile:(ProfileIOS*)profile {
  auto* brave_wallet_service =
      brave_wallet::BraveWalletServiceFactory::GetServiceForState(profile);
  if (!brave_wallet_service) {
    return nil;
  }
  mojo::PendingRemote<brave_wallet::mojom::ZCashWalletService> pending_remote;
  brave_wallet_service->Bind(pending_remote.InitWithNewPipeAndPassReceiver());
  return [[BraveWalletZCashWalletServiceMojoImpl alloc]
      initWithZCashWalletService:std::move(pending_remote)];
}
@end
