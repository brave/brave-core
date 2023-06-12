/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_wallet/simulation_service_factory.h"

#include <utility>

#include "base/no_destructor.h"
#include "brave/browser/brave_wallet/brave_wallet_context_utils.h"
#include "brave/components/brave_wallet/browser/simulation_service.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "content/public/browser/storage_partition.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace brave_wallet {

// static
SimulationServiceFactory* SimulationServiceFactory::GetInstance() {
  static base::NoDestructor<SimulationServiceFactory> instance;
  return instance.get();
}

// static
mojo::PendingRemote<mojom::SimulationService>
SimulationServiceFactory::GetForContext(content::BrowserContext* context) {
  if (!IsAllowedForContext(context)) {
    return mojo::PendingRemote<mojom::SimulationService>();
  }

  return static_cast<SimulationService*>(
             GetInstance()->GetServiceForBrowserContext(context, true))
      ->MakeRemote();
}

// static
SimulationService* SimulationServiceFactory::GetServiceForContext(
    content::BrowserContext* context) {
  if (!IsAllowedForContext(context)) {
    return nullptr;
  }
  return static_cast<SimulationService*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

// static
void SimulationServiceFactory::BindForContext(
    content::BrowserContext* context,
    mojo::PendingReceiver<mojom::SimulationService> receiver) {
  auto* simulation_service =
      SimulationServiceFactory::GetServiceForContext(context);
  if (simulation_service) {
    simulation_service->Bind(std::move(receiver));
  }
}

SimulationServiceFactory::SimulationServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "SimulationService",
          BrowserContextDependencyManager::GetInstance()) {}

SimulationServiceFactory::~SimulationServiceFactory() = default;

KeyedService* SimulationServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  auto* default_storage_partition = context->GetDefaultStoragePartition();
  auto shared_url_loader_factory =
      default_storage_partition->GetURLLoaderFactoryForBrowserProcess();

  return new SimulationService(shared_url_loader_factory);
}

content::BrowserContext* SimulationServiceFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return chrome::GetBrowserContextRedirectedInIncognito(context);
}

}  // namespace brave_wallet
