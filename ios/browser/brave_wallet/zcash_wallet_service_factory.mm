/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/brave_wallet/zcash_wallet_service_factory.h"

#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_wallet_service.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "brave/ios/browser/brave_wallet/keyring_service_factory.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/keyed_service/ios/browser_state_dependency_manager.h"
#include "ios/chrome/browser/shared/model/application_context/application_context.h"
#include "ios/chrome/browser/shared/model/browser_state/browser_state_otr_helper.h"
#include "ios/chrome/browser/shared/model/browser_state/chrome_browser_state.h"
#include "ios/web/public/browser_state.h"

namespace brave_wallet {

// static
mojo::PendingRemote<mojom::ZCashWalletService>
ZCashWalletServiceFactory::GetForBrowserState(
    ChromeBrowserState* browser_state) {
  return static_cast<ZCashWalletService*>(
             GetInstance()->GetServiceForBrowserState(browser_state, true))
      ->MakeRemote();
}

// static
ZCashWalletService* ZCashWalletServiceFactory::GetServiceForState(
    ChromeBrowserState* browser_state) {
  return static_cast<ZCashWalletService*>(
      GetInstance()->GetServiceForBrowserState(browser_state, true));
}

// static
ZCashWalletServiceFactory* ZCashWalletServiceFactory::GetInstance() {
  static base::NoDestructor<ZCashWalletServiceFactory> instance;
  return instance.get();
}

ZCashWalletServiceFactory::ZCashWalletServiceFactory()
    : BrowserStateKeyedServiceFactory(
          "ZCashWalletService",
          BrowserStateDependencyManager::GetInstance()) {
  DependsOn(KeyringServiceFactory::GetInstance());
}

ZCashWalletServiceFactory::~ZCashWalletServiceFactory() = default;

std::unique_ptr<KeyedService>
ZCashWalletServiceFactory::BuildServiceInstanceFor(
    web::BrowserState* context) const {
  auto* browser_state = ChromeBrowserState::FromBrowserState(context);
  if (!IsZCashEnabled()) {
    return nullptr;
  }
  std::unique_ptr<ZCashWalletService> zcash_service(new ZCashWalletService(
      KeyringServiceFactory::GetServiceForState(browser_state),
      browser_state->GetPrefs(), browser_state->GetSharedURLLoaderFactory()));
  return zcash_service;
}

bool ZCashWalletServiceFactory::ServiceIsNULLWhileTesting() const {
  return true;
}

web::BrowserState* ZCashWalletServiceFactory::GetBrowserStateToUse(
    web::BrowserState* context) const {
  return GetBrowserStateRedirectedInIncognito(context);
}

}  // namespace brave_wallet
