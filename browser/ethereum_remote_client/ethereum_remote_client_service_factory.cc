/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ethereum_remote_client/ethereum_remote_client_service_factory.h"

#include <memory>

#include "brave/browser/ethereum_remote_client/ethereum_remote_client_delegate_impl.h"
#include "brave/browser/ethereum_remote_client/ethereum_remote_client_service.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "extensions/buildflags/buildflags.h"

#if BUILDFLAG(ENABLE_EXTENSIONS)
#include "extensions/browser/extension_registry_factory.h"
#include "extensions/browser/extension_system.h"
#include "extensions/browser/extension_system_provider.h"
#include "extensions/browser/extensions_browser_client.h"
#endif  // #if BUILDFLAG(ENABLE_EXTENSIONS)

// static
EthereumRemoteClientServiceFactory*
EthereumRemoteClientServiceFactory::GetInstance() {
  return base::Singleton<EthereumRemoteClientServiceFactory>::get();
}

// static
EthereumRemoteClientService* EthereumRemoteClientServiceFactory::GetForContext(
    content::BrowserContext* context) {
  return static_cast<EthereumRemoteClientService*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

EthereumRemoteClientServiceFactory::EthereumRemoteClientServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "EthereumRemoteClientService",
          BrowserContextDependencyManager::GetInstance()) {
#if BUILDFLAG(ENABLE_EXTENSIONS)
  DependsOn(extensions::ExtensionRegistryFactory::GetInstance());
  DependsOn(
      extensions::ExtensionsBrowserClient::Get()->GetExtensionSystemFactory());
#endif  // #if BUILDFLAG(ENABLE_EXTENSIONS)
}

EthereumRemoteClientServiceFactory::~EthereumRemoteClientServiceFactory() {}

KeyedService* EthereumRemoteClientServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  return new EthereumRemoteClientService(
      context, std::make_unique<EthereumRemoteClientDelegateImpl>());
}

content::BrowserContext*
EthereumRemoteClientServiceFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return chrome::GetBrowserContextRedirectedInIncognito(context);
}

bool EthereumRemoteClientServiceFactory::ServiceIsCreatedWithBrowserContext()
    const {
  return true;
}
