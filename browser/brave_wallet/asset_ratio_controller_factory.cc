/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_wallet/asset_ratio_controller_factory.h"

#include "brave/browser/brave_wallet/brave_wallet_context_utils.h"
#include "brave/components/brave_wallet/browser/asset_ratio_controller.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "content/public/browser/storage_partition.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace brave_wallet {

// static
AssetRatioControllerFactory* AssetRatioControllerFactory::GetInstance() {
  return base::Singleton<AssetRatioControllerFactory>::get();
}

// static
mojo::PendingRemote<mojom::AssetRatioController>
AssetRatioControllerFactory::GetForContext(content::BrowserContext* context) {
  if (!IsAllowedForContext(context))
    return mojo::PendingRemote<mojom::AssetRatioController>();

  return static_cast<AssetRatioController*>(
             GetInstance()->GetServiceForBrowserContext(context, true))
      ->MakeRemote();
}

// static
AssetRatioController* AssetRatioControllerFactory::GetControllerForContext(
    content::BrowserContext* context) {
  if (!IsAllowedForContext(context)) {
    return nullptr;
  }
  return static_cast<AssetRatioController*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

AssetRatioControllerFactory::AssetRatioControllerFactory()
    : BrowserContextKeyedServiceFactory(
          "AssetRatioController",
          BrowserContextDependencyManager::GetInstance()) {}

AssetRatioControllerFactory::~AssetRatioControllerFactory() {}

KeyedService* AssetRatioControllerFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  auto* default_storage_partition = context->GetDefaultStoragePartition();
  auto shared_url_loader_factory =
      default_storage_partition->GetURLLoaderFactoryForBrowserProcess();

  return new AssetRatioController(shared_url_loader_factory);
}

content::BrowserContext* AssetRatioControllerFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return chrome::GetBrowserContextRedirectedInIncognito(context);
}

}  // namespace brave_wallet
