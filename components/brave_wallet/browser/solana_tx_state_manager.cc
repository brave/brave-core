/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/solana_tx_state_manager.h"

#include <optional>
#include <utility>

#include "base/values.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/solana_tx_meta.h"

namespace brave_wallet {

SolanaTxStateManager::SolanaTxStateManager(
    TxStorageDelegate& delegate,
    AccountResolverDelegate& account_resolver_delegate)
    : TxStateManager(delegate, account_resolver_delegate) {}

SolanaTxStateManager::~SolanaTxStateManager() = default;

std::unique_ptr<SolanaTxMeta> SolanaTxStateManager::ValueToSolanaTxMeta(
    const base::Value::Dict& value) {
  return std::unique_ptr<SolanaTxMeta>{
      static_cast<SolanaTxMeta*>(ValueToTxMeta(value).release())};
}

mojom::CoinType SolanaTxStateManager::GetCoinType() const {
  return mojom::CoinType::SOL;
}

std::unique_ptr<TxMeta> SolanaTxStateManager::ValueToTxMeta(
    const base::Value::Dict& value) {
  std::unique_ptr<SolanaTxMeta> meta = std::make_unique<SolanaTxMeta>();

  if (!ValueToBaseTxMeta(value, meta.get())) {
    return nullptr;
  }

  const base::Value::Dict* tx_value = value.FindDict("tx");
  if (!tx_value) {
    return nullptr;
  }
  auto tx = SolanaTransaction::FromValue(*tx_value);
  if (!tx) {
    return nullptr;
  }
  meta->set_tx(std::move(tx));

  const base::Value::Dict* signature_status_value =
      value.FindDict("signature_status");
  if (!signature_status_value) {
    return nullptr;
  }
  std::optional<SolanaSignatureStatus> signature_status =
      SolanaSignatureStatus::FromValue(*signature_status_value);
  if (!signature_status) {
    return nullptr;
  }
  meta->set_signature_status(*signature_status);

  return meta;
}

std::unique_ptr<SolanaTxMeta> SolanaTxStateManager::GetSolanaTx(
    const std::string& id) {
  return std::unique_ptr<SolanaTxMeta>{
      static_cast<SolanaTxMeta*>(TxStateManager::GetTx(id).release())};
}

}  // namespace brave_wallet
