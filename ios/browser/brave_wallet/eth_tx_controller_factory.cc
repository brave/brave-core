/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/brave_wallet/eth_tx_controller_factory.h"

#include <utility>

#include "brave/components/brave_wallet/browser/eth_tx_controller.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/factory/eth_tx_controller_factory_helper.h"
#include "brave/ios/browser/brave_wallet/eth_json_rpc_controller_factory.h"
#include "brave/ios/browser/brave_wallet/keyring_controller_factory.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/keyed_service/ios/browser_state_dependency_manager.h"
#include "ios/chrome/browser/browser_state/browser_state_otr_helper.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"

namespace brave_wallet {

// static
mojom::EthTxController* EthTxControllerFactory::GetForBrowserState(
    ChromeBrowserState* browser_state) {
  return static_cast<EthTxController*>(
      GetInstance()->GetServiceForBrowserState(browser_state, true));
}

// static
EthTxControllerFactory* EthTxControllerFactory::GetInstance() {
  return base::Singleton<EthTxControllerFactory>::get();
}

EthTxControllerFactory::EthTxControllerFactory()
    : BrowserStateKeyedServiceFactory(
          "EthTxController",
          BrowserStateDependencyManager::GetInstance()) {
  DependsOn(EthJsonRpcControllerFactory::GetInstance());
  DependsOn(KeyringControllerFactory::GetInstance());
}

EthTxControllerFactory::~EthTxControllerFactory() = default;

std::unique_ptr<KeyedService> EthTxControllerFactory::BuildServiceInstanceFor(
    web::BrowserState* context) const {
  auto* browser_state = ChromeBrowserState::FromBrowserState(context);
  auto* rpc_controller =
      EthJsonRpcControllerFactory::GetControllerForBrowserState(browser_state);
  auto* keyring_controller =
      KeyringControllerFactory::GetControllerForBrowserState(browser_state);
  return brave_wallet::BuildEthTxController(rpc_controller, keyring_controller,
                                            browser_state->GetPrefs());
}

bool EthTxControllerFactory::ServiceIsNULLWhileTesting() const {
  return true;
}

web::BrowserState* EthTxControllerFactory::GetBrowserStateToUse(
    web::BrowserState* context) const {
  return GetBrowserStateRedirectedInIncognito(context);
}

}  // namespace brave_wallet
