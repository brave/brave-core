/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_wallet/json_rpc_service_factory.h"

#include <utility>

#include "brave/browser/brave_wallet/brave_wallet_context_utils.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/storage_partition.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace brave_wallet {

// static
JsonRpcServiceFactory* JsonRpcServiceFactory::GetInstance() {
  return base::Singleton<JsonRpcServiceFactory>::get();
}

// static
mojo::PendingRemote<mojom::JsonRpcService> JsonRpcServiceFactory::GetForContext(
    content::BrowserContext* context) {
  if (!IsAllowedForContext(context)) {
    return mojo::PendingRemote<mojom::JsonRpcService>();
  }

  return static_cast<JsonRpcService*>(
             GetInstance()->GetServiceForBrowserContext(context, true))
      ->MakeRemote();
}

// static
JsonRpcService* JsonRpcServiceFactory::GetServiceForContext(
    content::BrowserContext* context) {
  if (!IsAllowedForContext(context)) {
    return nullptr;
  }
  return static_cast<JsonRpcService*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

// static
void JsonRpcServiceFactory::BindForContext(
    content::BrowserContext* context,
    mojo::PendingReceiver<mojom::JsonRpcService> receiver) {
  auto* json_rpc_service = JsonRpcServiceFactory::GetServiceForContext(context);
  if (json_rpc_service) {
    json_rpc_service->Bind(std::move(receiver));
  }
}

JsonRpcServiceFactory::JsonRpcServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "JsonRpcService",
          BrowserContextDependencyManager::GetInstance()) {}

JsonRpcServiceFactory::~JsonRpcServiceFactory() = default;

KeyedService* JsonRpcServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  auto* default_storage_partition = context->GetDefaultStoragePartition();
  auto shared_url_loader_factory =
      default_storage_partition->GetURLLoaderFactoryForBrowserProcess();
  return new JsonRpcService(shared_url_loader_factory,
                            user_prefs::UserPrefs::Get(context),
                            g_browser_process->local_state());
}

content::BrowserContext* JsonRpcServiceFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return chrome::GetBrowserContextRedirectedInIncognito(context);
}

}  // namespace brave_wallet
