/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/cardano/cardano_tx_state_manager.h"

#include <optional>
#include <utility>

#include "base/values.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_transaction.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_tx_meta.h"
#include "brave/components/brave_wallet/browser/tx_meta.h"

namespace brave_wallet {

CardanoTxStateManager::CardanoTxStateManager(
    TxStorageDelegate& delegate,
    AccountResolverDelegate& account_resolver_delegate)
    : TxStateManager(delegate, account_resolver_delegate) {}

CardanoTxStateManager::~CardanoTxStateManager() = default;

std::unique_ptr<CardanoTxMeta> CardanoTxStateManager::GetCardanoTx(
    const std::string& id) {
  return std::unique_ptr<CardanoTxMeta>{
      static_cast<CardanoTxMeta*>(TxStateManager::GetTx(id).release())};
}

std::unique_ptr<CardanoTxMeta> CardanoTxStateManager::ValueToCardanoTxMeta(
    const base::DictValue& value) {
  return std::unique_ptr<CardanoTxMeta>{
      static_cast<CardanoTxMeta*>(ValueToTxMeta(value).release())};
}

mojom::CoinType CardanoTxStateManager::GetCoinType() const {
  return mojom::CoinType::ADA;
}

std::unique_ptr<TxMeta> CardanoTxStateManager::ValueToTxMeta(
    const base::DictValue& value) {
  std::unique_ptr<CardanoTxMeta> meta = std::make_unique<CardanoTxMeta>();

  if (!ValueToBaseTxMeta(value, meta.get())) {
    return nullptr;
  }
  const base::DictValue* tx = value.FindDict("tx");
  if (!tx) {
    return nullptr;
  }
  std::optional<CardanoTransaction> tx_from_value =
      CardanoTransaction::FromValue(*tx);
  if (!tx_from_value) {
    return nullptr;
  }
  meta->set_tx(std::make_unique<CardanoTransaction>(std::move(*tx_from_value)));
  return meta;
}

}  // namespace brave_wallet
