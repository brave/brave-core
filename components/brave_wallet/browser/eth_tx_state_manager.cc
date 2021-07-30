/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/eth_tx_state_manager.h"

#include <utility>

#include "base/guid.h"
#include "base/logging.h"
#include "base/util/values/values_util.h"
#include "base/values.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/eip1559_transaction.h"
#include "brave/components/brave_wallet/browser/eip2930_transaction.h"
#include "brave/components/brave_wallet/browser/eth_address.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"

namespace brave_wallet {

EthTxStateManager::EthTxStateManager(PrefService* prefs) : prefs_(prefs) {}
EthTxStateManager::~EthTxStateManager() = default;

EthTxStateManager::TxMeta::TxMeta() : tx(std::make_unique<EthTransaction>()) {}
EthTxStateManager::TxMeta::TxMeta(std::unique_ptr<EthTransaction> tx_in)
    : tx(std::move(tx_in)) {}
EthTxStateManager::TxMeta::~TxMeta() = default;
bool EthTxStateManager::TxMeta::operator==(const TxMeta& meta) const {
  return id == meta.id && status == meta.status && from == meta.from &&
         last_gas_price == meta.last_gas_price &&
         created_time == meta.created_time &&
         submitted_time == meta.submitted_time &&
         confirmed_time == meta.confirmed_time &&
         tx_receipt == meta.tx_receipt && tx_hash == meta.tx_hash &&
         *tx == *meta.tx;
}

std::string EthTxStateManager::GenerateMetaID() {
  return base::GenerateGUID();
}

base::Value EthTxStateManager::TxMetaToValue(const TxMeta& meta) {
  base::Value dict(base::Value::Type::DICTIONARY);
  dict.SetStringKey("id", meta.id);
  dict.SetIntKey("status", static_cast<int>(meta.status));
  dict.SetStringKey("from", meta.from.ToHex());
  dict.SetStringKey("last_gas_price", Uint256ValueToHex(meta.last_gas_price));
  dict.SetKey("created_time", util::TimeToValue(meta.created_time));
  dict.SetKey("submitted_time", util::TimeToValue(meta.submitted_time));
  dict.SetKey("confirmed_time", util::TimeToValue(meta.confirmed_time));
  dict.SetKey("tx_receipt", TransactionReceiptToValue(meta.tx_receipt));
  dict.SetStringKey("tx_hash", meta.tx_hash);
  dict.SetKey("tx", meta.tx->ToValue());

  return dict;
}

std::unique_ptr<EthTxStateManager::TxMeta> EthTxStateManager::ValueToTxMeta(
    const base::Value& value) {
  std::unique_ptr<EthTxStateManager::TxMeta> meta =
      std::make_unique<EthTxStateManager::TxMeta>();
  const std::string* id = value.FindStringKey("id");
  if (!id)
    return nullptr;
  meta->id = *id;

  absl::optional<int> status = value.FindIntKey("status");
  if (!status)
    return nullptr;
  meta->status = static_cast<EthTxStateManager::TransactionStatus>(*status);

  const std::string* from = value.FindStringKey("from");
  if (!from)
    return nullptr;
  meta->from = EthAddress::FromHex(*from);

  const std::string* last_gas_price = value.FindStringKey("last_gas_price");
  if (!last_gas_price)
    return nullptr;
  uint256_t last_gas_price_uint;
  if (!HexValueToUint256(*last_gas_price, &last_gas_price_uint))
    return nullptr;
  meta->last_gas_price = last_gas_price_uint;

  const base::Value* created_time = value.FindKey("created_time");
  if (!created_time)
    return nullptr;
  absl::optional<base::Time> created_time_from_value =
      util::ValueToTime(created_time);
  if (!created_time_from_value)
    return nullptr;
  meta->created_time = *created_time_from_value;

  const base::Value* submitted_time = value.FindKey("submitted_time");
  if (!submitted_time)
    return nullptr;
  absl::optional<base::Time> submitted_time_from_value =
      util::ValueToTime(submitted_time);
  if (!submitted_time_from_value)
    return nullptr;
  meta->submitted_time = *submitted_time_from_value;

  const base::Value* confirmed_time = value.FindKey("confirmed_time");
  if (!confirmed_time)
    return nullptr;
  absl::optional<base::Time> confirmed_time_from_value =
      util::ValueToTime(confirmed_time);
  if (!confirmed_time_from_value)
    return nullptr;
  meta->confirmed_time = *confirmed_time_from_value;

  const base::Value* tx_receipt = value.FindKey("tx_receipt");
  if (!tx_receipt)
    return nullptr;
  absl::optional<TransactionReceipt> tx_receipt_from_value =
      ValueToTransactionReceipt(*tx_receipt);
  meta->tx_receipt = *tx_receipt_from_value;

  const std::string* tx_hash = value.FindStringKey("tx_hash");
  if (!tx_hash)
    return nullptr;
  meta->tx_hash = *tx_hash;

  const base::Value* tx = value.FindKey("tx");
  if (!tx)
    return nullptr;
  absl::optional<int> type = tx->FindIntKey("type");
  if (!type)
    return nullptr;

  switch (static_cast<uint8_t>(*type)) {
    case 0: {
      absl::optional<EthTransaction> tx_from_value =
          EthTransaction::FromValue(*tx);
      if (!tx_from_value)
        return nullptr;
      meta->tx = std::make_unique<EthTransaction>(*tx_from_value);
      break;
    }
    case 1: {
      absl::optional<Eip2930Transaction> tx_from_value =
          Eip2930Transaction::FromValue(*tx);
      if (!tx_from_value)
        return nullptr;
      meta->tx = std::make_unique<Eip2930Transaction>(*tx_from_value);
      break;
    }
    case 2: {
      absl::optional<Eip1559Transaction> tx_from_value =
          Eip1559Transaction::FromValue(*tx);
      if (!tx_from_value)
        return nullptr;
      meta->tx = std::make_unique<Eip1559Transaction>(*tx_from_value);
      break;
    }
    default:
      LOG(ERROR) << "tx type is not supported";
      break;
  }

  return meta;
}

void EthTxStateManager::AddOrUpdateTx(const TxMeta& meta) {
  DictionaryPrefUpdate update(prefs_, kBraveWalletTransactions);
  base::DictionaryValue* dict = update.Get();
  dict->SetKey(meta.id, TxMetaToValue(meta));
}

std::unique_ptr<EthTxStateManager::TxMeta> EthTxStateManager::GetTx(
    const std::string& id) {
  const base::DictionaryValue* dict =
      prefs_->GetDictionary(kBraveWalletTransactions);
  if (!dict)
    return nullptr;
  const base::Value* value = dict->FindKey(id);
  if (!value)
    return nullptr;

  return ValueToTxMeta(*value);
}

void EthTxStateManager::DeleteTx(const std::string& id) {
  DictionaryPrefUpdate update(prefs_, kBraveWalletTransactions);
  base::DictionaryValue* dict = update.Get();
  dict->RemoveKey(id);
}

void EthTxStateManager::WipeTxs() {
  prefs_->ClearPref(kBraveWalletTransactions);
}

std::vector<std::unique_ptr<EthTxStateManager::TxMeta>>
EthTxStateManager::GetTransactionsByStatus(TransactionStatus status,
                                           absl::optional<EthAddress> from) {
  std::vector<std::unique_ptr<EthTxStateManager::TxMeta>> result;
  const base::DictionaryValue* value =
      prefs_->GetDictionary(kBraveWalletTransactions);
  for (base::DictionaryValue::Iterator iter(*value); !iter.IsAtEnd();
       iter.Advance()) {
    std::unique_ptr<EthTxStateManager::TxMeta> meta =
        ValueToTxMeta(iter.value());
    if (!meta) {
      continue;
    }
    if (meta->status == status) {
      if (from.has_value() && meta->from != *from)
        continue;
      result.push_back(std::move(meta));
    }
  }
  return result;
}

}  // namespace brave_wallet
