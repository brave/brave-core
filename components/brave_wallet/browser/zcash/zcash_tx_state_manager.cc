/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/zcash/zcash_tx_state_manager.h"

#include <optional>
#include <utility>

#include "base/values.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/tx_meta.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_transaction.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_tx_meta.h"

namespace brave_wallet {

ZCashTxStateManager::ZCashTxStateManager(
    TxStorageDelegate& delegate,
    AccountResolverDelegate& account_resolver_delegate)
    : TxStateManager(delegate, account_resolver_delegate) {}

ZCashTxStateManager::~ZCashTxStateManager() = default;

std::unique_ptr<ZCashTxMeta> ZCashTxStateManager::GetZCashTx(
    const std::string& id) {
  return std::unique_ptr<ZCashTxMeta>{
      static_cast<ZCashTxMeta*>(TxStateManager::GetTx(id).release())};
}

std::unique_ptr<ZCashTxMeta> ZCashTxStateManager::ValueToZCashTxMeta(
    const base::Value::Dict& value) {
  return std::unique_ptr<ZCashTxMeta>{
      static_cast<ZCashTxMeta*>(ValueToTxMeta(value).release())};
}

mojom::CoinType ZCashTxStateManager::GetCoinType() const {
  return mojom::CoinType::ZEC;
}

std::unique_ptr<TxMeta> ZCashTxStateManager::ValueToTxMeta(
    const base::Value::Dict& value) {
  std::unique_ptr<ZCashTxMeta> meta = std::make_unique<ZCashTxMeta>();

  if (!ValueToBaseTxMeta(value, meta.get())) {
    return nullptr;
  }
  const base::Value::Dict* tx = value.FindDict("tx");
  if (!tx) {
    return nullptr;
  }
  std::optional<ZCashTransaction> tx_from_value =
      ZCashTransaction::FromValue(*tx);
  if (!tx_from_value) {
    return nullptr;
  }
  meta->set_tx(std::make_unique<ZCashTransaction>(std::move(*tx_from_value)));
  return meta;
}

}  // namespace brave_wallet
