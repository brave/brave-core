/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_wallet/tx_service_factory.h"

#include <memory>
#include <utility>

#include "base/no_destructor.h"
#include "base/task/sequenced_task_runner.h"
#include "brave/browser/brave_wallet/asset_ratio_service_factory.h"
#include "brave/browser/brave_wallet/bitcoin_wallet_service_factory.h"
#include "brave/browser/brave_wallet/brave_wallet_context_utils.h"
#include "brave/browser/brave_wallet/json_rpc_service_factory.h"
#include "brave/browser/brave_wallet/keyring_service_factory.h"
#include "brave/browser/brave_wallet/zcash_wallet_service_factory.h"
#include "brave/components/brave_wallet/browser/tx_service.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/user_prefs/user_prefs.h"

namespace brave_wallet {

// static
TxServiceFactory* TxServiceFactory::GetInstance() {
  static base::NoDestructor<TxServiceFactory> instance;
  return instance.get();
}

// static
mojo::PendingRemote<mojom::TxService> TxServiceFactory::GetForContext(
    content::BrowserContext* context) {
  if (!IsAllowedForContext(context)) {
    return mojo::PendingRemote<mojom::TxService>();
  }

  return static_cast<TxService*>(
             GetInstance()->GetServiceForBrowserContext(context, true))
      ->MakeRemote();
}

// static
mojo::PendingRemote<mojom::EthTxManagerProxy>
TxServiceFactory::GetEthTxManagerProxyForContext(
    content::BrowserContext* context) {
  if (!IsAllowedForContext(context)) {
    return mojo::PendingRemote<mojom::EthTxManagerProxy>();
  }

  return static_cast<TxService*>(
             GetInstance()->GetServiceForBrowserContext(context, true))
      ->MakeEthTxManagerProxyRemote();
}

// static
mojo::PendingRemote<mojom::SolanaTxManagerProxy>
TxServiceFactory::GetSolanaTxManagerProxyForContext(
    content::BrowserContext* context) {
  if (!IsAllowedForContext(context)) {
    return mojo::PendingRemote<mojom::SolanaTxManagerProxy>();
  }

  return static_cast<TxService*>(
             GetInstance()->GetServiceForBrowserContext(context, true))
      ->MakeSolanaTxManagerProxyRemote();
}

// static
mojo::PendingRemote<mojom::FilTxManagerProxy>
TxServiceFactory::GetFilTxManagerProxyForContext(
    content::BrowserContext* context) {
  if (!IsAllowedForContext(context)) {
    return mojo::PendingRemote<mojom::FilTxManagerProxy>();
  }

  return static_cast<TxService*>(
             GetInstance()->GetServiceForBrowserContext(context, true))
      ->MakeFilTxManagerProxyRemote();
}

// static
TxService* TxServiceFactory::GetServiceForContext(
    content::BrowserContext* context) {
  if (!IsAllowedForContext(context)) {
    return nullptr;
  }
  return static_cast<TxService*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

// static
void TxServiceFactory::BindForContext(
    content::BrowserContext* context,
    mojo::PendingReceiver<mojom::TxService> receiver) {
  auto* tx_service = TxServiceFactory::GetServiceForContext(context);
  if (tx_service) {
    tx_service->Bind(std::move(receiver));
  }
}

// static
void TxServiceFactory::BindEthTxManagerProxyForContext(
    content::BrowserContext* context,
    mojo::PendingReceiver<mojom::EthTxManagerProxy> receiver) {
  auto* tx_service = TxServiceFactory::GetServiceForContext(context);
  if (tx_service) {
    tx_service->BindEthTxManagerProxy(std::move(receiver));
  }
}

// static
void TxServiceFactory::BindSolanaTxManagerProxyForContext(
    content::BrowserContext* context,
    mojo::PendingReceiver<mojom::SolanaTxManagerProxy> receiver) {
  auto* tx_service = TxServiceFactory::GetServiceForContext(context);
  if (tx_service) {
    tx_service->BindSolanaTxManagerProxy(std::move(receiver));
  }
}

// static
void TxServiceFactory::BindFilTxManagerProxyForContext(
    content::BrowserContext* context,
    mojo::PendingReceiver<mojom::FilTxManagerProxy> receiver) {
  auto* tx_service = TxServiceFactory::GetServiceForContext(context);
  if (tx_service) {
    tx_service->BindFilTxManagerProxy(std::move(receiver));
  }
}

TxServiceFactory::TxServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "TxService",
          BrowserContextDependencyManager::GetInstance()) {
  DependsOn(JsonRpcServiceFactory::GetInstance());
  DependsOn(BitcoinWalletServiceFactory::GetInstance());
  DependsOn(KeyringServiceFactory::GetInstance());
  DependsOn(AssetRatioServiceFactory::GetInstance());
  DependsOn(ZCashWalletServiceFactory::GetInstance());
}

TxServiceFactory::~TxServiceFactory() = default;

KeyedService* TxServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  return new TxService(
      JsonRpcServiceFactory::GetServiceForContext(context),
      BitcoinWalletServiceFactory::GetServiceForContext(context),
      ZCashWalletServiceFactory::GetServiceForContext(context),
      KeyringServiceFactory::GetServiceForContext(context),
      user_prefs::UserPrefs::Get(context), context->GetPath(),
      base::SequencedTaskRunner::GetCurrentDefault());
}

content::BrowserContext* TxServiceFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return chrome::GetBrowserContextRedirectedInIncognito(context);
}

}  // namespace brave_wallet
