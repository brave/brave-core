/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/api/brave_wallet/brave_wallet_api.h"

#include "base/strings/sys_string_conversions.h"
#include "brave/components/brave_wallet/browser/blockchain_registry.h"
#include "brave/components/brave_wallet/browser/brave_wallet_p3a.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "brave/components/brave_wallet/browser/ethereum_provider_impl.h"
#include "brave/components/brave_wallet/browser/solana_provider_impl.h"
#include "brave/components/brave_wallet/resources/grit/brave_wallet_script_generated.h"
#include "brave/ios/browser/api/brave_wallet/brave_wallet.mojom.objc+private.h"
#include "brave/ios/browser/api/brave_wallet/brave_wallet_provider_delegate_ios+private.h"
#include "brave/ios/browser/api/brave_wallet/brave_wallet_provider_delegate_ios.h"
#include "brave/ios/browser/brave_wallet/brave_wallet_service_factory.h"
#include "brave/ios/browser/brave_wallet/json_rpc_service_factory.h"
#include "brave/ios/browser/brave_wallet/keyring_service_factory.h"
#include "brave/ios/browser/brave_wallet/tx_service_factory.h"
#include "components/grit/brave_components_resources.h"
#include "ios/chrome/browser/content_settings/model/host_content_settings_map_factory.h"
#include "ios/chrome/browser/shared/model/browser_state/chrome_browser_state.h"
#include "ios/chrome/browser/shared/model/browser_state/chrome_browser_state_manager.h"
#include "ui/base/resource/resource_bundle.h"

BraveWalletProviderScriptKey const BraveWalletProviderScriptKeyEthereum =
    @"ethereum_provider.js";
BraveWalletProviderScriptKey const BraveWalletProviderScriptKeySolana =
    @"solana_provider.js";
BraveWalletProviderScriptKey const BraveWalletProviderScriptKeySolanaWeb3 =
    @"solana_web3.js";
BraveWalletProviderScriptKey const BraveWalletProviderScriptKeyWalletStandard =
    @"wallet_standard.js";

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@implementation BraveWalletAPI {
  ChromeBrowserState* _mainBrowserState;  // NOT OWNED
  NSMutableDictionary<NSNumber* /* BraveWalletCoinType */,
                      NSDictionary<BraveWalletProviderScriptKey, NSString*>*>*
      _providerScripts;
}

- (instancetype)initWithBrowserState:(ChromeBrowserState*)mainBrowserState {
  if ((self = [super init])) {
    _mainBrowserState = mainBrowserState;
    _providerScripts = [[NSMutableDictionary alloc] init];
  }
  return self;
}

+ (id<BraveWalletBlockchainRegistry>)blockchainRegistry {
  auto* registry = brave_wallet::BlockchainRegistry::GetInstance();
  return [[BraveWalletBlockchainRegistryMojoImpl alloc]
      initWithBlockchainRegistry:registry->MakeRemote()];
}

- (nullable id<BraveWalletEthereumProvider>)
    ethereumProviderWithDelegate:(id<BraveWalletProviderDelegate>)delegate
               isPrivateBrowsing:(bool)isPrivateBrowsing {
  auto* browserState = _mainBrowserState;
  if (isPrivateBrowsing) {
    browserState = browserState->GetOffTheRecordChromeBrowserState();
  }

  auto* json_rpc_service =
      brave_wallet::JsonRpcServiceFactory::GetServiceForState(browserState);
  if (!json_rpc_service) {
    return nil;
  }

  auto* tx_service =
      brave_wallet::TxServiceFactory::GetServiceForState(browserState);
  if (!tx_service) {
    return nil;
  }

  auto* keyring_service =
      brave_wallet::KeyringServiceFactory::GetServiceForState(browserState);
  if (!keyring_service) {
    return nil;
  }

  auto* brave_wallet_service =
      brave_wallet::BraveWalletServiceFactory::GetServiceForState(browserState);
  if (!brave_wallet_service) {
    return nil;
  }

  auto provider = std::make_unique<brave_wallet::EthereumProviderImpl>(
      ios::HostContentSettingsMapFactory::GetForBrowserState(browserState),
      json_rpc_service, tx_service, keyring_service, brave_wallet_service,
      std::make_unique<brave_wallet::BraveWalletProviderDelegateBridge>(
          delegate),
      browserState->GetPrefs());
  return [[BraveWalletEthereumProviderOwnedImpl alloc]
      initWithEthereumProvider:std::move(provider)];
}

- (nullable id<BraveWalletSolanaProvider>)
    solanaProviderWithDelegate:(id<BraveWalletProviderDelegate>)delegate
             isPrivateBrowsing:(bool)isPrivateBrowsing {
  auto* browserState = _mainBrowserState;
  if (isPrivateBrowsing) {
    browserState = browserState->GetOffTheRecordChromeBrowserState();
  }

  auto* keyring_service =
      brave_wallet::KeyringServiceFactory::GetServiceForState(browserState);
  if (!keyring_service) {
    return nil;
  }

  auto* brave_wallet_service =
      brave_wallet::BraveWalletServiceFactory::GetServiceForState(browserState);
  if (!brave_wallet_service) {
    return nil;
  }

  auto* tx_service =
      brave_wallet::TxServiceFactory::GetServiceForState(browserState);
  if (!tx_service) {
    return nil;
  }

  auto* json_rpc_service =
      brave_wallet::JsonRpcServiceFactory::GetServiceForState(browserState);
  if (!json_rpc_service) {
    return nil;
  }
  auto* host_content_settings_map =
      ios::HostContentSettingsMapFactory::GetForBrowserState(browserState);
  if (!host_content_settings_map) {
    return nil;
  }

  auto provider = std::make_unique<brave_wallet::SolanaProviderImpl>(
      *host_content_settings_map, keyring_service, brave_wallet_service,
      tx_service, json_rpc_service,
      std::make_unique<brave_wallet::BraveWalletProviderDelegateBridge>(
          delegate));
  return [[BraveWalletSolanaProviderOwnedImpl alloc]
      initWithSolanaProvider:std::move(provider)];
}

- (NSString*)resourceForID:(int)resource_id {
  // The resource bundle is not available until after WebMainParts is setup
  auto& resource_bundle = ui::ResourceBundle::GetSharedInstance();
  std::string resource_string = "";
  if (resource_bundle.IsGzipped(resource_id)) {
    resource_string =
        std::string(resource_bundle.LoadDataResourceString(resource_id));
  } else {
    resource_string =
        std::string(resource_bundle.GetRawDataResource(resource_id));
  }
  return base::SysUTF8ToNSString(resource_string);
}

- (NSDictionary<BraveWalletProviderScriptKey, NSString*>*)
    providerScriptsForCoinType:(BraveWalletCoinType)coinType {
  auto cachedScript = _providerScripts[@(coinType)];
  if (cachedScript) {
    return cachedScript;
  }
  auto resource_ids =
      ^std::vector<std::pair<BraveWalletProviderScriptKey, int>> {
    switch (coinType) {
      case BraveWalletCoinTypeEth:
        return {std::make_pair(
            BraveWalletProviderScriptKeyEthereum,
            IDR_BRAVE_WALLET_SCRIPT_ETHEREUM_PROVIDER_SCRIPT_BUNDLE_JS)};
      case BraveWalletCoinTypeSol:
        return {std::make_pair(
                    BraveWalletProviderScriptKeySolana,
                    IDR_BRAVE_WALLET_SCRIPT_SOLANA_PROVIDER_SCRIPT_BUNDLE_JS),
                std::make_pair(BraveWalletProviderScriptKeySolanaWeb3,
                               IDR_BRAVE_WALLET_SOLANA_WEB3_JS),
                std::make_pair(BraveWalletProviderScriptKeyWalletStandard,
                               IDR_BRAVE_WALLET_STANDARD_JS)};
      case BraveWalletCoinTypeFil:
        // Currently not supported
        return {std::make_pair(@"", 0)};
      case BraveWalletCoinTypeBtc:
        // Currently not supported
        return {std::make_pair(@"", 0)};
      case BraveWalletCoinTypeZec:
        // Currently not supported
        return {std::make_pair(@"", 0)};
    }
    return {std::make_pair(@"", 0)};
  }
  ();
  const auto scripts = [[NSMutableDictionary alloc] init];
  for (auto resource : resource_ids) {
    auto key = resource.first;
    auto resource_id = resource.second;
    scripts[key] = [self resourceForID:resource_id];
  }
  _providerScripts[@(coinType)] = [scripts copy];
  return scripts;
}

- (nullable id<BraveWalletBraveWalletP3A>)walletP3A {
  auto* service = brave_wallet::BraveWalletServiceFactory::GetServiceForState(
      _mainBrowserState);
  if (!service) {
    return nil;
  }
  return [[BraveWalletBraveWalletP3AMojoImpl alloc]
      initWithBraveWalletP3A:service->GetBraveWalletP3A()->MakeRemote()];
}

@end
