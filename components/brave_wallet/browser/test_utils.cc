/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/test_utils.h"

#include <utility>

#include "base/files/scoped_temp_dir.h"
#include "base/memory/raw_ref.h"
#include "base/notreached.h"
#include "base/run_loop.h"
#include "base/scoped_observation.h"
#include "base/strings/string_number_conversions.h"
#include "base/task/sequenced_task_runner.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/tx_storage_delegate.h"
#include "brave/components/brave_wallet/browser/tx_storage_delegate_impl.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {
namespace {

std::string NewAccName(mojom::KeyringId keyring_id, uint32_t index) {
  auto prefix = [&keyring_id]() -> std::string {
    switch (keyring_id) {
      case mojom::KeyringId::kFilecoin:
        return "Filecoin Account";
      case mojom::KeyringId::kFilecoinTestnet:
        return "Filecoin Testnet Account";
      case mojom::KeyringId::kSolana:
        return "Solana Account";
      case mojom::KeyringId::kDefault:
        return "Ethereum Account";
      case mojom::KeyringId::kBitcoin84:
        return "Bitcoin Account";
      case mojom::KeyringId::kBitcoin84Testnet:
        return "Bitcoin Testnet Account";
      case mojom::KeyringId::kZCashMainnet:
        return "ZCash Mainnet Account";
      case mojom::KeyringId::kZCashTestnet:
        return "ZCash Testnet Account";
    }
    NOTREACHED();
    return "";
  };

  return prefix() + " " + base::NumberToString(index);
}

}  // namespace

AccountUtils::AccountUtils(KeyringService* keyring_service)
    : keyring_service_(keyring_service) {}

mojom::AccountInfoPtr AccountUtils::GetDerivedAccount(
    mojom::KeyringId keyring_id,
    uint32_t index) {
  auto all_accounts = keyring_service_->GetAllAccountsSync();
  for (auto& acc : all_accounts->accounts) {
    if (acc->account_id->keyring_id != keyring_id ||
        acc->account_id->kind != mojom::AccountKind::kDerived) {
      continue;
    }
    if (index == 0) {
      return acc->Clone();
    }
    --index;
  }
  return nullptr;
}

mojom::AccountInfoPtr AccountUtils::CreateDerivedAccount(
    mojom::KeyringId keyring_id,
    const std::string& name) {
  auto acc =
      keyring_service_
          ->AddAccountSync(GetCoinForKeyring(keyring_id), keyring_id, name)
          ->Clone();
  EXPECT_TRUE(acc);
  return acc;
}

mojom::AccountInfoPtr AccountUtils::EnsureAccount(mojom::KeyringId keyring_id,
                                                  uint32_t index) {
  for (auto i = 0u; i <= index; ++i) {
    if (!GetDerivedAccount(keyring_id, i)) {
      EXPECT_TRUE(CreateDerivedAccount(keyring_id, NewAccName(keyring_id, i)));
    }
  }
  auto acc = GetDerivedAccount(keyring_id, index);
  EXPECT_TRUE(acc);
  return acc;
}

mojom::AccountInfoPtr AccountUtils::EthAccount(uint32_t index) {
  return GetDerivedAccount(mojom::KeyringId::kDefault, index);
}

mojom::AccountIdPtr AccountUtils::EthAccountId(uint32_t index) {
  return EthAccount(index)->account_id.Clone();
}

mojom::AccountInfoPtr AccountUtils::EthUnkownAccount() {
  auto account_id = EthUnkownAccountId();
  return mojom::AccountInfo::New(account_id.Clone(), account_id->address,
                                 "Unknown Eth Account", nullptr);
}

mojom::AccountIdPtr AccountUtils::EthUnkownAccountId() {
  return MakeAccountId(mojom::CoinType::ETH, mojom::KeyringId::kDefault,
                       mojom::AccountKind::kDerived,
                       "0x1111111111111111111111111111111111111111");
}

mojom::AccountInfoPtr AccountUtils::EnsureEthAccount(uint32_t index) {
  return EnsureAccount(mojom::KeyringId::kDefault, index);
}

mojom::AccountInfoPtr AccountUtils::EnsureSolAccount(uint32_t index) {
  return EnsureAccount(mojom::KeyringId::kSolana, index);
}

mojom::AccountInfoPtr AccountUtils::EnsureFilAccount(uint32_t index) {
  return EnsureAccount(mojom::KeyringId::kFilecoin, index);
}

mojom::AccountInfoPtr AccountUtils::EnsureFilTestAccount(uint32_t index) {
  return EnsureAccount(mojom::KeyringId::kFilecoinTestnet, index);
}

mojom::AccountInfoPtr AccountUtils::EnsureBtcAccount(uint32_t index) {
  return EnsureAccount(mojom::KeyringId::kBitcoin84, index);
}

mojom::AccountInfoPtr AccountUtils::EnsureBtcTestAccount(uint32_t index) {
  return EnsureAccount(mojom::KeyringId::kBitcoin84Testnet, index);
}

mojom::AccountInfoPtr AccountUtils::CreateEthAccount(const std::string& name) {
  return CreateDerivedAccount(mojom::KeyringId::kDefault, name);
}

mojom::AccountInfoPtr AccountUtils::CreateSolAccount(const std::string& name) {
  return CreateDerivedAccount(mojom::KeyringId::kSolana, name);
}

mojom::AccountInfoPtr AccountUtils::CreateFilAccount(const std::string& name) {
  return CreateDerivedAccount(mojom::KeyringId::kFilecoin, name);
}

mojom::AccountInfoPtr AccountUtils::CreateFilTestAccount(
    const std::string& name) {
  return CreateDerivedAccount(mojom::KeyringId::kFilecoinTestnet, name);
}

mojom::AccountInfoPtr AccountUtils::CreateBtcAccount(const std::string& name) {
  return CreateDerivedAccount(mojom::KeyringId::kBitcoin84, name);
}

mojom::AccountInfoPtr AccountUtils::CreateBtcTestAccount(
    const std::string& name) {
  return CreateDerivedAccount(mojom::KeyringId::kBitcoin84Testnet, name);
}

mojom::AccountInfoPtr AccountUtils::CreateEthHWAccount() {
  std::string address = "0xA99D71De40D67394eBe68e4D0265cA6C9D421029";

  std::vector<mojom::HardwareWalletAccountPtr> hw_accounts;
  hw_accounts.push_back(mojom::HardwareWalletAccount::New(
      address, "m/44'/60'/1'/0/0", "HW Account " + address, "Ledger", "device1",
      mojom::CoinType::ETH, mojom::KeyringId::kDefault));

  auto added_accounts =
      keyring_service_->AddHardwareAccountsSync(std::move(hw_accounts));
  return std::move(added_accounts[0]);
}

mojom::AccountIdPtr AccountUtils::FindAccountIdByAddress(
    const std::string& address) {
  auto all_accounts = keyring_service_->GetAllAccountsSync();
  for (auto& acc : all_accounts->accounts) {
    if (acc->address == address) {
      return acc->account_id.Clone();
    }
  }
  return nullptr;
}

void WaitForTxStorageDelegateInitialized(TxStorageDelegate* delegate) {
  if (delegate->IsInitialized()) {
    return;
  }

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

bool AccountResolverDelegateForTest::ValidateAccountId(
    const mojom::AccountIdPtr& account_id) {
  for (auto& acc : accounts_) {
    if (acc == account_id) {
      return true;
    }
  }
  return false;
}

}  // namespace brave_wallet
