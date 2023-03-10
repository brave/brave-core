/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_wallet/bitcoin_rpc_service_factory.h"

#include <utility>

#include "brave/browser/brave_wallet/brave_wallet_context_utils.h"
#include "brave/browser/brave_wallet/keyring_service_factory.h"
#include "brave/components/brave_wallet/browser/bitcoin_rpc_service.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/storage_partition.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace brave_wallet {

// static
BitcoinRpcServiceFactory* BitcoinRpcServiceFactory::GetInstance() {
  return base::Singleton<BitcoinRpcServiceFactory>::get();
}

// static
mojo::PendingRemote<mojom::BitcoinRpcService>
BitcoinRpcServiceFactory::GetForContext(content::BrowserContext* context) {
  if (!IsAllowedForContext(context)) {
    return mojo::PendingRemote<mojom::BitcoinRpcService>();
  }

  return static_cast<BitcoinRpcService*>(
             GetInstance()->GetServiceForBrowserContext(context, true))
      ->MakeRemote();
}

// static
BitcoinRpcService* BitcoinRpcServiceFactory::GetServiceForContext(
    content::BrowserContext* context) {
  if (!IsAllowedForContext(context)) {
    return nullptr;
  }
  return static_cast<BitcoinRpcService*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

// static
void BitcoinRpcServiceFactory::BindForContext(
    content::BrowserContext* context,
    mojo::PendingReceiver<mojom::BitcoinRpcService> receiver) {
  auto* bitcoin_rpc_service =
      BitcoinRpcServiceFactory::GetServiceForContext(context);
  if (bitcoin_rpc_service) {
    bitcoin_rpc_service->Bind(std::move(receiver));
  }
}

BitcoinRpcServiceFactory::BitcoinRpcServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "BitcoinRpcService",
          BrowserContextDependencyManager::GetInstance()) {
  DependsOn(KeyringServiceFactory::GetInstance());
}

BitcoinRpcServiceFactory::~BitcoinRpcServiceFactory() = default;

KeyedService* BitcoinRpcServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  auto* default_storage_partition = context->GetDefaultStoragePartition();
  auto shared_url_loader_factory =
      default_storage_partition->GetURLLoaderFactoryForBrowserProcess();
  return new BitcoinRpcService(
      KeyringServiceFactory::GetServiceForContext(context),
      shared_url_loader_factory, user_prefs::UserPrefs::Get(context),
      g_browser_process->local_state());
}

content::BrowserContext* BitcoinRpcServiceFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return chrome::GetBrowserContextRedirectedInIncognito(context);
}

}  // namespace brave_wallet
