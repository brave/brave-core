/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_wallet/buy_and_sell_service_factory.h"

#include <utility>

#include "brave/browser/brave_wallet/brave_wallet_context_utils.h"
#include "brave/components/brave_wallet/browser/buy_and_sell_service.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/storage_partition.h"

namespace brave_wallet {

// static
BuyAndSellServiceFactory* BuyAndSellServiceFactory::GetInstance() {
  static base::NoDestructor<BuyAndSellServiceFactory> instance;
  return instance.get();
}

// static
mojo::PendingRemote<mojom::BuyAndSellService>
BuyAndSellServiceFactory::GetForContext(content::BrowserContext* context) {
  if (!IsAllowedForContext(context)) {
    return mojo::PendingRemote<mojom::BuyAndSellService>();
  }

  return static_cast<BuyAndSellService*>(
             GetInstance()->GetServiceForBrowserContext(context, true))
      ->MakeRemote();
}

// static
BuyAndSellService* BuyAndSellServiceFactory::GetServiceForContext(
    content::BrowserContext* context) {
  if (!IsAllowedForContext(context)) {
    return nullptr;
  }
  return static_cast<BuyAndSellService*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

// static
void BuyAndSellServiceFactory::BindForContext(
    content::BrowserContext* context,
    mojo::PendingReceiver<mojom::BuyAndSellService> receiver) {
  auto* buy_and_sell_service =
      BuyAndSellServiceFactory::GetServiceForContext(context);
  if (buy_and_sell_service) {
    buy_and_sell_service->Bind(std::move(receiver));
  }
}

BuyAndSellServiceFactory::BuyAndSellServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "BuyAndSellService",
          BrowserContextDependencyManager::GetInstance()) {}

BuyAndSellServiceFactory::~BuyAndSellServiceFactory() = default;

KeyedService* BuyAndSellServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  auto* default_storage_partition = context->GetDefaultStoragePartition();
  auto shared_url_loader_factory =
      default_storage_partition->GetURLLoaderFactoryForBrowserProcess();

  return new BuyAndSellService(shared_url_loader_factory);
}

content::BrowserContext* BuyAndSellServiceFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return chrome::GetBrowserContextRedirectedInIncognito(context);
}

}  // namespace brave_wallet
