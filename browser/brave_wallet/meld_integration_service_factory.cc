/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_wallet/meld_integration_service_factory.h"

#include <memory>
#include <utility>

#include "brave/browser/brave_wallet/brave_wallet_context_utils.h"
#include "brave/components/brave_wallet/browser/meld_integration_service.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/storage_partition.h"

namespace brave_wallet {

// static
MeldIntegrationServiceFactory* MeldIntegrationServiceFactory::GetInstance() {
  static base::NoDestructor<MeldIntegrationServiceFactory> instance;
  return instance.get();
}

// static
mojo::PendingRemote<mojom::MeldIntegrationService>
MeldIntegrationServiceFactory::GetForContext(content::BrowserContext* context) {
  if (!IsAllowedForContext(context)) {
    return mojo::PendingRemote<mojom::MeldIntegrationService>();
  }

  return static_cast<MeldIntegrationService*>(
             GetInstance()->GetServiceForBrowserContext(context, true))
      ->MakeRemote();
}

// static
MeldIntegrationService* MeldIntegrationServiceFactory::GetServiceForContext(
    content::BrowserContext* context) {
  if (!IsAllowedForContext(context)) {
    return nullptr;
  }
  return static_cast<MeldIntegrationService*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

// static
void MeldIntegrationServiceFactory::BindForContext(
    content::BrowserContext* context,
    mojo::PendingReceiver<mojom::MeldIntegrationService> receiver) {
  auto* meld_integration_service =
      MeldIntegrationServiceFactory::GetServiceForContext(context);
  if (meld_integration_service) {
    meld_integration_service->Bind(std::move(receiver));
  }
}

MeldIntegrationServiceFactory::MeldIntegrationServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "MeldIntegrationService",
          BrowserContextDependencyManager::GetInstance()) {}

MeldIntegrationServiceFactory::~MeldIntegrationServiceFactory() = default;

std::unique_ptr<KeyedService>
MeldIntegrationServiceFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  return std::make_unique<MeldIntegrationService>(
      context->GetDefaultStoragePartition()
          ->GetURLLoaderFactoryForBrowserProcess());
}

content::BrowserContext* MeldIntegrationServiceFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return GetBrowserContextRedirectedInIncognito(context);
}

}  // namespace brave_wallet
