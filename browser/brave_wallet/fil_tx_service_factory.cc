/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_wallet/fil_tx_service_factory.h"

#include <memory>
#include <utility>

#include "brave/browser/brave_wallet/asset_ratio_service_factory.h"
#include "brave/browser/brave_wallet/brave_wallet_context_utils.h"
#include "brave/browser/brave_wallet/json_rpc_service_factory.h"
#include "brave/browser/brave_wallet/keyring_service_factory.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/fil_tx_service.h"
#include "brave/components/brave_wallet/factory/eth_tx_service_factory_helper.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/storage_partition.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace brave_wallet {

// static
FilTxServiceFactory* FilTxServiceFactory::GetInstance() {
  return base::Singleton<FilTxServiceFactory>::get();
}

// static
mojo::PendingRemote<mojom::FilTxService> FilTxServiceFactory::GetForContext(
    content::BrowserContext* context) {
  if (!IsAllowedForContext(context)) {
    return mojo::PendingRemote<mojom::FilTxService>();
  }

  return static_cast<FilTxService*>(
             GetInstance()->GetServiceForBrowserContext(context, true))
      ->MakeRemote();
}

// static
FilTxService* FilTxServiceFactory::GetServiceForContext(
    content::BrowserContext* context) {
  if (!IsAllowedForContext(context)) {
    return nullptr;
  }
  return static_cast<FilTxService*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

// static
void FilTxServiceFactory::BindForContext(
    content::BrowserContext* context,
    mojo::PendingReceiver<mojom::FilTxService> receiver) {
  auto* fil_tx_service = FilTxServiceFactory::GetServiceForContext(context);
  if (fil_tx_service) {
    fil_tx_service->Bind(std::move(receiver));
  }
}

FilTxServiceFactory::FilTxServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "FilTxService",
          BrowserContextDependencyManager::GetInstance()) {
  DependsOn(brave_wallet::JsonRpcServiceFactory::GetInstance());
  DependsOn(brave_wallet::KeyringServiceFactory::GetInstance());
  DependsOn(brave_wallet::AssetRatioServiceFactory::GetInstance());
}

FilTxServiceFactory::~FilTxServiceFactory() {}

KeyedService* FilTxServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  return brave_wallet::BuildFilTxService(
             JsonRpcServiceFactory::GetServiceForContext(context),
             KeyringServiceFactory::GetServiceForContext(context),
             AssetRatioServiceFactory::GetServiceForContext(context),
             user_prefs::UserPrefs::Get(context))
      .release();
}

content::BrowserContext* FilTxServiceFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return chrome::GetBrowserContextRedirectedInIncognito(context);
}

}  // namespace brave_wallet
