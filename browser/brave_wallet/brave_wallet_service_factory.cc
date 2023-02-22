/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_wallet/brave_wallet_service_factory.h"

#include <memory>
#include <utility>

#include "brave/browser/brave_wallet/brave_wallet_context_utils.h"
#include "brave/browser/brave_wallet/json_rpc_service_factory.h"
#include "brave/browser/brave_wallet/keyring_service_factory.h"
#include "brave/browser/brave_wallet/tx_service_factory.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service_delegate.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/storage_partition.h"

namespace brave_wallet {

// static
BraveWalletServiceFactory* BraveWalletServiceFactory::GetInstance() {
  return base::Singleton<BraveWalletServiceFactory>::get();
}

// static
mojo::PendingRemote<mojom::BraveWalletService>
BraveWalletServiceFactory::GetForContext(content::BrowserContext* context) {
  if (!IsAllowedForContext(context)) {
    return mojo::PendingRemote<mojom::BraveWalletService>();
  }

  return static_cast<BraveWalletService*>(
             GetInstance()->GetServiceForBrowserContext(context, true))
      ->MakeRemote();
}

// static
BraveWalletService* BraveWalletServiceFactory::GetServiceForContext(
    content::BrowserContext* context) {
  if (!IsAllowedForContext(context)) {
    return nullptr;
  }
  return static_cast<BraveWalletService*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

// static
void BraveWalletServiceFactory::BindForContext(
    content::BrowserContext* context,
    mojo::PendingReceiver<mojom::BraveWalletService> receiver) {
  auto* brave_wallet_service =
      BraveWalletServiceFactory::GetServiceForContext(context);
  if (brave_wallet_service) {
    brave_wallet_service->Bind(std::move(receiver));
  }
}

BraveWalletServiceFactory::BraveWalletServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "BraveWalletService",
          BrowserContextDependencyManager::GetInstance()) {
  DependsOn(KeyringServiceFactory::GetInstance());
  DependsOn(JsonRpcServiceFactory::GetInstance());
  DependsOn(TxServiceFactory::GetInstance());
}

BraveWalletServiceFactory::~BraveWalletServiceFactory() = default;

KeyedService* BraveWalletServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  auto* default_storage_partition = context->GetDefaultStoragePartition();
  auto shared_url_loader_factory =
      default_storage_partition->GetURLLoaderFactoryForBrowserProcess();
  return new BraveWalletService(
      shared_url_loader_factory, BraveWalletServiceDelegate::Create(context),
      KeyringServiceFactory::GetServiceForContext(context),
      JsonRpcServiceFactory::GetServiceForContext(context),
      TxServiceFactory::GetServiceForContext(context),
      user_prefs::UserPrefs::Get(context), g_browser_process->local_state());
}

content::BrowserContext* BraveWalletServiceFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return chrome::GetBrowserContextRedirectedInIncognito(context);
}

}  // namespace brave_wallet
