/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_wallet/keyring_controller_factory.h"

#include "brave/browser/brave_wallet/brave_wallet_context_utils.h"
#include "brave/components/brave_wallet/browser/keyring_controller.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/user_prefs/user_prefs.h"

namespace brave_wallet {

// static
KeyringControllerFactory* KeyringControllerFactory::GetInstance() {
  return base::Singleton<KeyringControllerFactory>::get();
}

// static
mojo::PendingRemote<mojom::KeyringController>
KeyringControllerFactory::GetForContext(content::BrowserContext* context) {
  if (!IsAllowedForContext(context))
    return mojo::PendingRemote<mojom::KeyringController>();

  return static_cast<KeyringController*>(
             GetInstance()->GetServiceForBrowserContext(context, true))
      ->MakeRemote();
}

// static
KeyringController* KeyringControllerFactory::GetControllerForContext(
    content::BrowserContext* context) {
  if (!IsAllowedForContext(context)) {
    return nullptr;
  }
  return static_cast<KeyringController*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

KeyringControllerFactory::KeyringControllerFactory()
    : BrowserContextKeyedServiceFactory(
          "KeyringController",
          BrowserContextDependencyManager::GetInstance()) {}

KeyringControllerFactory::~KeyringControllerFactory() {}

KeyedService* KeyringControllerFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  return new KeyringController(user_prefs::UserPrefs::Get(context));
}

content::BrowserContext* KeyringControllerFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return chrome::GetBrowserContextRedirectedInIncognito(context);
}

}  // namespace brave_wallet
