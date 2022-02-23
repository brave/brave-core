/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/eth_tx_state_manager.h"

#include <utility>

#include "base/json/values_util.h"
#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/values.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/eip1559_transaction.h"
#include "brave/components/brave_wallet/browser/eip2930_transaction.h"
#include "brave/components/brave_wallet/browser/eth_tx_meta.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/browser/tx_meta.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/eth_address.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"

namespace brave_wallet {
namespace {
constexpr size_t kMaxConfirmedTxNum = 10;
constexpr size_t kMaxRejectedTxNum = 10;
}  // namespace

EthTxStateManager::EthTxStateManager(PrefService* prefs,
                                     JsonRpcService* json_rpc_service)
    : prefs_(prefs), json_rpc_service_(json_rpc_service), weak_factory_(this) {
  DCHECK(json_rpc_service_);
}

EthTxStateManager::~EthTxStateManager() = default;

// static
std::unique_ptr<EthTxMeta> EthTxStateManager::ValueToTxMeta(
    const base::Value& value) {
  std::unique_ptr<EthTxMeta> meta = std::make_unique<EthTxMeta>();
  const std::string* id = value.FindStringKey("id");
  if (!id)
    return nullptr;
  meta->set_id(*id);

  absl::optional<int> status = value.FindIntKey("status");
  if (!status)
    return nullptr;
  meta->set_status(static_cast<mojom::TransactionStatus>(*status));

  const std::string* from = value.FindStringKey("from");
  if (!from)
    return nullptr;
  meta->set_from(*from);

  const base::Value* created_time = value.FindKey("created_time");
  if (!created_time)
    return nullptr;
  absl::optional<base::Time> created_time_from_value =
      base::ValueToTime(created_time);
  if (!created_time_from_value)
    return nullptr;
  meta->set_created_time(*created_time_from_value);

  const base::Value* submitted_time = value.FindKey("submitted_time");
  if (!submitted_time)
    return nullptr;
  absl::optional<base::Time> submitted_time_from_value =
      base::ValueToTime(submitted_time);
  if (!submitted_time_from_value)
    return nullptr;
  meta->set_submitted_time(*submitted_time_from_value);

  const base::Value* confirmed_time = value.FindKey("confirmed_time");
  if (!confirmed_time)
    return nullptr;
  absl::optional<base::Time> confirmed_time_from_value =
      base::ValueToTime(confirmed_time);
  if (!confirmed_time_from_value)
    return nullptr;
  meta->set_confirmed_time(*confirmed_time_from_value);

  const base::Value* tx_receipt = value.FindKey("tx_receipt");
  if (!tx_receipt)
    return nullptr;
  absl::optional<TransactionReceipt> tx_receipt_from_value =
      ValueToTransactionReceipt(*tx_receipt);
  meta->set_tx_receipt(*tx_receipt_from_value);

  const std::string* tx_hash = value.FindStringKey("tx_hash");
  if (!tx_hash)
    return nullptr;
  meta->set_tx_hash(*tx_hash);

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
      meta->set_tx(std::make_unique<EthTransaction>(*tx_from_value));
      break;
    }
    case 1: {
      absl::optional<Eip2930Transaction> tx_from_value =
          Eip2930Transaction::FromValue(*tx);
      if (!tx_from_value)
        return nullptr;
      meta->set_tx(std::make_unique<Eip2930Transaction>(*tx_from_value));
      break;
    }
    case 2: {
      absl::optional<Eip1559Transaction> tx_from_value =
          Eip1559Transaction::FromValue(*tx);
      if (!tx_from_value)
        return nullptr;
      meta->set_tx(std::make_unique<Eip1559Transaction>(*tx_from_value));
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
  base::Value* dict = update.Get();
  const std::string path =
      GetNetworkId(prefs_, json_rpc_service_->GetChainId()) + "." + meta.id();
  bool is_add = dict->FindPath(path) == nullptr;
  dict->SetPath(path, meta.ToValue());
  if (!is_add) {
    for (auto& observer : observers_)
      observer.OnTransactionStatusChanged(meta.ToTransactionInfo());
    return;
  }

  for (auto& observer : observers_)
    observer.OnNewUnapprovedTx(meta.ToTransactionInfo());

  // We only keep most recent 10 confirmed and rejected tx metas per network
  RetireTxByStatus(mojom::TransactionStatus::Confirmed, kMaxConfirmedTxNum);
  RetireTxByStatus(mojom::TransactionStatus::Rejected, kMaxRejectedTxNum);
}

std::unique_ptr<TxMeta> EthTxStateManager::GetTx(const std::string& id) {
  const base::Value* dict = prefs_->GetDictionary(kBraveWalletTransactions);
  if (!dict)
    return nullptr;
  const base::Value* value = dict->FindPath(
      GetNetworkId(prefs_, json_rpc_service_->GetChainId()) + "." + id);
  if (!value)
    return nullptr;

  return ValueToTxMeta(*value);
}

void EthTxStateManager::DeleteTx(const std::string& id) {
  DictionaryPrefUpdate update(prefs_, kBraveWalletTransactions);
  base::Value* dict = update.Get();
  dict->RemovePath(GetNetworkId(prefs_, json_rpc_service_->GetChainId()) + "." +
                   id);
}

void EthTxStateManager::WipeTxs() {
  prefs_->ClearPref(kBraveWalletTransactions);
}

std::vector<std::unique_ptr<TxMeta>> EthTxStateManager::GetTransactionsByStatus(
    absl::optional<mojom::TransactionStatus> status,
    absl::optional<EthAddress> from) {
  std::vector<std::unique_ptr<TxMeta>> result;
  std::string from_string = from.has_value() ? from->ToChecksumAddress() : "";
  return GetTransactionsByStatus(status, from_string);
}

std::vector<std::unique_ptr<TxMeta>> EthTxStateManager::GetTransactionsByStatus(
    absl::optional<mojom::TransactionStatus> status,
    const std::string& from) {
  std::vector<std::unique_ptr<TxMeta>> result;
  const base::Value* dict = prefs_->GetDictionary(kBraveWalletTransactions);
  const base::Value* network_dict =
      dict->FindKey(GetNetworkId(prefs_, json_rpc_service_->GetChainId()));
  if (!network_dict)
    return result;

  for (const auto it : network_dict->DictItems()) {
    std::unique_ptr<TxMeta> meta = ValueToTxMeta(it.second);
    if (!meta) {
      continue;
    }
    if (!status.has_value() || meta->status() == *status) {
      if (!from.empty() && meta->from() != from)
        continue;
      result.push_back(std::move(meta));
    }
  }
  return result;
}

void EthTxStateManager::RetireTxByStatus(mojom::TransactionStatus status,
                                         size_t max_num) {
  if (status != mojom::TransactionStatus::Confirmed &&
      status != mojom::TransactionStatus::Rejected)
    return;
  auto tx_metas = GetTransactionsByStatus(status, absl::nullopt);
  if (tx_metas.size() > max_num) {
    TxMeta* oldest_meta = nullptr;
    for (const auto& tx_meta : tx_metas) {
      if (!oldest_meta) {
        oldest_meta = tx_meta.get();
      } else {
        if (tx_meta->status() == mojom::TransactionStatus::Confirmed &&
            tx_meta->confirmed_time() < oldest_meta->confirmed_time()) {
          oldest_meta = tx_meta.get();
        } else if (tx_meta->status() == mojom::TransactionStatus::Rejected &&
                   tx_meta->created_time() < oldest_meta->created_time()) {
          oldest_meta = tx_meta.get();
        }
      }
    }
    DeleteTx(oldest_meta->id());
  }
}

void EthTxStateManager::AddObserver(EthTxStateManager::Observer* observer) {
  observers_.AddObserver(observer);
}

void EthTxStateManager::RemoveObserver(EthTxStateManager::Observer* observer) {
  observers_.RemoveObserver(observer);
}

std::unique_ptr<EthTxMeta> EthTxStateManager::GetEthTx(const std::string& id) {
  return std::unique_ptr<EthTxMeta>{
      static_cast<EthTxMeta*>(GetTx(id).release())};
}

}  // namespace brave_wallet
