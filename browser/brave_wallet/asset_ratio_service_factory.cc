/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_wallet/asset_ratio_service_factory.h"

#include <memory>
#include <utility>

#include "base/no_destructor.h"
#include "brave/browser/brave_wallet/brave_wallet_context_utils.h"
#include "brave/components/brave_wallet/browser/asset_ratio_service.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "content/public/browser/storage_partition.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace brave_wallet {

// static
AssetRatioServiceFactory* AssetRatioServiceFactory::GetInstance() {
  static base::NoDestructor<AssetRatioServiceFactory> instance;
  return instance.get();
}

// static
mojo::PendingRemote<mojom::AssetRatioService>
AssetRatioServiceFactory::GetForContext(content::BrowserContext* context) {
  if (!IsAllowedForContext(context)) {
    return mojo::PendingRemote<mojom::AssetRatioService>();
  }

  return static_cast<AssetRatioService*>(
             GetInstance()->GetServiceForBrowserContext(context, true))
      ->MakeRemote();
}

// static
AssetRatioService* AssetRatioServiceFactory::GetServiceForContext(
    content::BrowserContext* context) {
  if (!IsAllowedForContext(context)) {
    return nullptr;
  }
  return static_cast<AssetRatioService*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

// static
void AssetRatioServiceFactory::BindForContext(
    content::BrowserContext* context,
    mojo::PendingReceiver<mojom::AssetRatioService> receiver) {
  auto* asset_ratio_service =
      AssetRatioServiceFactory::GetServiceForContext(context);
  if (asset_ratio_service) {
    asset_ratio_service->Bind(std::move(receiver));
  }
}

AssetRatioServiceFactory::AssetRatioServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "AssetRatioService",
          BrowserContextDependencyManager::GetInstance()) {}

AssetRatioServiceFactory::~AssetRatioServiceFactory() = default;

std::unique_ptr<KeyedService>
AssetRatioServiceFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  return std::make_unique<AssetRatioService>(
      context->GetDefaultStoragePartition()
          ->GetURLLoaderFactoryForBrowserProcess());
}

content::BrowserContext* AssetRatioServiceFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return GetBrowserContextRedirectedInIncognito(context);
}

}  // namespace brave_wallet
