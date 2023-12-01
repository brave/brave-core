/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/tx_storage_delegate_impl.h"

#include <optional>
#include <utility>

#include "base/files/file_path.h"
#include "base/functional/bind.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/common/brave_wallet_constants.h"
#include "components/prefs/pref_service.h"
#include "components/value_store/value_store_frontend.h"
#include "components/value_store/value_store_task_runner.h"

namespace brave_wallet {

namespace {

constexpr char kValueStoreDatabaseUMAClientName[] = "BraveWallet";
constexpr base::FilePath::CharType kWalletStorageName[] =
    FILE_PATH_LITERAL("Brave Wallet Storage");
// key used in transaction storage
constexpr char kStorageTransactions[] = "transactions";

}  // namespace

TxStorageDelegateImpl::TxStorageDelegateImpl(
    PrefService* prefs,
    scoped_refptr<value_store::ValueStoreFactory> store_factory,
    scoped_refptr<base::SequencedTaskRunner> ui_task_runner) {
  store_ = std::make_unique<value_store::ValueStoreFrontend>(
      store_factory, base::FilePath(kWalletStorageName),
      kValueStoreDatabaseUMAClientName, ui_task_runner,
      value_store::GetValueStoreTaskRunner());
  // txs_ has been set during migration.
  if (!MigrateTransactionsFromPrefsToDB(prefs)) {
    Initialize();
  } else {
    initialized_ = true;
  }
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

void TxStorageDelegateImpl::Initialize() {
  store_->Get(kStorageTransactions,
              base::BindOnce(&TxStorageDelegateImpl::OnTxsRead,
                             weak_factory_.GetWeakPtr()));
}

void TxStorageDelegateImpl::OnTxsRead(std::optional<base::Value> txs) {
  if (txs) {
    txs_ = std::move(txs->GetDict());
  }
  initialized_ = true;
  for (auto& observer : observers_) {
    observer.OnStorageInitialized();
  }
}

void TxStorageDelegateImpl::ScheduleWrite() {
  DCHECK(initialized_) << "storage is not initialized yet";
  store_->Set(kStorageTransactions, base::Value(txs_.Clone()));
}

void TxStorageDelegateImpl::Clear() {
  txs_.clear();
  store_->Remove(kStorageTransactions);
}

void TxStorageDelegateImpl::AddObserver(
    TxStorageDelegateImpl::Observer* observer) {
  observers_.AddObserver(observer);
}

void TxStorageDelegateImpl::RemoveObserver(
    TxStorageDelegateImpl::Observer* observer) {
  observers_.RemoveObserver(observer);
}

bool TxStorageDelegateImpl::MigrateTransactionsFromPrefsToDB(
    PrefService* prefs) {
  if (prefs->GetBoolean(kBraveWalletTransactionsFromPrefsToDBMigrated)) {
    return false;
  }

  if (!prefs->HasPrefPath(kBraveWalletTransactions)) {
    prefs->SetBoolean(kBraveWalletTransactionsFromPrefsToDBMigrated, true);
    return false;
  }

  txs_ = prefs->GetDict(kBraveWalletTransactions).Clone();
  store_->Set(kStorageTransactions, base::Value(txs_.Clone()));

  // Keep kBraveWalletTransactions in case we need to revert the migration and
  // remove it when we delete the pref
  prefs->SetBoolean(kBraveWalletTransactionsFromPrefsToDBMigrated, true);
  return true;
}

}  // namespace brave_wallet
