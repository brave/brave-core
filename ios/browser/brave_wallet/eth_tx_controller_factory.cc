/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/brave_wallet/eth_tx_controller_factory.h"

#include "brave/components/brave_wallet/browser/eth_nonce_tracker.h"
#include "brave/components/brave_wallet/browser/eth_tx_controller.h"
#include "brave/components/brave_wallet/browser/eth_tx_state_manager.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/ios/browser/brave_wallet/eth_json_rpc_controller_factory.h"
#include "brave/ios/browser/brave_wallet/keyring_controller_factory.h"
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
  auto tx_state_manager = std::make_unique<EthTxStateManager>(
      browser_state->GetPrefs(),
      EthJsonRpcControllerFactory::GetControllerForBrowserState(browser_state)
          ->MakeRemote());
  auto eth_nonce_tracker = std::make_unique<EthNonceTracker>(
      tx_state_manager.get(),
      EthJsonRpcControllerFactory::GetControllerForBrowserState(browser_state));
  auto eth_pending_tx_tracker = std::make_unique<EthPendingTxTracker>(
      tx_state_manager.get(),
      EthJsonRpcControllerFactory::GetControllerForBrowserState(browser_state),
      eth_nonce_tracker.get());
  std::unique_ptr<EthTxController> controller(new EthTxController(
      EthJsonRpcControllerFactory::GetControllerForBrowserState(browser_state),
      KeyringControllerFactory::GetControllerForBrowserState(browser_state),
      std::move(tx_state_manager), std::move(eth_nonce_tracker),
      std::move(eth_pending_tx_tracker), browser_state->GetPrefs()));
  return controller;
}

bool EthTxControllerFactory::ServiceIsNULLWhileTesting() const {
  return true;
}

web::BrowserState* EthTxControllerFactory::GetBrowserStateToUse(
    web::BrowserState* context) const {
  return GetBrowserStateRedirectedInIncognito(context);
}

}  // namespace brave_wallet
