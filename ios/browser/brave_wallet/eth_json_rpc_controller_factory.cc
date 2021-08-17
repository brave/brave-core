/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/brave_wallet/eth_json_rpc_controller_factory.h"

#include "brave/components/brave_wallet/browser/eth_json_rpc_controller.h"
#include "components/keyed_service/ios/browser_state_dependency_manager.h"
#include "ios/chrome/browser/browser_state/browser_state_otr_helper.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace brave_wallet {

// static
mojom::EthJsonRpcController* EthJsonRpcControllerFactory::GetForBrowserState(
    ChromeBrowserState* browser_state) {
  return static_cast<EthJsonRpcController*>(
      GetInstance()->GetServiceForBrowserState(browser_state, true));
}

// static
EthJsonRpcControllerFactory* EthJsonRpcControllerFactory::GetInstance() {
  return base::Singleton<EthJsonRpcControllerFactory>::get();
}

EthJsonRpcControllerFactory::EthJsonRpcControllerFactory()
    : BrowserStateKeyedServiceFactory(
          "EthJsonRpcController",
          BrowserStateDependencyManager::GetInstance()) {}

EthJsonRpcControllerFactory::~EthJsonRpcControllerFactory() = default;

std::unique_ptr<KeyedService>
EthJsonRpcControllerFactory::BuildServiceInstanceFor(
    web::BrowserState* context) const {
  auto* browser_state = ChromeBrowserState::FromBrowserState(context);
  std::unique_ptr<EthJsonRpcController> eth_json_rpc_controller(
      new EthJsonRpcController(browser_state->GetSharedURLLoaderFactory(),
                               browser_state->GetPrefs()));
  return eth_json_rpc_controller;
}

bool EthJsonRpcControllerFactory::ServiceIsNULLWhileTesting() const {
  return true;
}

}  // namespace brave_wallet
