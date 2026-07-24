/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/tx_state_manager.h"

#include <optional>
#include <utility>

#include "base/check.h"
#include "base/check_op.h"
#include "base/json/values_util.h"
#include "base/types/optional_ref.h"
#include "base/values.h"
#include "brave/components/brave_wallet/browser/account_resolver_delegate.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/browser/solana_message.h"
#include "brave/components/brave_wallet/browser/tx_meta.h"
#include "brave/components/brave_wallet/browser/tx_storage.h"
#include "url/origin.h"

namespace brave_wallet {

namespace {

constexpr size_t kMaxConfirmedTxNum = 500;
constexpr size_t kMaxRejectedTxNum = 500;

}  // namespace

bool TxStateManager::ValueToBaseTxMeta(const base::DictValue& value,
                                       TxMeta* meta) {
  const std::string* id = value.FindString("id");
  if (!id) {
    return false;
  }
  meta->set_id(*id);

  std::optional<int> status = value.FindInt("status");
  if (!status) {
    return false;
  }
  const auto tx_status = static_cast<mojom::TransactionStatus>(*status);
  if (!mojom::IsKnownEnumValue(tx_status)) {
    return false;
  }
  meta->set_status(tx_status);
  const std::string* from_account_id = value.FindString("from_account_id");
  const std::string* from_address = value.FindString("from");
  auto account_id = account_resolver_delegate_->ResolveAccountId(
      from_account_id, from_address);
  if (!account_id) {
    return false;
  }
  meta->set_from(std::move(account_id));

  const base::Value* created_time = value.Find("created_time");
  if (!created_time) {
    return false;
  }
  std::optional<base::Time> created_time_from_value =
      base::ValueToTime(created_time);
  if (!created_time_from_value) {
    return false;
  }
  meta->set_created_time(*created_time_from_value);

  const base::Value* submitted_time = value.Find("submitted_time");
  if (!submitted_time) {
    return false;
  }
  std::optional<base::Time> submitted_time_from_value =
      base::ValueToTime(submitted_time);
  if (!submitted_time_from_value) {
    return false;
  }
  meta->set_submitted_time(*submitted_time_from_value);

  const base::Value* confirmed_time = value.Find("confirmed_time");
  if (!confirmed_time) {
    return false;
  }
  std::optional<base::Time> confirmed_time_from_value =
      base::ValueToTime(confirmed_time);
  if (!confirmed_time_from_value) {
    return false;
  }
  meta->set_confirmed_time(*confirmed_time_from_value);

  const std::string* tx_hash = value.FindString("tx_hash");
  if (!tx_hash) {
    return false;
  }
  meta->set_tx_hash(*tx_hash);

  const std::string* origin_spec = value.FindString("origin");
  // That's ok not to have origin.
  if (origin_spec) {
    meta->set_origin(url::Origin::Create(GURL(*origin_spec)));
    DCHECK(!meta->origin()->opaque());
  }

  const auto coin_int = value.FindInt("coin");
  if (!coin_int) {
    return false;
  }
  const auto coin = static_cast<mojom::CoinType>(coin_int.value());
  if (!mojom::IsKnownEnumValue(coin)) {
    return false;
  }
  if (coin != meta->GetCoinType()) {
    return false;
  }

  const auto* chain_id_string = value.FindString("chain_id");
  if (!chain_id_string) {
    return false;
  }
  meta->set_chain_id(*chain_id_string);

  const base::DictValue* swap_info_value = value.FindDict("swap_info");
  if (swap_info_value) {
    auto swap_info = ValueToSwapInfo(*swap_info_value);
    if (swap_info) {
      meta->set_swap_info(std::move(swap_info));
    }
  }

  return true;
}

TxStateManager::TxStateManager(
    TxStorage& tx_storage,
    AccountResolverDelegate& account_resolver_delegate)
    : tx_storage_(tx_storage),
      account_resolver_delegate_(account_resolver_delegate),
      weak_factory_(this) {}

TxStateManager::~TxStateManager() = default;

bool TxStateManager::AddOrUpdateTx(const TxMeta& meta) {
  DCHECK(meta.from());
  DCHECK_EQ(GetCoinType(), meta.GetCoinType());

  if (!tx_storage_->IsInitialized()) {
    return false;
  }
  bool is_add = false;
  {
    auto update = tx_storage_->CreateScopedTxsUpdate();
    is_add = update->Find(meta.id()) == nullptr;
    update->Set(meta.id(), meta.ToValue());
    if (is_add) {
      // We only keep most recent 1k confirmed plus rejected tx metas per
      // network
      RetireTxByStatus(update, meta.chain_id(),
                       mojom::TransactionStatus::Confirmed, kMaxConfirmedTxNum);
      RetireTxByStatus(update, meta.chain_id(),
                       mojom::TransactionStatus::Rejected, kMaxRejectedTxNum);
    }
  }

  if (is_add) {
    for (auto& observer : observers_) {
      observer.OnNewUnapprovedTx(meta.ToTransactionInfo());
    }
  } else {
    for (auto& observer : observers_) {
      observer.OnTransactionStatusChanged(meta.ToTransactionInfo());
    }
  }

  return true;
}

std::unique_ptr<TxMeta> TxStateManager::GetTx(const std::string& meta_id) {
  if (!tx_storage_->IsInitialized()) {
    return nullptr;
  }
  const auto& txs = tx_storage_->GetTxs();
  const base::DictValue* value = txs.FindDict(meta_id);
  if (!value) {
    return nullptr;
  }

  return ValueToTxMeta(*value);
}

bool TxStateManager::DeleteTx(const std::string& meta_id) {
  if (!tx_storage_->IsInitialized()) {
    return false;
  }
  {
    auto update = tx_storage_->CreateScopedTxsUpdate();
    update->Remove(meta_id);
  }
  return true;
}

std::vector<std::unique_ptr<TxMeta>> TxStateManager::GetTransactionsByStatus(
    const std::optional<std::string>& chain_id,
    const std::optional<mojom::TransactionStatus>& status,
    const mojom::AccountIdPtr& from) {
  DCHECK(from);
  return GetTransactionsByStatus(chain_id, status,
                                 std::make_optional(from.Clone()));
}

std::vector<std::unique_ptr<TxMeta>> TxStateManager::GetTransactionsByStatus(
    const std::optional<std::string>& chain_id,
    const std::optional<mojom::TransactionStatus>& status,
    const std::optional<mojom::AccountIdPtr>& from) {
  std::vector<std::unique_ptr<TxMeta>> result;
  if (!tx_storage_->IsInitialized()) {
    return result;
  }
  const auto& txs = tx_storage_->GetTxs();

  for (const auto it : txs) {
    auto* meta_dict = it.second.GetIfDict();
    if (!meta_dict) {
      continue;
    }

    std::unique_ptr<TxMeta> meta = ValueToTxMeta(*meta_dict);
    if (!meta) {
      continue;
    }
    if (meta->from()->coin != GetCoinType()) {
      continue;
    }
    if (chain_id.has_value() && meta->chain_id() != *chain_id) {
      continue;
    }
    if (status.has_value() && meta->status() != *status) {
      continue;
    }
    if (from.has_value() && meta->from() != *from) {
      continue;
    }

    result.push_back(std::move(meta));
  }

  return result;
}

void TxStateManager::RetireTxByStatus(TxStorage::ScopedTxsUpdate& update,
                                      const std::string& chain_id,
                                      mojom::TransactionStatus status,
                                      size_t max_num) {
  if (no_retire_for_testing_) {
    return;
  }

  if (status != mojom::TransactionStatus::Confirmed &&
      status != mojom::TransactionStatus::Rejected) {
    return;
  }

  base::DictValue& txs = *update;

  auto const status_type = static_cast<int>(status);
  auto const coin_type = static_cast<int>(GetCoinType());

  const char* time_key =
      (status == mojom::TransactionStatus::Confirmed ? "confirmed_time"
                                                     : "created_time");

  // Points into `txs`; left at end() until we find a candidate to retire.
  auto oldest_it = txs.end();
  uint64_t num_txs = 0;

  // Everything is smaller than this.
  base::Time oldest_time = base::Time::Max();

  for (auto it = txs.begin(); it != txs.end(); ++it) {
    const auto& value = it->second;

    const auto* dict = value.GetIfDict();
    if (!dict) {
      continue;
    }

    const auto coin = dict->FindInt("coin");
    if (!coin || coin.value() != coin_type) {
      continue;
    }

    const auto* tx_chain_id = dict->FindString("chain_id");
    if (!tx_chain_id || *tx_chain_id != chain_id) {
      continue;
    }

    const auto tx_status = dict->FindInt("status");
    if (!tx_status || tx_status.value() != status_type) {
      continue;
    }

    ++num_txs;

    const auto* time_val = dict->Find(time_key);
    if (!time_val) {
      continue;
    }

    auto t = base::ValueToTime(time_val);
    if (!t || *t >= oldest_time) {
      continue;
    }

    oldest_time = *t;
    oldest_it = it;
  }

  if (num_txs > max_num) {
    CHECK(oldest_it != txs.end());
    txs.erase(oldest_it);
  }
}

void TxStateManager::AddObserver(TxStateManager::Observer* observer) {
  observers_.AddObserver(observer);
}

void TxStateManager::RemoveObserver(TxStateManager::Observer* observer) {
  observers_.RemoveObserver(observer);
}

void TxStateManager::SetNoRetireForTesting(bool no_retire) {
  no_retire_for_testing_ = no_retire;
}

}  // namespace brave_wallet
