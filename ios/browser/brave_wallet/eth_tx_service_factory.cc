/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/brave_wallet/eth_tx_service_factory.h"

#include <utility>

#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/factory/eth_tx_service_factory_helper.h"
#include "brave/ios/browser/brave_wallet/asset_ratio_service_factory.h"
#include "brave/ios/browser/brave_wallet/json_rpc_service_factory.h"
#include "brave/ios/browser/brave_wallet/keyring_service_factory.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/keyed_service/ios/browser_state_dependency_manager.h"
#include "ios/chrome/browser/browser_state/browser_state_otr_helper.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"

namespace brave_wallet {

// static
mojo::PendingRemote<mojom::EthTxService>
EthTxServiceFactory::GetForBrowserState(ChromeBrowserState* browser_state) {
  return static_cast<EthTxService*>(
             GetInstance()->GetServiceForBrowserState(browser_state, true))
      ->MakeRemote();
}

// static
EthTxService* EthTxServiceFactory::GetServiceForState(
    ChromeBrowserState* browser_state) {
  return static_cast<EthTxService*>(
      GetInstance()->GetServiceForBrowserState(browser_state, true));
}

// static
EthTxServiceFactory* EthTxServiceFactory::GetInstance() {
  return base::Singleton<EthTxServiceFactory>::get();
}

EthTxServiceFactory::EthTxServiceFactory()
    : BrowserStateKeyedServiceFactory(
          "EthTxService",
          BrowserStateDependencyManager::GetInstance()) {
  DependsOn(JsonRpcServiceFactory::GetInstance());
  DependsOn(KeyringServiceFactory::GetInstance());
  DependsOn(AssetRatioServiceFactory::GetInstance());
}

EthTxServiceFactory::~EthTxServiceFactory() = default;

std::unique_ptr<KeyedService> EthTxServiceFactory::BuildServiceInstanceFor(
    web::BrowserState* context) const {
  auto* browser_state = ChromeBrowserState::FromBrowserState(context);
  auto* json_rpc_service =
      JsonRpcServiceFactory::GetServiceForState(browser_state);
  auto* keyring_service =
      KeyringServiceFactory::GetServiceForState(browser_state);
  auto* asset_ratio_service =
      AssetRatioServiceFactory::GetServiceForState(browser_state);
  return brave_wallet::BuildEthTxService(json_rpc_service, keyring_service,
                                         asset_ratio_service,
                                         browser_state->GetPrefs());
}

bool EthTxServiceFactory::ServiceIsNULLWhileTesting() const {
  return true;
}

web::BrowserState* EthTxServiceFactory::GetBrowserStateToUse(
    web::BrowserState* context) const {
  return GetBrowserStateRedirectedInIncognito(context);
}

}  // namespace brave_wallet
