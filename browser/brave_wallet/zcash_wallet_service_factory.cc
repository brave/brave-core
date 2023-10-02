/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_wallet/zcash_wallet_service_factory.h"

#include <utility>

#include "base/no_destructor.h"
#include "brave/browser/brave_wallet/brave_wallet_context_utils.h"
#include "brave/browser/brave_wallet/keyring_service_factory.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_wallet_service.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/storage_partition.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace brave_wallet {

// static
ZCashWalletServiceFactory* ZCashWalletServiceFactory::GetInstance() {
  static base::NoDestructor<ZCashWalletServiceFactory> instance;
  return instance.get();
}

// static
mojo::PendingRemote<mojom::ZCashWalletService>
ZCashWalletServiceFactory::GetForContext(content::BrowserContext* context) {
  if (!IsAllowedForContext(context)) {
    return mojo::PendingRemote<mojom::ZCashWalletService>();
  }

  if (!IsZCashEnabled()) {
    return mojo::PendingRemote<mojom::ZCashWalletService>();
  }

  return static_cast<ZCashWalletService*>(
             GetInstance()->GetServiceForBrowserContext(context, true))
      ->MakeRemote();
}

// static
ZCashWalletService* ZCashWalletServiceFactory::GetServiceForContext(
    content::BrowserContext* context) {
  if (!IsAllowedForContext(context)) {
    return nullptr;
  }
  if (!IsZCashEnabled()) {
    return nullptr;
  }
  return static_cast<ZCashWalletService*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

// static
void ZCashWalletServiceFactory::BindForContext(
    content::BrowserContext* context,
    mojo::PendingReceiver<mojom::ZCashWalletService> receiver) {
  auto* zcash_service =
      ZCashWalletServiceFactory::GetServiceForContext(context);
  if (zcash_service) {
    zcash_service->Bind(std::move(receiver));
  }
}

ZCashWalletServiceFactory::ZCashWalletServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "ZCashWalletService",
          BrowserContextDependencyManager::GetInstance()) {
  DependsOn(KeyringServiceFactory::GetInstance());
}

ZCashWalletServiceFactory::~ZCashWalletServiceFactory() = default;

KeyedService* ZCashWalletServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  auto* default_storage_partition = context->GetDefaultStoragePartition();
  auto shared_url_loader_factory =
      default_storage_partition->GetURLLoaderFactoryForBrowserProcess();
  return new ZCashWalletService(
      KeyringServiceFactory::GetServiceForContext(context),
      user_prefs::UserPrefs::Get(context), shared_url_loader_factory);
}

content::BrowserContext* ZCashWalletServiceFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return chrome::GetBrowserContextRedirectedInIncognito(context);
}

}  // namespace brave_wallet
