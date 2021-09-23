/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_wallet/brave_wallet_service_factory.h"

#include <memory>

#include "brave/browser/brave_wallet/brave_wallet_context_utils.h"
#include "brave/browser/brave_wallet/brave_wallet_importer_delegate_impl.h"
#include "brave/components/brave_wallet/browser/brave_wallet_importer_delegate.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/user_prefs/user_prefs.h"

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

BraveWalletServiceFactory::BraveWalletServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "BraveWalletService",
          BrowserContextDependencyManager::GetInstance()) {}

BraveWalletServiceFactory::~BraveWalletServiceFactory() = default;

KeyedService* BraveWalletServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  return new BraveWalletService(
#if defined(OS_ANDROID)
      std::make_unique<BraveWalletImporterDelegate>(),
#else
      std::make_unique<BraveWalletImporterDelegateImpl>(context),
#endif
      user_prefs::UserPrefs::Get(context));
}

content::BrowserContext* BraveWalletServiceFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return chrome::GetBrowserContextRedirectedInIncognito(context);
}

}  // namespace brave_wallet
