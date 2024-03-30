/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/brave_wallet/bitcoin_wallet_service_factory.h"

#include <utility>

#include "base/no_destructor.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/ios/browser/brave_wallet/keyring_service_factory.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/keyed_service/ios/browser_state_dependency_manager.h"
#include "ios/chrome/browser/shared/model/browser_state/browser_state_otr_helper.h"
#include "ios/chrome/browser/shared/model/browser_state/chrome_browser_state.h"
#include "ios/web/public/thread/web_task_traits.h"
#include "ios/web/public/thread/web_thread.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace brave_wallet {

// static
mojo::PendingRemote<mojom::BitcoinWalletService>
BitcoinWalletServiceFactory::GetForBrowserState(
    ChromeBrowserState* browser_state) {
  return static_cast<BitcoinWalletService*>(
             GetInstance()->GetServiceForBrowserState(browser_state, true))
      ->MakeRemote();
}

// static
BitcoinWalletService* BitcoinWalletServiceFactory::GetServiceForState(
    ChromeBrowserState* browser_state) {
  return static_cast<BitcoinWalletService*>(
      GetInstance()->GetServiceForBrowserState(browser_state, true));
}

// static
BitcoinWalletServiceFactory* BitcoinWalletServiceFactory::GetInstance() {
  static base::NoDestructor<BitcoinWalletServiceFactory> instance;
  return instance.get();
}

BitcoinWalletServiceFactory::BitcoinWalletServiceFactory()
    : BrowserStateKeyedServiceFactory(
          "BitcoinWalletService",
          BrowserStateDependencyManager::GetInstance()) {
  DependsOn(KeyringServiceFactory::GetInstance());
}

BitcoinWalletServiceFactory::~BitcoinWalletServiceFactory() = default;

std::unique_ptr<KeyedService>
BitcoinWalletServiceFactory::BuildServiceInstanceFor(
    web::BrowserState* context) const {
  auto* browser_state = ChromeBrowserState::FromBrowserState(context);

  std::unique_ptr<BitcoinWalletService> service(new BitcoinWalletService(
      KeyringServiceFactory::GetServiceForState(browser_state),
      browser_state->GetPrefs(), browser_state->GetSharedURLLoaderFactory()));

  return service;
}

bool BitcoinWalletServiceFactory::ServiceIsNULLWhileTesting() const {
  return true;
}

web::BrowserState* BitcoinWalletServiceFactory::GetBrowserStateToUse(
    web::BrowserState* context) const {
  return GetBrowserStateRedirectedInIncognito(context);
}

}  // namespace brave_wallet
