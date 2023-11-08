/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_wallet/zcash_rpc_service_factory.h"

#include <utility>

#include "base/no_destructor.h"
#include "brave/browser/brave_wallet/brave_wallet_context_utils.h"
#include "brave/browser/brave_wallet/keyring_service_factory.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_rpc.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/storage_partition.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace brave_wallet {

// static
ZCashRpcServiceFactory* ZCashRpcServiceFactory::GetInstance() {
  static base::NoDestructor<ZCashRpcServiceFactory> instance;
  return instance.get();
}

// static
ZCashRpc* ZCashRpcServiceFactory::GetServiceForContext(
    content::BrowserContext* context) {
  if (!IsAllowedForContext(context)) {
    return nullptr;
  }
  if (!IsZCashEnabled()) {
    return nullptr;
  }
  return static_cast<ZCashRpc*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

ZCashRpcServiceFactory::ZCashRpcServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "ZCashRpc",
          BrowserContextDependencyManager::GetInstance()) {
  DependsOn(KeyringServiceFactory::GetInstance());
}

ZCashRpcServiceFactory::~ZCashRpcServiceFactory() = default;

KeyedService* ZCashRpcServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  auto* default_storage_partition = context->GetDefaultStoragePartition();
  auto shared_url_loader_factory =
      default_storage_partition->GetURLLoaderFactoryForBrowserProcess();
  return new ZCashRpc(user_prefs::UserPrefs::Get(context),
                      shared_url_loader_factory);
}

content::BrowserContext* ZCashRpcServiceFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return chrome::GetBrowserContextRedirectedInIncognito(context);
}

}  // namespace brave_wallet
