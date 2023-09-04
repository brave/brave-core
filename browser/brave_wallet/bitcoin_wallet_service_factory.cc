/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_wallet/bitcoin_wallet_service_factory.h"

#include <utility>

#include "base/no_destructor.h"
#include "brave/browser/brave_wallet/brave_wallet_context_utils.h"
#include "brave/browser/brave_wallet/keyring_service_factory.h"
#include "brave/components/brave_wallet/browser/bitcoin/bitcoin_wallet_service.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/storage_partition.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace brave_wallet {

// static
BitcoinWalletServiceFactory* BitcoinWalletServiceFactory::GetInstance() {
  static base::NoDestructor<BitcoinWalletServiceFactory> instance;
  return instance.get();
}

// static
mojo::PendingRemote<mojom::BitcoinWalletService>
BitcoinWalletServiceFactory::GetForContext(content::BrowserContext* context) {
  if (!IsAllowedForContext(context)) {
    return mojo::PendingRemote<mojom::BitcoinWalletService>();
  }

  if (!IsBitcoinEnabled()) {
    return mojo::PendingRemote<mojom::BitcoinWalletService>();
  }

  return static_cast<BitcoinWalletService*>(
             GetInstance()->GetServiceForBrowserContext(context, true))
      ->MakeRemote();
}

// static
BitcoinWalletService* BitcoinWalletServiceFactory::GetServiceForContext(
    content::BrowserContext* context) {
  if (!IsAllowedForContext(context)) {
    return nullptr;
  }
  if (!IsBitcoinEnabled()) {
    return nullptr;
  }
  return static_cast<BitcoinWalletService*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

// static
void BitcoinWalletServiceFactory::BindForContext(
    content::BrowserContext* context,
    mojo::PendingReceiver<mojom::BitcoinWalletService> receiver) {
  auto* bitcoin_rpc_service =
      BitcoinWalletServiceFactory::GetServiceForContext(context);
  if (bitcoin_rpc_service) {
    bitcoin_rpc_service->Bind(std::move(receiver));
  }
}

BitcoinWalletServiceFactory::BitcoinWalletServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "BitcoinWalletService",
          BrowserContextDependencyManager::GetInstance()) {
  DependsOn(KeyringServiceFactory::GetInstance());
}

BitcoinWalletServiceFactory::~BitcoinWalletServiceFactory() = default;

KeyedService* BitcoinWalletServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  auto* default_storage_partition = context->GetDefaultStoragePartition();
  auto shared_url_loader_factory =
      default_storage_partition->GetURLLoaderFactoryForBrowserProcess();
  return new BitcoinWalletService(
      KeyringServiceFactory::GetServiceForContext(context),
      user_prefs::UserPrefs::Get(context), shared_url_loader_factory);
}

content::BrowserContext* BitcoinWalletServiceFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return chrome::GetBrowserContextRedirectedInIncognito(context);
}

}  // namespace brave_wallet
