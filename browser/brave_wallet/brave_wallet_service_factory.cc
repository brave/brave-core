/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_wallet/brave_wallet_service_factory.h"

#include <memory>

#include "brave/browser/brave_wallet/brave_wallet_delegate_impl.h"
#include "brave/components/brave_wallet/brave_wallet_service.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "extensions/buildflags/buildflags.h"

#if BUILDFLAG(ENABLE_EXTENSIONS)
#include "extensions/browser/extension_registry_factory.h"
#include "extensions/browser/extension_system.h"
#include "extensions/browser/extension_system_provider.h"
#include "extensions/browser/extensions_browser_client.h"
#endif  // #if BUILDFLAG(ENABLE_EXTENSIONS)

// static
BraveWalletServiceFactory* BraveWalletServiceFactory::GetInstance() {
  return base::Singleton<BraveWalletServiceFactory>::get();
}

// static
BraveWalletService* BraveWalletServiceFactory::GetForProfile(Profile* profile) {
  return static_cast<BraveWalletService*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

BraveWalletServiceFactory::BraveWalletServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "BraveWalletService",
          BrowserContextDependencyManager::GetInstance()) {
#if BUILDFLAG(ENABLE_EXTENSIONS)
  DependsOn(extensions::ExtensionRegistryFactory::GetInstance());
  DependsOn(
      extensions::ExtensionsBrowserClient::Get()->GetExtensionSystemFactory());
#endif  // #if BUILDFLAG(ENABLE_EXTENSIONS)
}

BraveWalletServiceFactory::~BraveWalletServiceFactory() {}

KeyedService* BraveWalletServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  return new BraveWalletService(Profile::FromBrowserContext(context),
                                std::make_unique<BraveWalletDelegateImpl>());
}

content::BrowserContext* BraveWalletServiceFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return chrome::GetBrowserContextRedirectedInIncognito(context);
}
