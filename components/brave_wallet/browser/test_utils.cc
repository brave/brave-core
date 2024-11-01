/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/test_utils.h"

#include <memory>
#include <utility>

#include "base/containers/contains.h"
#include "base/files/scoped_temp_dir.h"
#include "base/memory/raw_ref.h"
#include "base/notreached.h"
#include "base/run_loop.h"
#include "base/scoped_observation.h"
#include "base/strings/string_number_conversions.h"
#include "base/task/sequenced_task_runner.h"
#include "brave/components/brave_wallet/browser/bitcoin/bitcoin_test_utils.h"
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
        return "Zcash Mainnet Account";
      case mojom::KeyringId::kZCashTestnet:
        return "Zcash Testnet Account";
      case mojom::KeyringId::kBitcoinImport:
        return "Bitcoin Imported Account";
      case mojom::KeyringId::kBitcoinImportTestnet:
        return "Bitcoin Imported Testnet Account";
      case mojom::KeyringId::kBitcoinHardware:
        return "Bitcoin Hardware Account";
      case mojom::KeyringId::kBitcoinHardwareTestnet:
        return "Bitcoin Hardware Testnet Account";
    }
    NOTREACHED_IN_MIGRATION();
    return "";
  };

  return prefix() + " " + base::NumberToString(index);
}

}  // namespace

AccountUtils::AccountUtils(KeyringService* keyring_service)
    : keyring_service_(keyring_service) {}

void AccountUtils::CreateWallet(const std::string& mnemonic,
                                const std::string& password) {
  keyring_service_->CreateWalletInternal(mnemonic, password, false, false);
}

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

mojom::AccountInfoPtr AccountUtils::GetImportedAccount(
    mojom::KeyringId keyring_id,
    uint32_t index) {
  EXPECT_TRUE(IsBitcoinImportKeyring(keyring_id));

  auto all_accounts = keyring_service_->GetAllAccountsSync();
  for (auto& acc : all_accounts->accounts) {
    if (acc->account_id->keyring_id != keyring_id ||
        acc->account_id->kind != mojom::AccountKind::kImported) {
      continue;
    }
    if (index == 0) {
      return acc->Clone();
    }
    --index;
  }
  return nullptr;
}

mojom::AccountInfoPtr AccountUtils::GetHardwareAccount(
    mojom::KeyringId keyring_id,
    uint32_t index) {
  EXPECT_TRUE(IsBitcoinHardwareKeyring(keyring_id));

  auto all_accounts = keyring_service_->GetAllAccountsSync();
  for (auto& acc : all_accounts->accounts) {
    if (acc->account_id->keyring_id != keyring_id ||
        acc->account_id->kind != mojom::AccountKind::kHardware) {
      continue;
    }
    if (index == 0) {
      return acc->Clone();
    }
    --index;
  }
  return nullptr;
}

mojom::AccountInfoPtr AccountUtils::CreateImportedAccount(
    mojom::KeyringId keyring_id,
    const std::string& name) {
  EXPECT_TRUE(IsBitcoinImportKeyring(keyring_id));
  const auto network = GetNetworkForBitcoinKeyring(keyring_id);
  auto acc =
      keyring_service_
          ->ImportBitcoinAccountSync(name,
                                     (network == mojom::kBitcoinMainnet)
                                         ? kBtcMainnetImportAccount0
                                         : kBtcTestnetImportAccount0,
                                     GetNetworkForBitcoinKeyring(keyring_id))
          ->Clone();
  EXPECT_TRUE(acc);
  return acc;
}

mojom::AccountInfoPtr AccountUtils::CreateHardwareAccount(
    mojom::KeyringId keyring_id,
    const std::string& name) {
  EXPECT_TRUE(IsBitcoinHardwareKeyring(keyring_id));
  const auto network = GetNetworkForBitcoinKeyring(keyring_id);

  mojom::HardwareWalletAccountPtr hw_account;
  if (keyring_id == mojom::KeyringId::kBitcoinHardware) {
    hw_account = mojom::HardwareWalletAccount::New(
        "xpub6C9TRymDq1G8ueHrv4Etbvzv1ARp4fFAHezEuLQ7X3VcZM7ZKco3aBup3fyzSHhnbF"
        "fXtXF3m8EWTwk1TMvTVSciQ1BHxtvjMGcGLkCE2nz",
        "derivation_path", "Btc hw account", mojom::HardwareVendor::kLedger,
        "device_id", mojom::KeyringId::kBitcoinHardware);
  } else if (keyring_id == mojom::KeyringId::kBitcoinHardwareTestnet) {
    hw_account = mojom::HardwareWalletAccount::New(
        "tpubD6NzVbkrYhZ4XgiXtGrdW5XDAPFCL9h7we1vwNCpn8tGbBcgfVYjXyhWo4E1xkh56h"
        "jod1RhGjxbaTLV3X4FyWuejifB9jusQ46QzG87VKp",
        "derivation_path", "Btc hw testnet account",
        mojom::HardwareVendor::kLedger, "device_id",
        mojom::KeyringId::kBitcoinHardwareTestnet);
  }
  auto acc =
      keyring_service_->AddBitcoinHardwareAccountSync(std::move(hw_account))
          ->Clone();
  EXPECT_TRUE(acc);
  return acc;
}

mojom::AccountInfoPtr AccountUtils::EnsureAccount(mojom::KeyringId keyring_id,
                                                  uint32_t index) {
  if (IsBitcoinImportKeyring(keyring_id)) {
    for (auto i = 0u; i <= index; ++i) {
      if (!GetImportedAccount(keyring_id, i)) {
        EXPECT_TRUE(
            CreateImportedAccount(keyring_id, NewAccName(keyring_id, i)));
      }
    }
    auto acc = GetImportedAccount(keyring_id, index);
    EXPECT_TRUE(acc);
    return acc;
  }
  if (IsBitcoinHardwareKeyring(keyring_id)) {
    for (auto i = 0u; i <= index; ++i) {
      if (!GetHardwareAccount(keyring_id, i)) {
        EXPECT_TRUE(
            CreateHardwareAccount(keyring_id, NewAccName(keyring_id, i)));
      }
    }
    auto acc = GetHardwareAccount(keyring_id, index);
    EXPECT_TRUE(acc);
    return acc;
  }

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

mojom::AccountInfoPtr AccountUtils::EnsureZecAccount(uint32_t index) {
  return EnsureAccount(mojom::KeyringId::kZCashMainnet, index);
}

mojom::AccountInfoPtr AccountUtils::EnsureZecTestAccount(uint32_t index) {
  return EnsureAccount(mojom::KeyringId::kZCashTestnet, index);
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

mojom::AccountInfoPtr AccountUtils::CreateZecAccount(const std::string& name) {
  return CreateDerivedAccount(mojom::KeyringId::kZCashMainnet, name);
}

mojom::AccountInfoPtr AccountUtils::CreateZecTestAccount(
    const std::string& name) {
  return CreateDerivedAccount(mojom::KeyringId::kZCashTestnet, name);
}

mojom::AccountInfoPtr AccountUtils::CreateEthHWAccount() {
  std::string address = "0xA99D71De40D67394eBe68e4D0265cA6C9D421029";

  std::vector<mojom::HardwareWalletAccountPtr> hw_accounts;
  hw_accounts.push_back(mojom::HardwareWalletAccount::New(
      address, "m/44'/60'/1'/0/0", "HW Account " + address,
      mojom::HardwareVendor::kLedger, "device1", mojom::KeyringId::kDefault));

  auto added_accounts =
      keyring_service_->AddHardwareAccountsSync(std::move(hw_accounts));
  return std::move(added_accounts[0]);
}

mojom::AccountInfoPtr AccountUtils::CreateBtcHWAccount() {
  mojom::HardwareWalletAccountPtr account = mojom::HardwareWalletAccount::New(
      "xpub661MyMwAqRbcFtXgS5sYJABqqG9YLmC4Q1Rdap9gSE8NqtwybGhePY2gZ29ESFjqJoCu"
      "1Rupje8YtGqsefD265TMg7usUDFdp6W1EGMcet8",
      "m/84'/0'/0'", "HW Account", mojom::HardwareVendor::kLedger, "device1",
      mojom::KeyringId::kBitcoinHardware);

  return keyring_service_->AddBitcoinHardwareAccountSync(account.Clone());
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

std::vector<mojom::AccountInfoPtr> AccountUtils::AllAccounts(
    mojom::KeyringId keyring_id) {
  return AllAccounts(std::vector<mojom::KeyringId>{keyring_id});
}

std::vector<mojom::AccountInfoPtr> AccountUtils::AllAccounts(
    const std::vector<mojom::KeyringId>& keyring_ids) {
  std::vector<mojom::AccountInfoPtr> result;
  for (auto& acc : keyring_service_->GetAllAccountInfos()) {
    if (base::Contains(keyring_ids, acc->account_id->keyring_id)) {
      result.push_back(acc->Clone());
    }
  }
  return result;
}

std::vector<mojom::AccountInfoPtr> AccountUtils::AllEthAccounts() {
  return AllAccounts(mojom::KeyringId::kDefault);
}

std::vector<mojom::AccountInfoPtr> AccountUtils::AllSolAccounts() {
  return AllAccounts(mojom::KeyringId::kSolana);
}

std::vector<mojom::AccountInfoPtr> AccountUtils::AllFilAccounts() {
  return AllAccounts(mojom::KeyringId::kFilecoin);
}

std::vector<mojom::AccountInfoPtr> AccountUtils::AllFilTestAccounts() {
  return AllAccounts(mojom::KeyringId::kFilecoinTestnet);
}

std::vector<mojom::AccountInfoPtr> AccountUtils::AllBtcAccounts() {
  return AllAccounts({mojom::KeyringId::kBitcoin84,
                      mojom::KeyringId::kBitcoinImport,
                      mojom::KeyringId::kBitcoinHardware});
}

std::vector<mojom::AccountInfoPtr> AccountUtils::AllBtcTestAccounts() {
  return AllAccounts({mojom::KeyringId::kBitcoin84Testnet,
                      mojom::KeyringId::kBitcoinImportTestnet,
                      mojom::KeyringId::kBitcoinHardwareTestnet});
}

std::vector<mojom::AccountInfoPtr> AccountUtils::AllZecAccounts() {
  return AllAccounts(mojom::KeyringId::kZCashMainnet);
}

std::vector<mojom::AccountInfoPtr> AccountUtils::AllZecTestAccounts() {
  return AllAccounts(mojom::KeyringId::kZCashTestnet);
}

TestBraveWalletServiceDelegate::TestBraveWalletServiceDelegate() {
  EXPECT_TRUE(temp_dir_.CreateUniqueTempDir());
}

base::FilePath TestBraveWalletServiceDelegate::GetWalletBaseDirectory() {
  return temp_dir_.GetPath();
}

bool TestBraveWalletServiceDelegate::IsPrivateWindow() {
  return false;
}

// static
std::unique_ptr<BraveWalletServiceDelegate>
TestBraveWalletServiceDelegate::Create() {
  return std::make_unique<TestBraveWalletServiceDelegate>();
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
    const raw_ref<base::RunLoop> run_loop_;
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
  auto delegate = std::make_unique<TxStorageDelegateImpl>(
      prefs, store_factory, base::SequencedTaskRunner::GetCurrentDefault());
  WaitForTxStorageDelegateInitialized(delegate.get());
  return delegate;
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
