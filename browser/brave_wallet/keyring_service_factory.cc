/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_wallet/keyring_service_factory.h"

#include <utility>

#include "brave/browser/brave_wallet/brave_wallet_context_utils.h"
#include "brave/browser/brave_wallet/json_rpc_service_factory.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/user_prefs/user_prefs.h"

namespace brave_wallet {

// static
KeyringServiceFactory* KeyringServiceFactory::GetInstance() {
  return base::Singleton<KeyringServiceFactory>::get();
}

// static
mojo::PendingRemote<mojom::KeyringService> KeyringServiceFactory::GetForContext(
    content::BrowserContext* context) {
  if (!IsAllowedForContext(context))
    return mojo::PendingRemote<mojom::KeyringService>();

  return static_cast<KeyringService*>(
             GetInstance()->GetServiceForBrowserContext(context, true))
      ->MakeRemote();
}

// static
KeyringService* KeyringServiceFactory::GetServiceForContext(
    content::BrowserContext* context) {
  if (!IsAllowedForContext(context)) {
    return nullptr;
  }
  return static_cast<KeyringService*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

// static
void KeyringServiceFactory::BindForContext(
    content::BrowserContext* context,
    mojo::PendingReceiver<mojom::KeyringService> receiver) {
  auto* keyring_service = KeyringServiceFactory::GetServiceForContext(context);
  if (keyring_service) {
    keyring_service->Bind(std::move(receiver));
  }
}

KeyringServiceFactory::KeyringServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "KeyringService",
          BrowserContextDependencyManager::GetInstance()) {
  DependsOn(JsonRpcServiceFactory::GetInstance());
}

KeyringServiceFactory::~KeyringServiceFactory() = default;

KeyedService* KeyringServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  return new KeyringService(
      JsonRpcServiceFactory::GetServiceForContext(context),
      user_prefs::UserPrefs::Get(context), g_browser_process->local_state());
}

content::BrowserContext* KeyringServiceFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return chrome::GetBrowserContextRedirectedInIncognito(context);
}

}  // namespace brave_wallet
