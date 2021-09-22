/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/brave_wallet/keyring_controller_factory.h"

#include "brave/components/brave_wallet/browser/keyring_controller.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/keyed_service/ios/browser_state_dependency_manager.h"
#include "ios/chrome/browser/browser_state/browser_state_otr_helper.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#include "ios/web/public/browser_state.h"

namespace brave_wallet {

// static
mojom::KeyringController* KeyringControllerFactory::GetForBrowserState(
    ChromeBrowserState* browser_state) {
  return static_cast<KeyringController*>(
      GetInstance()->GetServiceForBrowserState(browser_state, true));
}

// static
KeyringController* KeyringControllerFactory::GetControllerForBrowserState(
    ChromeBrowserState* browser_state) {
  return static_cast<KeyringController*>(
      GetInstance()->GetServiceForBrowserState(browser_state, true));
}

// static
KeyringControllerFactory* KeyringControllerFactory::GetInstance() {
  return base::Singleton<KeyringControllerFactory>::get();
}

KeyringControllerFactory::KeyringControllerFactory()
    : BrowserStateKeyedServiceFactory(
          "KeyringController",
          BrowserStateDependencyManager::GetInstance()) {}

KeyringControllerFactory::~KeyringControllerFactory() = default;

std::unique_ptr<KeyedService> KeyringControllerFactory::BuildServiceInstanceFor(
    web::BrowserState* context) const {
  auto* browser_state = ChromeBrowserState::FromBrowserState(context);
  std::unique_ptr<KeyringController> keyring_controller(
      new KeyringController(browser_state->GetPrefs()));
  return keyring_controller;
}

bool KeyringControllerFactory::ServiceIsNULLWhileTesting() const {
  return true;
}

web::BrowserState* KeyringControllerFactory::GetBrowserStateToUse(
    web::BrowserState* context) const {
  return GetBrowserStateRedirectedInIncognito(context);
}

}  // namespace brave_wallet
