/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_wallet/rpc_controller_factory.h"

#include "brave/browser/brave_wallet/brave_wallet_context_utils.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/eth_json_rpc_controller.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/storage_partition.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace brave_wallet {

// static
RpcControllerFactory* RpcControllerFactory::GetInstance() {
  return base::Singleton<RpcControllerFactory>::get();
}

// static
mojo::PendingRemote<mojom::EthJsonRpcController>
RpcControllerFactory::GetForContext(content::BrowserContext* context) {
  if (!IsAllowedForContext(context)) {
    return mojo::PendingRemote<mojom::EthJsonRpcController>();
  }

  return static_cast<EthJsonRpcController*>(
             GetInstance()->GetServiceForBrowserContext(context, true))
      ->MakeRemote();
}

// static
EthJsonRpcController* RpcControllerFactory::GetControllerForContext(
    content::BrowserContext* context) {
  if (!IsAllowedForContext(context)) {
    return nullptr;
  }
  return static_cast<EthJsonRpcController*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

RpcControllerFactory::RpcControllerFactory()
    : BrowserContextKeyedServiceFactory(
          "RpcController",
          BrowserContextDependencyManager::GetInstance()) {}

RpcControllerFactory::~RpcControllerFactory() {}

KeyedService* RpcControllerFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  auto* default_storage_partition = context->GetDefaultStoragePartition();
  auto shared_url_loader_factory =
      default_storage_partition->GetURLLoaderFactoryForBrowserProcess();
  return new EthJsonRpcController(shared_url_loader_factory,
                                  user_prefs::UserPrefs::Get(context));
}

content::BrowserContext* RpcControllerFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return chrome::GetBrowserContextRedirectedInIncognito(context);
}

}  // namespace brave_wallet
