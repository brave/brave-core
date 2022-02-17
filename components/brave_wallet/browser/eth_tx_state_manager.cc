/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/eth_tx_state_manager.h"

#include <utility>

#include "base/guid.h"
#include "base/json/values_util.h"
#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/values.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/eip1559_transaction.h"
#include "brave/components/brave_wallet/browser/eip2930_transaction.h"
#include "brave/components/brave_wallet/browser/eth_data_parser.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
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
  json_rpc_service_->AddObserver(observer_receiver_.BindNewPipeAndPassRemote());
  chain_id_ = json_rpc_service_->GetChainId();
  network_url_ = json_rpc_service_->GetNetworkUrl();
}
EthTxStateManager::~EthTxStateManager() = default;

EthTxStateManager::TxMeta::TxMeta() : tx(std::make_unique<EthTransaction>()) {}
EthTxStateManager::TxMeta::TxMeta(std::unique_ptr<EthTransaction> tx_in)
    : tx(std::move(tx_in)) {}
EthTxStateManager::TxMeta::~TxMeta() = default;
bool EthTxStateManager::TxMeta::operator==(const TxMeta& meta) const {
  return id == meta.id && status == meta.status && from == meta.from &&
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
  dict.SetStringKey("from", meta.from.ToChecksumAddress());
  dict.SetKey("created_time", base::TimeToValue(meta.created_time));
  dict.SetKey("submitted_time", base::TimeToValue(meta.submitted_time));
  dict.SetKey("confirmed_time", base::TimeToValue(meta.confirmed_time));
  dict.SetKey("tx_receipt", TransactionReceiptToValue(meta.tx_receipt));
  dict.SetStringKey("tx_hash", meta.tx_hash);
  dict.SetKey("tx", meta.tx->ToValue());

  return dict;
}

mojom::TransactionInfoPtr EthTxStateManager::TxMetaToTransactionInfo(
    const TxMeta& meta) {
  std::string chain_id;
  std::string max_priority_fee_per_gas;
  std::string max_fee_per_gas;

  mojom::GasEstimation1559Ptr gas_estimation_1559_ptr = nullptr;
  if (meta.tx->type() == 1) {
    // When type is 1 it's always Eip2930Transaction
    auto* tx2930 = reinterpret_cast<Eip2930Transaction*>(meta.tx.get());
    chain_id = Uint256ValueToHex(tx2930->chain_id());
  } else if (meta.tx->type() == 2) {
    // When type is 2 it's always Eip1559Transaction
    auto* tx1559 = reinterpret_cast<Eip1559Transaction*>(meta.tx.get());
    chain_id = Uint256ValueToHex(tx1559->chain_id());
    max_priority_fee_per_gas =
        Uint256ValueToHex(tx1559->max_priority_fee_per_gas());
    max_fee_per_gas = Uint256ValueToHex(tx1559->max_fee_per_gas());
    gas_estimation_1559_ptr =
        Eip1559Transaction::GasEstimation::ToMojomGasEstimation1559(
            tx1559->gas_estimation());
  }

  mojom::TransactionType tx_type;
  std::vector<std::string> tx_params;
  std::vector<std::string> tx_args;
  std::string data = "0x0";
  if (meta.tx->data().size() > 0) {
    data = "0x" + base::HexEncode(meta.tx->data());
  }
  if (!GetTransactionInfoFromData(data, &tx_type, &tx_params, &tx_args)) {
    LOG(ERROR) << "Error parsing transaction data: " << data;
  }

  return mojom::TransactionInfo::New(
      meta.id, meta.from.ToChecksumAddress(), meta.tx_hash,
      mojom::TxData1559::New(
          mojom::TxData::New(
              meta.tx->nonce() ? Uint256ValueToHex(meta.tx->nonce().value())
                               : "",
              Uint256ValueToHex(meta.tx->gas_price()),
              Uint256ValueToHex(meta.tx->gas_limit()),
              meta.tx->to().ToChecksumAddress(),
              Uint256ValueToHex(meta.tx->value()), meta.tx->data()),
          chain_id, max_priority_fee_per_gas, max_fee_per_gas,
          std::move(gas_estimation_1559_ptr)),
      meta.status, tx_type, tx_params, tx_args,
      base::Milliseconds(meta.created_time.ToJavaTime()),
      base::Milliseconds(meta.submitted_time.ToJavaTime()),
      base::Milliseconds(meta.confirmed_time.ToJavaTime()));
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
  meta->status = static_cast<mojom::TransactionStatus>(*status);

  const std::string* from = value.FindStringKey("from");
  if (!from)
    return nullptr;
  meta->from = EthAddress::FromHex(*from);

  const base::Value* created_time = value.FindKey("created_time");
  if (!created_time)
    return nullptr;
  absl::optional<base::Time> created_time_from_value =
      base::ValueToTime(created_time);
  if (!created_time_from_value)
    return nullptr;
  meta->created_time = *created_time_from_value;

  const base::Value* submitted_time = value.FindKey("submitted_time");
  if (!submitted_time)
    return nullptr;
  absl::optional<base::Time> submitted_time_from_value =
      base::ValueToTime(submitted_time);
  if (!submitted_time_from_value)
    return nullptr;
  meta->submitted_time = *submitted_time_from_value;

  const base::Value* confirmed_time = value.FindKey("confirmed_time");
  if (!confirmed_time)
    return nullptr;
  absl::optional<base::Time> confirmed_time_from_value =
      base::ValueToTime(confirmed_time);
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
  base::Value* dict = update.Get();
  const std::string path = GetNetworkId(prefs_, chain_id_) + "." + meta.id;
  bool is_add = dict->FindPath(path) == nullptr;
  dict->SetPath(path, TxMetaToValue(meta));
  if (!is_add) {
    for (auto& observer : observers_)
      observer.OnTransactionStatusChanged(TxMetaToTransactionInfo(meta));
    return;
  }

  for (auto& observer : observers_)
    observer.OnNewUnapprovedTx(TxMetaToTransactionInfo(meta));

  // We only keep most recent 10 confirmed and rejected tx metas per network
  RetireTxByStatus(mojom::TransactionStatus::Confirmed, kMaxConfirmedTxNum);
  RetireTxByStatus(mojom::TransactionStatus::Rejected, kMaxRejectedTxNum);
}

std::unique_ptr<EthTxStateManager::TxMeta> EthTxStateManager::GetTx(
    const std::string& id) {
  const base::Value* dict = prefs_->GetDictionary(kBraveWalletTransactions);
  if (!dict)
    return nullptr;
  const base::Value* value =
      dict->FindPath(GetNetworkId(prefs_, chain_id_) + "." + id);
  if (!value)
    return nullptr;

  return ValueToTxMeta(*value);
}

void EthTxStateManager::DeleteTx(const std::string& id) {
  DictionaryPrefUpdate update(prefs_, kBraveWalletTransactions);
  base::Value* dict = update.Get();
  dict->RemovePath(GetNetworkId(prefs_, chain_id_) + "." + id);
}

void EthTxStateManager::WipeTxs() {
  prefs_->ClearPref(kBraveWalletTransactions);
}

std::vector<std::unique_ptr<EthTxStateManager::TxMeta>>
EthTxStateManager::GetTransactionsByStatus(
    absl::optional<mojom::TransactionStatus> status,
    absl::optional<EthAddress> from) {
  std::vector<std::unique_ptr<EthTxStateManager::TxMeta>> result;
  const base::Value* dict = prefs_->GetDictionary(kBraveWalletTransactions);
  const base::Value* network_dict =
      dict->FindKey(GetNetworkId(prefs_, chain_id_));
  if (!network_dict)
    return result;

  for (const auto it : network_dict->DictItems()) {
    std::unique_ptr<EthTxStateManager::TxMeta> meta = ValueToTxMeta(it.second);
    if (!meta) {
      continue;
    }
    if (!status.has_value() || meta->status == *status) {
      if (from.has_value() && meta->from != *from)
        continue;
      result.push_back(std::move(meta));
    }
  }
  return result;
}

void EthTxStateManager::ChainChangedEvent(const std::string& chain_id) {
  chain_id_ = chain_id;
  network_url_ = json_rpc_service_->GetNetworkUrl();
}

void EthTxStateManager::OnAddEthereumChainRequestCompleted(
    const std::string& chain_id,
    const std::string& error) {}

void EthTxStateManager::RetireTxByStatus(mojom::TransactionStatus status,
                                         size_t max_num) {
  if (status != mojom::TransactionStatus::Confirmed &&
      status != mojom::TransactionStatus::Rejected)
    return;
  auto tx_metas = GetTransactionsByStatus(status, absl::nullopt);
  if (tx_metas.size() > max_num) {
    EthTxStateManager::TxMeta* oldest_meta = nullptr;
    for (const auto& tx_meta : tx_metas) {
      if (!oldest_meta) {
        oldest_meta = tx_meta.get();
      } else {
        if (tx_meta->status == mojom::TransactionStatus::Confirmed &&
            tx_meta->confirmed_time < oldest_meta->confirmed_time) {
          oldest_meta = tx_meta.get();
        } else if (tx_meta->status == mojom::TransactionStatus::Rejected &&
                   tx_meta->created_time < oldest_meta->created_time) {
          oldest_meta = tx_meta.get();
        }
      }
    }
    DeleteTx(oldest_meta->id);
  }
}

void EthTxStateManager::AddObserver(EthTxStateManager::Observer* observer) {
  observers_.AddObserver(observer);
}

void EthTxStateManager::RemoveObserver(EthTxStateManager::Observer* observer) {
  observers_.RemoveObserver(observer);
}

}  // namespace brave_wallet
