/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_wallet/swap_controller_factory.h"

#include "brave/browser/brave_wallet/brave_wallet_context_utils.h"
#include "brave/components/brave_wallet/browser/swap_controller.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "content/public/browser/storage_partition.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace brave_wallet {

// static
SwapControllerFactory* SwapControllerFactory::GetInstance() {
  return base::Singleton<SwapControllerFactory>::get();
}

// static
mojo::PendingRemote<mojom::SwapController> SwapControllerFactory::GetForContext(
    content::BrowserContext* context) {
  if (!IsAllowedForContext(context))
    return mojo::PendingRemote<mojom::SwapController>();

  return static_cast<SwapController*>(
             GetInstance()->GetServiceForBrowserContext(context, true))
      ->MakeRemote();
}

// static
SwapController* SwapControllerFactory::GetControllerForContext(
    content::BrowserContext* context) {
  if (!IsAllowedForContext(context)) {
    return nullptr;
  }
  return static_cast<SwapController*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

SwapControllerFactory::SwapControllerFactory()
    : BrowserContextKeyedServiceFactory(
          "SwapController",
          BrowserContextDependencyManager::GetInstance()) {}

SwapControllerFactory::~SwapControllerFactory() {}

KeyedService* SwapControllerFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  auto* default_storage_partition = context->GetDefaultStoragePartition();
  auto shared_url_loader_factory =
      default_storage_partition->GetURLLoaderFactoryForBrowserProcess();

  return new SwapController(shared_url_loader_factory);
}

content::BrowserContext* SwapControllerFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return chrome::GetBrowserContextRedirectedInIncognito(context);
}

}  // namespace brave_wallet
