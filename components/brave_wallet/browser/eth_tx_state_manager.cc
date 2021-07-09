/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/eth_tx_state_manager.h"

#include "base/guid.h"
#include "base/logging.h"
#include "base/util/values/values_util.h"
#include "base/values.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/eip2930_transaction.h"
#include "brave/components/brave_wallet/browser/eth_address.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"

namespace brave_wallet {

EthTxStateManager::EthTxStateManager(PrefService* prefs) : prefs_(prefs) {
  const base::DictionaryValue* value =
      prefs_->GetDictionary(kBraveWalletTransactions);
  for (base::DictionaryValue::Iterator iter(*value); !iter.IsAtEnd();
       iter.Advance()) {
    const std::string id = iter.key();
    absl::optional<TxMeta> meta = ValueToTxMeta(iter.value());
    if (!meta) {
      LOG(ERROR) << "invalid TxMeta, id=" << id;
      continue;
    }
    tx_meta_map_[id] = *meta;
  }
}
EthTxStateManager::~EthTxStateManager() = default;

EthTxStateManager::TxMeta::TxMeta() = default;
EthTxStateManager::TxMeta::TxMeta(const EthTransaction& tx) : tx(tx) {}
EthTxStateManager::TxMeta::TxMeta(const TxMeta& meta) = default;
EthTxStateManager::TxMeta::~TxMeta() = default;

std::string EthTxStateManager::GenerateMetaID() {
  return base::GenerateGUID();
}

base::Value EthTxStateManager::TxMetaToValue(const TxMeta& meta) {
  base::Value dict(base::Value::Type::DICTIONARY);
  dict.SetIntKey("status", static_cast<int>(meta.status));
  dict.SetStringKey("from", meta.from.ToHex());
  dict.SetStringKey("last_gas_price", Uint256ValueToHex(meta.last_gas_price));
  dict.SetKey("created_time", util::TimeToValue(meta.created_time));
  dict.SetKey("submitted_time", util::TimeToValue(meta.submitted_time));
  dict.SetKey("confirmed_time", util::TimeToValue(meta.confirmed_time));
  dict.SetKey("tx_receipt", TransactionReceiptToValue(meta.tx_receipt));
  dict.SetStringKey("tx_hash", meta.tx_hash);
  dict.SetKey("tx", meta.tx.ToValue());

  return dict;
}

absl::optional<EthTxStateManager::TxMeta> EthTxStateManager::ValueToTxMeta(
    const base::Value& value) {
  EthTxStateManager::TxMeta meta;
  absl::optional<int> status = value.FindIntKey("status");
  if (!status)
    return absl::nullopt;
  meta.status = static_cast<EthTxStateManager::TransactionStatus>(*status);

  const std::string* from = value.FindStringKey("from");
  if (!from)
    return absl::nullopt;
  meta.from = EthAddress::FromHex(*from);

  const std::string* last_gas_price = value.FindStringKey("last_gas_price");
  if (!last_gas_price)
    return absl::nullopt;
  uint256_t last_gas_price_uint;
  if (!HexValueToUint256(*last_gas_price, &last_gas_price_uint))
    return absl::nullopt;
  meta.last_gas_price = last_gas_price_uint;

  const base::Value* created_time = value.FindKey("created_time");
  if (!created_time)
    return absl::nullopt;
  absl::optional<base::Time> created_time_from_value =
      util::ValueToTime(created_time);
  if (!created_time_from_value)
    return absl::nullopt;
  meta.created_time = *created_time_from_value;

  const base::Value* submitted_time = value.FindKey("submitted_time");
  if (!submitted_time)
    return absl::nullopt;
  absl::optional<base::Time> submitted_time_from_value =
      util::ValueToTime(submitted_time);
  if (!submitted_time_from_value)
    return absl::nullopt;
  meta.submitted_time = *submitted_time_from_value;

  const base::Value* confirmed_time = value.FindKey("confirmed_time");
  if (!confirmed_time)
    return absl::nullopt;
  absl::optional<base::Time> confirmed_time_from_value =
      util::ValueToTime(confirmed_time);
  if (!confirmed_time_from_value)
    return absl::nullopt;
  meta.confirmed_time = *confirmed_time_from_value;

  const base::Value* tx_receipt = value.FindKey("tx_receipt");
  if (!tx_receipt)
    return absl::nullopt;
  absl::optional<TransactionReceipt> tx_receipt_from_value =
      ValueToTransactionReceipt(*tx_receipt);
  meta.tx_receipt = *tx_receipt_from_value;

  const std::string* tx_hash = value.FindStringKey("tx_hash");
  if (!tx_hash)
    return absl::nullopt;
  meta.tx_hash = *tx_hash;

  const base::Value* tx = value.FindKey("tx");
  if (!tx)
    return absl::nullopt;
  absl::optional<int> type = tx->FindIntKey("type");
  if (!type)
    return absl::nullopt;

  absl::optional<EthTransaction> tx_from_value;
  switch (static_cast<uint8_t>(*type)) {
    case 0:
      tx_from_value = EthTransaction::FromValue(*tx);
      break;
    case 1:
      tx_from_value = Eip2930Transaction::FromValue(*tx);
      break;
    default:
      LOG(ERROR) << "tx type is not supported";
      break;
  }
  if (!tx_from_value)
    return absl::nullopt;
  meta.tx = *tx_from_value;

  return meta;
}

void EthTxStateManager::AddOrUpdateTx(const TxMeta& meta) {
  DictionaryPrefUpdate update(prefs_, kBraveWalletTransactions);
  base::DictionaryValue* dict = update.Get();
  dict->SetKey(meta.id, TxMetaToValue(meta));
  tx_meta_map_[meta.id] = meta;
}

bool EthTxStateManager::GetTx(const std::string& id, TxMeta* meta) {
  if (!meta)
    return false;
  auto iter = tx_meta_map_.find(id);
  if (iter == tx_meta_map_.end())
    return false;

  *meta = iter->second;

  return true;
}

void EthTxStateManager::DeleteTx(const std::string& id) {
  DictionaryPrefUpdate update(prefs_, kBraveWalletTransactions);
  base::DictionaryValue* dict = update.Get();
  dict->RemoveKey(id);
  tx_meta_map_.erase(id);
}

void EthTxStateManager::WipeTxs() {
  prefs_->ClearPref(kBraveWalletTransactions);
  tx_meta_map_.clear();
}

std::vector<EthTxStateManager::TxMeta>
EthTxStateManager::GetTransactionsByStatus(TransactionStatus status,
                                           absl::optional<EthAddress> from) {
  std::vector<EthTxStateManager::TxMeta> result;
  for (auto& tx_meta : tx_meta_map_) {
    if (tx_meta.second.status == status) {
      if (from.has_value() && tx_meta.second.from != *from)
        continue;
      result.push_back(tx_meta.second);
    }
  }
  return result;
}

}  // namespace brave_wallet
