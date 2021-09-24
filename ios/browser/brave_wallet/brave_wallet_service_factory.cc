/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/brave_wallet/brave_wallet_service_factory.h"

#include <memory>

#include "brave/components/brave_wallet/browser/brave_wallet_importer_delegate.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/keyed_service/ios/browser_state_dependency_manager.h"
#include "ios/chrome/browser/browser_state/browser_state_otr_helper.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#include "ios/web/public/browser_state.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace brave_wallet {

// static
mojom::BraveWalletService* BraveWalletServiceFactory::GetForBrowserState(
    ChromeBrowserState* browser_state) {
  return static_cast<BraveWalletService*>(
      GetInstance()->GetServiceForBrowserState(browser_state, true));
}

// static
BraveWalletServiceFactory* BraveWalletServiceFactory::GetInstance() {
  return base::Singleton<BraveWalletServiceFactory>::get();
}

BraveWalletServiceFactory::BraveWalletServiceFactory()
    : BrowserStateKeyedServiceFactory(
          "BraveWalletService",
          BrowserStateDependencyManager::GetInstance()) {}

BraveWalletServiceFactory::~BraveWalletServiceFactory() = default;

std::unique_ptr<KeyedService>
BraveWalletServiceFactory::BuildServiceInstanceFor(
    web::BrowserState* context) const {
  auto* browser_state = ChromeBrowserState::FromBrowserState(context);
  std::unique_ptr<BraveWalletService> controller(
      new BraveWalletService(std::make_unique<BraveWalletImporterDelegate>(),
                             browser_state->GetPrefs()));
  return controller;
}

bool BraveWalletServiceFactory::ServiceIsNULLWhileTesting() const {
  return true;
}

web::BrowserState* BraveWalletServiceFactory::GetBrowserStateToUse(
    web::BrowserState* context) const {
  return GetBrowserStateRedirectedInIncognito(context);
}

}  // namespace brave_wallet
