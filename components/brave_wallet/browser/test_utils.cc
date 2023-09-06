/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/test_utils.h"

#include "base/files/scoped_temp_dir.h"
#include "base/memory/raw_ref.h"
#include "base/run_loop.h"
#include "base/scoped_observation.h"
#include "base/task/sequenced_task_runner.h"
#include "brave/components/brave_wallet/browser/tx_storage_delegate.h"
#include "brave/components/brave_wallet/browser/tx_storage_delegate_impl.h"

namespace brave_wallet {

void WaitForTxStorageDelegateInitialized(TxStorageDelegate* delegate) {
  base::RunLoop run_loop;
  class TestTxStorageDelegateObserver : public TxStorageDelegate::Observer {
   public:
    explicit TestTxStorageDelegateObserver(TxStorageDelegate* delegate,
                                           base::RunLoop& run_loop)
        : run_loop_(run_loop) {
      observation_.Observe(delegate);
    }

    void OnStorageInitialized() override { run_loop_->Quit(); }

   private:
    base::ScopedObservation<TxStorageDelegate, TxStorageDelegate::Observer>
        observation_{this};
    raw_ref<base::RunLoop> run_loop_;
  } observer(delegate, run_loop);
  run_loop.Run();
}

scoped_refptr<value_store::TestValueStoreFactory> GetTestValueStoreFactory(
    base::ScopedTempDir& temp_dir) {
  CHECK(temp_dir.CreateUniqueTempDir());

  base::FilePath db_path = temp_dir.GetPath().AppendASCII("temp_db");

  return new value_store::TestValueStoreFactory(db_path);
}

std::unique_ptr<TxStorageDelegateImpl> GetTxStorageDelegateForTest(
    PrefService* prefs,
    scoped_refptr<value_store::ValueStoreFactory> store_factory) {
  return std::make_unique<TxStorageDelegateImpl>(
      prefs, store_factory, base::SequencedTaskRunner::GetCurrentDefault());
}

AccountResolverDelegateForTest::AccountResolverDelegateForTest() = default;
AccountResolverDelegateForTest::~AccountResolverDelegateForTest() = default;

mojom::AccountIdPtr AccountResolverDelegateForTest::RegisterAccount(
    mojom::AccountIdPtr account_id) {
  accounts_.push_back(account_id->Clone());
  return account_id;
}

mojom::AccountIdPtr AccountResolverDelegateForTest::ResolveAccountId(
    const std::string* from_account_id,
    const std::string* from_address) {
  for (auto& acc : accounts_) {
    if (from_account_id && acc->unique_key == *from_account_id) {
      return acc->Clone();
    }
    if (from_address && acc->address == *from_address) {
      return acc->Clone();
    }
  }

  return nullptr;
}

}  // namespace brave_wallet
