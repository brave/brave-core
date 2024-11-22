/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/tx_storage_delegate_impl.h"

#include <optional>
#include <utility>

#include "base/files/file_path.h"
#include "base/functional/bind.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/common/brave_wallet_constants.h"
#include "components/prefs/pref_service.h"
#include "components/value_store/value_store_frontend.h"
#include "components/value_store/value_store_task_runner.h"

namespace brave_wallet {

// DEPRECATED 01/2024. For migration only.
std::optional<mojom::CoinType> GetCoinTypeFromPrefKey_DEPRECATED(
    std::string_view key);

namespace {

constexpr char kValueStoreDatabaseUMAClientName[] = "BraveWallet";
constexpr base::FilePath::CharType kWalletStorageName[] =
    FILE_PATH_LITERAL("Brave Wallet Storage");
// key used in transaction storage
constexpr char kStorageTransactionsKey[] = "transactions";

// Converts from tree of dicts `coin.network_id.txid: tx` to `txid: tx` dict
// Each tx gets coin field.
base::Value::Dict MigrateToOneLevelDict(const base::Value::Dict& txs) {
  base::Value::Dict result;
  for (const auto [coin_key, networks_dict] : txs) {
    if (!networks_dict.is_dict()) {
      continue;
    }
    auto coin = GetCoinTypeFromPrefKey_DEPRECATED(coin_key);
    if (!coin) {
      continue;
    }
    for (const auto [network_id, tx_dict] : networks_dict.GetDict()) {
      if (!tx_dict.is_dict()) {
        continue;
      }

      for (const auto [meta_id, tx] : tx_dict.GetDict()) {
        if (!tx.is_dict()) {
          continue;
        }
        // tx already has chain_id.
        DCHECK(tx.GetDict().FindString("chain_id"));

        auto new_dict = tx.GetDict().Clone();
        new_dict.Set("coin", static_cast<int>(*coin));
        result.Set(meta_id, std::move(new_dict));
      }
    }
  }

  return result;
}

}  // namespace

TxStorageDelegateImpl::TxStorageDelegateImpl(
    PrefService* prefs,
    scoped_refptr<value_store::ValueStoreFactory> store_factory,
    scoped_refptr<base::SequencedTaskRunner> ui_task_runner)
    : prefs_(prefs) {
  store_ = MakeValueStoreFrontend(store_factory, ui_task_runner);

  MigrateTransactionsFromPrefsToDB();

  Initialize();
}

TxStorageDelegateImpl::~TxStorageDelegateImpl() = default;

bool TxStorageDelegateImpl::IsInitialized() const {
  return initialized_;
}

const base::Value::Dict& TxStorageDelegateImpl::GetTxs() const {
  return txs_;
}

base::Value::Dict& TxStorageDelegateImpl::GetTxs() {
  return txs_;
}

std::unique_ptr<value_store::ValueStoreFrontend>
TxStorageDelegateImpl::MakeValueStoreFrontend(
    scoped_refptr<value_store::ValueStoreFactory> store_factory,
    scoped_refptr<base::SequencedTaskRunner> ui_task_runner) {
  return std::make_unique<value_store::ValueStoreFrontend>(
      std::move(store_factory), base::FilePath(kWalletStorageName),
      kValueStoreDatabaseUMAClientName, std::move(ui_task_runner),
      value_store::GetValueStoreTaskRunner());
}

void TxStorageDelegateImpl::Initialize() {
  store_->Get(kStorageTransactionsKey,
              base::BindOnce(&TxStorageDelegateImpl::OnTxsInitialRead,
                             weak_factory_.GetWeakPtr()));
}

void TxStorageDelegateImpl::OnTxsInitialRead(std::optional<base::Value> txs) {
  if (txs) {
    txs_ = std::move(txs->GetDict());
  }
  initialized_ = true;
  RunDBMigrations();
  for (auto& observer : observers_) {
    observer.OnStorageInitialized();
  }
}

void TxStorageDelegateImpl::RunDBMigrations() {
  bool schedule_write = false;
  // Added 01/2024
  if (!prefs_->GetBoolean(kBraveWalletTransactionsDBFormatMigrated)) {
    prefs_->SetBoolean(kBraveWalletTransactionsDBFormatMigrated, true);

    txs_ = MigrateToOneLevelDict(txs_);
    schedule_write = !txs_.empty();
  }

  if (schedule_write) {
    ScheduleWrite();
  }
}

void TxStorageDelegateImpl::ScheduleWrite() {
  if (disable_writes_for_testing_) {
    return;
  }

  DCHECK(initialized_) << "storage is not initialized yet";
  store_->Set(kStorageTransactionsKey, base::Value(txs_.Clone()));
}

void TxStorageDelegateImpl::DisableWritesForTesting(bool disable) {
  disable_writes_for_testing_ = disable;
}

void TxStorageDelegateImpl::Clear() {
  txs_.clear();
  store_->Remove(kStorageTransactionsKey);
}

void TxStorageDelegateImpl::AddObserver(
    TxStorageDelegateImpl::Observer* observer) {
  observers_.AddObserver(observer);
}

void TxStorageDelegateImpl::RemoveObserver(
    TxStorageDelegateImpl::Observer* observer) {
  observers_.RemoveObserver(observer);
}

bool TxStorageDelegateImpl::MigrateTransactionsFromPrefsToDB() {
  if (prefs_->GetBoolean(kBraveWalletTransactionsFromPrefsToDBMigrated)) {
    return false;
  }

  if (!prefs_->HasPrefPath(kBraveWalletTransactions)) {
    prefs_->SetBoolean(kBraveWalletTransactionsFromPrefsToDBMigrated, true);
    return false;
  }

  auto& txs = prefs_->GetDict(kBraveWalletTransactions);
  store_->Set(kStorageTransactionsKey, base::Value(txs.Clone()));

  // Keep kBraveWalletTransactions in case we need to revert the migration and
  // remove it when we delete the pref
  prefs_->SetBoolean(kBraveWalletTransactionsFromPrefsToDBMigrated, true);
  return true;
}

}  // namespace brave_wallet
