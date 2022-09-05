/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_wallet/brave_wallet_pin_service_factory.h"

#include <memory>
#include <utility>

#include "brave/browser/brave_wallet/brave_wallet_context_utils.h"
#include "brave/browser/brave_wallet/json_rpc_service_factory.h"
#include "brave/browser/ipfs/ipfs_local_pin_service_factory.h"
#include "brave/browser/ipfs/ipfs_remote_pin_service_factory.h"
#include "brave/components/brave_wallet/browser/brave_wallet_pin_service.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service_delegate.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/user_prefs/user_prefs.h"

namespace brave_wallet {

// static
BraveWalletPinServiceFactory* BraveWalletPinServiceFactory::GetInstance() {
  return base::Singleton<BraveWalletPinServiceFactory>::get();
}

// static
mojo::PendingRemote<mojom::WalletPinService>
BraveWalletPinServiceFactory::GetForContext(content::BrowserContext* context) {
  if (!IsAllowedForContext(context)) {
    return mojo::PendingRemote<mojom::WalletPinService>();
  }

  return GetServiceForContext(context)->MakeRemote();
}

// static
BraveWalletPinService* BraveWalletPinServiceFactory::GetServiceForContext(
    content::BrowserContext* context) {
  if (!IsAllowedForContext(context)) {
    return nullptr;
  }
  return static_cast<BraveWalletPinService*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

// static
void BraveWalletPinServiceFactory::BindForContext(
    content::BrowserContext* context,
    mojo::PendingReceiver<mojom::WalletPinService> receiver) {
  auto* service = BraveWalletPinServiceFactory::GetServiceForContext(context);
  if (service) {
    service->Bind(std::move(receiver));
  }
}

BraveWalletPinServiceFactory::BraveWalletPinServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "BraveWalletPinService",
          BrowserContextDependencyManager::GetInstance()) {
  DependsOn(brave_wallet::JsonRpcServiceFactory::GetInstance());
  DependsOn(ipfs::IpfsRemotePinServiceFactory::GetInstance());
  DependsOn(ipfs::IpfsLocalPinServiceFactory::GetInstance());
}

BraveWalletPinServiceFactory::~BraveWalletPinServiceFactory() = default;

KeyedService* BraveWalletPinServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  return new BraveWalletPinService(
      user_prefs::UserPrefs::Get(context),
      JsonRpcServiceFactory::GetServiceForContext(context),
      ipfs::IpfsRemotePinServiceFactory::GetServiceForContext(context),
      ipfs::IpfsLocalPinServiceFactory::GetServiceForContext(context));
}

content::BrowserContext* BraveWalletPinServiceFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return chrome::GetBrowserContextRedirectedInIncognito(context);
}

}  // namespace brave_wallet
