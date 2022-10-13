/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/wallet_connect/wallet_connect_service_factory.h"

#include <utility>

#include "brave/browser/brave_wallet/brave_wallet_context_utils.h"
#include "brave/components/wallet_connect/wallet_connect_service.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/user_prefs/user_prefs.h"

namespace wallet_connect {

// static
WalletConnectServiceFactory* WalletConnectServiceFactory::GetInstance() {
  return base::Singleton<WalletConnectServiceFactory>::get();
}

// static
mojo::PendingRemote<mojom::WalletConnectService>
WalletConnectServiceFactory::GetForContext(content::BrowserContext* context) {
  if (!brave_wallet::IsAllowedForContext(context))
    return mojo::PendingRemote<mojom::WalletConnectService>();

  return static_cast<WalletConnectService*>(
             GetInstance()->GetServiceForBrowserContext(context, true))
      ->MakeRemote();
}

// static
WalletConnectService* WalletConnectServiceFactory::GetServiceForContext(
    content::BrowserContext* context) {
  if (!brave_wallet::IsAllowedForContext(context)) {
    return nullptr;
  }
  return static_cast<WalletConnectService*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

// static
void WalletConnectServiceFactory::BindForContext(
    content::BrowserContext* context,
    mojo::PendingReceiver<mojom::WalletConnectService> receiver) {
  auto* wallet_connect_service =
      WalletConnectServiceFactory::GetServiceForContext(context);
  if (wallet_connect_service) {
    wallet_connect_service->Bind(std::move(receiver));
  }
}

WalletConnectServiceFactory::WalletConnectServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "WalletConnectService",
          BrowserContextDependencyManager::GetInstance()) {
  // DependsOn(EthereumProviderImpl);
}

WalletConnectServiceFactory::~WalletConnectServiceFactory() = default;

KeyedService* WalletConnectServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  return new WalletConnectService();
}

content::BrowserContext* WalletConnectServiceFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return chrome::GetBrowserContextRedirectedInIncognito(context);
}

}  // namespace wallet_connect
