/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#import "brave/ios/browser/api/wallet/brave_wallet_service_factory.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "components/keyed_service/ios/browser_state_dependency_manager.h"
#include "ios/chrome/browser/browser_state/browser_state_otr_helper.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

// static
brave_wallet::BraveWalletService* BraveWalletServiceFactory::GetForBrowserState(
    ChromeBrowserState* browser_state) {
  return static_cast<brave_wallet::BraveWalletService*>(
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
  std::unique_ptr<brave_wallet::BraveWalletService> wallet_service(
      new brave_wallet::BraveWalletService(
          browser_state->GetPrefs(),
          browser_state->GetSharedURLLoaderFactory()));
  return wallet_service;
}

web::BrowserState* BraveWalletServiceFactory::GetBrowserStateToUse(
    web::BrowserState* context) const {
  return ChromeBrowserState::FromBrowserState(context);
}

bool BraveWalletServiceFactory::ServiceIsNULLWhileTesting() const {
  return true;
}
