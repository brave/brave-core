// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/brave_wallet/brave_wallet_auto_pin_service_factory.h"

#include <memory>
#include <utility>

#include "brave/browser/brave_wallet/brave_wallet_context_utils.h"
#include "brave/browser/brave_wallet/brave_wallet_pin_service_factory.h"
#include "brave/browser/brave_wallet/brave_wallet_service_factory.h"
// TODO(cypt4) : Refactor brave/browser/ipfs into separate component (#27486)
#include "brave/browser/ipfs/ipfs_service_factory.h"  // nogncheck

#include "brave/components/brave_wallet/browser/brave_wallet_pin_service.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"

#include "chrome/browser/profiles/incognito_helpers.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/user_prefs/user_prefs.h"

namespace brave_wallet {

// static
BraveWalletAutoPinServiceFactory*
BraveWalletAutoPinServiceFactory::GetInstance() {
  return base::Singleton<BraveWalletAutoPinServiceFactory>::get();
}

// static
mojo::PendingRemote<mojom::WalletAutoPinService>
BraveWalletAutoPinServiceFactory::GetForContext(
    content::BrowserContext* context) {
  if (!IsAllowedForContext(context)) {
    return mojo::PendingRemote<mojom::WalletAutoPinService>();
  }

  auto* service = GetServiceForContext(context);

  if (!service) {
    return mojo::PendingRemote<mojom::WalletAutoPinService>();
  }

  return service->MakeRemote();
}

// static
BraveWalletAutoPinService*
BraveWalletAutoPinServiceFactory::GetServiceForContext(
    content::BrowserContext* context) {
  if (!IsAllowedForContext(context)) {
    return nullptr;
  }
  if (!ipfs::IpfsServiceFactory::IsIpfsEnabled(context)) {
    return nullptr;
  }
  if (!brave_wallet::IsNftPinningEnabled()) {
    return nullptr;
  }
  return static_cast<BraveWalletAutoPinService*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

// static
void BraveWalletAutoPinServiceFactory::BindForContext(
    content::BrowserContext* context,
    mojo::PendingReceiver<mojom::WalletAutoPinService> receiver) {
  auto* service =
      BraveWalletAutoPinServiceFactory::GetServiceForContext(context);
  if (service) {
    service->Bind(std::move(receiver));
  }
}

BraveWalletAutoPinServiceFactory::BraveWalletAutoPinServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "BraveWalletAutoPinService",
          BrowserContextDependencyManager::GetInstance()) {
  DependsOn(BraveWalletServiceFactory::GetInstance());
  DependsOn(BraveWalletPinServiceFactory::GetInstance());
}

BraveWalletAutoPinServiceFactory::~BraveWalletAutoPinServiceFactory() = default;

KeyedService* BraveWalletAutoPinServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  return new BraveWalletAutoPinService(
      user_prefs::UserPrefs::Get(context),
      BraveWalletServiceFactory::GetServiceForContext(context),
      BraveWalletPinServiceFactory::GetServiceForContext(context));
}

content::BrowserContext*
BraveWalletAutoPinServiceFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return chrome::GetBrowserContextRedirectedInIncognito(context);
}

}  // namespace brave_wallet
