/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/tx_state_manager.h"

#include <utility>

#include "base/json/values_util.h"
#include "base/values.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/browser/tx_meta.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "url/origin.h"

namespace brave_wallet {

namespace {

constexpr size_t kMaxConfirmedTxNum = 10;
constexpr size_t kMaxRejectedTxNum = 10;

}  // namespace

// static
bool TxStateManager::ValueToTxMeta(const base::Value::Dict& value,
                                   TxMeta* meta) {
  const std::string* id = value.FindString("id");
  if (!id)
    return false;
  meta->set_id(*id);

  absl::optional<int> status = value.FindInt("status");
  if (!status)
    return false;
  meta->set_status(static_cast<mojom::TransactionStatus>(*status));
  const std::string* from = value.FindString("from");
  if (!from)
    return false;
  meta->set_from(*from);

  const base::Value* created_time = value.Find("created_time");
  if (!created_time)
    return false;
  absl::optional<base::Time> created_time_from_value =
      base::ValueToTime(created_time);
  if (!created_time_from_value)
    return false;
  meta->set_created_time(*created_time_from_value);

  const base::Value* submitted_time = value.Find("submitted_time");
  if (!submitted_time)
    return false;
  absl::optional<base::Time> submitted_time_from_value =
      base::ValueToTime(submitted_time);
  if (!submitted_time_from_value)
    return false;
  meta->set_submitted_time(*submitted_time_from_value);

  const base::Value* confirmed_time = value.Find("confirmed_time");
  if (!confirmed_time)
    return false;
  absl::optional<base::Time> confirmed_time_from_value =
      base::ValueToTime(confirmed_time);
  if (!confirmed_time_from_value)
    return false;
  meta->set_confirmed_time(*confirmed_time_from_value);

  const std::string* tx_hash = value.FindString("tx_hash");
  if (!tx_hash)
    return false;
  meta->set_tx_hash(*tx_hash);

  const std::string* origin_spec = value.FindString("origin");
  // That's ok not to have origin.
  if (origin_spec) {
    meta->set_origin(url::Origin::Create(GURL(*origin_spec)));
    DCHECK(!meta->origin()->opaque());
  }

  const std::string* group_id = value.FindString("group_id");
  if (group_id) {
    meta->set_group_id(*group_id);
  }

  return true;
}

TxStateManager::TxStateManager(PrefService* prefs,
                               JsonRpcService* json_rpc_service)
    : prefs_(prefs), json_rpc_service_(json_rpc_service), weak_factory_(this) {
  DCHECK(json_rpc_service_);
}

TxStateManager::~TxStateManager() = default;

void TxStateManager::AddOrUpdateTx(const TxMeta& meta) {
  DictionaryPrefUpdate update(prefs_, kBraveWalletTransactions);
  base::Value::Dict& dict = update.Get()->GetDict();
  const std::string path = GetTxPrefPathPrefix() + "." + meta.id();

  bool is_add = dict.FindByDottedPath(path) == nullptr;
  dict.SetByDottedPath(path, meta.ToValue());
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

std::unique_ptr<TxMeta> TxStateManager::GetTx(const std::string& id) {
  const auto& dict = prefs_->GetDict(kBraveWalletTransactions);
  const base::Value::Dict* value =
      dict.FindDictByDottedPath(GetTxPrefPathPrefix() + "." + id);
  if (!value)
    return nullptr;

  return ValueToTxMeta(*value);
}

void TxStateManager::DeleteTx(const std::string& id) {
  DictionaryPrefUpdate update(prefs_, kBraveWalletTransactions);
  base::Value* dict = update.Get();
  dict->GetDict().RemoveByDottedPath(GetTxPrefPathPrefix() + "." + id);
}

void TxStateManager::WipeTxs() {
  DictionaryPrefUpdate update(prefs_, kBraveWalletTransactions);
  base::Value* dict = update.Get();
  dict->GetDict().RemoveByDottedPath(GetTxPrefPathPrefix());
}

std::vector<std::unique_ptr<TxMeta>> TxStateManager::GetTransactionsByStatus(
    absl::optional<mojom::TransactionStatus> status,
    absl::optional<std::string> from) {
  std::vector<std::unique_ptr<TxMeta>> result;
  const auto& dict = prefs_->GetDict(kBraveWalletTransactions);
  const base::Value::Dict* network_dict =
      dict.FindDictByDottedPath(GetTxPrefPathPrefix());
  if (!network_dict)
    return result;

  for (const auto it : *network_dict) {
    std::unique_ptr<TxMeta> meta = ValueToTxMeta(it.second.GetDict());
    if (!meta) {
      continue;
    }
    if (!status.has_value() || meta->status() == *status) {
      if (from.has_value() && meta->from() != *from)
        continue;
      result.push_back(std::move(meta));
    }
  }
  return result;
}

void TxStateManager::RetireTxByStatus(mojom::TransactionStatus status,
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
    DCHECK(oldest_meta);
    DeleteTx(oldest_meta->id());
  }
}

void TxStateManager::AddObserver(TxStateManager::Observer* observer) {
  observers_.AddObserver(observer);
}

void TxStateManager::RemoveObserver(TxStateManager::Observer* observer) {
  observers_.RemoveObserver(observer);
}

}  // namespace brave_wallet
