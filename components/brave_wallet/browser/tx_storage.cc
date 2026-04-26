/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/tx_storage.h"

#include <memory>
#include <optional>
#include <utility>

#include "base/check.h"
#include "base/files/file_path.h"
#include "base/functional/bind.h"
#include "base/task/sequenced_task_runner.h"
#include "components/prefs/pref_service.h"
#include "components/value_store/value_store_factory_impl.h"
#include "components/value_store/value_store_frontend.h"
#include "components/value_store/value_store_task_runner.h"

namespace brave_wallet {

namespace {

constexpr char kValueStoreDatabaseUMAClientName[] = "BraveWallet";
constexpr base::FilePath::CharType kWalletStorageName[] =
    FILE_PATH_LITERAL("Brave Wallet Storage");
// key used in transaction storage
constexpr char kStorageTransactionsKey[] = "transactions";

std::unique_ptr<value_store::ValueStoreFrontend> CreateValueStoreFrontend(
    const base::FilePath& wallet_base_directory) {
  return std::make_unique<value_store::ValueStoreFrontend>(
      base::MakeRefCounted<value_store::ValueStoreFactoryImpl>(
          wallet_base_directory),
      base::FilePath(kWalletStorageName), kValueStoreDatabaseUMAClientName,
      base::SequencedTaskRunner::GetCurrentDefault(),
      value_store::GetValueStoreTaskRunner());
}

}  // namespace

TxStorage::TxStorage(std::unique_ptr<value_store::ValueStoreFrontend> store)
    : store_(std::move(store)) {
  Initialize();
}

TxStorage::~TxStorage() = default;

TxStorage::ScopedTxsUpdate::ScopedTxsUpdate(TxStorage& tx_storage)
    : tx_storage_(tx_storage) {}

TxStorage::ScopedTxsUpdate::~ScopedTxsUpdate() {
  tx_storage_->ScheduleWrite();
}

base::DictValue& TxStorage::ScopedTxsUpdate::Get() {
  return tx_storage_->txs_;
}

bool TxStorage::IsInitialized() const {
  return initialized_;
}

const base::DictValue& TxStorage::GetTxs() const {
  return txs_;
}

void TxStorage::Initialize() {
  if (!store_) {
    base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE, base::BindOnce(&TxStorage::OnTxsInitialRead,
                                  weak_factory_.GetWeakPtr(), std::nullopt));
    return;
  }

  store_->Get(
      kStorageTransactionsKey,
      base::BindOnce(&TxStorage::OnTxsInitialRead, weak_factory_.GetWeakPtr()));
}

void TxStorage::OnTxsInitialRead(std::optional<base::Value> txs) {
  if (txs) {
    txs_ = std::move(txs->GetDict());
  }
  initialized_ = true;
  RunDBMigrations();
  if (on_initialized_callback_for_testing_) {
    std::move(on_initialized_callback_for_testing_).Run();
  }
}

void TxStorage::RunDBMigrations() {
  bool schedule_write = false;

  // Placeholder for future DB migrations.

  if (schedule_write) {
    ScheduleWrite();
  }
}

void TxStorage::ScheduleWrite() {
  if (disable_writes_for_testing_) {
    return;
  }

  DCHECK(initialized_) << "storage is not initialized yet";
  if (store_) {
    store_->Set(kStorageTransactionsKey, base::Value(txs_.Clone()));
  }
}

void TxStorage::DisableWritesForTesting(bool disable) {
  disable_writes_for_testing_ = disable;
}

void TxStorage::SetOnInitializedCallbackForTesting(  // IN-TEST
    base::OnceClosure callback) {
  on_initialized_callback_for_testing_ = std::move(callback);
}

void TxStorage::Clear() {
  txs_.clear();

  if (store_) {
    store_->Remove(kStorageTransactionsKey);
  }
}

TxStorage::ScopedTxsUpdate TxStorage::CreateScopedTxsUpdate() {
  return ScopedTxsUpdate(*this);
}

// static
std::unique_ptr<TxStorage> TxStorage::MakeWithDbStorage(
    const base::FilePath& wallet_base_directory) {
  return std::make_unique<TxStorage>(
      CreateValueStoreFrontend(wallet_base_directory));
}

// static
std::unique_ptr<TxStorage> TxStorage::MakeWithMemoryOnlyStorage() {
  return std::make_unique<TxStorage>(nullptr);
}

}  // namespace brave_wallet
