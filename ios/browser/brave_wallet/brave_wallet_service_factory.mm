/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/brave_wallet/brave_wallet_service_factory.h"

#include <memory>

#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service_delegate.h"
#include "brave/ios/browser/brave_wallet/bitcoin_wallet_service_factory.h"
#include "brave/ios/browser/brave_wallet/json_rpc_service_factory.h"
#include "brave/ios/browser/brave_wallet/keyring_service_factory.h"
#include "brave/ios/browser/brave_wallet/tx_service_factory.h"
#include "brave/ios/browser/brave_wallet/zcash_wallet_service_factory.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/keyed_service/ios/browser_state_dependency_manager.h"
#include "ios/chrome/browser/shared/model/application_context/application_context.h"
#include "ios/chrome/browser/shared/model/browser_state/browser_state_otr_helper.h"
#include "ios/chrome/browser/shared/model/browser_state/chrome_browser_state.h"
#include "ios/web/public/browser_state.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace brave_wallet {

// static
mojo::PendingRemote<mojom::BraveWalletService>
BraveWalletServiceFactory::GetForBrowserState(
    ChromeBrowserState* browser_state) {
  return static_cast<BraveWalletService*>(
             GetInstance()->GetServiceForBrowserState(browser_state, true))
      ->MakeRemote();
}

// static
BraveWalletService* BraveWalletServiceFactory::GetServiceForState(
    ChromeBrowserState* browser_state) {
  return static_cast<BraveWalletService*>(
      GetInstance()->GetServiceForBrowserState(browser_state, true));
}

// static
BraveWalletServiceFactory* BraveWalletServiceFactory::GetInstance() {
  static base::NoDestructor<BraveWalletServiceFactory> instance;
  return instance.get();
}

BraveWalletServiceFactory::BraveWalletServiceFactory()
    : BrowserStateKeyedServiceFactory(
          "BraveWalletService",
          BrowserStateDependencyManager::GetInstance()) {
  DependsOn(KeyringServiceFactory::GetInstance());
  DependsOn(JsonRpcServiceFactory::GetInstance());
  DependsOn(BitcoinWalletServiceFactory::GetInstance());
  DependsOn(TxServiceFactory::GetInstance());
  DependsOn(ZCashWalletServiceFactory::GetInstance());
}

BraveWalletServiceFactory::~BraveWalletServiceFactory() = default;

std::unique_ptr<KeyedService>
BraveWalletServiceFactory::BuildServiceInstanceFor(
    web::BrowserState* context) const {
  auto* browser_state = ChromeBrowserState::FromBrowserState(context);
  std::unique_ptr<BraveWalletService> service(new BraveWalletService(
      browser_state->GetSharedURLLoaderFactory(),
      std::make_unique<BraveWalletServiceDelegate>(),
      KeyringServiceFactory::GetServiceForState(browser_state),
      JsonRpcServiceFactory::GetServiceForState(browser_state),
      TxServiceFactory::GetServiceForState(browser_state),
      BitcoinWalletServiceFactory::GetServiceForState(browser_state),
      ZCashWalletServiceFactory::GetServiceForState(browser_state),
      browser_state->GetPrefs(), GetApplicationContext()->GetLocalState(),
      browser_state->IsOffTheRecord()));
  return service;
}

bool BraveWalletServiceFactory::ServiceIsNULLWhileTesting() const {
  return true;
}

web::BrowserState* BraveWalletServiceFactory::GetBrowserStateToUse(
    web::BrowserState* context) const {
  return GetBrowserStateRedirectedInIncognito(context);
}

}  // namespace brave_wallet
