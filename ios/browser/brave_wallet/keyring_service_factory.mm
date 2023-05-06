/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/brave_wallet/keyring_service_factory.h"

#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/ios/browser/brave_wallet/json_rpc_service_factory.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/keyed_service/ios/browser_state_dependency_manager.h"
#include "ios/chrome/browser/application_context/application_context.h"
#include "ios/chrome/browser/shared/model/browser_state/browser_state_otr_helper.h"
#include "ios/chrome/browser/shared/model/browser_state/chrome_browser_state.h"
#include "ios/web/public/browser_state.h"

namespace brave_wallet {

// static
mojo::PendingRemote<mojom::KeyringService>
KeyringServiceFactory::GetForBrowserState(ChromeBrowserState* browser_state) {
  return static_cast<KeyringService*>(
             GetInstance()->GetServiceForBrowserState(browser_state, true))
      ->MakeRemote();
}

// static
KeyringService* KeyringServiceFactory::GetServiceForState(
    ChromeBrowserState* browser_state) {
  return static_cast<KeyringService*>(
      GetInstance()->GetServiceForBrowserState(browser_state, true));
}

// static
KeyringServiceFactory* KeyringServiceFactory::GetInstance() {
  return base::Singleton<KeyringServiceFactory>::get();
}

KeyringServiceFactory::KeyringServiceFactory()
    : BrowserStateKeyedServiceFactory(
          "KeyringService",
          BrowserStateDependencyManager::GetInstance()) {
  DependsOn(brave_wallet::JsonRpcServiceFactory::GetInstance());
}

KeyringServiceFactory::~KeyringServiceFactory() = default;

std::unique_ptr<KeyedService> KeyringServiceFactory::BuildServiceInstanceFor(
    web::BrowserState* context) const {
  auto* browser_state = ChromeBrowserState::FromBrowserState(context);
  std::unique_ptr<KeyringService> keyring_service(new KeyringService(
      JsonRpcServiceFactory::GetServiceForState(browser_state),
      browser_state->GetPrefs(), GetApplicationContext()->GetLocalState()));
  return keyring_service;
}

bool KeyringServiceFactory::ServiceIsNULLWhileTesting() const {
  return true;
}

web::BrowserState* KeyringServiceFactory::GetBrowserStateToUse(
    web::BrowserState* context) const {
  return GetBrowserStateRedirectedInIncognito(context);
}

}  // namespace brave_wallet
