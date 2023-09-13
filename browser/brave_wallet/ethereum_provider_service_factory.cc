/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_wallet/ethereum_provider_service_factory.h"

#include <utility>

#include "brave/browser/brave_wallet/brave_wallet_context_utils.h"
#include "brave/browser/brave_wallet/brave_wallet_service_factory.h"
#include "brave/browser/brave_wallet/json_rpc_service_factory.h"
#include "brave/browser/brave_wallet/keyring_service_factory.h"
#include "brave/browser/brave_wallet/tx_service_factory.h"
#include "brave/components/brave_wallet/browser/brave_wallet_provider_delegate.h"
#include "brave/components/brave_wallet/browser/ethereum_provider_service.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/user_prefs/user_prefs.h"

namespace brave_wallet {

// static
EthereumProviderServiceFactory* EthereumProviderServiceFactory::GetInstance() {
  return base::Singleton<EthereumProviderServiceFactory>::get();
}

// static
mojo::PendingRemote<mojom::EthereumProvider>
EthereumProviderServiceFactory::GetForContext(
    content::BrowserContext* context) {
  if (!IsAllowedForContext(context)) {
    return mojo::PendingRemote<mojom::EthereumProvider>();
  }

  return static_cast<EthereumProviderService*>(
             GetInstance()->GetServiceForBrowserContext(context, true))
      ->MakeRemote();
}

// static
EthereumProviderService* EthereumProviderServiceFactory::GetServiceForContext(
    content::BrowserContext* context) {
  if (!IsAllowedForContext(context)) {
    return nullptr;
  }
  return static_cast<EthereumProviderService*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

// static
void EthereumProviderServiceFactory::BindForContext(
    content::BrowserContext* context,
    mojo::PendingReceiver<mojom::EthereumProvider> receiver,
    std::unique_ptr<BraveWalletProviderDelegate> delegate) {
  auto* ethereum_provider_service =
      EthereumProviderServiceFactory::GetServiceForContext(context);
  if (ethereum_provider_service) {
    ethereum_provider_service->Bind(std::move(receiver), std::move(delegate));
  }
}

EthereumProviderServiceFactory::EthereumProviderServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "EthereumProviderService",
          BrowserContextDependencyManager::GetInstance()) {
  DependsOn(HostContentSettingsMapFactory::GetInstance());
  DependsOn(JsonRpcServiceFactory::GetInstance());
  DependsOn(TxServiceFactory::GetInstance());
  DependsOn(KeyringServiceFactory::GetInstance());
  DependsOn(BraveWalletServiceFactory::GetInstance());
}

EthereumProviderServiceFactory::~EthereumProviderServiceFactory() = default;

KeyedService* EthereumProviderServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  return new EthereumProviderService(
      HostContentSettingsMapFactory::GetForProfile(context),
      JsonRpcServiceFactory::GetServiceForContext(context),
      TxServiceFactory::GetServiceForContext(context),
      KeyringServiceFactory::GetServiceForContext(context),
      BraveWalletServiceFactory::GetServiceForContext(context),
      user_prefs::UserPrefs::Get(context));
}

content::BrowserContext* EthereumProviderServiceFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return chrome::GetBrowserContextRedirectedInIncognito(context);
}

}  // namespace brave_wallet
