/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_wallet/brave_wallet_ipfs_service_factory.h"

#include <memory>
#include <utility>

#include "base/no_destructor.h"
#include "brave/browser/brave_wallet/brave_wallet_context_utils.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/storage_partition.h"

namespace brave_wallet {

// static
BraveWalletIpfsServiceFactory* BraveWalletIpfsServiceFactory::GetInstance() {
  static base::NoDestructor<BraveWalletIpfsServiceFactory> instance;
  return instance.get();
}

// static
mojo::PendingRemote<mojom::IpfsService>
BraveWalletIpfsServiceFactory::GetForContext(content::BrowserContext* context) {
  if (!IsAllowedForContext(context)) {
    return mojo::PendingRemote<mojom::IpfsService>();
  }

  return static_cast<BraveWalletIpfsService*>(
             GetInstance()->GetServiceForBrowserContext(context, true))
      ->MakeRemote();
}

// static
BraveWalletIpfsService* BraveWalletIpfsServiceFactory::GetServiceForContext(
    content::BrowserContext* context) {
  if (!IsAllowedForContext(context)) {
    return nullptr;
  }
  return static_cast<BraveWalletIpfsService*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

// static
void BraveWalletIpfsServiceFactory::BindForContext(
    content::BrowserContext* context,
    mojo::PendingReceiver<mojom::IpfsService> receiver) {
  auto* ipfs_service =
      BraveWalletIpfsServiceFactory::GetServiceForContext(context);
  if (ipfs_service) {
    ipfs_service->Bind(std::move(receiver));
  }
}

BraveWalletIpfsServiceFactory::BraveWalletIpfsServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "BraveWalletIpfsService",
          BrowserContextDependencyManager::GetInstance()) {}

BraveWalletIpfsServiceFactory::~BraveWalletIpfsServiceFactory() = default;

std::unique_ptr<KeyedService>
BraveWalletIpfsServiceFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  return std::make_unique<BraveWalletIpfsService>(
      user_prefs::UserPrefs::Get(context));
}

content::BrowserContext* BraveWalletIpfsServiceFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return chrome::GetBrowserContextRedirectedInIncognito(context);
}

}  // namespace brave_wallet
