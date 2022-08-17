/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_wallet/swap_service_factory.h"

#include <utility>

#include "brave/browser/brave_wallet/brave_wallet_context_utils.h"
#include "brave/browser/brave_wallet/json_rpc_service_factory.h"
#include "brave/components/brave_wallet/browser/swap_service.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "content/public/browser/storage_partition.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace brave_wallet {

// static
SwapServiceFactory* SwapServiceFactory::GetInstance() {
  return base::Singleton<SwapServiceFactory>::get();
}

// static
mojo::PendingRemote<mojom::SwapService> SwapServiceFactory::GetForContext(
    content::BrowserContext* context) {
  if (!IsAllowedForContext(context))
    return mojo::PendingRemote<mojom::SwapService>();

  return static_cast<SwapService*>(
             GetInstance()->GetServiceForBrowserContext(context, true))
      ->MakeRemote();
}

// static
SwapService* SwapServiceFactory::GetServiceForContext(
    content::BrowserContext* context) {
  if (!IsAllowedForContext(context)) {
    return nullptr;
  }
  return static_cast<SwapService*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

// static
void SwapServiceFactory::BindForContext(
    content::BrowserContext* context,
    mojo::PendingReceiver<mojom::SwapService> receiver) {
  auto* swap_service = SwapServiceFactory::GetServiceForContext(context);
  if (swap_service) {
    swap_service->Bind(std::move(receiver));
  }
}

SwapServiceFactory::SwapServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "SwapService",
          BrowserContextDependencyManager::GetInstance()) {
  DependsOn(JsonRpcServiceFactory::GetInstance());
}

SwapServiceFactory::~SwapServiceFactory() = default;

KeyedService* SwapServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  auto* default_storage_partition = context->GetDefaultStoragePartition();
  auto shared_url_loader_factory =
      default_storage_partition->GetURLLoaderFactoryForBrowserProcess();

  return new SwapService(shared_url_loader_factory,
                         JsonRpcServiceFactory::GetServiceForContext(context));
}

content::BrowserContext* SwapServiceFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return chrome::GetBrowserContextRedirectedInIncognito(context);
}

}  // namespace brave_wallet
