/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/zcash/zcash_wallet_service.h"

#include <array>
#include <memory>
#include <optional>
#include <string>
#include <utility>

#include "base/memory/scoped_refptr.h"
#include "base/strings/string_number_conversions.h"
#include "base/test/bind.h"
#include "base/test/mock_callback.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "brave/components/brave_wallet/browser/brave_wallet_prefs.h"
#include "brave/components/brave_wallet/browser/test_utils.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_complete_transaction_task.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_rpc.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "brave/components/brave_wallet/common/features.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "brave/components/brave_wallet/common/test_utils.h"
#include "brave/components/brave_wallet/common/zcash_utils.h"
#include "brave/components/services/brave_wallet/public/mojom/zcash_decoder.mojom.h"
#include "build/build_config.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

#if BUILDFLAG(ENABLE_ORCHARD)
#include "base/task/sequenced_task_runner.h"
#include "base/task/thread_pool.h"
#include "base/test/scoped_run_loop_timeout.h"
#include "base/threading/sequence_bound.h"
#include "brave/components/brave_wallet/browser/internal/orchard_bundle_manager.h"
#include "brave/components/brave_wallet/browser/internal/orchard_sync_state.h"
#include "brave/components/brave_wallet/browser/internal/orchard_test_utils.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_auto_sync_manager.h"
#endif

using testing::_;
using testing::Eq;
using testing::SaveArg;
using testing::Truly;
using testing::WithArg;

namespace brave_wallet {

namespace {

#if BUILDFLAG(ENABLE_ORCHARD)
constexpr char kGateJuniorMnemonic[] =
    "gate junior chunk maple cage select orange circle price air tortoise "
    "jelly art frequent fence middle ice moral wage toddler attitude sign "
    "lesson grain";
#endif

std::array<uint8_t, 32> GetTxId(const std::string& hex_string) {
  std::vector<uint8_t> vec;
  std::array<uint8_t, 32> sized_vec;

  base::HexStringToBytes(hex_string, &vec);
  std::reverse(vec.begin(), vec.end());
  std::copy_n(vec.begin(), 32, sized_vec.begin());
  return sized_vec;
}

#if BUILDFLAG(ENABLE_ORCHARD) && !BUILDFLAG(IS_ANDROID)
void AppendMerklePath(OrchardNoteWitness& witness, const std::string& hex) {
  OrchardMerkleHash hash;
  base::span(hash).copy_from(*PrefixedHexStringToBytes(hex));
  witness.merkle_path.push_back(hash);
}
#endif

class MockZCashRPC : public ZCashRpc {
 public:
  MockZCashRPC() : ZCashRpc(nullptr, nullptr) {}
  ~MockZCashRPC() override {}
  MOCK_METHOD3(GetUtxoList,
               void(const std::string& chain_id,
                    const std::string& address,
                    GetUtxoListCallback callback));

  MOCK_METHOD2(GetLatestBlock,
               void(const std::string& chain_id,
                    GetLatestBlockCallback callback));

  MOCK_METHOD3(GetTransaction,
               void(const std::string& chain_id,
                    const std::string& tx_hash,
                    GetTransactionCallback callback));

  MOCK_METHOD3(SendTransaction,
               void(const std::string& chain_id,
                    base::span<const uint8_t> data,
                    SendTransactionCallback callback));

  MOCK_METHOD5(IsKnownAddress,
               void(const std::string& chain_id,
                    const std::string& addr,
                    uint64_t block_start,
                    uint64_t block_end,
                    IsKnownAddressCallback callback));

  MOCK_METHOD2(GetLatestTreeState,
               void(const std::string& chain_id,
                    GetTreeStateCallback callback));

  MOCK_METHOD3(GetTreeState,
               void(const std::string& chain_id,
                    zcash::mojom::BlockIDPtr block_id,
                    GetTreeStateCallback callback));

  MOCK_METHOD2(GetLightdInfo,
               void(const std::string& chain_id,
                    GetLightdInfoCallback callback));
};

#if BUILDFLAG(ENABLE_ORCHARD)
class MockOrchardSyncState : public OrchardSyncState {
 public:
  explicit MockOrchardSyncState(const base::FilePath& path_to_database)
      : OrchardSyncState(path_to_database) {}
  ~MockOrchardSyncState() override {}

  MOCK_METHOD2(
      GetSpendableNotes,
      base::expected<std::optional<OrchardSyncState::SpendableNotesBundle>,
                     OrchardStorage::Error>(
          const mojom::AccountIdPtr& account_id,
          const OrchardAddrRawPart& internal_addr));

  MOCK_METHOD3(CalculateWitnessForCheckpoint,
               base::expected<std::vector<OrchardInput>, OrchardStorage::Error>(
                   const mojom::AccountIdPtr& account_id,
                   const std::vector<OrchardInput>& notes,
                   uint32_t checkpoint_position));

  MOCK_METHOD3(GetMaxCheckpointedHeight,
               base::expected<std::optional<uint32_t>, OrchardStorage::Error>(
                   const mojom::AccountIdPtr& account_id,
                   uint32_t chain_tip_height,
                   uint32_t min_confirmations));
};

class MockOrchardSyncStateProxy : public OrchardSyncState {
 public:
  MockOrchardSyncStateProxy(const base::FilePath& file_path,
                            OrchardSyncState* instance)
      : OrchardSyncState(file_path), instance_(instance) {}

  ~MockOrchardSyncStateProxy() override {}

  base::expected<std::optional<OrchardSyncState::SpendableNotesBundle>,
                 OrchardStorage::Error>
  GetSpendableNotes(const mojom::AccountIdPtr& account_id,
                    const OrchardAddrRawPart& internal_addr) override {
    return instance_->GetSpendableNotes(account_id, internal_addr);
  }

  base::expected<std::vector<OrchardInput>, OrchardStorage::Error>
  CalculateWitnessForCheckpoint(const mojom::AccountIdPtr& account_id,
                                const std::vector<OrchardInput>& notes,
                                uint32_t checkpoint_position) override {
    return instance_->CalculateWitnessForCheckpoint(account_id, notes,
                                                    checkpoint_position);
  }

 private:
  raw_ptr<OrchardSyncState> instance_;
};
#endif

}  // namespace

class ZCashWalletServiceUnitTest : public testing::Test {
 public:
  ZCashWalletServiceUnitTest() {}

  ~ZCashWalletServiceUnitTest() override = default;

  void SetUp() override {
    feature_list_.InitAndEnableFeature(
        brave_wallet::features::kBraveWalletZCashFeature);
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    db_path_ = temp_dir_.GetPath().Append(FILE_PATH_LITERAL("orchard.db"));
    brave_wallet::RegisterProfilePrefs(prefs_.registry());
    brave_wallet::RegisterLocalStatePrefs(local_state_.registry());
    keyring_service_ =
        std::make_unique<KeyringService>(nullptr, &prefs_, &local_state_);
#if BUILDFLAG(ENABLE_ORCHARD)
    mocked_sync_state_ = std::make_unique<MockOrchardSyncState>(db_path_);
#endif
    zcash_wallet_service_ = std::make_unique<ZCashWalletService>(
        db_path_, *keyring_service_,
        std::make_unique<testing::NiceMock<MockZCashRPC>>());
    GetAccountUtils().CreateWallet(kMnemonicDivideCruise, kTestWalletPassword);
    zcash_account_ =
        GetAccountUtils().EnsureAccount(mojom::KeyringId::kZCashMainnet, 0);
    ASSERT_TRUE(zcash_account_);

    ON_CALL(zcash_rpc(), GetLightdInfo(_, _))
        .WillByDefault([&](const std::string& chain_id,
                           ZCashRpc::GetLightdInfoCallback callback) {
          EXPECT_EQ(chain_id, mojom::kZCashMainnet);
          auto response = zcash::mojom::LightdInfo::New("c2d6d0b4");
          std::move(callback).Run(std::move(response));
        });
  }

  AccountUtils GetAccountUtils() {
    return AccountUtils(keyring_service_.get());
  }

  mojom::AccountIdPtr account_id() const {
    return zcash_account_->account_id.Clone();
  }

  testing::NiceMock<MockZCashRPC>& zcash_rpc() {
    return static_cast<testing::NiceMock<MockZCashRPC>&>(
        zcash_wallet_service_->zcash_rpc());
  }

  base::FilePath& db_path() { return db_path_; }

#if BUILDFLAG(ENABLE_ORCHARD)
  std::map<mojom::AccountIdPtr, std::unique_ptr<ZCashAutoSyncManager>>&
  auto_sync_managers() {
    return zcash_wallet_service_->auto_sync_managers_;
  }

  base::SequenceBound<OrchardSyncState>& sync_state() {
    return zcash_wallet_service_->sync_state_;
  }

  MockOrchardSyncState& mock_orchard_sync_state() {
    return *mocked_sync_state_;
  }

  void OverrideSyncStateForTesting(
      base::SequenceBound<OrchardSyncState> sync_state) {
    zcash_wallet_service_->OverrideSyncStateForTesting(std::move(sync_state));
  }
#endif  // BUILDFLAG(ENABLE_ORCHARD)

  KeyringService* keyring_service() { return keyring_service_.get(); }

 protected:
  base::test::ScopedFeatureList feature_list_;
  base::ScopedTempDir temp_dir_;
  base::FilePath db_path_;

  mojom::AccountInfoPtr zcash_account_;

  sync_preferences::TestingPrefServiceSyncable prefs_;
  sync_preferences::TestingPrefServiceSyncable local_state_;

  std::unique_ptr<KeyringService> keyring_service_;
#if BUILDFLAG(ENABLE_ORCHARD)
  std::unique_ptr<MockOrchardSyncState> mocked_sync_state_;
#endif
  std::unique_ptr<ZCashWalletService> zcash_wallet_service_;

  base::test::TaskEnvironment task_environment_;
};

TEST_F(ZCashWalletServiceUnitTest, GetBalance) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndEnableFeatureWithParameters(
      features::kBraveWalletZCashFeature,
      {{"zcash_shielded_transactions_enabled", "false"}});
  auto account =
      GetAccountUtils().EnsureAccount(mojom::KeyringId::kZCashMainnet, 1);
  keyring_service()->UpdateNextUnusedAddressForZCashAccount(account->account_id,
                                                            1, 0);

  ON_CALL(zcash_rpc(), GetLatestBlock(_, _))
      .WillByDefault(
          [&](const std::string& chain_id,
              ZCashRpc::GetLatestBlockCallback callback) {
            EXPECT_EQ(chain_id, mojom::kZCashMainnet);
            auto response = zcash::mojom::BlockID::New(
                2625446u,
                *PrefixedHexStringToBytes("0x0000000001a01b5fd794e4b071443974c8"
                                          "35b3e0ff8f96bf3600e07afdbf89c5"));
            std::move(callback).Run(std::move(response));
          });

  ON_CALL(zcash_rpc(), IsKnownAddress(_, _, _, _, _))
      .WillByDefault([](const std::string& chain_id, const std::string& addr,
                        uint64_t block_start, uint64_t block_end,
                        ZCashRpc::IsKnownAddressCallback callback) {
        EXPECT_EQ(chain_id, mojom::kZCashMainnet);
        // Receiver addresses
        if (addr == "t1ShtibD2UJkYTeGPxeLrMf3jvE11S4Lpwj") {
          std::move(callback).Run(true);
          return;
        }
        if (addr == "t1aW1cW7wf6KMuKrjDinyv9tK6F6hrBkRAY") {
          std::move(callback).Run(true);
          return;
        }
        if (addr == "t1aW1cW7wf6KMuKrjDinyv9tK6F6hrBkRAY") {
          std::move(callback).Run(true);
          return;
        }
        if (addr == "t1MF6q7rTYJMMKLgzQ58mCuo76EVhLfSAkW") {
          std::move(callback).Run(true);
          return;
        }
        std::move(callback).Run(false);
      });

  ON_CALL(zcash_rpc(), GetUtxoList(_, _, _))
      .WillByDefault(
          [&](const std::string& chain_id, const std::string& address,
              ZCashRpc::GetUtxoListCallback callback) {
            std::vector<zcash::mojom::ZCashUtxoPtr> utxos;
            if (address == "t1ShtibD2UJkYTeGPxeLrMf3jvE11S4Lpwj") {
              auto utxo = zcash::mojom::ZCashUtxo::New(
                  "t1aFpD4qebqwbSAZLF4E8ZGmrTk36b1cocZ" /* address */,
                  *PrefixedHexStringToBytes(
                      "0x1b7a7109cec77ae38e57f4f0ec53a4046b08361abb92c62d9567ac"
                      "e684f633ab") /* tx id */,
                  0u /* index */,
                  *PrefixedHexStringToBytes("0x76a914b3b55981e7bf53e10fe51aa4f4"
                                            "5fdef06dec783d88ac") /*script*/,
                  10u /* amount */, 2468320u /* block */);
              utxos.push_back(std::move(utxo));
            } else if (address == "t1aW1cW7wf6KMuKrjDinyv9tK6F6hrBkRAY") {
              auto utxo = zcash::mojom::ZCashUtxo::New(
                  "t1aFpD4qebqwbSAZLF4E8ZGmrTk36b1cocZ" /* address */,
                  *PrefixedHexStringToBytes(
                      "0x1b7a7109cec77ae38e57f4f0ec53a4046b08361abb92c62d9567ac"
                      "e684f633ab") /* tx id */,
                  0u /* index */,
                  *PrefixedHexStringToBytes("0x76a914b3b55981e7bf53e10fe51aa4f4"
                                            "5fdef06dec783d88ac") /*script*/,
                  20u /* amount */, 2468320u /* block */);
              utxos.push_back(std::move(utxo));
            } else if (address == "t1MF6q7rTYJMMKLgzQ58mCuo76EVhLfSAkW") {
              auto utxo = zcash::mojom::ZCashUtxo::New(
                  "t1aFpD4qebqwbSAZLF4E8ZGmrTk36b1cocZ" /* address */,
                  *PrefixedHexStringToBytes(
                      "0x1b7a7109cec77ae38e57f4f0ec53a4046b08361abb92c62d9567ac"
                      "e684f633ab") /* tx id */,
                  0u /* index */,
                  *PrefixedHexStringToBytes("0x76a914b3b55981e7bf53e10fe51aa4f4"
                                            "5fdef06dec783d88ac") /*script*/,
                  20u /* amount */, 2468320u /* block */);
              utxos.push_back(std::move(utxo));
            }
            auto response =
                zcash::mojom::GetAddressUtxosResponse::New(std::move(utxos));
            std::move(callback).Run(std::move(response));
          });

  base::MockCallback<ZCashWalletService::GetBalanceCallback> balance_callback;
  EXPECT_CALL(balance_callback, Run(_, _))
      .WillOnce([&](mojom::ZCashBalancePtr balance,
                    std::optional<std::string> error) {
        EXPECT_EQ(balance->total_balance, 50u);
        EXPECT_EQ(balance->transparent_balance, 50u);
        EXPECT_EQ(balance->shielded_balance, 0u);
      });

  zcash_wallet_service_->GetBalance(account->account_id.Clone(),
                                    balance_callback.Get());
  task_environment_.RunUntilIdle();
}

#if BUILDFLAG(ENABLE_ORCHARD)
TEST_F(ZCashWalletServiceUnitTest, GetBalanceWithShielded) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndEnableFeatureWithParameters(
      features::kBraveWalletZCashFeature,
      {{"zcash_shielded_transactions_enabled", "true"}});
  keyring_service()->Reset();
  keyring_service()->RestoreWallet(kMnemonicDivideCruise, kTestWalletPassword,
                                   false, base::DoNothing());

  auto account =
      GetAccountUtils().EnsureAccount(mojom::KeyringId::kZCashMainnet, 1);
  keyring_service()->UpdateNextUnusedAddressForZCashAccount(account->account_id,
                                                            1, 0);

  ON_CALL(zcash_rpc(), GetLatestBlock(_, _))
      .WillByDefault(
          [&](const std::string& chain_id,
              ZCashRpc::GetLatestBlockCallback callback) {
            auto response = zcash::mojom::BlockID::New(
                2625446u,
                *PrefixedHexStringToBytes("0x0000000001a01b5fd794e4b071443974c8"
                                          "35b3e0ff8f96bf3600e07afdbf89c5"));
            std::move(callback).Run(std::move(response));
          });

  ON_CALL(zcash_rpc(), IsKnownAddress(_, _, _, _, _))
      .WillByDefault([](const std::string& chain_id, const std::string& addr,
                        uint64_t block_start, uint64_t block_end,
                        ZCashRpc::IsKnownAddressCallback callback) {
        // Receiver addresses
        if (addr == "t1ShtibD2UJkYTeGPxeLrMf3jvE11S4Lpwj") {
          std::move(callback).Run(true);
          return;
        }
        std::move(callback).Run(false);
      });

  ON_CALL(zcash_rpc(), GetUtxoList(_, _, _))
      .WillByDefault(
          [&](const std::string& chain_id, const std::string& address,
              ZCashRpc::GetUtxoListCallback callback) {
            std::vector<zcash::mojom::ZCashUtxoPtr> utxos;
            if (address == "t1ShtibD2UJkYTeGPxeLrMf3jvE11S4Lpwj") {
              auto utxo = zcash::mojom::ZCashUtxo::New(
                  "t1aFpD4qebqwbSAZLF4E8ZGmrTk36b1cocZ" /* address */,
                  *PrefixedHexStringToBytes(
                      "0x1b7a7109cec77ae38e57f4f0ec53a4046b08361abb92c62d9567ac"
                      "e684f633ab") /* tx id */,
                  0u /* index */,
                  *PrefixedHexStringToBytes("0x76a914b3b55981e7bf53e10fe51aa4f4"
                                            "5fdef06dec783d88ac") /*script*/,
                  10u /* amount */, 2468320u /* block */);
              utxos.push_back(std::move(utxo));
            }
            auto response =
                zcash::mojom::GetAddressUtxosResponse::New(std::move(utxos));
            std::move(callback).Run(std::move(response));
          });

  ON_CALL(mock_orchard_sync_state(), GetSpendableNotes(_, _))
      .WillByDefault([](const mojom::AccountIdPtr& account_id,
                        const OrchardAddrRawPart& internal_addr) {
        OrchardSyncState::SpendableNotesBundle spendable_notes_bundle;
        {
          OrchardNote note;
          note.amount = 10u;
          spendable_notes_bundle.all_notes.push_back(note);
          spendable_notes_bundle.spendable_notes.push_back(note);
        }
        {
          OrchardNote note;
          note.amount = 20u;
          spendable_notes_bundle.all_notes.push_back(note);
        }
        return spendable_notes_bundle;
      });

  base::SequenceBound<MockOrchardSyncStateProxy> overrided_sync_state(
      base::SequencedTaskRunner::GetCurrentDefault(), db_path(),
      &mock_orchard_sync_state());
  OverrideSyncStateForTesting(std::move(overrided_sync_state));

  base::MockCallback<ZCashWalletService::GetBalanceCallback> balance_callback;
  EXPECT_CALL(balance_callback, Run(_, _))
      .WillOnce([&](mojom::ZCashBalancePtr balance,
                    std::optional<std::string> error) {
        EXPECT_EQ(balance->total_balance, 20u);
        EXPECT_EQ(balance->transparent_balance, 10u);
        EXPECT_EQ(balance->shielded_balance, 10u);
        EXPECT_EQ(balance->shielded_pending_balance, 20u);
      });
  zcash_wallet_service_->GetBalance(account->account_id.Clone(),
                                    balance_callback.Get());
  task_environment_.RunUntilIdle();
  {
    // Cleanup old MockOrchardSyncStateProxy to prevent dangling pointer error
    base::SequenceBound<MockOrchardSyncStateProxy> empty_sync_state(
        base::ThreadPool::CreateSequencedTaskRunner({base::MayBlock()}),
        db_path(), nullptr);
    OverrideSyncStateForTesting(std::move(empty_sync_state));
    task_environment_.RunUntilIdle();
  }
}

TEST_F(ZCashWalletServiceUnitTest, GetBalanceWithShielded_FeatureDisabled) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndEnableFeatureWithParameters(
      features::kBraveWalletZCashFeature,
      {{"zcash_shielded_transactions_enabled", "false"}});
  keyring_service()->Reset();
  keyring_service()->RestoreWallet(kMnemonicDivideCruise, kTestWalletPassword,
                                   false, base::DoNothing());
  auto account =
      GetAccountUtils().EnsureAccount(mojom::KeyringId::kZCashMainnet, 1);
  keyring_service()->UpdateNextUnusedAddressForZCashAccount(account->account_id,
                                                            1, 0);

  ON_CALL(zcash_rpc(), GetLatestBlock(_, _))
      .WillByDefault(
          [&](const std::string& chain_id,
              ZCashRpc::GetLatestBlockCallback callback) {
            auto response = zcash::mojom::BlockID::New(
                2625446u,
                *PrefixedHexStringToBytes("0x0000000001a01b5fd794e4b071443974c8"
                                          "35b3e0ff8f96bf3600e07afdbf89c5"));
            std::move(callback).Run(std::move(response));
          });

  ON_CALL(zcash_rpc(), IsKnownAddress(_, _, _, _, _))
      .WillByDefault([](const std::string& chain_id, const std::string& addr,
                        uint64_t block_start, uint64_t block_end,
                        ZCashRpc::IsKnownAddressCallback callback) {
        // Receiver addresses
        if (addr == "t1ShtibD2UJkYTeGPxeLrMf3jvE11S4Lpwj") {
          std::move(callback).Run(true);
          return;
        }
        std::move(callback).Run(false);
      });

  ON_CALL(zcash_rpc(), GetUtxoList(_, _, _))
      .WillByDefault(
          [&](const std::string& chain_id, const std::string& address,
              ZCashRpc::GetUtxoListCallback callback) {
            std::vector<zcash::mojom::ZCashUtxoPtr> utxos;
            if (address == "t1ShtibD2UJkYTeGPxeLrMf3jvE11S4Lpwj") {
              auto utxo = zcash::mojom::ZCashUtxo::New(
                  "t1aFpD4qebqwbSAZLF4E8ZGmrTk36b1cocZ" /* address */,
                  *PrefixedHexStringToBytes(
                      "0x1b7a7109cec77ae38e57f4f0ec53a4046b08361abb92c62d9567ac"
                      "e684f633ab") /* tx id */,
                  0u /* index */,
                  *PrefixedHexStringToBytes("0x76a914b3b55981e7bf53e10fe51aa4f4"
                                            "5fdef06dec783d88ac") /*script*/,
                  10u /* amount */, 2468320u /* block */);
              utxos.push_back(std::move(utxo));
            }
            auto response =
                zcash::mojom::GetAddressUtxosResponse::New(std::move(utxos));
            std::move(callback).Run(std::move(response));
          });

  OrchardNote note;
  note.amount = 10u;

  auto update_notes_callback = base::BindLambdaForTesting(
      [](base::expected<OrchardStorage::Result, OrchardStorage::Error>) {});

  OrchardBlockScanner::Result result = CreateResultForTesting(
      OrchardTreeState(), std::vector<OrchardCommitment>(), 50000, "hash50000");
  result.discovered_notes = std::vector<OrchardNote>({note});
  result.found_spends = std::vector<OrchardNoteSpend>();

  sync_state()
      .AsyncCall(&OrchardSyncState::ApplyScanResults)
      .WithArgs(account->account_id.Clone(), std::move(result))
      .Then(std::move(update_notes_callback));

  task_environment_.RunUntilIdle();

  base::MockCallback<ZCashWalletService::GetBalanceCallback> balance_callback;
  EXPECT_CALL(balance_callback, Run(_, _))
      .WillOnce([&](mojom::ZCashBalancePtr balance,
                    std::optional<std::string> error) {
        EXPECT_EQ(balance->total_balance, 10u);
        EXPECT_EQ(balance->transparent_balance, 10u);
        EXPECT_EQ(balance->shielded_balance, 0u);
      });
  zcash_wallet_service_->GetBalance(account->account_id.Clone(),
                                    balance_callback.Get());
  task_environment_.RunUntilIdle();
}
#endif  // BUILDFLAG(ENABLE_ORCHARD)

// https://zcashblockexplorer.com/transactions/3bc513afc84befb9774f667eb4e63266a7229ab1fdb43476dd7c3a33d16b3101/raw
TEST_F(ZCashWalletServiceUnitTest, SignAndPostTransaction) {
  {
    auto account =
        GetAccountUtils().EnsureAccount(mojom::KeyringId::kZCashMainnet, 0);
    auto account_id = account->account_id.Clone();
    keyring_service_->UpdateNextUnusedAddressForZCashAccount(account_id, 2, 2);
  }

  ZCashTransaction zcash_transaction;
  zcash_transaction.set_locktime(2286687);
  {
    ZCashTransaction::TxInput input;
    input.utxo_outpoint.txid = GetTxId(
        "70f1aa91889eee3e5ba60231a2e625e60480dc2e43ddc9439dc4fe8f09a1a278");
    input.utxo_outpoint.index = 0;

    input.utxo_address = "t1c61yifRMgyhMsBYsFDBa5aEQkgU65CGau";
    input.utxo_value = 537000;
    input.script_pub_key =
        ZCashAddressToScriptPubkey(input.utxo_address, false).value();

    zcash_transaction.transparent_part().inputs.push_back(std::move(input));
  }

  {
    ZCashTransaction::TxOutput output;
    output.address = "t1KrG29yWzoi7Bs2pvsgXozZYPvGG4D3sGi";
    output.amount = 500000;
    output.script_pubkey =
        ZCashAddressToScriptPubkey(output.address, false).value();

    zcash_transaction.transparent_part().outputs.push_back(std::move(output));
  }

  {
    ZCashTransaction::TxOutput output;
    output.address = "t1c61yifRMgyhMsBYsFDBa5aEQkgU65CGau";
    output.script_pubkey =
        ZCashAddressToScriptPubkey(output.address, false).value();
    output.amount = 35000;

    zcash_transaction.transparent_part().outputs.push_back(std::move(output));
  }

  zcash_transaction.set_fee(2000u);

  ON_CALL(zcash_rpc(), GetLatestBlock(_, _))
      .WillByDefault([](const std::string& chain_id,
                        ZCashRpc::GetLatestBlockCallback callback) {
        EXPECT_EQ(chain_id, mojom::kZCashMainnet);
        zcash::mojom::BlockIDPtr response = zcash::mojom::BlockID::New();
        response->height = 2286687;
        std::move(callback).Run(std::move(response));
      });

  base::MockCallback<ZCashWalletService::SignAndPostTransactionCallback>
      sign_callback;

  ZCashTransaction signed_tx;
  EXPECT_CALL(
      sign_callback,
      Run("3bc513afc84befb9774f667eb4e63266a7229ab1fdb43476dd7c3a33d16b3101", _,
          ""))
      .WillOnce(
          WithArg<1>([&](const ZCashTransaction& tx) { signed_tx = tx; }));

  std::vector<uint8_t> captured_data;
  EXPECT_CALL(zcash_rpc(), SendTransaction(_, _, _))
      .WillOnce([&](const std::string& chain_id, base::span<const uint8_t> data,
                    ZCashRpc::SendTransactionCallback callback) {
        captured_data = std::vector<uint8_t>(data.begin(), data.end());
        zcash::mojom::SendResponsePtr response =
            zcash::mojom::SendResponse::New();
        response->error_code = 0;
        std::move(callback).Run(std::move(response));
      });

  zcash_wallet_service_->SignAndPostTransaction(
      account_id(), std::move(zcash_transaction), sign_callback.Get());
  task_environment_.RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&sign_callback);

  EXPECT_EQ(ToHex(signed_tx.transparent_part().inputs[0].script_sig),
            "0x47304402202fc68ead746e8e93bb661ac79e71e1d3d84fd0f2aac76a8cb"
            "4fa831a847787ff022028efe32152f282d7167c40d62b07aedad73a66c7"
            "a3548413f289e2aef3da96b30121028754aaa5d9198198ecf5fd1849cbf"
            "38a92ed707e2f181bd354c73a4a87854c67");

  EXPECT_EQ(ToHex(captured_data),
            "0x050000800a27a726b4d0d6c25fe4220073e422000178a2a1098ffec49d43"
            "c9dd432edc8004e625e6a23102a65b3eee9e8891aaf170000000006a473044"
            "02202fc68ead746e8e93bb661ac79e71e1d3d84fd0f2aac76a8cb4fa831a84"
            "7787ff022028efe32152f282d7167c40d62b07aedad73a66c7a3548413f289"
            "e2aef3da96b30121028754aaa5d9198198ecf5fd1849cbf38a92ed707e2f18"
            "1bd354c73a4a87854c67ffffffff0220a10700000000001976a91415af26f9"
            "b71022a01eade958cd05145f7ba5afe688acb8880000000000001976a914c7"
            "cb443e547988b992adc1b47427ce6c40f3ca9e88ac000000");
}

TEST_F(ZCashWalletServiceUnitTest, AddressDiscovery) {
  ON_CALL(zcash_rpc(), GetLatestBlock(_, _))
      .WillByDefault([](const std::string& chain_id,
                        ZCashRpc::GetLatestBlockCallback callback) {
        zcash::mojom::BlockIDPtr response = zcash::mojom::BlockID::New();
        response->height = 2286687;
        std::move(callback).Run(std::move(response));
      });

  ON_CALL(zcash_rpc(), IsKnownAddress(_, _, _, _, _))
      .WillByDefault([](const std::string& chain_id, const std::string& addr,
                        uint64_t block_start, uint64_t block_end,
                        ZCashRpc::IsKnownAddressCallback callback) {
        EXPECT_EQ(2286687u, block_end);
        // Receiver addresses
        if (addr == "t1c61yifRMgyhMsBYsFDBa5aEQkgU65CGau") {
          std::move(callback).Run(true);
          return;
        }
        if (addr == "t1V7JBWXRYPA19nBLBFTm8669DhQgErMAnK") {
          std::move(callback).Run(true);
          return;
        }
        if (addr == "t1UCYMSUdkGXEyeKPqgwiDn8NwGv5JKmJoL") {
          std::move(callback).Run(false);
          return;
        }
        // Change addresses
        if (addr == "t1RDtGXzcfchmtrE8pGLorefgtspgcNZbrE") {
          std::move(callback).Run(true);
          return;
        }
        if (addr == "t1VUdyCuqWgeBPJvfhWvHLD5iDUfkdLrwWz") {
          std::move(callback).Run(true);
          return;
        }
        if (addr == "t1QJuws2nGqDNJEKsKniUPDNLbMw5R9ixGj") {
          std::move(callback).Run(false);
          return;
        }
      });

  {
    bool callback_called = false;
    auto discovery_callback = base::BindLambdaForTesting(
        [&](ZCashWalletService::RunDiscoveryResult result) {
          EXPECT_EQ((*result)[0]->address_string,
                    "t1UCYMSUdkGXEyeKPqgwiDn8NwGv5JKmJoL");
          EXPECT_EQ((*result)[1]->address_string,
                    "t1QJuws2nGqDNJEKsKniUPDNLbMw5R9ixGj");
          callback_called = true;
        });

    auto account =
        GetAccountUtils().EnsureAccount(mojom::KeyringId::kZCashMainnet, 0);
    auto account_id = account->account_id.Clone();

    zcash_wallet_service_->RunDiscovery(std::move(account_id),
                                        std::move(discovery_callback));
    task_environment_.RunUntilIdle();

    EXPECT_TRUE(callback_called);
  }

  ON_CALL(zcash_rpc(), IsKnownAddress(_, _, _, _, _))
      .WillByDefault([](const std::string& chain_id, const std::string& addr,
                        uint64_t block_start, uint64_t block_end,
                        ZCashRpc::IsKnownAddressCallback callback) {
        EXPECT_EQ(2286687u, block_end);
        // Receiver addresses
        if (addr == "t1UCYMSUdkGXEyeKPqgwiDn8NwGv5JKmJoL") {
          std::move(callback).Run(true);
          return;
        }
        if (addr == "t1JEfEPQDGruzd7Q42pdwHmR4sRHGLRF48m") {
          std::move(callback).Run(false);
          return;
        }
        // Change addresses
        if (addr == "t1QJuws2nGqDNJEKsKniUPDNLbMw5R9ixGj") {
          std::move(callback).Run(true);
          return;
        }
        if (addr == "t1gKxueg76TtvVmMQ6swDmvHxtmLTSQv6KP") {
          std::move(callback).Run(false);
          return;
        }
      });

  {
    bool callback_called = false;
    auto discovery_callback = base::BindLambdaForTesting(
        [&](ZCashWalletService::RunDiscoveryResult result) {
          EXPECT_EQ((*result)[0]->address_string,
                    "t1JEfEPQDGruzd7Q42pdwHmR4sRHGLRF48m");
          EXPECT_EQ((*result)[1]->address_string,
                    "t1gKxueg76TtvVmMQ6swDmvHxtmLTSQv6KP");
          callback_called = true;
        });

    auto account =
        GetAccountUtils().EnsureAccount(mojom::KeyringId::kZCashMainnet, 0);
    auto account_id = account->account_id.Clone();

    zcash_wallet_service_->RunDiscovery(std::move(account_id),
                                        std::move(discovery_callback));
    task_environment_.RunUntilIdle();

    EXPECT_TRUE(callback_called);
  }
}

TEST_F(ZCashWalletServiceUnitTest, AddressDiscovery_FromPrefs) {
  ON_CALL(zcash_rpc(), GetLatestBlock(_, _))
      .WillByDefault([](const std::string& chain_id,
                        ZCashRpc::GetLatestBlockCallback callback) {
        zcash::mojom::BlockIDPtr response = zcash::mojom::BlockID::New();
        response->height = 2286687;
        std::move(callback).Run(std::move(response));
      });

  ON_CALL(zcash_rpc(), IsKnownAddress(_, _, _, _, _))
      .WillByDefault([](const std::string& chain_id, const std::string& addr,
                        uint64_t block_start, uint64_t block_end,
                        ZCashRpc::IsKnownAddressCallback callback) {
        EXPECT_EQ(2286687u, block_end);
        // Receiver addresses
        if (addr == "t1UCYMSUdkGXEyeKPqgwiDn8NwGv5JKmJoL") {
          std::move(callback).Run(true);
          return;
        }
        if (addr == "t1JEfEPQDGruzd7Q42pdwHmR4sRHGLRF48m") {
          std::move(callback).Run(false);
          return;
        }
        // Change addresses
        if (addr == "t1RDtGXzcfchmtrE8pGLorefgtspgcNZbrE") {
          std::move(callback).Run(false);
          return;
        }
      });

  {
    auto account =
        GetAccountUtils().EnsureAccount(mojom::KeyringId::kZCashMainnet, 0);
    auto account_id = account->account_id.Clone();
    keyring_service_->UpdateNextUnusedAddressForZCashAccount(account_id, 2,
                                                             std::nullopt);
  }

  {
    bool callback_called = false;
    auto discovery_callback = base::BindLambdaForTesting(
        [&](ZCashWalletService::RunDiscoveryResult result) {
          EXPECT_EQ((*result)[0]->address_string,
                    "t1JEfEPQDGruzd7Q42pdwHmR4sRHGLRF48m");
          EXPECT_EQ((*result)[1]->address_string,
                    "t1RDtGXzcfchmtrE8pGLorefgtspgcNZbrE");
          callback_called = true;
          callback_called = true;
        });

    auto account =
        GetAccountUtils().EnsureAccount(mojom::KeyringId::kZCashMainnet, 0);
    auto account_id = account->account_id.Clone();

    zcash_wallet_service_->RunDiscovery(std::move(account_id),
                                        std::move(discovery_callback));
    task_environment_.RunUntilIdle();

    EXPECT_TRUE(callback_called);
  }
}

TEST_F(ZCashWalletServiceUnitTest, GetTransactionType_Mainnet) {
  auto account_1 =
      GetAccountUtils().EnsureAccount(mojom::KeyringId::kZCashMainnet, 0);
  auto account_id_1 = account_1->account_id.Clone();

  auto btc_account_1 =
      GetAccountUtils().EnsureAccount(mojom::KeyringId::kBitcoin84, 0);
  auto btc_account_id_1 = btc_account_1->account_id.Clone();

  // https://github.com/Electric-Coin-Company/zcash-android-wallet-sdk/blob/v2.0.6/sdk-incubator-lib/src/main/java/cash/z/ecc/android/sdk/fixture/WalletFixture.kt

  // Not a ZCash account.
  {
    base::MockCallback<ZCashWalletService::GetTransactionTypeCallback> callback;
    EXPECT_CALL(callback, Run(Eq(mojom::ZCashTxType::kUnknown),
                              Eq(mojom::ZCashAddressError::kNotZCashAccount)));
    zcash_wallet_service_->GetTransactionType(
        btc_account_id_1.Clone(), false, "t1JP7PHu72xHztsZiwH6cye4yvC9Prb3EvQ",
        callback.Get());
  }

  // Normal transparent address - mainnet.
  {
    base::MockCallback<ZCashWalletService::GetTransactionTypeCallback> callback;
    EXPECT_CALL(callback, Run(Eq(mojom::ZCashTxType::kTransparentToTransparent),
                              Eq(mojom::ZCashAddressError::kNoError)));
    zcash_wallet_service_->GetTransactionType(
        account_id_1.Clone(), false, "t1JP7PHu72xHztsZiwH6cye4yvC9Prb3EvQ",
        callback.Get());
  }

  // Testnet address with mainnet account (network mismatch).
  {
    base::MockCallback<ZCashWalletService::GetTransactionTypeCallback> callback;
    EXPECT_CALL(
        callback,
        Run(Eq(mojom::ZCashTxType::kUnknown),
            Eq(mojom::ZCashAddressError::kInvalidAddressNetworkMismatch)));
    zcash_wallet_service_->GetTransactionType(
        account_id_1.Clone(), false, "tmP3uLtGx5GPddkq8a6ddmXhqJJ3vy6tpTE",
        callback.Get());
  }

  // Wrong transparent address.
  {
    base::MockCallback<ZCashWalletService::GetTransactionTypeCallback> callback;
    EXPECT_CALL(callback,
                Run(Eq(mojom::ZCashTxType::kUnknown),
                    Eq(mojom::ZCashAddressError::kInvalidTransparentAddress)));
    zcash_wallet_service_->GetTransactionType(account_id_1.Clone(), false,
                                              "t1xxx", callback.Get());
  }

  // Malformed transparent address.
  {
    base::MockCallback<ZCashWalletService::GetTransactionTypeCallback> callback;
    EXPECT_CALL(callback,
                Run(Eq(mojom::ZCashTxType::kUnknown),
                    Eq(mojom::ZCashAddressError::kInvalidTransparentAddress)));
    zcash_wallet_service_->GetTransactionType(
        account_id_1.Clone(), false, "t1JP7PHu72xHztsZiwH6cye4yvC9Prb3Ev0",
        callback.Get());
  }

  // Eth address.
  {
    base::MockCallback<ZCashWalletService::GetTransactionTypeCallback> callback;
    EXPECT_CALL(callback,
                Run(Eq(mojom::ZCashTxType::kUnknown),
                    Eq(mojom::ZCashAddressError::kInvalidTransparentAddress)));
    zcash_wallet_service_->GetTransactionType(
        account_id_1.Clone(), false,
        "0xA4bE3C94e8c1B7D2F9e6Bf3E1D9A2cC45B6F9A12", callback.Get());
  }

#if BUILDFLAG(ENABLE_ORCHARD)
  // Eth address, shielded pool.
  {
    base::MockCallback<ZCashWalletService::GetTransactionTypeCallback> callback;
    EXPECT_CALL(callback,
                Run(Eq(mojom::ZCashTxType::kUnknown),
                    Eq(mojom::ZCashAddressError::kInvalidUnifiedAddress)));
    zcash_wallet_service_->GetTransactionType(
        account_id_1.Clone(), true,
        "0xA4bE3C94e8c1B7D2F9e6Bf3E1D9A2cC45B6F9A12", callback.Get());
  }
#else
  // Eth address, shielded pool.
  {
    base::MockCallback<ZCashWalletService::GetTransactionTypeCallback> callback;
    EXPECT_CALL(callback,
                Run(Eq(mojom::ZCashTxType::kUnknown),
                    Eq(mojom::ZCashAddressError::kInvalidTransparentAddress)));
    zcash_wallet_service_->GetTransactionType(
        account_id_1.Clone(), true,
        "0xA4bE3C94e8c1B7D2F9e6Bf3E1D9A2cC45B6F9A12", callback.Get());
  }
#endif

  // Unified address - mainnet.
  {
    base::MockCallback<ZCashWalletService::GetTransactionTypeCallback> callback;
    EXPECT_CALL(callback, Run(Eq(mojom::ZCashTxType::kTransparentToTransparent),
                              Eq(mojom::ZCashAddressError::kNoError)));
    zcash_wallet_service_->GetTransactionType(
        account_id_1.Clone(), false,
        // Address contains transparent part
        "u1lmy8anuylj33arxh3sx7ysq54tuw7zehsv6pdeeaqlrhkjhm3uvl9egqxqfd7hcsp3ms"
        "zp6jxxx0gsw0ldp5wyu95r4mfzlueh8h5xhrjqgz7xtxp3hvw45dn4gfrz5j54ryg6reyf"
        "0",
        callback.Get());
  }

  // Malformed unified address - mainnet.
  {
    base::MockCallback<ZCashWalletService::GetTransactionTypeCallback> callback;
    EXPECT_CALL(callback,
                Run(Eq(mojom::ZCashTxType::kUnknown),
                    Eq(mojom::ZCashAddressError::
                           kInvalidUnifiedAddressMissingTransparentPart)));
    zcash_wallet_service_->GetTransactionType(
        account_id_1.Clone(), false,
        "u1lmy8anuylj33arxh3sx7ysq54tuw7zehsv6pdeeaqlrhkjhm3uvl9egqxqfd7hcsp3ms"
        "zp6jxxx0gsw0ldp5wyu95r4mfzlueh8h5xhrjqgz7xtxp3hvw45dn4gfrz5j54ryg6reyf"
        "1",
        callback.Get());
  }

#if BUILDFLAG(ENABLE_ORCHARD)
  // Malformed unified address - mainnet, shielded pool.
  {
    base::MockCallback<ZCashWalletService::GetTransactionTypeCallback> callback;
    EXPECT_CALL(callback,
                Run(Eq(mojom::ZCashTxType::kUnknown),
                    Eq(mojom::ZCashAddressError::
                           kInvalidUnifiedAddressMissingOrchardPart)));
    zcash_wallet_service_->GetTransactionType(
        account_id_1.Clone(), true,
        "u1lmy8anuylj33arxh3sx7ysq54tuw7zehsv6pdeeaqlrhkjhm3uvl9egqxqfd7hcsp3ms"
        "zp6jxxx0gsw0ldp5wyu95r4mfzlueh8h5xhrjqgz7xtxp3hvw45dn4gfrz5j54ryg6reyf"
        "1",
        callback.Get());
  }
#else
  // Malformed unified address - mainnet, shielded pool.
  {
    base::MockCallback<ZCashWalletService::GetTransactionTypeCallback> callback;
    EXPECT_CALL(callback,
                Run(Eq(mojom::ZCashTxType::kUnknown),
                    Eq(mojom::ZCashAddressError::
                           kInvalidUnifiedAddressMissingTransparentPart)));
    zcash_wallet_service_->GetTransactionType(
        account_id_1.Clone(), true,
        "u1lmy8anuylj33arxh3sx7ysq54tuw7zehsv6pdeeaqlrhkjhm3uvl9egqxqfd7hcsp3ms"
        "zp6jxxx0gsw0ldp5wyu95r4mfzlueh8h5xhrjqgz7xtxp3hvw45dn4gfrz5j54ryg6reyf"
        "1",
        callback.Get());
  }
#endif

#if BUILDFLAG(ENABLE_ORCHARD)
  // Sapling unified address - mainnet.
  {
    base::MockCallback<ZCashWalletService::GetTransactionTypeCallback> callback;
    EXPECT_CALL(callback,
                Run(Eq(mojom::ZCashTxType::kUnknown),
                    Eq(mojom::ZCashAddressError::
                           kInvalidUnifiedAddressMissingOrchardPart)));
    zcash_wallet_service_->GetTransactionType(
        account_id_1.Clone(), true,
        "u187vrwl4ampyxd5m6aj38n4ndkmj8v6gs97hkt23aps3sn5k89a0gk2smluexgdprcrtm"
        "5"
        "6ezc5c7tjwlrnnl79tjtrxmqd42c5mpyz7g",
        callback.Get());
  }
#else
  // Sapling unified address - mainnet.
  {
    base::MockCallback<ZCashWalletService::GetTransactionTypeCallback> callback;
    EXPECT_CALL(callback,
                Run(Eq(mojom::ZCashTxType::kUnknown),
                    Eq(mojom::ZCashAddressError::
                           kInvalidUnifiedAddressMissingTransparentPart)));
    zcash_wallet_service_->GetTransactionType(
        account_id_1.Clone(), true,
        "u187vrwl4ampyxd5m6aj38n4ndkmj8v6gs97hkt23aps3sn5k89a0gk2smluexgdprcrtm"
        "5"
        "6ezc5c7tjwlrnnl79tjtrxmqd42c5mpyz7g",
        callback.Get());
  }
#endif

#if BUILDFLAG(ENABLE_ORCHARD)
  // Sapling unified address - mainnet, shielded pool.
  {
    base::MockCallback<ZCashWalletService::GetTransactionTypeCallback> callback;
    EXPECT_CALL(callback,
                Run(Eq(mojom::ZCashTxType::kUnknown),
                    Eq(mojom::ZCashAddressError::
                           kInvalidUnifiedAddressMissingOrchardPart)));
    zcash_wallet_service_->GetTransactionType(
        account_id_1.Clone(), true,
        "u187vrwl4ampyxd5m6aj38n4ndkmj8v6gs97hkt23aps3sn5k89a0gk2smluexgdprcrtm"
        "5"
        "6ezc5c7tjwlrnnl79tjtrxmqd42c5mpyz7g",
        callback.Get());
  }
#else
  // Sapling unified address - mainnet, shielded pool.
  {
    base::MockCallback<ZCashWalletService::GetTransactionTypeCallback> callback;
    EXPECT_CALL(callback,
                Run(Eq(mojom::ZCashTxType::kUnknown),
                    Eq(mojom::ZCashAddressError::
                           kInvalidUnifiedAddressMissingTransparentPart)));
    zcash_wallet_service_->GetTransactionType(
        account_id_1.Clone(), true,
        "u187vrwl4ampyxd5m6aj38n4ndkmj8v6gs97hkt23aps3sn5k89a0gk2smluexgdprcrtm"
        "5"
        "6ezc5c7tjwlrnnl79tjtrxmqd42c5mpyz7g",
        callback.Get());
  }
#endif

  // Testnet unified address with mainnet account (network mismatch).
  {
    base::MockCallback<ZCashWalletService::GetTransactionTypeCallback> callback;
    EXPECT_CALL(
        callback,
        Run(Eq(mojom::ZCashTxType::kUnknown),
            Eq(mojom::ZCashAddressError::kInvalidAddressNetworkMismatch)));
    zcash_wallet_service_->GetTransactionType(
        account_id_1.Clone(), false,
        "utest1vergg5jkp4xy8sqfasw6s5zkdpnxvfxlxh35uuc3me7dp596y2r05t6dv9htwe3p"
        "f8ksrfr8ksca2lskzjanqtl8uqp5vln3zyy246ejtx86vqftp73j7jg9099jxafyjhfm6u"
        "956j3",
        callback.Get());
  }

  // Wrong unified address.
  {
    base::MockCallback<ZCashWalletService::GetTransactionTypeCallback> callback;
    EXPECT_CALL(callback,
                Run(Eq(mojom::ZCashTxType::kUnknown),
                    Eq(mojom::ZCashAddressError::
                           kInvalidUnifiedAddressMissingTransparentPart)));
    zcash_wallet_service_->GetTransactionType(account_id_1.Clone(), false,
                                              "u1xx", callback.Get());
  }

  // Shielded addresses disabled
  // Unified with transparent part.
  {
    base::test::ScopedFeatureList feature_list;
    feature_list.InitAndEnableFeatureWithParameters(
        features::kBraveWalletZCashFeature,
        {{"zcash_shielded_transactions_enabled", "false"}});

    base::MockCallback<ZCashWalletService::GetTransactionTypeCallback> callback;
    EXPECT_CALL(callback, Run(Eq(mojom::ZCashTxType::kTransparentToTransparent),
                              Eq(mojom::ZCashAddressError::kNoError)));
    zcash_wallet_service_->GetTransactionType(
        account_id_1.Clone(), false,
        "u19hwdcqxhkapje2p0744gq96parewuffyeg0kg3q3taq040zwqh2wxjwyxzs6l9dulzua"
        "p43ya7mq7q3mu2hjafzlwylvystjlc6n294emxww9xm8qn6tcldqkq4k9ccsqzmjeqk9yp"
        "kss572ut324nmxke666jm8lhkpt85gzq58d50rfnd7wufke8jjhc3lhswxrdr57ah42xck"
        "h2j",
        callback.Get());
  }

  // Empty address.
  {
    base::MockCallback<ZCashWalletService::GetTransactionTypeCallback> callback;
    EXPECT_CALL(callback,
                Run(Eq(mojom::ZCashTxType::kUnknown),
                    Eq(mojom::ZCashAddressError::kInvalidTransparentAddress)));
    zcash_wallet_service_->GetTransactionType(account_id_1.Clone(), false, "",
                                              callback.Get());
  }

#if BUILDFLAG(ENABLE_ORCHARD)
  // Empty address, shielded pool.
  {
    base::MockCallback<ZCashWalletService::GetTransactionTypeCallback> callback;
    EXPECT_CALL(callback,
                Run(Eq(mojom::ZCashTxType::kUnknown),
                    Eq(mojom::ZCashAddressError::kInvalidUnifiedAddress)));
    zcash_wallet_service_->GetTransactionType(account_id_1.Clone(), true, "",
                                              callback.Get());
  }
#else
  // Empty address, shielded pool.
  {
    base::MockCallback<ZCashWalletService::GetTransactionTypeCallback> callback;
    EXPECT_CALL(callback,
                Run(Eq(mojom::ZCashTxType::kUnknown),
                    Eq(mojom::ZCashAddressError::kInvalidTransparentAddress)));
    zcash_wallet_service_->GetTransactionType(account_id_1.Clone(), true, "",
                                              callback.Get());
  }
#endif
}

TEST_F(ZCashWalletServiceUnitTest, GetTransactionType_Testnet) {
  auto account_1 =
      GetAccountUtils().EnsureAccount(mojom::KeyringId::kZCashTestnet, 0);
  auto account_id_1 = account_1->account_id.Clone();

  // Normal transparent address - testnet.
  {
    base::MockCallback<ZCashWalletService::GetTransactionTypeCallback> callback;
    EXPECT_CALL(callback, Run(Eq(mojom::ZCashTxType::kTransparentToTransparent),
                              Eq(mojom::ZCashAddressError::kNoError)));
    zcash_wallet_service_->GetTransactionType(
        account_id_1.Clone(), false, "tmP3uLtGx5GPddkq8a6ddmXhqJJ3vy6tpTE",
        callback.Get());
  }

  // Malformed transparent address - testnet.
  {
    base::MockCallback<ZCashWalletService::GetTransactionTypeCallback> callback;
    EXPECT_CALL(callback,
                Run(Eq(mojom::ZCashTxType::kUnknown),
                    Eq(mojom::ZCashAddressError::kInvalidTransparentAddress)));
    zcash_wallet_service_->GetTransactionType(
        account_id_1.Clone(), false, "tmP3uLtGx5GPddkq8a6ddmXhqJJ3vy6tpT0",
        callback.Get());
  }

  // Mainnet address with testnet account (network mismatch).
  {
    base::MockCallback<ZCashWalletService::GetTransactionTypeCallback> callback;
    EXPECT_CALL(
        callback,
        Run(Eq(mojom::ZCashTxType::kUnknown),
            Eq(mojom::ZCashAddressError::kInvalidAddressNetworkMismatch)));
    zcash_wallet_service_->GetTransactionType(
        account_id_1.Clone(), false, "t1JP7PHu72xHztsZiwH6cye4yvC9Prb3EvQ",
        callback.Get());
  }

  // Wrong transparent address.
  {
    base::MockCallback<ZCashWalletService::GetTransactionTypeCallback> callback;
    EXPECT_CALL(callback,
                Run(Eq(mojom::ZCashTxType::kUnknown),
                    Eq(mojom::ZCashAddressError::kInvalidTransparentAddress)));
    zcash_wallet_service_->GetTransactionType(account_id_1.Clone(), false,
                                              "tmxxx", callback.Get());
  }

  // Eth address.
  {
    base::MockCallback<ZCashWalletService::GetTransactionTypeCallback> callback;
    EXPECT_CALL(callback,
                Run(Eq(mojom::ZCashTxType::kUnknown),
                    Eq(mojom::ZCashAddressError::kInvalidTransparentAddress)));
    zcash_wallet_service_->GetTransactionType(
        account_id_1.Clone(), false,
        "0xA4bE3C94e8c1B7D2F9e6Bf3E1D9A2cC45B6F9A12", callback.Get());
  }

#if BUILDFLAG(ENABLE_ORCHARD)
  // Eth address, shielded pool.
  {
    base::MockCallback<ZCashWalletService::GetTransactionTypeCallback> callback;
    EXPECT_CALL(callback,
                Run(Eq(mojom::ZCashTxType::kUnknown),
                    Eq(mojom::ZCashAddressError::kInvalidUnifiedAddress)));
    zcash_wallet_service_->GetTransactionType(
        account_id_1.Clone(), true,
        "0xA4bE3C94e8c1B7D2F9e6Bf3E1D9A2cC45B6F9A12", callback.Get());
  }
#else
  // Eth address, shielded pool.
  {
    base::MockCallback<ZCashWalletService::GetTransactionTypeCallback> callback;
    EXPECT_CALL(callback,
                Run(Eq(mojom::ZCashTxType::kUnknown),
                    Eq(mojom::ZCashAddressError::kInvalidTransparentAddress)));
    zcash_wallet_service_->GetTransactionType(
        account_id_1.Clone(), true,
        "0xA4bE3C94e8c1B7D2F9e6Bf3E1D9A2cC45B6F9A12", callback.Get());
  }
#endif

  // Unified address - testnet.
  {
    base::MockCallback<ZCashWalletService::GetTransactionTypeCallback> callback;
    EXPECT_CALL(callback, Run(Eq(mojom::ZCashTxType::kTransparentToTransparent),
                              Eq(mojom::ZCashAddressError::kNoError)));
    zcash_wallet_service_->GetTransactionType(
        account_id_1.Clone(), false,
        "utest1vergg5jkp4xy8sqfasw6s5zkdpnxvfxlxh35uuc3me7dp596y2r05t6dv9htwe3p"
        "f8ksrfr8ksca2lskzjanqtl8uqp5vln3zyy246ejtx86vqftp73j7jg9099jxafyjhfm6u"
        "956j3",
        callback.Get());
  }

  // Malformed Unified address - testnet.
  {
    base::MockCallback<ZCashWalletService::GetTransactionTypeCallback> callback;
    EXPECT_CALL(callback,
                Run(Eq(mojom::ZCashTxType::kUnknown),
                    Eq(mojom::ZCashAddressError::
                           kInvalidUnifiedAddressMissingTransparentPart)));
    zcash_wallet_service_->GetTransactionType(
        account_id_1.Clone(), false,
        "utest1vergg5jkp4xy8sqfasw6s5zkdpnxvfxlxh35uuc3me7dp596y2r05t6dv9htwe3p"
        "f8ksrfr8ksca2lskzjanqtl8uqp5vln3zyy246ejtx86vqftp73j7jg9099jxafyjhfm6u"
        "956j0",
        callback.Get());
  }

#if BUILDFLAG(ENABLE_ORCHARD)
  // Malformed Unified address - testnet, shielded pool.
  {
    base::MockCallback<ZCashWalletService::GetTransactionTypeCallback> callback;
    EXPECT_CALL(callback,
                Run(Eq(mojom::ZCashTxType::kUnknown),
                    Eq(mojom::ZCashAddressError::
                           kInvalidUnifiedAddressMissingOrchardPart)));
    zcash_wallet_service_->GetTransactionType(
        account_id_1.Clone(), true,
        "utest1vergg5jkp4xy8sqfasw6s5zkdpnxvfxlxh35uuc3me7dp596y2r05t6dv9htwe3p"
        "f8ksrfr8ksca2lskzjanqtl8uqp5vln3zyy246ejtx86vqftp73j7jg9099jxafyjhfm6u"
        "956j0",
        callback.Get());
  }
#else
  // Malformed Unified address - testnet, shielded pool.
  {
    base::MockCallback<ZCashWalletService::GetTransactionTypeCallback> callback;
    EXPECT_CALL(callback,
                Run(Eq(mojom::ZCashTxType::kUnknown),
                    Eq(mojom::ZCashAddressError::
                           kInvalidUnifiedAddressMissingTransparentPart)));
    zcash_wallet_service_->GetTransactionType(
        account_id_1.Clone(), true,
        "utest1vergg5jkp4xy8sqfasw6s5zkdpnxvfxlxh35uuc3me7dp596y2r05t6dv9htwe3p"
        "f8ksrfr8ksca2lskzjanqtl8uqp5vln3zyy246ejtx86vqftp73j7jg9099jxafyjhfm6u"
        "956j0",
        callback.Get());
  }
#endif

  // Mainnet unified address with testnet account (network mismatch).
  {
    base::MockCallback<ZCashWalletService::GetTransactionTypeCallback> callback;
    EXPECT_CALL(
        callback,
        Run(Eq(mojom::ZCashTxType::kUnknown),
            Eq(mojom::ZCashAddressError::kInvalidAddressNetworkMismatch)));
    zcash_wallet_service_->GetTransactionType(
        account_id_1.Clone(), false,
        "u1lmy8anuylj33arxh3sx7ysq54tuw7zehsv6pdeeaqlrhkjhm3uvl9egqxqfd7hcsp3ms"
        "zp6jxxx0gsw0ldp5wyu95r4mfzlueh8h5xhrjqgz7xtxp3hvw45dn4gfrz5j54ryg6reyf"
        "0",
        callback.Get());
  }

  // Wrong unified address.
  {
    base::MockCallback<ZCashWalletService::GetTransactionTypeCallback> callback;
    EXPECT_CALL(callback,
                Run(Eq(mojom::ZCashTxType::kUnknown),
                    Eq(mojom::ZCashAddressError::
                           kInvalidUnifiedAddressMissingTransparentPart)));
    zcash_wallet_service_->GetTransactionType(account_id_1.Clone(), false,
                                              "utest1xx", callback.Get());
  }

  // Empty address.
  {
    base::MockCallback<ZCashWalletService::GetTransactionTypeCallback> callback;
    EXPECT_CALL(callback,
                Run(Eq(mojom::ZCashTxType::kUnknown),
                    Eq(mojom::ZCashAddressError::kInvalidTransparentAddress)));
    zcash_wallet_service_->GetTransactionType(account_id_1.Clone(), false, "",
                                              callback.Get());
  }

#if BUILDFLAG(ENABLE_ORCHARD)
  // Empty address, shielded pool.
  {
    base::MockCallback<ZCashWalletService::GetTransactionTypeCallback> callback;
    EXPECT_CALL(callback,
                Run(Eq(mojom::ZCashTxType::kUnknown),
                    Eq(mojom::ZCashAddressError::kInvalidUnifiedAddress)));
    zcash_wallet_service_->GetTransactionType(account_id_1.Clone(), true, "",
                                              callback.Get());
  }
#else
  // Empty address, shielded pool.
  {
    base::MockCallback<ZCashWalletService::GetTransactionTypeCallback> callback;
    EXPECT_CALL(callback,
                Run(Eq(mojom::ZCashTxType::kUnknown),
                    Eq(mojom::ZCashAddressError::kInvalidTransparentAddress)));
    zcash_wallet_service_->GetTransactionType(account_id_1.Clone(), true, "",
                                              callback.Get());
  }
#endif
}

#if BUILDFLAG(ENABLE_ORCHARD)

TEST_F(ZCashWalletServiceUnitTest, AutoSync) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndEnableFeatureWithParameters(
      features::kBraveWalletZCashFeature,
      {{"zcash_shielded_transactions_enabled", "true"}});

  keyring_service()->Reset();
  keyring_service()->RestoreWallet(kGateJuniorMnemonic, kTestWalletPassword,
                                   false, base::DoNothing());
  auto account_1 =
      GetAccountUtils().EnsureAccount(mojom::KeyringId::kZCashMainnet, 0);
  auto account_id_1 = account_1->account_id.Clone();

  ON_CALL(zcash_rpc(), GetLatestBlock(_, _))
      .WillByDefault([&](const std::string& chain_id,
                         ZCashRpc::GetLatestBlockCallback callback) {
        EXPECT_EQ(chain_id, mojom::kZCashMainnet);
        auto response =
            zcash::mojom::BlockID::New(100000u, std::vector<uint8_t>());
        std::move(callback).Run(std::move(response));
      });

  ON_CALL(zcash_rpc(), GetTreeState(_, _, _))
      .WillByDefault(
          [&](const std::string& chain_id, zcash::mojom::BlockIDPtr block_id,
              ZCashRpc::GetTreeStateCallback callback) {
            EXPECT_EQ(chain_id, mojom::kZCashMainnet);
            EXPECT_EQ(block_id->height, 100000u - kChainReorgBlockDelta);
            auto tree_state = zcash::mojom::TreeState::New(
                "main" /* network */,
                100000u - kChainReorgBlockDelta /* height */,
                "hexhexhex2" /* hash */, 123 /* time */, "" /* sapling tree */,
                "" /* orchard tree */);
            std::move(callback).Run(std::move(tree_state));
          });

  EXPECT_FALSE(auto_sync_managers().contains(account_id_1));
  {
    base::MockCallback<ZCashWalletService::MakeAccountShieldedCallback>
        make_account_shielded_callback;
    EXPECT_CALL(make_account_shielded_callback, Run(Eq(std::nullopt)));

    zcash_wallet_service_->MakeAccountShielded(
        account_id_1.Clone(), 0, make_account_shielded_callback.Get());
    task_environment_.RunUntilIdle();
  }

  keyring_service()->Lock();
  task_environment_.RunUntilIdle();
  EXPECT_FALSE(auto_sync_managers().contains(account_id_1));
  keyring_service()->Unlock(kTestWalletPassword, base::DoNothing());
  task_environment_.RunUntilIdle();
  EXPECT_TRUE(auto_sync_managers().contains(account_id_1));
  EXPECT_TRUE(auto_sync_managers()[account_id_1.Clone()]->IsStarted());
}

TEST_F(ZCashWalletServiceUnitTest, ZCashAccountInfo) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndEnableFeatureWithParameters(
      features::kBraveWalletZCashFeature,
      {{"zcash_shielded_transactions_enabled", "true"}});

  keyring_service()->Reset();
  keyring_service()->RestoreWallet(kGateJuniorMnemonic, kTestWalletPassword,
                                   false, base::DoNothing());
  auto account_1 =
      GetAccountUtils().EnsureAccount(mojom::KeyringId::kZCashMainnet, 0);
  auto account_id_1 = account_1->account_id.Clone();
  {
    base::MockCallback<ZCashWalletService::GetZCashAccountInfoCallback>
        get_zcash_account_info_callback;
    EXPECT_CALL(get_zcash_account_info_callback, Run(_))
        .WillOnce(
            [&](mojom::ZCashAccountInfoPtr account_info) {
              EXPECT_EQ(account_info->unified_address.value(),
                        "u1gjrzpk0v0v2ae359cp296zapth9mw8xseyzhu44a4ftux3gn8gh9"
                        "hmzazrz6f3yvjyglrchz68g0s2hwpjknw3eywxgp0tn3p5p3g94w4j"
                        "dfked5as22p9y3ftkyt59eh7phch995yh");
              EXPECT_EQ(account_info->orchard_address.value(),
                        "u1qtnwpp2gg5r745auv2r5cvc4v0q8sr8nd3xcg48ck92xul8t6tmv"
                        "urkzksfln94mh2amfxjemwwtmvys4l40xlkxck5fpgqxzuqxs2jq");
              EXPECT_EQ(
                  account_info->orchard_internal_address.value(),
                  "u1dl9dtss80tsutx3xfje4vlndwhc2f2pernhhpxfsz9vw6nr0zz"
                  "lkw9p2m22xjcn5588fp3tnta9uqhpk4nh06xumwvt8ff7w653g5pvk");
            });
    zcash_wallet_service_->GetZCashAccountInfo(
        account_id_1.Clone(), get_zcash_account_info_callback.Get());
    task_environment_.RunUntilIdle();
  }
}

TEST_F(ZCashWalletServiceUnitTest, ValidateShielding) {
  auto account_1 =
      GetAccountUtils().EnsureAccount(mojom::KeyringId::kZCashMainnet, 0);
  auto account_id_1 = account_1->account_id.Clone();

  {
    base::test::ScopedFeatureList feature_list;
    feature_list.InitAndEnableFeatureWithParameters(
        features::kBraveWalletZCashFeature,
        {{"zcash_shielded_transactions_enabled", "true"}});

    base::MockCallback<ZCashWalletService::GetTransactionTypeCallback> callback;
    EXPECT_CALL(callback, Run(Eq(mojom::ZCashTxType::kShielding),
                              Eq(mojom::ZCashAddressError::kNoError)));
    auto account_info = keyring_service_->GetZCashAccountInfo(account_id_1);
    zcash_wallet_service_->GetTransactionType(
        account_id_1.Clone(), false,
        account_info->orchard_internal_address.value(), callback.Get());
  }

  {
    auto account_2 =
        GetAccountUtils().EnsureAccount(mojom::KeyringId::kZCashMainnet, 0);
    auto account_id_2 = account_2->account_id.Clone();
    base::test::ScopedFeatureList feature_list;
    feature_list.InitAndEnableFeatureWithParameters(
        features::kBraveWalletZCashFeature,
        {{"zcash_shielded_transactions_enabled", "true"}});

    base::MockCallback<ZCashWalletService::GetTransactionTypeCallback> callback;
    EXPECT_CALL(callback, Run(Eq(mojom::ZCashTxType::kShielding),
                              Eq(mojom::ZCashAddressError::kNoError)));
    auto account_info = keyring_service_->GetZCashAccountInfo(account_id_2);
    zcash_wallet_service_->GetTransactionType(
        account_id_1.Clone(), false,
        account_info->orchard_internal_address.value(), callback.Get());
  }
}

TEST_F(ZCashWalletServiceUnitTest, ValidateOrchardUnifiedAddress) {
  auto account_1 =
      GetAccountUtils().EnsureAccount(mojom::KeyringId::kZCashMainnet, 0);
  auto account_id_1 = account_1->account_id.Clone();

  // Shielded addresses disabled
  {
    base::test::ScopedFeatureList feature_list;
    feature_list.InitAndEnableFeatureWithParameters(
        features::kBraveWalletZCashFeature,
        {{"zcash_shielded_transactions_enabled", "false"}});

    base::MockCallback<ZCashWalletService::GetTransactionTypeCallback> callback;
    EXPECT_CALL(callback,
                Run(Eq(mojom::ZCashTxType::kUnknown),
                    Eq(mojom::ZCashAddressError::
                           kInvalidUnifiedAddressMissingTransparentPart)));
    zcash_wallet_service_->GetTransactionType(
        account_id_1.Clone(), false,
        "u1ay3aawlldjrmxqnjf5medr5ma6p3acnet464ht8lmwplq5cd3"
        "ugytcmlf96rrmtgwldc75x94qn4n8pgen36y8tywlq6yjk7lkf3"
        "fa8wzjrav8z2xpxqnrnmjxh8tmz6jhfh425t7f3vy6p4pd3zmqa"
        "yq49efl2c4xydc0gszg660q9p",
        callback.Get());
  }

  // Shielded addresses enabled
  {
    base::test::ScopedFeatureList feature_list;
    feature_list.InitAndEnableFeatureWithParameters(
        features::kBraveWalletZCashFeature,
        {{"zcash_shielded_transactions_enabled", "true"}});

    base::MockCallback<ZCashWalletService::GetTransactionTypeCallback> callback;
    EXPECT_CALL(callback, Run(Eq(mojom::ZCashTxType::kTransparentToOrchard),
                              Eq(mojom::ZCashAddressError::kNoError)));
    zcash_wallet_service_->GetTransactionType(
        account_id_1.Clone(), false,
        "u1ay3aawlldjrmxqnjf5medr5ma6p3acnet464ht8lmwplq5cd3"
        "ugytcmlf96rrmtgwldc75x94qn4n8pgen36y8tywlq6yjk7lkf3"
        "fa8wzjrav8z2xpxqnrnmjxh8tmz6jhfh425t7f3vy6p4pd3zmqa"
        "yq49efl2c4xydc0gszg660q9p",
        callback.Get());
  }

  // Shielded addresses enabled
  {
    base::test::ScopedFeatureList feature_list;
    feature_list.InitAndEnableFeatureWithParameters(
        features::kBraveWalletZCashFeature,
        {{"zcash_shielded_transactions_enabled", "true"}});

    base::MockCallback<ZCashWalletService::GetTransactionTypeCallback> callback;
    EXPECT_CALL(callback, Run(Eq(mojom::ZCashTxType::kOrchardToOrchard),
                              Eq(mojom::ZCashAddressError::kNoError)));
    zcash_wallet_service_->GetTransactionType(
        account_id_1.Clone(), true,
        "u1ay3aawlldjrmxqnjf5medr5ma6p3acnet464ht8lmwplq5cd3"
        "ugytcmlf96rrmtgwldc75x94qn4n8pgen36y8tywlq6yjk7lkf3"
        "fa8wzjrav8z2xpxqnrnmjxh8tmz6jhfh425t7f3vy6p4pd3zmqa"
        "yq49efl2c4xydc0gszg660q9p",
        callback.Get());
  }
}

TEST_F(ZCashWalletServiceUnitTest, MakeAccountShielded) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndEnableFeatureWithParameters(
      features::kBraveWalletZCashFeature,
      {{"zcash_shielded_transactions_enabled", "true"}});

  keyring_service()->Reset();
  keyring_service()->RestoreWallet(kGateJuniorMnemonic, kTestWalletPassword,
                                   false, base::DoNothing());
  auto account_1 =
      GetAccountUtils().EnsureAccount(mojom::KeyringId::kZCashMainnet, 0);
  auto account_2 =
      GetAccountUtils().EnsureAccount(mojom::KeyringId::kZCashMainnet, 1);

  auto account_id_1 = account_1->account_id.Clone();
  auto account_id_2 = account_2->account_id.Clone();

  ON_CALL(zcash_rpc(), GetLatestBlock(_, _))
      .WillByDefault([&](const std::string& chain_id,
                         ZCashRpc::GetLatestBlockCallback callback) {
        auto response =
            zcash::mojom::BlockID::New(100000u, std::vector<uint8_t>());
        std::move(callback).Run(std::move(response));
      });

  ON_CALL(zcash_rpc(), GetTreeState(_, _, _))
      .WillByDefault(
          [&](const std::string& chain_id, zcash::mojom::BlockIDPtr block_id,
              ZCashRpc::GetTreeStateCallback callback) {
            EXPECT_EQ(block_id->height, 100000u - kChainReorgBlockDelta);
            auto tree_state = zcash::mojom::TreeState::New(
                "main" /* network */,
                100000u - kChainReorgBlockDelta /* height */,
                "hexhexhex2" /* hash */, 123 /* time */, "" /* sapling tree */,
                "" /* orchard tree */);
            std::move(callback).Run(std::move(tree_state));
          });

  {
    base::MockCallback<ZCashWalletService::MakeAccountShieldedCallback>
        make_account_shielded_callback;
    EXPECT_CALL(make_account_shielded_callback, Run(Eq(std::nullopt)));

    zcash_wallet_service_->MakeAccountShielded(
        account_id_1.Clone(), 0, make_account_shielded_callback.Get());
    task_environment_.RunUntilIdle();
  }

  {
    base::MockCallback<ZCashWalletService::GetZCashAccountInfoCallback>
        get_zcash_account_info_callback;
    EXPECT_CALL(get_zcash_account_info_callback, Run(_))
        .WillOnce(
            [&](mojom::ZCashAccountInfoPtr account_info) {
              EXPECT_EQ(mojom::ZCashAccountShieldBirthday::New(
                            100000u - kChainReorgBlockDelta, "hexhexhex2"),
                        account_info->account_shield_birthday);
            });
    zcash_wallet_service_->GetZCashAccountInfo(
        account_id_1.Clone(), get_zcash_account_info_callback.Get());
    task_environment_.RunUntilIdle();
    EXPECT_TRUE(auto_sync_managers().contains(account_id_1));
    EXPECT_TRUE(auto_sync_managers()[account_id_1.Clone()]->IsStarted());
  }

  {
    base::MockCallback<ZCashWalletService::GetZCashAccountInfoCallback>
        get_zcash_account_info_callback;
    EXPECT_CALL(get_zcash_account_info_callback, Run(_))
        .WillOnce([&](mojom::ZCashAccountInfoPtr account_info) {
          EXPECT_TRUE(account_info->account_shield_birthday.is_null());
        });
    zcash_wallet_service_->GetZCashAccountInfo(
        account_id_2.Clone(), get_zcash_account_info_callback.Get());
    task_environment_.RunUntilIdle();
    EXPECT_FALSE(auto_sync_managers().contains(account_id_2));
  }
}

// Disabled on android due timeout failures
#if !BUILDFLAG(IS_ANDROID)

TEST_F(ZCashWalletServiceUnitTest, ShieldFunds_FailsOnNetworkError) {
  // Creating authorized orchard bundle may take a time
  base::test::ScopedRunLoopTimeout specific_timeout(FROM_HERE,
                                                    base::Minutes(1));
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndEnableFeatureWithParameters(
      features::kBraveWalletZCashFeature,
      {{"zcash_shielded_transactions_enabled", "true"}});

  keyring_service()->Reset();
  keyring_service()->RestoreWallet(kGateJuniorMnemonic, kTestWalletPassword,
                                   false, base::DoNothing());
  OrchardBundleManager::OverrideRandomSeedForTesting(70972);
  auto account =
      GetAccountUtils().EnsureAccount(mojom::KeyringId::kZCashMainnet, 0);
  auto account_id = account->account_id.Clone();
  keyring_service()->UpdateNextUnusedAddressForZCashAccount(account_id, 1, 0);
  ON_CALL(zcash_rpc(), GetUtxoList(_, _, _))
      .WillByDefault(
          [&](const std::string& chain_id, const std::string& address,
              ZCashRpc::GetUtxoListCallback callback) {
            std::vector<zcash::mojom::ZCashUtxoPtr> utxos;
            if (address == "t1aFpD4qebqwbSAZLF4E8ZGmrTk36b1cocZ") {
              auto utxo = zcash::mojom::ZCashUtxo::New(
                  "t1aFpD4qebqwbSAZLF4E8ZGmrTk36b1cocZ" /* address */,
                  *PrefixedHexStringToBytes(
                      "0x1b7a7109cec77ae38e57f4f0ec53a4046b08361abb92c62d9567ac"
                      "e684f633ab") /* tx id */,
                  0u /* index */,
                  *PrefixedHexStringToBytes("0x76a914b3b55981e7bf53e10fe51aa4f4"
                                            "5fdef06dec783d88ac") /*script*/,
                  500000u /* amount */, 2468320u /* block */);
              utxos.push_back(std::move(utxo));
            }
            auto response =
                zcash::mojom::GetAddressUtxosResponse::New(std::move(utxos));
            std::move(callback).Run(std::move(response));
          });
  ON_CALL(zcash_rpc(), GetLatestBlock(_, _))
      .WillByDefault([&](const std::string& chain_id,
                         ZCashRpc::GetLatestBlockCallback callback) {
        auto response =
            zcash::mojom::BlockID::New(100000u, std::vector<uint8_t>());
        std::move(callback).Run(std::move(response));
      });
  ON_CALL(zcash_rpc(), GetTreeState(_, _, _))
      .WillByDefault([&](const std::string& chain_id,
                         zcash::mojom::BlockIDPtr block_id,
                         ZCashRpc::GetTreeStateCallback callback) {
        std::move(callback).Run(base::unexpected("error"));
      });

  base::MockCallback<ZCashWalletService::ShieldAllFundsCallback>
      shield_funds_callback;
  EXPECT_CALL(shield_funds_callback, Run(_, _))
      .WillOnce([&](const std::optional<std::string>& result,
                    const std::optional<std::string>& error) {
        EXPECT_FALSE(result);
        EXPECT_TRUE(error);
      });
  zcash_wallet_service_->ShieldAllFunds(account_id.Clone(),
                                        shield_funds_callback.Get());
  task_environment_.RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&shield_funds_callback);
}

// Shield*Funds tests are disabled on Windows x86 due to timeout.
// See https://github.com/brave/brave-browser/issues/39698.
#if BUILDFLAG(IS_WIN) && defined(ARCH_CPU_X86)
#define MAYBE_ShieldAllFunds DISABLED_ShieldAllFunds
#define MAYBE_ShieldFunds DISABLED_ShieldFunds
#define MAYBE_SendShieldedFunds DISABLED_SendShieldedFunds
#define MAYBE_UnshieldFunds DISABLED_UnshieldFunds
#else
#define MAYBE_ShieldAllFunds ShieldAllFunds
#define MAYBE_ShieldFunds ShieldFunds
#define MAYBE_SendShieldedFunds SendShieldedFunds
#define MAYBE_UnshieldFunds UnshieldFunds
#endif

// https://3xpl.com/zcash/transaction/5e07d5b298f2862f2332effc833f5fe9157eb9ca00e03f394663e81b397515a9
TEST_F(ZCashWalletServiceUnitTest, MAYBE_ShieldFunds) {
  // Creating authorized orchard bundle may take a time
  base::test::ScopedRunLoopTimeout specific_timeout(FROM_HERE,
                                                    base::Minutes(1));
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndEnableFeatureWithParameters(
      features::kBraveWalletZCashFeature,
      {{"zcash_shielded_transactions_enabled", "true"}});

  keyring_service()->Reset();
  keyring_service()->RestoreWallet(kGateJuniorMnemonic, kTestWalletPassword,
                                   false, base::DoNothing());
  OrchardBundleManager::OverrideRandomSeedForTesting(10987);
  auto account =
      GetAccountUtils().EnsureAccount(mojom::KeyringId::kZCashMainnet, 1);
  auto account_id = account->account_id.Clone();
  keyring_service()->UpdateNextUnusedAddressForZCashAccount(account_id, 1, 0);

  ON_CALL(zcash_rpc(), IsKnownAddress(_, _, _, _, _))
      .WillByDefault([](const std::string& chain_id, const std::string& addr,
                        uint64_t block_start, uint64_t block_end,
                        ZCashRpc::IsKnownAddressCallback callback) {
        std::move(callback).Run(false);
      });

  ON_CALL(zcash_rpc(), GetLatestBlock(_, _))
      .WillByDefault(
          [&](const std::string& chain_id,
              ZCashRpc::GetLatestBlockCallback callback) {
            auto response = zcash::mojom::BlockID::New(
                2625446u,
                *PrefixedHexStringToBytes("0x0000000001a01b5fd794e4b071443974c8"
                                          "35b3e0ff8f96bf3600e07afdbf89c5"));
            std::move(callback).Run(std::move(response));
          });

  ON_CALL(zcash_rpc(), GetTreeState(_, _, _))
      .WillByDefault([&](const std::string& chain_id,
                         zcash::mojom::BlockIDPtr block_id,
                         ZCashRpc::GetTreeStateCallback callback) {
        auto tree_state = zcash::mojom::TreeState::New(
            "main" /* network */, 2625446 /* height */,
            "0000000001a01b5fd794e4b071443974c835b3e0ff8f96bf3600e07afdbf89c5"
            /* hash */,
            1724753444 /* time */,
            "0173faae56e2d8d74d8f822f264f068d14e13294657280b772923b9e607999542c"
            "014f7fe5593d8884dcd084c0cd715ce7cedf15637b267629098b3c9c64e12de205"
            "1f00013f029c997cdd5f2aabbaeda87464452698ac288471a6da8a52fed40ad858"
            "b03200000001fea7d027088cd5229046540cee27d20ea8300ad5eb3c57bb365b03"
            "901a0ae127000161086dcb6c1c9e9b060a564cd1f68f17f29e49ea4865cb75eabb"
            "12eed9ed5c1f018a91c3eaeefeff1da395c51a7e0ea24b69acde116aaef9f8191f"
            "b27089f23c5e00015dd875137451b1d4af6a06cb3969c99b50d8df3378f4527205"
            "c3f29d48e556060001ae94e769c003efcb6891de2cde1c38b62006f4bfac132d42"
            "d6abc118fac8261100000001b254e3cfba937bcd40621e1d623a943440b0838925"
            "46f081aab37058fd1b763f000000013e2598f743726006b8de42476ed56a55a756"
            "29a7b82e430c4e7c101a69e9b02a011619f99023a69bb647eab2d2aa1a73c3673c"
            "74bb033c3c4930eacda19e6fd93b0000000160272b134ca494b602137d89e528c7"
            "51c06d3ef4a87a45f33af343c15060cc1e0000000000" /* sapling tree */,
            "01b8d435b0d87da1bb5de04c11c1d8601141dfe0904614ec3d59426d27cc3b7824"
            "01f0e38c4140cf2fe8fa6234d07569ec6310910b1dc7df92b88118b0f3a7bc333e"
            "1f0000000000016bc8acce0f0a243b6351b13f53e3ce19a4ec6e494104f506a754"
            "c3268a9a7f15012efa15d83c3fd6589d6d4a5bb8813d1c5f7ff333e62834dec08f"
            "e3558381d61601d05d08c51cbb09059afff5b0c7dc6b3236cbd742936b013d9ad7"
            "91a20998302300000112852a88a1d88b30c5a3bd2f532500e25379fc536934f2bb"
            "e26a2fcdea5dff250185edfe70c36f5d1c3216c4750bbd8f11fe1ab96802f9eaf2"
            "73f2e85ea0709c1d0001f427516d6600178ef99cc049193c413103980c407ea348"
            "9a687b3c3479d2300c00000112278dfeae9949f887b70ae81e084f8897a5054627"
            "acef3efd01c8b29793d522000160040850b766b126a2b4843fcdfdffa5d5cab3f5"
            "3bc860a3bef68958b5f066170001cc2dcaa338b312112db04b435a706d63244dd4"
            "35238f0aa1e9e1598d35470810012dcc4273c8a0ed2337ecf7879380a07e7d427c"
            "7f9d82e538002bd1442978402c01daf63debf5b40df902dae98dadc029f281474d"
            "190cddecef1b10653248a234150001e2bca6a8d987d668defba89dc082196a9226"
            "34ed88e065c669e526bb8815ee1b000000000000" /* orchard tree */);
        std::move(callback).Run(std::move(tree_state));
      });

  std::unique_ptr<MockOrchardSyncState> mocked_sync_state =
      std::make_unique<MockOrchardSyncState>(db_path());
  ON_CALL(*mocked_sync_state, GetSpendableNotes(_, _))
      .WillByDefault([&](const mojom::AccountIdPtr& account_id,
                         const OrchardAddrRawPart& internal_addr) {
        OrchardSyncState::SpendableNotesBundle spendable_notes_bundle;
        return spendable_notes_bundle;
      });

  ON_CALL(zcash_rpc(), GetUtxoList(_, _, _))
      .WillByDefault(
          [&](const std::string& chain_id, const std::string& address,
              ZCashRpc::GetUtxoListCallback callback) {
            std::vector<zcash::mojom::ZCashUtxoPtr> utxos;
            if (address == "t1aVmWeikjJuR4yfuxQznCMAxPoxZggz82J") {
              auto utxo = zcash::mojom::ZCashUtxo::New(
                  "t1aVmWeikjJuR4yfuxQznCMAxPoxZggz82J" /* address */,
                  *PrefixedHexStringToBytes(
                      "0x6f31c69b7af52379871903fda4396e37117d9e1bbc94a972cda1c9"
                      "77742455fa") /* tx id */,
                  0u /* index */,
                  *PrefixedHexStringToBytes("0x76a914b658ec0220a588bfafb3816eca"
                                            "a03baabd99c0a888ac") /*script*/,
                  200000u /* amount */, 2625431u /* block */);
              utxos.push_back(std::move(utxo));
            }
            auto response =
                zcash::mojom::GetAddressUtxosResponse::New(std::move(utxos));
            std::move(callback).Run(std::move(response));
          });

  OrchardMemo memo;
  memo.fill('a');

  std::optional<ZCashTransaction> created_transaction;
  base::MockCallback<ZCashWalletService::CreateTransactionCallback>
      create_transaction_callback;
  EXPECT_CALL(create_transaction_callback, Run(_))
      .WillOnce([&](base::expected<ZCashTransaction, std::string> tx) {
        EXPECT_EQ(tx->memo(), memo);
        created_transaction = tx.value();
      });

  zcash_wallet_service_->CreateTransparentToOrchardTransaction(
      account_id.Clone(),
      "u19hwdcqxhkapje2p0744gq96parewuffyeg0kg3q3taq040zwqh2wxjwyxzs6l9dulzuap4"
      "3ya7mq7q3mu2hjafzlwylvystjlc6n294emxww9xm8qn6tcldqkq4k9ccsqzmjeqk9ypkss5"
      "72ut324nmxke666jm8lhkpt85gzq58d50rfnd7wufke8jjhc3lhswxrdr57ah42xckh2j",
      100000, memo, create_transaction_callback.Get());
  task_environment_.RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&create_transaction_callback);

  std::vector<uint8_t> captured_data;
  EXPECT_CALL(zcash_rpc(), SendTransaction(_, _, _))
      .WillOnce([&](const std::string& chain_id, base::span<const uint8_t> data,
                    ZCashRpc::SendTransactionCallback callback) {
        captured_data = std::vector<uint8_t>(data.begin(), data.end());
        zcash::mojom::SendResponsePtr response =
            zcash::mojom::SendResponse::New();
        response->error_code = 0;
        std::move(callback).Run(std::move(response));
      });

  base::MockCallback<ZCashWalletService::SignAndPostTransactionCallback>
      sign_callback;

  zcash_wallet_service_->SignAndPostTransaction(
      account_id.Clone(), std::move(created_transaction.value()),
      sign_callback.Get());
  task_environment_.RunUntilIdle();

  EXPECT_EQ(
      "0x050000800a27a726b4d0d6c2a60f2800ba0f2800016f31c69b7af52379871903fda439"
      "6e37117d9e1bbc94a972cda1c977742455fa000000006a47304402207bf92fd0ca8ad354"
      "d7afce8ec9be410597c9bced1cbc9687f51d5f0bd5ecbc7602202b5d068d6eb9753fe7a0"
      "c3f5562abb37fae30514500427d68b5044bee5ad9c940121021ea251ab4b5ad3eb78aaf7"
      "62329627e90a052fac82cf47e758e83ef96a2f1d37ffffffff01084c0100000000001976"
      "a914811b9ccf3596c62713c3fb339567bbae3305d77788ac0000026248c26fa7bfde6e40"
      "5b4e9e319af6f1e0eadfe20d220f184cf01614e851359f69b98b9598c13f6bfbca1a8c61"
      "9aad9a276d74394ffc44a2ff7b9a5510f8613a2194e434351d9ff067fa47ffbb0aa699d4"
      "30880a53c5bb9ef758ea46549e1d3e83e566a0a85fc0b3dbc78b4c099fd81ef13f4dd8b6"
      "7095b33305b178928fcf04c4b86378999e79ae6cb15eade4e99664840491190d0b34fc0e"
      "65b4b6c67c9d3acb3361b2bc5d49760ae76092a6dcc2ca0a299372c65e1c606480340a56"
      "5315a250c6842268c9507fb91ed1ca2823fff598eb4027a815aaa2bae0a7408ac225e5c7"
      "8a5f6acd7c91f483f45e255b2575b2cd3ff9f5f62ff46eeb77d4ea642a437cbeb8d98f78"
      "b667c524fe28c97be265c8c36a1bb183fadd0f911bbc9118ac9caa7e0cb788e622a5d71d"
      "732c4246bcecc73dfaf9109f29baee0bcc4871e39531fb19dd9019efb14b3c056b803927"
      "48008ab65e1c54fd2a3a9d89661acd0d1b6f5c410a1a6da0fddc210e33f54d35fb263768"
      "21cf4f4b9adb8ca14b29fca8b3b253071d2842b39f75f1f793c3a53185f80e89126d4c9a"
      "8f6a196ad3e230a40b6c47e28b821bcb5786224e89b445ddac5d69eaea7f487a42250a12"
      "86bda360ce702bbb2a0fb59f58eb5fd6d2e67000c915ccdb6c32212bfc533471f5838a03"
      "c4a7a53caabc02e78268659d402f398c3c3e05ce463500dd0c426047edb67067bd5073ec"
      "ca28f32546b49dd96a3c8273d316d4ef1d715c700d4eb47c98986fa36eff39d2d46819f5"
      "b90d82b4bebdb46f50801a4ab2a2253c77a4402ccd5eb2f7edf4e76069b676899025dbf6"
      "eb2a2c863708c525d777d38802493b476e8b5e551cd8d6aaaa59576c40bec65c566f24b2"
      "47cdaaf9764de61a97d0fb2e04a74bb25c20b5f12947d0381c85c2f227a1f8128036b1f3"
      "8efa7ef3a4844d2b25a8ba0fe08476a1d96372a3decfbf61e35ff6c561de819d0d498c4d"
      "300e58b36f44d364e522790c67f048f121611124247b757c45afbe58bc8d334beb1d2c57"
      "fb7601913f4899cf332a131f8e59164cc3f4c21ee6cb061c652b61f6cd637d78dfadf200"
      "fce7fe54ad48bb731b9181f5a1d0385d04447de6c9a5fcc7f24c4eccfde0ae320ed9f79a"
      "f944143fb348e2e1e90b44072019dc7fa195f96fea7a410dc6df0362c9adaaf6d09c3c23"
      "d422c593e1d055a0138f22d95eb23b02d1a53c8b46ac84843a8d0d843c06b7e86219083b"
      "201204184dca8df1bc3a17d2a019fa60e2df4a240e286b1ea7e66aa7f86a211a4bb8185d"
      "82a07dcfdb4e08fe4a15ebe6ccd3c51dce0778e49a05303b48e76eb055349caa73e74447"
      "12532be3aab8522b6629b2fba150ebf5a8ac067e3affc965763387436b092a9cdd1731cc"
      "f0def1aea2577c334485e5060ac7230028369772b806ae9a5a8c9f9065c98189b4855fa8"
      "c060bbffcfcaf0e8a31060cb04c3e613da1acd9d27e2eb136538bc4c739ee302ae6d3c01"
      "e67967daff17ff8b329c6d9d17bb107dedd9a83a3e517483c95c3b323102ae4a9289e77d"
      "f7dbe06ab09f2422d10e1ffcd968b921078b51971cc51e5e3b0fbae2c33afac4a1495c59"
      "bbfe753e8c8e62c0707f8a9df438ecb01b903b42138e20e96c2104742436e52b288efafd"
      "8fdeb6ea8612a1082e23071cc4af554975d10e28d546e0a5ebaa5cd4dae6278341104100"
      "34f4da8b234bdfff2f65f0a2937c77a8d192b7840f2d8b94994392edcbf3554c2226f187"
      "2dcedcb13c01c3ccb26a9c5ef633e6382d5465af9002c8ee11e076307766b6d44f06806d"
      "3c801d5db14289eb8bf218e011b1d084bbd60984fea5cf727467a70742dadbd959d32540"
      "8e9dece7dc2e0a9322e5734901a95ef986dd9e5b045389bb0e1b2aec715d260086475861"
      "2ad6dc1d5caa67380166f02dc7d1bc5880d6a5c3718a50df32caf31a8c17415d153e4a41"
      "5a1e8742e17dd1437985bddae1ab614aa476975a42e952e78c11a635df74ec13aba9bfc2"
      "7ec7169578d278fe960b7600bc3ddcb7970064c96e5812d2607bd354a1daaac4c03d7ad5"
      "47a7442483e06eabf2ca79749e264d82c75c9c02bd8c42c2502d9541152ec49b94f441a1"
      "35ba12e862b012dcbd965ba8c83304e93eb5ae6684b7d29f4d9b8899eaba3d4a8578070d"
      "061ac75fc1da2042de615ba5e3f13ff010bf73a7a08221bf653f106c5abf8237d52eb059"
      "86a41fa586aac8974d593254ca346ad8a641bb2f8b9ac97e4e85f35a9fdae4e993c53408"
      "00c16e18c950f52e5207c031d450568bd2b1ecac2a1c332fa3875b7842323c4d771fda85"
      "39ccb0cf8f0ab127ba70c0036079feffffffffff413bc8ac281817b1b753fda68003673e"
      "a30a75c3a1ae06b43d552c35b9bef338fd601cdf5a22664e766e5a7f051db1d1b6f859a9"
      "4910722d640bbdbceed7cc0b13db229bd7e75ff49ddf1c45e2d81d4e418e523017988664"
      "fb670444a4f879c4e0839b6e49f29c7a8e57d38832c93d68e1d10bc0a4f1b774faefb28d"
      "d93a66e41690b1853eea48b73481131a3ae70b11f900ec2f3608c30b86791b864e7648fb"
      "cd191024515a32975169589342f3d8a5e7d03d537b2c25e66e423457fa7103a4e430b48c"
      "87ce98c577a81ff1e8497aeb9cdce2e0b39f8efa2c2ac78b3aca30bc8d0714e01d325343"
      "09875a787d737c5b84fc6bd6ace3bac852c1e6e96684f7cf5438b9f6adee0ce7bc522b40"
      "2634b7be7d5c67b78987fe67c09593b665b08e9a876937e8c470d41e79541bffa0341db4"
      "89b69d48858301a9e70a10a7ec3c465796b4358a33dc06fdfd411514b322f4bc0783fcb7"
      "e5fd507446c75bcd26a5db9b019f9b8c86204a78b0f31fabf59718b3eb92ba0a1ca55f36"
      "3b72b16b829d9f87247a884fea979be29715f3fd6d8ac92778e4e7ded9e614734dec481d"
      "92bc8d4c5ce4136c29ec446df01640e7bc9b67bea01354e0560af5473866e274b9231531"
      "e99d8560bf83040f3dd109926bb85607581340b5d29a0134e86fe29cbad612f8a4c8a550"
      "ee46682bca200e964378d28f4d30328fd2d87a6367c7bc54994003bbd1b6b98c0be3c9ac"
      "f9d953c10882e2595f71c4f7410b0b72a546e0e225f5bca81080a935f069679fefbbdd6f"
      "97983ce3232506c8681f25bb8e69b8d02991c5b6314c05db5910e470006b439c2f4cd432"
      "3b3555e905647d9221ba35ac2ad447a3a11532a79490d08afc665e4d49ba7dbc82043ff1"
      "fcd2c6a1815be2541a166f3209a89fb478d59078043f4ed0941fd425725462f025dd9b6b"
      "616a65fde0ffc1ea9984148d2ba4f60df62b101092af91d1b1c9322118ec9a563160d9b2"
      "f9aa5c05ae633c1487531b12dd7e3558538c7d7851cae4249b3dad00469d313b0c864495"
      "48a02ae53dad0cddf1a745deec603a146433026d0127b1d8c08e6c798475011526543297"
      "3bcde9975543f7bded7525b3cae625b3e89128371b7c89db8aa339b267aa228dc28192f5"
      "d388ba95f034a4338eb13f63ad79f5ff202d1f7cf86bb4e04e18067f297c4558704498a2"
      "115b5ac6e20823302c89cde9caaaed87ddfc38534e04b3d86d3ee2925c6b3e2d3e1e2041"
      "4935f6a04251faaf95d21a45c126b4a1cfcb23823cb016eb8ea0c694e2e0ff6222fb6d75"
      "e7393060fdcd5185c468b2c091e5394b2cc3ef69863d635a6a706fd755a1ffd087b18114"
      "5afbb60312025a5b9229bbfcae63e5c5ef5a3c14fc9b89db9464276247aebcbb16f034aa"
      "5d927b524524b3f92ae4f7331a7506962cbbf23cdff18e0927f7ea441469b777595cdb4e"
      "d382a3951e4a807de1d52a544c8ed4473cd823dbb3e408611f2ec52e2044f4dd29f40ca0"
      "1dc92c5e738c5d559ac68db73d791323017bc7f7bf1c891a26c19b94ba1234aa20c62a23"
      "d22239ac3f78bfb8e7c8c7da24a0ef61357c4eca48d5c444720825f2fcb823e1bad94d2f"
      "407a83e665217b5ffc4ccdd53119a35d692444f06b6a37fdcb4008d61df849171df3f011"
      "7ef96e8ab3884630288f525a14df0c49f20cb7cf77cfbbe40a1b49130990451db770d9f5"
      "b9331e499a4dd7f6e6ee0703facb9d882d093877fe879ca434abaf10e578b1123b718bf8"
      "e5b099d71442f5c962da8f1198e5c3c4dcb99d715d6d7380194d58ad2ccf80c410899cc6"
      "83917cd6ccf925abf03fc3dffecb4dd7ed511271e21d47d5ae18f970ef1783b45ebb1710"
      "cd15a927db34cf07919f52dd28cf8776d91e2acafa98f0f6486e31825e88e87f6fe71d7d"
      "bf26522f8da3278e69abeeffc8bd3202719004649e38bff962fcef60c664ba0e8f55cbd4"
      "c9a2c66cb23b752df0139d7dc840332204685dc2d858783bcd7cb3ec9620416d3a2511ed"
      "556fd17683ee8ad926ab25f8cba8b7e9406f840767fb097b9180ed61fd51f260488f42bb"
      "413f7920bfaa6566b19bb55b4af106d99db908a44a9bec8f5b77ca6ea5fe7a4a12a7bd97"
      "25d40964181c5df248e779b332283c904195a115bf174d1743d967c906aaaffd54ab057f"
      "93181f2c297734dd07ad364e47ad685c616189d09f214ec876254cc37e31ad9b0bcadabd"
      "4a71d9aa70fb38b541eac22ef38e8a2b0bdc1dcbb40f8e3dd0067ccb77bcb7550018005c"
      "3a6b2c0ae605b0c77100aa937c12640abc25c00c54bfa9820a72631e54679e9deda43cf4"
      "1ceaefb00473ee51f3a2517c6e890109b67b51cd1fa9029680047cd47ee3124ecde66d41"
      "268e0077d50db5d17ddfc40d17361ef8a058eb0a9c4a97326a399c5ef043bd0a4f9a4d7a"
      "b61bea81c5af2b55ffcce33cf46cc40724d17f5e2a9986495fabf3d13a8db527d249c88a"
      "6de51dbf9894434b2a06663450e1555214260bd67cd1d80e51c7e1a239f72f2d012663bc"
      "273e957cca3a0eb842e4cbe3b7722f3ee9a85ddb02ae206a99c8da59cbfe35a6d62576ff"
      "c155c9f01b96f781a3090b0b181e2ca17edc826a6b386d90beb3ca66b2a28fd0161fa469"
      "55496c8aca9313a55174a278a41b122a396d1b0d68dd854b9c7620f0e5c2ce9e72d33d1e"
      "1eaf07bc6b4a0abe41483d859afba775091f20a064c23b6c85d66540c50db4a04e7117cd"
      "4429f419d6d1bdd25e897d82ee57a6a32941635dcf344938a0483700ac2a326fbf45b73a"
      "e8547039351e2bd1d0d1d21e2213c85d66b8aeb68bdcca16beb51caecc79e8650e3353c1"
      "60470e1be063966b279a57f15fd9eab27918bf7df6a1381aa25367f0c96143a18e339dbe"
      "96fb6eafc686c52c24115d19892df073c0871fc91bd61a198160e082e43188d50e4d1ad2"
      "b81ab186c1ff5134b798f6dd61950105648273370f6e15bfaf646d3a097d99f9929f2a63"
      "2407102596bc7664ada133d303cc45c451717ec94953fcf21751a9d79a02374cf2c81c34"
      "d96eb1d44ba112a8d2bb5222ba5e6d9f6a33f7aa6a48ec95f6c8a4a629c2628a577119be"
      "1443010ce0a8f83d97c973b2300dde50c120b1dff07ab40e2ce5dd05b04b4271ea922df3"
      "50c23ff6b0aa8c384d7718c53410ed012150192ca75ec1ba817afc0ceb271d9ae6cc5c1d"
      "a1f1cc04ad6e500ef266e9a4b24d6b5853d9940870711a271bbf2390450fe45c91b1915e"
      "63ae5d1a75fa47d446f1b6914abf7b2e8c8d53b84dab3d11bbb416776b68043574241d9b"
      "78907fa82aa4fdd346ca562fb975013625570149aaaa3a7744b957f100ab21f1937b4419"
      "ead8b79c2d9ac7ae472151a1c6352608e48f970bf5e1bd2740d3911162b7361ef7a806e7"
      "16522092347092d7f9981b2a9b11ff8a4079a37a79d8af1bf89451eb9c487aeb30d7e480"
      "44165e3babf1329dd40933c8e9194f4a0df255c48e3f2a53de7e977567b3075a488444a4"
      "8f3e2387d82f974663c25151a534af46ad165b8d326c1de9ea2d5f8486eb44bf32861e89"
      "df6c25a369a30f10e1cc4fce45ceb485e226d293c9dfa0fa4adf07282064200b305d7e53"
      "ad5a2445cdcee87d2c00e00bfb8359ee6e34bd02a4fd13c0bbe702e4ebbd9436a233aa2c"
      "0c496490ee11eaf3fa7532e9ec3b24cff9f95654931a1ee011f332685212f3a9ba8119eb"
      "815795c788722d31e7b96c3dcda18702dff0377fcf94c246a769fba320a13598a99ba793"
      "c81ee9375b4870783aa252198b1e39d099a215284183a9a8dba6f5a9a939841e08984c3a"
      "08f330b5fc923bc38cd4359508fd7dde89a0be6dba5adc2d368d747264ccc27d5d708f09"
      "73eedd6744d33ca54e335e722b26f27a93fa317d9da05811864f2193b3d6b9c8821b5b27"
      "139b0bbb36c60020091b251879096b13e88509ee3a065dff1e741cbdf17af7ffafc227d1"
      "5f634e6ffc451a99325334fa0f95d3a248db8d77c60e697f68d0a546493b09411600de57"
      "579b7fd09923e14d0f5a9633ee53e6ba2d8811f2f8bde0a66e8823c5c9c8dd2b2788bfcf"
      "bc3758b33ea19c23f48fa77a26283d08c12c2355dd011183d1fb4d8ce36cd7c1695717d5"
      "37b41da1a195ec0e920361fbf864001dd38e068130adb79529ff64b799ffbfb6c43f63f5"
      "add1f81de396e97d8635380336b528f3e44854b78a658578a5fa75fd795c3d3e3f6d805b"
      "b837e425a8e1f45a1d0c137ad3fb7ed51f1c43a4c0b288c2435ada4b95a87e1f791c6a19"
      "389b861c9f1b3227a317be1f4da5548fbf9b2667aa86deec014e82f04ff7a87d94c1d156"
      "2f17159bc98e1d595fe807dc1d2a2c0d61844ca91e775e570f2bfd458f6796ae56911903"
      "ae4c5635beddda4b96551d87127936fd7ff70da7d35c5825bedb0475c7400500513fbe81"
      "964fc874769c6f4fb6d0403c1971f095df8ded365f78fe887bc91069eb917792291948fa"
      "0bba7e9b5ef3b42803e614289119dc04e24ecbb718b9158e9829b7f0049a20c52969bd37"
      "aada8c5ebd3188b00c3529c703780cf308900d534612c09a94ba051f844b26dfc5eb03b6"
      "21c5c08a10e9db97ae1f79560db33e925e6a409d30f6f76e26b1e4169c898281e400e432"
      "a5ecbc2cfe9960bba7cb20aed7ff9c1bad1371cc9ee2fafc7d861a1956e512f911c7673e"
      "3ace4607b4ed2bddb7c4d58b81135bb8d9264b0d8a6e119d414a55a081cd5647482e5b59"
      "27df2151fccdbcb7c9b7f4004735396b3a93011d8f18aa5b610fed63fb18189efcda0b59"
      "e6436835882a1797dcc2a3cfe42795a6b87951415380bdf5b21a651c2553069d5eb5d99c"
      "3472cacb2c611b28f40fd8cc607069a51aff6e5b134e288aee60334cb56f363df66ae944"
      "a69f33c900d5f6063c090d27121ced9c212411b887193ba9c0cf65bc52517b7c5b5844b9"
      "ea185b2ed0d175e67e3946e47e7a454bc0b52f067bf948bd3a53c9723c3d524f8983360c"
      "79a596befbb5af9d094ac4f20e1c3819f3780e9db9e16a9350c1df9dc40d53517cfadc94"
      "057337b9c2203024179333faf8ee33c6eae5428e3a3b68d8409e6fadd893a397a2f8eac5"
      "e2f68c9310ef23c3274a44478912f0b004b1b595f405898a68fbb62a18f874c3e68d4852"
      "1e7511928e373f8da06ff81f5234da909778e60da91340b255460350e41c9778147a1cfd"
      "dfbab40ce5c34f221a1c00881d554dbc464e9793a72110314fc551ec41852f9c7f6fd1ee"
      "362ac43943b79cd563c0c493786446bd60e196d2aed25ed6ac0c1ce5caa69d374d8ae46a"
      "e04cc4bad19bf4ce4c517e25a001267a9d5fab152dc91146d1980cfc6c0cb4bd9131262b"
      "d58edd38a2e747cd236322a403ef69b94efa1dee7c048cfdf4de11e52b86a1a916479173"
      "2ac1bff6336e41189dc94e77c5d40c23f13691e4dbd08445a68233757095598fdbce8df8"
      "995b1eafd34d34afc0de0e9d0898b3a090a50e9ca490ab36dd0afb243a3b1cf877eed1e9"
      "5ca244df8b4400d52f6bb1ecaee7c6a871322bd3b73859cf8c7a14e0f59a27285e8cabc2"
      "d50f028b880bb25d5ca64d69b23ec8e065fb36405dade25830491f298c7a1592bb430296"
      "37021abf0f58be80ba8ff6d8ab2581134da3880e00441a237f92e9922d1a0206f889b22f"
      "670859fee7e6ccd1d3bfc8d5bb331a200cad6e82e910d2d88278160ad4f094a59957b9ad"
      "9ef810a1f31d0a37b9a6c09f6f91a1ddc41e747f9ded39c28261acfbb81ff3ab56749ea6"
      "8d1b6012f5a260221a98ef60db9efabf911b0b377e0d6de6b5b9c57709e2973422f55ffa"
      "0a6ef17916f8643063e750867df11970605c1f92dcc25697c2f2969a6268b1e12e748826"
      "a0506601a689316860a63414b3287a56374fbe04c4e8736325dc5cd997565e8d7943fb0f"
      "08b6593febc31a0774f1b09e444acd3d2549e24d7f2aff59f8d96e353b4c538aa10ceb54"
      "8dd82b144de1ebd6c4f96fdeb2effbcddbf3e969fdc776ec711ffbfd893f73558c1f3296"
      "d69252a4e2ababa6297e7e645ab2e1845d3014d4df35fb8fe7a84f59bb0b15d6d7443aff"
      "eed9dda55e3ead9501cae34bfb67fc246a4ce04085d52775a849110310f6fe5022fe01a4"
      "40d3f1567a61004f9263f832cfc89deb37d8a8c5758709ffb6d2cd266b2a78a11231e152"
      "9b47c0c12a9f9aeb8d473a7b49b88089ed853d91b996819a3eafc374b25a043714e1f9d5"
      "f6f2c73f8ac38b8ca2724057364705fbf478e26623793a2d0c340c9e6e354e52cbfa03a7"
      "10153b338ea79fe7852d153419ba191a6a6423ee7dd783f602e56d70abd14d6f043c174a"
      "7f0303809e9d30bd1fa42b386aa6489b7166264d354458f6c4a5a5ff52bc1833f1d09f47"
      "4a09308cabf5b69734eb08610c889a4626f435c766064c799ea4152690341e061dad06c5"
      "09ad6d182d237e6eebdf03a600ba3dfd8c849e7266a88669707cba8fa1a428f02f1c6795"
      "e2cd53ae0f8e2a2ce09c45e86283c2bddcb12bb3988eb48967e7229751b40ee1546c1bbc"
      "9b908da9080ac1b0ba8e7e7c41a177e1bf034e1c3426232e3d0325c20a0e813a8a978284"
      "9408ac0632b86facd327d5a6f14059707f3317cfdbfc045cde2f04bbb3ba3ea9c783123e"
      "8e27b124574b1a509f3b2904cfaf35a6b763ca79ca0493b37e1a57112077622515c7fc05"
      "2566299037dcc3842c7432f20fa5106ff90254d1f246be1d5943fb295bc655f7d8277cfe"
      "048cc2d18ede08a957d49066ff060d19bd0f27f45986ba2f042c68d92341dac971929dfe"
      "5f592d160ef72771585f357a42839bcf8e39a22e6b9754705c6c1cd1f1430a61267906c6"
      "e6321057d12eb5510b393c86cd76810639e97989ff749eef59969f490f540ae642aaa5b9"
      "d7c0a45b15eacfb79319105ced5bf5c3a42e0f30849d9057d73a361f6f685f99a7bbeee6"
      "9568a18ab5002bd09b7e03914d2062d1a91a9ff802a42aa1657bdfe49653c0b4af9b947a"
      "a5d706479aeed07aad2a1d52e697be988ef61ad53798aeb20be62f9af713ee27230af263"
      "6581e67988d3bb1461966e4299d81a1e7c2298e459610796a195d686561084d2320d4731"
      "910d8a7c6ef45b8097ad391b0731486315b1128548dbb1a1e9cd947830bb0bd135fb1d89"
      "cf27a2ac604f33c9f02f08da6f41a7a943ddb28af6942b6e2f49845f55011a55f87dfec0"
      "ab750e0ae348b9302137ee274607da7a917b4c304ac06920e931229acb07d03aebd91b32"
      "223430f1e006cf4723044c9d14e8c6f4ba49abfa2f50231f6e93a66fa5401ec74dc7691f"
      "f5455959d3f2cbbbca43382c64ea0d332e45bca951b663c1df5b3f3008f50e9c68967c90"
      "009f2b9138297bc208ad16dc1bbf35aa1f047b3e3b8b2d76ee530465cc0274dcd7c3c364"
      "94a77b58cc7c443ae651343ce2e96f44d7e1316fea36300886a974b3fdf892e2c507dd40"
      "d5c60752efd24e4c83daf5fa685114026212d9631314480f015bec9f64c1a5d6a0a59862"
      "f5628d57b40340fd1f5433ae7e6db929c32426253715bfc5cc4efd3274a6da2f35d431ae"
      "7fcbce3949c8148b87f14248a9f95b67e24bcdf923a0d6a9ebba1f5610b848e0a02eee19"
      "23302c3489994211465d9cffc38dff8f6b996b18c102217df405592497663ed2a1a92746"
      "4fc5f56a12aa04a425dea0120f0c7675f160be1a333d77696d8e13a266120fa462e2b9c9"
      "cc3025d85aa8909690fb617f3e7ae88be3e679fc2696d6e669a11946bd8632fca5784fae"
      "6cc955a7e87962a2f606fb41eb468c3decd4a4691c5d01da1fef9f0c43c58bf9e8617d1e"
      "23459e860b95a906a6357404af6f20e1a1c81471cc2ce89d2fe9943293d7e5aff9a3a556"
      "42814e24f5c876c7a6c4a39ae9a12afd1cffe507f52bce887c559cc5bdb497848802098b"
      "7317826ddeddff0af802040eb5cf0e9e1717c416737a0a520c5125b565b163046a71903d"
      "d6560261038805d58702072b12ed88b61dfdca71073637096bbae2260a7a9872a56a890b"
      "2f303a6fef64000950a120cc3752ce8c4b3c94f71cfc837c8a8c033624c05a812d1a1ed9"
      "6ddf4156f1fa4e9a7ffb5b83787b37281a0535b37dd59cc1637101cd3d48298af75d3019"
      "db273180ce66f81ade02e16b9cce55350b41e0c11b7c6e714eb12ce994a2dac0bad8f412"
      "3b3f9eca16a2e7cfc46264eb32bdc8329108776e55da1c75aeb9fbd302cb9e5c54afa69c"
      "8d20f977450be88186cb65a0c4afc43af3042ad5b6779f13875f0f78d60a6fdb0cfcd03a"
      "5c37673214b491581aca785a62f931c39400c28e319f066e65f46b5eef0f88265000ebf9"
      "2888610e0d4c6086e02934057f2eec47967b1a9ea3dc1cea2da9e63977e55bb81b845045"
      "7d3f39bdef2520db9f77549df5a2aa79b415f78aec8712fd10c5b0b11827c0a3ab590afe"
      "1e9516de06e8cef4ea14653db41a9e569eb9a3c4a6d2e672e93adcd66d9df6287d412d64"
      "b9ff5e0fc92b85228b6d20ddb6a3bda264c79921eb8b1892ccc5e0f3831b3a95b521cefb"
      "425abfa363dc65eba8a9c16c9de928e0df78241e759e192ea7401a59bafbc7eb0c49802e"
      "319af6b7b509ddbabae561df66c93eae9e5e588db4a51e3f98da4cd1d3aa24b456e77ceb"
      "91c5027eafe8ca1c2f6cfce8eeeadc8ee8eb330975197c2149e11994fa0fe1edb2c58313"
      "b3776f1013db380fc8c86d5d39370a5eb6154859979f9151adc010eff1dae2ea48c94eba"
      "1e5d0eeae9a4a781b9da2c50652a29039c8476f9b483d51addf71962c915d13cba6bd4c0"
      "e93bdf57a500155bd2f08b557a4d6c1b532233b4ec6be0d4b3da901fdf9ff82cccb57b7a"
      "c3211170567358fa5b8e0bce8542b4a427f484d27f8ec899dc2759daee56c4d67b8f3d9c"
      "f65829564d2215f8cfa1bfa4c45f21df56096b0d77f4c5f27c9116af7f7036807bd2477d"
      "48af6cfa27d88bf7ca8a9499ab81c2b60c8de3cfb75ba4b2e91a11ecceb04b0997cffa6e"
      "dd49dabff0a88035f5fffd30dd66b26f51f93cca7f668d2cc0cff04ae2f270a43f2dcca3"
      "c84c8d34bff78717db4b34cdd99b4e1cb3ed2e951d16478883aa375305dddd7703962d03"
      "ef145a55fb1dbc99572f41d1ed4a2339e4cbb9073a54850a9aadb668d7272f69ee29f27d"
      "d879441f9e7f328e45913786f5de74037c0dfb6bff91b0db619e91f3f8e9bbe79eb07a8a"
      "eb30f36a806b1d9be5e933feb5fdfac1af4a217dde7ebe9064aab95c411f060d133a3b49"
      "9e613f1c1d68a28b9fd423b09feade89498037e85c4d04f3a04079fe70b0a4fb23512cc8"
      "0d15a6752941b7f61431e993ce0fad96539f281aca549df191be6f1559eb2c99969f88e5"
      "e3b0ea15b42d10206d10fda513828ebdec3786f17db5bd91e8990f3e9bc8b7d072dc31c9"
      "40ca90543af02af245ad47fde53bab4cae526c224bedbf583a24dccd5e0d07542342572f"
      "dbb94a7c714fe505b9754b5866ecb3249cd9245367dd786a3e7c847790d02026480b08fb"
      "256b735e41f4481bfe47450fa30580b33375e22d2a55663ce85ff973ce09ec81b7b4cad0"
      "4f6beb385430e45e174c1e2b947f332c4c60ae149d6177f0e06499021a76d73eb57e8b81"
      "44fc2f2d64a3b87fa680e55a2056c03ded367bdb986781e8adb50ed9889a053f8d1ea97e"
      "454b32c1470e1e1015104879300aec3a229b37af94c54704af6d4d9a2cd37208faf5204b"
      "a39199590e29ad9f641584da7d652a2652b0f61a0eea3f50727a5227a65dae53f7b52be3"
      "27b88754b9e41e60c533949c21ee453559dab08ac6f689b34720b364c1143097d95668d4"
      "55b1373904b24cb34300f6950d8228e6bf8d41472acf2de3b0977a8a70622e2de4351922"
      "315efdb64405d3208374391e9ab77800db70800aa1a81116aad3abd2a64fd5889ac9ec48"
      "ac84a3ab1bcf330fed261bb713d50e5fadb0348645adf79558ebb025fdff0bd50f67bc99"
      "e36228833e0c90f48266a339b34c290e993e9214a20b55529fc3600394d2920e60210b87"
      "ed8512f2379bb4a74e2a477c1f6aea74ed6dadf9734d4a17c26b62f7b1eb766abac9b0bd"
      "a9dc9532d46fa98d46219cec1844656aff10d610eff055b2842c2a9b8e1880e0dcf58f1d"
      "2f70e472bee1fa4b0c21a556142060a3f3353db45118eaa5751a8f491b849b3a5c82de05"
      "aabaec2cc00de57b2f4691ba85261d398c60380ae34aa5a8a21d2771bdf04260ba181d06"
      "57f3335f2c6904434836c6f25b7d9c79f6c9b5fa5174b4c05fdd8ed57954d6701512a4e6"
      "74bc105ac0eac78c53f4ea26ff9840e680e18371cbcb76ae1354a069a0b6ad7c7daac354"
      "3d71f56309e53b5b15e600732ddc0aeaa4b4523338ab3983c3f428ee686282211bbf76f0"
      "98541194e4d6d212ee7126066a7d150878e17fc8cf0a0b9fd84edc304012488b8ed383cc"
      "164ecac1ff6912fab90fdf8028caea74348b00a31a1b763001ecae5bb343cd0e4afdc13c"
      "66f734dbd92198f42ab415db8f2170a4b2a3204d922ace5483ff835f14a875737cff842a"
      "f00b4244d366481a1280633be94d98646aa072c5359c357b4da8e203a51317ce69e0063e"
      "e47cffceaff0eb60627a401d655263a1115dd90e4a47ac05ebd038dcf1d8fae48fdb98ee"
      "d2192005fa3fbde30094fb855c3ef6998fcdc7b9dcc205",
      ToHex(captured_data));
}

// https://zcashblockexplorer.com/transactions/9956437828356014ae531b71f6a0337bb7980abf5e4d3d572a117a3f97db8a15/raw
TEST_F(ZCashWalletServiceUnitTest, MAYBE_ShieldAllFunds) {
  // Creating authorized orchard bundle may take a time
  base::test::ScopedRunLoopTimeout specific_timeout(FROM_HERE,
                                                    base::Minutes(1));
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndEnableFeatureWithParameters(
      features::kBraveWalletZCashFeature,
      {{"zcash_shielded_transactions_enabled", "true"}});

  keyring_service()->Reset();
  keyring_service()->RestoreWallet(kGateJuniorMnemonic, kTestWalletPassword,
                                   false, base::DoNothing());
  OrchardBundleManager::OverrideRandomSeedForTesting(70972);
  auto account =
      GetAccountUtils().EnsureAccount(mojom::KeyringId::kZCashMainnet, 0);
  auto account_id = account->account_id.Clone();
  keyring_service()->UpdateNextUnusedAddressForZCashAccount(account_id, 1, 0);

  ON_CALL(zcash_rpc(), GetLatestBlock(_, _))
      .WillByDefault(
          [&](const std::string& chain_id,
              ZCashRpc::GetLatestBlockCallback callback) {
            auto response = zcash::mojom::BlockID::New(
                2468414u,
                *PrefixedHexStringToBytes("0x0000000000b9f12d757cf10d5164c8eb2d"
                                          "ceb79efbebd15939ac0c2ef69857c5"));
            std::move(callback).Run(std::move(response));
          });

  ON_CALL(zcash_rpc(), GetTreeState(_, _, _))
      .WillByDefault(
          [&](const std::string& chain_id, zcash::mojom::BlockIDPtr block_id,
              ZCashRpc::GetTreeStateCallback callback) {
            auto tree_state = zcash::mojom::TreeState::New(
                "main" /* network */, 2468414 /* height */,
                "0000000000b9f12d757cf10d5164c8eb2dceb79efbebd15939ac0c2ef69857"
                "c5" /* hash */,
                1712914512 /* time */,
                "01740ed9378b958778ac2d8a128e36848f67903994a4cf565b425a53e5affd"
                "871501473fede979cebe2b429991465790546bef4901f61e778d37d61221e3"
                "2586105d1a0001cd1db7e5bc10f29fb18c9e55a9e184d766134e87ac4968fb"
                "b8b844fc0671894a000001ec648eb9058abba1de4682536919524da9ad66ba"
                "ee597381975ad457e0f7f76b0001dfab397aca94c3d3f406ac25b4d37f29dd"
                "82f4fa4fd68b6312cbdef65c330e090000000001f5b1b613f54794f437ed0a"
                "2b77e4fe97ecefb18c2476bf5d3ec87f3c94903d2901067187080be7f0b727"
                "3e4331b7505d7c985974c9c461fa602d80cc999289432501e3644c42c5d7ec"
                "d832a2c662dcd397824930ebe37b37bc61ec5af84f724c2571000132c52534"
                "3fc4ebe79ab6515e9d9fceb916d920394bad5926a1afe7f46badef420001d1"
                "b36bbba8e6e1be8f09baf2b829bafc4ccd89ad25fb730d2b8a995b60fc3a67"
                "01d8ccea507421a590ed38116b834189cdc22421b4764179fa4e364803dfa6"
                "6d56018cf6f5034a55fed59d1d832620916a43c756d2379e50ef9440fc4c3e"
                "7a29aa2300011619f99023a69bb647eab2d2aa1a73c3673c74bb033c3c4930"
                "eacda19e6fd93b0000000160272b134ca494b602137d89e528c751c06d3ef4"
                "a87a45f33af343c15060cc1e" /* sapling tree */,
                "01f53cd6d046829fa6bde0d6517f00cb395f6276d29f54d6ee8cc03a2589f3"
                "ad0a01a021c60843a6a4f385a94d8d595ba1135fd2db406e451066fd361907"
                "cf13a7191f0161e3b206d91efbeef0e45e83127ff401976364df189a2a1762"
                "cc46b61ce0c61300000138ae9fd5b9da68da1b63af95d4c32863d02729960e"
                "70c6eb7cc6144ee994ec0301f8e1b87f0a823799d6e516dfdb7e1dd44e2696"
                "cc82ee6c16a73b22104604ef36000001288222e46d9892f23a2e2d6fbf6e93"
                "31f6aa139f2ca3e7e0e7e1fc83c7778708016c33eca8e2b92ca46f0cdfd799"
                "5778fd0eb625df98f488ab1d1a90fa8825b43f00010d2fb3b0880ce34e2643"
                "18a8faeace5e051b47afcbcba1edfab81aa66bd7ff2f00000001be7e467513"
                "4c4441539879962acf4c9ce2523471c82f11a2bfe90d910e5ac11901d38650"
                "8c9fabdc60836bfe3c7251fcbdd4617180a804d40fa29dc25fb9c0aa3401cf"
                "3bf92f69798e68555548afcce1648add1fb2548d64fa9a1ec22a3e26e78901"
                "01e637281deb58dff0c44ba13149b784a95da1b493005efd057e6f4ac20ef5"
                "d81d000001cc2dcaa338b312112db04b435a706d63244dd435238f0aa1e9e1"
                "598d35470810012dcc4273c8a0ed2337ecf7879380a07e7d427c7f9d82e538"
                "002bd1442978402c01daf63debf5b40df902dae98dadc029f281474d190cdd"
                "ecef1b10653248a234150001e2bca6a8d987d668defba89dc082196a922634"
                "ed88e065c669e526bb8815ee1b000000000000" /* orchard tree */);
            std::move(callback).Run(std::move(tree_state));
          });

  ON_CALL(zcash_rpc(), GetUtxoList(_, _, _))
      .WillByDefault(
          [&](const std::string& chain_id, const std::string& address,
              ZCashRpc::GetUtxoListCallback callback) {
            std::vector<zcash::mojom::ZCashUtxoPtr> utxos;
            if (address == "t1aFpD4qebqwbSAZLF4E8ZGmrTk36b1cocZ") {
              auto utxo = zcash::mojom::ZCashUtxo::New(
                  "t1aFpD4qebqwbSAZLF4E8ZGmrTk36b1cocZ" /* address */,
                  *PrefixedHexStringToBytes(
                      "0x1b7a7109cec77ae38e57f4f0ec53a4046b08361abb92c62d9567ac"
                      "e684f633ab") /* tx id */,
                  0u /* index */,
                  *PrefixedHexStringToBytes("0x76a914b3b55981e7bf53e10fe51aa4f4"
                                            "5fdef06dec783d88ac") /*script*/,
                  500000u /* amount */, 2468320u /* block */);
              utxos.push_back(std::move(utxo));
            }
            auto response =
                zcash::mojom::GetAddressUtxosResponse::New(std::move(utxos));
            std::move(callback).Run(std::move(response));
          });

  std::vector<uint8_t> captured_data;
  EXPECT_CALL(zcash_rpc(), SendTransaction(_, _, _))
      .WillOnce([&](const std::string& chain_id, base::span<const uint8_t> data,
                    ZCashRpc::SendTransactionCallback callback) {
        captured_data = std::vector<uint8_t>(data.begin(), data.end());
        zcash::mojom::SendResponsePtr response =
            zcash::mojom::SendResponse::New();
        response->error_code = 0;
        std::move(callback).Run(std::move(response));
      });

  base::MockCallback<ZCashWalletService::ShieldAllFundsCallback>
      shield_funds_callback;
  EXPECT_CALL(shield_funds_callback, Run(_, _));

  zcash_wallet_service_->ShieldAllFunds(account_id.Clone(),
                                        shield_funds_callback.Get());
  task_environment_.RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&shield_funds_callback);

  EXPECT_EQ(
      "0x050000800a27a726b4d0d6c23eaa250052aa2500011b7a7109cec77ae38e57f4f0ec53"
      "a4046b08361abb92c62d9567ace684f633ab000000006a47304402200abf02ccf7a5a6b3"
      "d8496e0546844ebd033a2f6099139c9a5bce0c27aea21932022050b713ad76062ff97ca9"
      "ea9d6b2e3a236077a013b597a8ffc7beb20e476a3bf0012103e1dc8e28b0318ef94cadbf"
      "a0c9781ff69a09c1424c0b5ec3333132b1fe3aae8bffffffff00000002bcbc1c4f50f320"
      "e7ded91927aaca380ad95d1c1e0aaa9be6ea362cf435f34d87da3403adcf62535c29608e"
      "673e0bb1a6be7992f08af8a2c35e6f7cefd7067f068812f9f058fa57ffc1c0910e6a19ad"
      "0a488a4eeec8a19d78f9910eac9e44b9acb0b90dd58b6427ac434ad73d2484bc25213f1c"
      "6b7251bac7d8152f63412a9d26f575246c6ab3195b8f96736ecabc45eee31c414f26f4d0"
      "178d666a7064532809a0db6c0f5e2286bd9e4c80f67d277ef9507b193634884bddc4e983"
      "08b54b2485ac2b0204dc3071c5ce33c4de04bf323cdef4a7d3ca46c8ba3d3f1d943e5363"
      "49d4104f1be5ac10e4bdb98bb3c9f87ad75489f25da171e64b8bf0829e6d15bec11a3191"
      "6c80723483552f77bf6b2d2562cbc306e67c30fb5c39e22c8636b4c62c84213426bbc233"
      "71a9461a98a7c6ea52913319056df7e050f4f99133620dbd159d070fd514bddebfe75b0c"
      "4b1c441e227bccc77be84c284850296568a2096f3661fbd7d38da9e3ebfee74b272ad1c5"
      "4a55e85ef323d7775cfbf84a532100395623a7bc6a0c9d82c9363d9d6b9e0efa33b04a3d"
      "1991f13edd4f29187075540549171a26b8e745dfe1cd0291a59617b06eb5e6d6612845e1"
      "9328a825aa7506dd7bc2ecc93a3d2dcd24b258fd372dc35b00ffe7925b5734267d532bda"
      "8dc0feac21023c2813a25e7aa301df3168e421b1b27db4fe97f54eab444707e346dc8107"
      "228ad218892da1dc28bc31046636d0d69e9310e0e7c1262051facf580c34bb2ff07029bc"
      "3d11f20f5bb76f12d5bab32772cb334811f7d1c64d9a4cba13cdbc12203597edba8f5a13"
      "8c54ba58dc27bd846453534b16906c67a49438ccf3504a007b90ab149e645ceb831472cf"
      "cc08b704f9f596039725c4021b8fed8e0a2a0b858f0e3d6e3ed58ab70dd7527ebc9aec93"
      "f36e3df6bd29fb69ff23d4857db3ab92c1ae80f81318c457a8b3d8eecb9f5c1d7154f590"
      "d4d2812f3029d594f1a452b5b8eedfa590169a9f0fcd572fb0936d5fadae183fffe470ff"
      "e7f6ff60e8c003db15cdcdd6f03b959e6004928284f41c31a0282eb7d186d03ea9f26482"
      "41012ee034d71b8bba3c4a5a4bdfe494feb4dabeed06b9823ab0e4ea61acf7026383baa9"
      "0ab7b6e56fc8323c3060a4e5c23d990c8bc0fc18a75dcd16aec6bfe05662a2f659d710ee"
      "aff78a16d44abbc27c287d8d2dadb5f29c206b982f9e02bfd34c275b3bd34c8260181c5f"
      "b97083d5b8b84cdc34ee3fd33be74c3b8d1caea377878b62630b4f4bd52ed2d32f397a45"
      "5b7ead654f234cc9af8dd531cf2b2f4076eaddacf1cf1e45fa98cdc5bc38c1e6fd8ace97"
      "9f354aab32423786051373c83b6d3c14e1d29c7345a04884fd4c83a8be7880c737a2ca13"
      "1b64f1483c435118e0e5c1fe33a1dfc8fc1fe01abbd10ab9713008cdc393fcb7a4dd0a01"
      "406fa8c0dca05f7ccb345a4dfac1f935d10fc7cc93e79e1bdca161efe66c95b57b621ebb"
      "fe89d4e8c9d45af606d0c0e350e699c719dcb6560f18d085b76c9f19ddada83f905bd554"
      "9737f196c447db36a270551a0a1408771c50cc37baab706ebdc1af83b09be320292256b3"
      "99e1ca72e140b51658b177994aa8dac731e89ce5dd25d70eda9767785dd81a998a583207"
      "8788b1cb1602bb4196b954bee804abacb4fbbe83babca9dea818125711e9871e1c3c179c"
      "1c7c653a639f36d3a6bc3c91f54a70fad9a1f68860601c8700ddda8f4e1f1ca4ef77dcc1"
      "04a34c2ba32152f9966a87f615e7f6dba9b799ed0afaf5bc6576a854976e7b34f2798ff3"
      "0599e15ed753ee5ddf3b35518da09719e1f1a143593f7b158a55f113b1ed657a6ba59e4e"
      "f9a6b2c536b107bd278deac4f6246d4c64a1b429ba78c2643eeea586057e557544e5a1a9"
      "17bcc1f9535f6d3ffef6bfe3fff51a97933eab9e1902a2383c8ccdc6a87b7bfda6818131"
      "368854483c4eb152b7530734855175cebba1f9579e419a10f98eede39388b67b7d90b5de"
      "73424c6519895e0f8e234acca9fc7a3b4cee7d9521eb76b17132f9cd20a250446e97c4c5"
      "031772d17aeeb24ae7fcfa8e48f76c2cea5e7f6dee4c5945e398fc6f7c56ca91552b5100"
      "1d50aabce4ed1e44447c8d9d2707aa6a6e68569d00144b5d8e053cdd3e886912836f60a4"
      "c78ffef279aef069a42f9753b458a77c6e9c3f6dd1a752fc404493a5099b80fe20b24f38"
      "4ebf72b9738cb416151a39a1feaea762ffc73a0c32cb35ac9a5723d8545c60e17117a83a"
      "5e8f5b99203679a96a9fa04943c369d3b02e481eee85d0e5db785d677b8f5e0964061c5e"
      "6617ac368292b93f87d007b30f037899f8ffffffffff11866eca74dc2209d5c4922bc6c0"
      "e7cd062c02ff95fff75ba31b3fe935b4cb28fd601c33c24e059f1316ec2d1beaa747f6c3"
      "04ee7f4daf0fed91565c1b90e2f769842dd05f1770d1a9b5f9429715238ad90349b3304b"
      "efaf41be596f6b6ed318677db496b76ce6d488231bab277c15808af3fc7e218537ded70e"
      "6540ffef33f6bccf05a9c839203fb1b289e773466dde397ce02ca0e8bbb90be82566b34c"
      "e7e0652b17920c839f0b3efbd17cf9d9d59780041be196b11827d482668efa056406050e"
      "9e847d8cff6600a2aa08deb5c65f59470e006a61e47626c4d94343128e3ae89d1b216e16"
      "17ce2ea2780f81ef9856dde3b6b751723ada34d488be578efa7bb168034dee63d80c6294"
      "b539ae275309c24730757646ddc3ea6f639d50ec4398d9b5276881285243be9c6318e23d"
      "7caea3475e522e15bff42fc5dd900b90caed622d862628abd64494d7f886b9dc68869950"
      "6409b3a287f813bdb032b00c2bef26ef16b06b4b9dd0063572dd1fa92b6c6f965822b9a1"
      "d885bb9cef2712a45d4b1bfe0ba4714ea7259046067a606be22636225990a6bb0981e29b"
      "8abe95a63398fb2dbdfad7d32a50c43a5a49a659143b0ba621ce40a015f71b2d96c0f46e"
      "9865ee2ca9e9a74dc0ffcac5fa9f69c43844f117ef7c5bea4a583e4fe8fde554c7f163c5"
      "322290579f41067cda5704fcba18060dbb4659cbeedad6d9971f231f3a9ffdea25292834"
      "db8a2ec98fdc6e296ac784d4632e615c74edad3dc1bf6e09a08c24433ee0ccb7fba249fa"
      "405a50702c5494121578b18f3f0d451795e0c5188bb09a000a64ab7d413ebd0f771dbbee"
      "d8d984ad184d7076c27bb6ca3d01913fc7b10dd72fc54eeac41de7dac17c22e4b4d00656"
      "4d34609f90e10ac6b002a0ee41e289da35d4b21c826216ac90873f05d7a9f76b1a3f4eb5"
      "0db77265c8206a776217b359bb3d1d0e441f6c91105d7328c5478794f699769e08696783"
      "f65fd117bb89d0053bbe815e3ef58562723c475675413e7083299d7f416283e573e96dca"
      "7de3a07f8be2aba8846125663ec39b77822cd8c1c348055300cd2eeeb47bae94ca821dae"
      "afc01cb8568e09b10ad7f7d1af4a8f4852823140515348676f172062d1d72ee80a1ecd3d"
      "db62afd8c88a752a810d728876f7505e8867feca6782ba563e7b696619d1570d993f26fc"
      "2c3c0f984e91d67ec780a431c093759e5133ef0677b0f6af33ca3ea4b1972168cea79040"
      "3d45a2f24378a9e94df75955948bf20d7a9f98b98305b258334c625f4b4179ad3d3e9ae9"
      "a3cae32f7375fb099ee0b4d3526af37c83d445711710d5e3e07c826d4b3fb622797d127f"
      "ab6b7b5fda599e8a9f479a56b5789a813ad47eb6282b2c6d23ff6537cb5a96d8d677e0d3"
      "628fac9b1cf43bf505125989b0f917762f727e0ab30df48a326dd7dc1869ebde0cb4f763"
      "715d6d428f91ba7700107adc4e71353f94763b03f86154fdab95a1b12c76b960d6368cd3"
      "290c5a7a34841e8337d0bdb1e4775e53b3b3fd39df0375f9cb414aabeb7702c7a5eb8334"
      "a021d86dcc7c96a7ba2d3a36b15047f8df4d97e07341189b008301483fcc990f37d70783"
      "9412aa4a65b0cb922868f054b29f319148daad482a580431b26817a06f5c9c8fa9b67c8b"
      "36dd31b7dcbe6af2ae1ffe7ac9af29eade831158a785f86543d282cc2d5845233d7a870d"
      "0252b87652c47207357108f06ce43f1a9b5643f68712f3e4bd067ea1a377e66cced8fa14"
      "c3b32fd6cf6d84e1429662dba421d07ca0ece1af657905e3aba8844e7b86930484144d5c"
      "d030d585076e0f568fdc962e90ab1d179472bffdcebff037943ac9acc8034ed983a047b7"
      "56e7081bac4053365037c2fe69644af538273d641645293981249ee852661b7523297392"
      "2c4052abeceed6563995cf1614864da674a084bf7b1aef1d14d7242369ba914594ae74f2"
      "dc417c6e4f60e0fce85e02457c19508a624943e7ae41f5377f33a3f80cf98ab7f300dd0c"
      "7badabe577e2a41fdaf5b746635db05c7c6951e72b70094c1ef0c9cce2b79f0b30da4504"
      "91c2fafacaa60a074b5c4efd7751171fc8200e0e03a4d89c16a37db3c2476c719a88e4a7"
      "852d16ac8408eafbcec296efe843c87db5761c78c5c535e3c43469ba1c609a3b23a3b714"
      "b27294b3d80b4598d8a594be830fc87d53f68dc82a6a550e0b9d89ca81dd72613bb156b9"
      "63402b1271ca0b942b910c3d205dfda73e7d371677d94c7b43b1c439d165cc51c0ebda49"
      "ac366a5e3b14245edd7adacc37aa2cea1f4391a4375f16286bb430c48818ed5b3c1dbc64"
      "a1a1f3d1ce1ed0e06d26c280083a006ea5fe6cc3d3a2d1a9c0400a70512ea2f02543bcc6"
      "740ec5694984b13314ba9bcccf033a78a3a020894ad123cee730e1ce21da8f64726ef4a0"
      "4f52b90af8ee21273ee5422cfdc8522d7f3031fc2aad6728a9ce454bcf9dac64765f6c4a"
      "d74eb271eafd90f2d314435fcc157241df8354fc0367c66b608ca532d320322e6e7d6890"
      "08eb7e030de7d9c2de2ec4f30d61ca7e37348df1158fbf45c89acf80d57911fa647c1639"
      "3a9ecc97d81793991743eda70136b954e2b5686074a7671f4ba1cad0eece5bd8447890c7"
      "0f9862a0b64ec3b211e3d73e6eb8a09254235e8bc4e3da2a115aef2872e14ac864521e53"
      "fbbd3c0f3bee9f36bd5aeda0a3d7aa084f6ef61900a23fd4098ba671684f93ec89ab6ee4"
      "3839b7e9b1c53c4d70ddf3a31646273ab0ef01704394f35c28b1d7c7053730ad145eb6bf"
      "c38d3a88622da7f7c00218be6e8dcc3dc1521811aaab7d5eb56db5a522d23f44b7d88028"
      "0277078299d3996ca20576333c34eb77a3f63b396e6499881ef08bb616b7bef278ee61f6"
      "06025d06e79d88ede6b7a5d1cceed0dc37fd461d3b89d6f00f5ca6161a05b64eb862c85c"
      "954a65253c4388c5a208fd6e32d7645d17a3a484c30e6b3acbad29a9086fe23b53f89f61"
      "4f731f44cab33812eb18c7e50d86df11c9abcc6b4c5797d890949fa8c4aac6f3e3b79073"
      "52372545e9116bfe1eab657bab8c4af094d68b32cb89a65b4708114be137a8fb19c91aa8"
      "94ee96c0041900fbec4a787ea4af9e9bb5abc568140a745a8c3ac8e6b7b8d5a38fd27433"
      "0443fbaa5734bfdfd1f976d17007d050fae6efde1fbac0d35aee6c652b4133613930bee7"
      "0977442f81c6fe91821bf0ab491626541799baae09fe4f81ef81b0c00db3ab772c0df60f"
      "cd3bc1086f05dc04a970b3a939e47995e35110af86eca4d23ad0134935d6daa435d91709"
      "7cded5d9f189ce5a50514b0d09e7d3c05d7e2b42139d1e7f076517e18939ca733d30bf3d"
      "50373e8f7068b52f5f8f73788b80010309e6274f2efe8aa0ed923aa597d5a4e4617a3377"
      "3455e00c58ae42cb34c80d8234b64241eef685ddcdc9f6fa2cc6c846057cc1b82858c30f"
      "b91b448838be04f43ae38027cd86e7c7025cf839335990c5b66845f27d7a8a3150d5b9c6"
      "40f6ad410d4d48fe97eb0e443923d2373259cb3134c1e0d23336608846f04d8b4580d559"
      "1cec8c5cc1e09eff4973f5313a9d147c15d5cba3010c7d8d3c2c808bb499b8f926286485"
      "f72a0c6f597ece43d86a4659edf014fc56b97da9e14b7a2c2400605c2acb713370c3c279"
      "297635ae720eccd881d26ef2ed88796a5a6f08cb9afd18c2338822f5ca0c84b7ce0a559f"
      "25bff113c1d31e7cebb7f0b9ebe47cf3f6202f5206b413d39c3529ce10e9a8ca4eac2362"
      "173c4a2da233ab1327aac4e704e31d8c3f9c754b3fbbc27c5f15a40aeef4caad8781893d"
      "6c51bd47d7895f6f61770e7c2224acbfb0eed0a20be5df4077f57181d1b515ed8f6bf2cb"
      "9b10f1eb020c200f2ff3827ed66ee3d9cf1aca11958fb0bfc89eee88e05dc26faf1ad4b5"
      "828ce6103a1f57998a3172a7940c1d79aeb3941d030c4198fc2f5a62bae5039c73690f63"
      "3655445bb466bc9bf2cd3dd28b06803a26132dd01c7d5f6ff63af1cafb39a05925a09d0a"
      "1d000a100ad7d6bf8f2375f35a992320ff31210058e021099715515b2d5dbe460c5508bf"
      "8c72f096d072c20ff2df2348047a92b6344798b64ac46a830ebbd9ad5a2dd9ab2fae622c"
      "a13da34f4130ee3c37973f71043e01da485e35120a8f321e37ab268e4081077b5cb995dd"
      "676dc1d3ed1ea187b1a49aaf16b3276830361f4ebc2ae0c33767f273a602cc72b05d9a44"
      "3d2393e912c0181d278b6f5a19e0591bfd03d93761e986302fed9435aa574837503a8f35"
      "c7306d3aba56979a1a9217d0afb2a84dcfae836da5eeb1ec948110a73f648bcf81e2672b"
      "2b724c2d2d4389b9bee1f89fd9666cec638d5185aedca78d0e1c12184211aaa27704580f"
      "345893ec68347b34d25a113cecad24a51332dc97559c281108e7cc47a171137839bab555"
      "fc1db45fecb84def5587fe467d485c3aead1083adefd21853ff96d7b34180d2dc0c04836"
      "ed80e6db954222acecc26a2ae9a0591b86ede2f24798e3cd2dc9ad3730aa4046b4d8c934"
      "042a068d325de4b409a88cf52d31bcdb2c78f4ae1f383a528b159398c88d3ad66ff2bacb"
      "9b9bc4d91b3e27da7604989685b55d6d209358a4074d1da5c19426edfcb278a3325e3293"
      "b4bcdbe651ce9ddbbbc908fb224e96edc73e03418da0ebee76b789aa9099371a228787ad"
      "8ad7153a03b212850428c82323fa52756c08a26a3cbdd52c7040c76ebf0f0456038c58af"
      "f10f7956212cf338d44ac70f3c75a962f23171c36ecfcc37da351f43e66e154234a4a0d2"
      "062a94d1ff819108b45228446446cf0c8415ceb0893d8d949bba74112ed1417f398c3c59"
      "c7c43e50c4aa42d68d919012027de8fa2082c4eb693a827ade5f13e812ed4b1cf68af837"
      "523c261c45e720d272250a5ca0190e0a1cd36a2b2ea8822a0c77dbfd0c256c164ea25269"
      "31cd5873bf5bec70580279818fe431d81cefdb5d2fb7ad5ca27e3168d6160d621970168b"
      "cd5ef9a769ad6436aedefdfd5de531860e5b2e28461a6107cbbc17c21456b6c6ab3ad9ea"
      "bd0b8bbaf27b910e70e2ee3e1dffca7597aa6e55699a81944693a7d4b522301e3cf90fdd"
      "d702e5ac0dd5b4b914459ae0a706fe533ed99327d3d1a3c167692218156962107ccd8f9e"
      "a4c8314d18ecfe8b31af4b4a4f38551c308deff92c971093280dd912b9e7f5c93ada08c1"
      "1198d3a61ea81e9f53dded8489d5e539dfa890c7f992e63a824d8bf264bfd07d3f27d1e5"
      "7583b295bf07e165142f0174a685385c7a912688ee523d00691db7a829ccf95495bac44f"
      "0c948f6f0ad2476eb0defd9d5160e3a645ba4b15ceb52bd82d31f19f9274082d2b24e851"
      "5fa0df87146f700e08de41fecbd6371659ecb2a91d098e068ddca50b4e153d448beffaf2"
      "bb059113c21407a4210c80fc8128548c2b0abc76e34bfd525f3ee70a8fb1847e32a13ef0"
      "fb56f8bf10f51d9327431e570b73b0c3d3dbcf29d2861a056196772f0e0ec22f5650893a"
      "8a77ee4196a6c501144cf7fd4998b45bc6e6e4ff436be122c462accbb368aa545df00d3d"
      "443f08290fec370dd5c3d32eccd0f8933ee9949833ef1630535161dff0685f406004d850"
      "24b4df09a9f03fb97b4c1d88393c7d8543a21491c8f78120f581e7c0a65f6660067d15b2"
      "ac4c8b973c9dbf0538a5738d9173cd61a5cd1230a6aecac74ac375082b5f78e219109b25"
      "acd5d353575a483338baee767ca83cbdf2016c099357c9b00adbed0e3ec16f4cf30eed8b"
      "f44e1ffe2c9be54083bcf12d60e4a24e95d4d4330e34564b2667f177cae46232523793cc"
      "a7104ff8f242d650dc6c8a37d6d54ad132f31749b0ebd261cb740fa7fcd7dbc3495e8ec7"
      "050f8da6b03e15bdf31341bd14620f0799baeab8769a58492a905aa904e3520a6b309327"
      "59d9d75dab616224250bf5587ab084b7a5632a0eeb890d9e8fef51781ff279b7561b117b"
      "e0de3bf90fe5e3629b845eca5c18c32793190b2f0ebee7dbce577ba9fd39aa1f54e841e7"
      "379176c226e48f7a15d25e2bc9dddd34e3ca74f7785e47c5a9d737466b64630c20270bc8"
      "90067d2cc09ad258142ec552d3ac363ef3405218c298cd07c5d4d5902ec47eeb35d8bad7"
      "8464648f2a82373640d03a790a5300c6f83963f264a4175c0b1e02de54aa2bf48c862591"
      "fac4cf5c325c4f2af9c4c04d4853569a924d9d361c63a8aede2e6042a59cc1192178bb1a"
      "4e86346b74924e8ba1c1ad8b8f3dff4f16ae672740060fccd0d81f6e9cb8cd8c205ffef1"
      "5c0b123216f8d91b7b2150ba17a63161ba221ade481025588b6bc21ff7ab71527abda0de"
      "85309f1ae0c0bd5b3f212fb7cf81283ed83a1c7b9eaa9632e6b1311da9f5c6f9b84841e5"
      "57829f97315172f4b0efbd5bb2806b50d6967e35f0d8f8d7793b60deefd87bf063b28ffa"
      "3f7a7f4ec26d2cddb4f08e6f1581fb49c884a747b13b415e44fcee44190c168c3a85517d"
      "55e170577e5988adfebc435b219852750beddafcba3f87e4b2b3c7ab10dba14475e13fff"
      "7e1f969225a6d069bf9e84044983b4b689712b1016cbc03a13d52aab204561d07550d914"
      "e28432f342db6d9af0c6acc8014de7a2fca22425293155c8d5375db6288b549be929a490"
      "729243aa909e9bc131dd29e6cac1048c13a3c290138ea44acb1696a6b1ad9f59a792170f"
      "fa31adf935fa01477cbfedf421c8b53a7ec51325de72818acd81609ed24f99989d4f8534"
      "7015f64b2a49d5b6310f851d367aecc11a0c37c48633eab02a9602c6afc243cb3b440dc8"
      "7d64377a12cc9c264bc7bd0d1ad1a82e72522be84a6ee11c254455b3c4ad9f92ad6b4cf1"
      "1c0d1c8269e753c27904c4276ba9e4f6b8340ab102721c7b3f3d3546a1acc4981868828f"
      "6de93365b507f3b75acf7fbf87fb627a15068732c10f259f53509b4a2c05d7f0b9d5618f"
      "beedd838daa54c177ed5424a14e21b39b5e639fbba1bc92937a64bdce30f1c36257203c9"
      "377b39b958112012d4c40cec4e9580bbc8285723193d7f3b082014d8265aa59e5f2779b2"
      "6618688871da749340b8dcc21beb915208af79c9be0f71e8734dae659dc89636070a9b2e"
      "325d1b2e97cdecccdda809b83ee02187e06ba1764223cad8832f4df10ed6b493b33765db"
      "47485ebde5214ee73a6ec581a4fa49e32c386111fac72dd7fe927497ebf5e76d9b293b1e"
      "21387aaa17d73a57d7b257ad00b9bdf9c51419f5c92869b481da721c5d7602c3dadb7898"
      "04a6821cc39d9b10411b127c4c5fd476334eb13d89dd52e49ef487ddd94ad8382c19fec5"
      "9c79143cdb97411980d83ac2c3db2629b2f298e8383b839fb91fcdf81b596ecad11be6e5"
      "e799f8d92464d395d44086761e0f04770fdab5a1c326408713fafe3fc15c13b998fe16aa"
      "7f1928d29ecad76063186870923580b44974e2282f99c961a4cb8c48160eb644efdbbec9"
      "08cb8be7ae3353a952b6f88dbdea292d388e5500541e328bc5d151a87aaff1ae3e90c36d"
      "9bd03bc6cc1734984437085221b7a2ce16c1dbb52e6384d2576560e0c78715a54ab29b29"
      "66d8eee5c03b08fa0358e89e655efe3586d73d82c8a85e88647a34bb76f672f659a95af0"
      "bca3a77f1c81d2fffc4578271e4526b3b86db5a07726eb4ca4eb69932ca38f8d2b0c22d7"
      "349012bfc9e3d4507df0f0b293c4955e1d5ffe4410d98525424a4a0ef31399d002e71857"
      "cc7898f25a04c341b14cf059216410a441e4876752416dba4d1ebe5b15db2199126dbdce"
      "b64819aa7b0aaf69c12f1ffe30cbcf750bb3055a24303fa93ad289631f1b29ab6cf806ad"
      "e6f4adfbf99a0b7c62e50ef2a6735070531ea626026afdeb3176fdb50011d04e9fe5520b"
      "86088935e5f91dd7f13eb3b8b45af20110d907b2a45e9ca726126a79a98c29de61fabccf"
      "af78289f72b4b084dd6c973f082f7a04189cc44f540adacc2b1c1c448c69815f9f2c4e77"
      "f9a0965dd88c7b87028b764a8f6eb16b0569bd72b0028a7211da1521971dbab6e3d66959"
      "c7ec0ed70922c12d26edc14fc1c7ff68936f6d8e86463e8619c5f1033b16566a27d118cc"
      "25f0a032616ae9d91b50c50d6663d50b0aa6f77232670f62829490c68322f01c3a026baf"
      "d3438fa26ca7b1b062f9820c92a31c058413f183a0ca01547aee6d813d4feea8b66285aa"
      "543e3232f6c2f701efabf434f8498944feae42a29b7c3df72d9c5d9d096565a4b3d3f514"
      "8b08a9a2d6153620c5227368e596c5decc3d63691e5319bdefe4dd1362897cbd192425fd"
      "861c70560243f8c0924509c30d8ea5f100a7639167cef87a3e8a0bdca2176d620d2f4766"
      "dba782c1530543b5f7906b4d364a43775aebce9d9e011d850bd646d2e6f2439123f3afd0"
      "9caa358df7d8df892b77501b127ddd63035eeda62056b066e87f560b4e3d18c2a54392ab"
      "14972b01136ac32b5e543bfdf1eee5be1d89235f5433266ff64c21daeed62bb0d8de1c66"
      "1a0d00f59732fccc51efb0650877e39613b238738281e768597962c44341b9dc3ee326fb"
      "3a468d0d19c0381951d8de89bce562bcfcea036bca0ce778b4a093f1114a30833c610563"
      "195cce6c62ddda2e469653234049c6bb7ba5698c4741b3b32febc850b23ea9dd5e6da58a"
      "dd1eb381e23e16e02f2dab9b888e2377fff650d92833e86a9cce46d6ba229bb096afbcc1"
      "787803bd8e1c87fcccef5d9acdda9a350dd3570142ec351171d01642a582cab4b473395b"
      "e349e41660a346558d14aaa622013cc4cc7290b820d1293c9011e90f5fac1c55feff0f50"
      "e3c06d8424e6b2ea05613a2e76d0a6bab75aea72768c839516cc56c550e88d18f0f046d1"
      "6870ce5c3beca0fb140bc37fa71e417a82b724537af9f36c4631f42c491e83a6509d7c58"
      "39094b9f5b1e937835480eb9a264e1b8d1b456ad6392e2344d1b7edd7decd16421b84ecd"
      "238f9773d06cb71948c93a479c9841a4d9b98bcba9bd63b49f4b70193046a4ab899cc232"
      "800b4916269ec95294747194ea3bfba5e7dabcfb44b9b08010aedc4acbaec7b5ad48ac49"
      "9585ed27f597976b310aa50e91af5772c9df21c031fcaaf4a40ff771bdc2a53bc1a89f5a"
      "e6c979d6aa622f98deede470b4c464fd36b122dbf1557c7fa9dac7e5cfe045e0e9600254"
      "2716b6f58bc84c7ea22b97e1136de913db947c80bc4931c0d3817fb714bb36250a0421f2"
      "67da1b5073ce0e781c660ff553869bbd69788a8e6cd3f1315df28a7648ea32f1c3011390"
      "ea9d416b13e7f8702158cd50ce9d8d2add88ca9e1e70dd301c797981dbe005a1427be180"
      "00d6902ca596c1412ac412dbce32a0a96fb1ed775abf794e76931712e8d23a7c135b5708"
      "b1f525adc45e9e229814ed45e5173dd0a795d56d3c749748c7e7fe97b784d51070627451"
      "8013d4ee72acf8e88e8bad08227fde5485d7118522e7455fb193cef5306605abb2615b3d"
      "2e2fd1ce9076a96b18bee1a104349ae11477d16c95bfa6b36c072c04281bf8508b4c85cd"
      "fdabd61932dfd5a942694fa2dc31bd27903195b6fbea06b23dd86f99c48c78fad8135ce3"
      "08e4c3ed171668539a45deef83ca3c71d20f3d2d4b6d768dbe63bb4232a3afa494f7e2f9"
      "8ef667f38cdfb6a213aad7ad3cb733a945e1d2e8af7452d2869f2c8930a19cf5a80dccfc"
      "62cda9888fc51cc6145afba8a9b10cf4170def45b7fe9b8f5a2ddbb7859eef215fc5b2f1"
      "8551fcd7a4b4235efb11efe5a893decdb1177c98995f240a829e4ec11d7180e7910a7ffb"
      "ec4058c2f74a6ad80ba79a8126a654df75485fe9469e074d5c61e050a1172c385ada6729"
      "387b380fd3c15809617b7ebc1a07566fbd903b6e56ac951982e629e581d60395c62add11"
      "252d87de43bc5e18799d932f03f043e7013d715086806a6b96d3a104c293c7082ab972d6"
      "b500313331ba7776d8a2ed7ff8c151490ef4f1697ece816352908859865e6b75ae27830d"
      "c4902d822b51136c4bc299921dcc6bd51dd22864d90e42fcd2df03a0ac65d426cbc9e939"
      "6091aab5bf91e85c25e3a4a000bf990a0afe4993b53015b47249a14746b9573d89d77695"
      "b164e79704ab793d64553714c35664ebdfa83665257014559fed5cadfb6a9861f12375b6"
      "2ab4b923ac37915ca4db52a8b93bac78852f580ab0f920f5fe71e33e878e905811243e54"
      "d1268d817e08e90f92147387b57c4ffcb27a04e0fe9d1cf7eac70fcd164e739333098c53"
      "707281a0e43f5e29d8d0c03f2a4642707ff93a47361a5b178fad56bd12dea5fe3eb66b12"
      "a2213e10169eea4b465bf71f1c457db36e011248b67958f3a4c7def635fa88ab74ae1a2b"
      "75b546e9cbcbfc72506757fd2523212603b7ee456f0e4317647023915aae01ba299fe89f"
      "1c2886f617a70fc0f51877af00b44c9c6f18a664a738a29ff62abad692d9828fbe4bea10"
      "c9f9b02678ffac04982a17bd6294f5154e1ccf4fbbe64930aeb61b0a93c5107d3aef2a4b"
      "f33ae9ed1bd2eb2df6219598a92720c82fbb051d7e3582473a083736274d46b2201ba9a3"
      "303367794c9853b4d5d10dde38479960e11015cc3e75b01e50c6f780fccbe86239ea3226"
      "f57438f7a7252812a609260bd1a7b692ed75a16966b899c95e100d4e357c720f11b6c9b1"
      "a0cc1ee2786fcff9586d891fd0748430170aeba911fc42a632",
      ToHex(captured_data));
}

// https://3xpl.com/zcash/transaction/1f4ca7c4b182620861632c9c35398c5f61a846e783070a1ebe30941b6f55ab78
TEST_F(ZCashWalletServiceUnitTest, MAYBE_SendShieldedFunds) {
  // Creating authorized orchard bundle may take a time
  base::test::ScopedRunLoopTimeout specific_timeout(FROM_HERE,
                                                    base::Minutes(1));
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndEnableFeatureWithParameters(
      features::kBraveWalletZCashFeature,
      {{"zcash_shielded_transactions_enabled", "true"}});

  ON_CALL(zcash_rpc(), GetLightdInfo(_, _))
      .WillByDefault([&](const std::string& chain_id,
                         ZCashRpc::GetLightdInfoCallback callback) {
        auto response = zcash::mojom::LightdInfo::New("C8E71055");
        std::move(callback).Run(std::move(response));
      });

  ON_CALL(mock_orchard_sync_state(), GetSpendableNotes(_, _))
      .WillByDefault(
          [&](const mojom::AccountIdPtr& account_id,
              const OrchardAddrRawPart& internal_addr) {
            OrchardSyncState::SpendableNotesBundle spendable_notes_bundle;
            OrchardNote note;
            base::span(note.addr).copy_from(*PrefixedHexStringToBytes(
                "0x7d42765851f3db7cfc87a0b9052a579207424fa8c56e3dae8abbf7a6705c"
                "805f035dda5e793ed48e97bd0f"));
            note.block_id = 2763539u;
            base::span(note.nullifier)
                .copy_from(*PrefixedHexStringToBytes(
                    "0x14aeb32cc0a16a9d242875b7001e8421a7585980b6186f05a73bbd45"
                    "2855b43a"));
            note.amount = 80000u;
            note.orchard_commitment_tree_position = 48973018u;
            base::span(note.rho).copy_from(
                *PrefixedHexStringToBytes("0xdbce238f803b249b7171cf406cd8dae5a0"
                                          "c1268a7771c9984ee4a0b17619bd0b"));
            base::span(note.seed).copy_from(
                *PrefixedHexStringToBytes("0x8bca3a0d5845270f9d5636f51fc285a653"
                                          "8506746a2507338cbc12ce9d40d119"));
            spendable_notes_bundle.spendable_notes.push_back(std::move(note));
            spendable_notes_bundle.anchor_block_id = 2765375u;
            return spendable_notes_bundle;
          });
  ON_CALL(mock_orchard_sync_state(), CalculateWitnessForCheckpoint(_, _, _))
      .WillByDefault(
          [&](const mojom::AccountIdPtr& account_id,
              const std::vector<OrchardInput>& notes,
              uint32_t checkpoint_position) {
            std::vector<OrchardInput> notes_with_witness = notes;
            OrchardNoteWitness witness;
            AppendMerklePath(witness,
                             "0x69ccc30028f8f48014b90c8df8d383128ee5683db98b86e"
                             "24b64830fcc32c51e");
            AppendMerklePath(witness,
                             "0x3013cca510315350e4000c72f50de9910896d9a68dafbe0"
                             "145627a76fc258514");
            AppendMerklePath(witness,
                             "0x7dfb668f84823c7196fc26a6e7664f9381c6df4de6af26e"
                             "5d5e34f2d838b0e2d");
            AppendMerklePath(witness,
                             "0x58d8d8690b5faa152c41a73aca8a2abf58d17616016b44a"
                             "85bcf993f193c4612");
            AppendMerklePath(witness,
                             "0x997ecea424a5d0eb7b2b2cc8b779525c22c024fca7ce3ef"
                             "675d7957ecadfe620");
            AppendMerklePath(witness,
                             "0x17cb5b1766780f0de0259b643a05093c21bd77982c609d2"
                             "55260b2869031d935");
            AppendMerklePath(witness,
                             "0x4b9ab554ad736be6ccc309694f49d01825251aa376d6cef"
                             "cf1725fbbebdb0c0a");
            AppendMerklePath(witness,
                             "0x5e8be723d9ceb74734eb5356ca66eceaf1a86c4b17a1284"
                             "cfcaa07b7d2691026");
            AppendMerklePath(witness,
                             "0xac6666cddd0b823ee8287ce1c5b47757463e8130b27bfa9"
                             "dcad48884eadcfd1e");
            AppendMerklePath(witness,
                             "0x4a4b16c8ac3b4211d33f5de74c90fb339739597048576c5"
                             "8f659572e606e0f2e");
            AppendMerklePath(witness,
                             "0x75265f7a40040cc3849b2c762386e714703ac5bc63d6049"
                             "3750833589cb2b110");
            AppendMerklePath(witness,
                             "0xd4f485cdc4cb25dec06db85e236ce91a09bc3f0b645cfff"
                             "74c0cb72e2283d23b");
            AppendMerklePath(witness,
                             "0x22ae2800cb93abe63b70c172de70362d9830e5380039888"
                             "4a7a64ff68ed99e0b");
            AppendMerklePath(witness,
                             "0x187110d92672c24cedb0979cdfc917a6053b310d145c031"
                             "c7292bb1d65b7661b");
            AppendMerklePath(witness,
                             "0x0f00a6eccdd778c92fa1940b57175068e04a57b96ad730d"
                             "b163a948a42baf22c");
            AppendMerklePath(witness,
                             "0x63f8dbd10df936f1734973e0b3bd25f4ed440566c923085"
                             "903f696bc6347ec0f");
            AppendMerklePath(witness,
                             "0x6f3f63aab58e63b6449583df5658a91972a20291c6311b5"
                             "b3e5240aff8d7d002");
            AppendMerklePath(witness,
                             "0x12278dfeae9949f887b70ae81e084f8897a5054627acef3"
                             "efd01c8b29793d522");
            AppendMerklePath(witness,
                             "0xca2ced953b7fb95e3ba986333da9e69cd355223c9297310"
                             "94b6c2174c7638d2e");
            AppendMerklePath(witness,
                             "0x60040850b766b126a2b4843fcdfdffa5d5cab3f53bc860a"
                             "3bef68958b5f06617");
            AppendMerklePath(witness,
                             "0x7097b04c2aa045a0deffcaca41c5ac92e694466578f5909"
                             "e72bb78d33310f705");
            AppendMerklePath(witness,
                             "0xcc2dcaa338b312112db04b435a706d63244dd435238f0aa"
                             "1e9e1598d35470810");
            AppendMerklePath(witness,
                             "0x2dcc4273c8a0ed2337ecf7879380a07e7d427c7f9d82e53"
                             "8002bd1442978402c");
            AppendMerklePath(witness,
                             "0xdaf63debf5b40df902dae98dadc029f281474d190cddece"
                             "f1b10653248a23415");
            AppendMerklePath(witness,
                             "0x1f91982912012669f74d0cfa1030ff37b152324e5b8346b"
                             "3335a0aaeb63a0a2d");
            AppendMerklePath(witness,
                             "0xe2bca6a8d987d668defba89dc082196a922634ed88e065c"
                             "669e526bb8815ee1b");
            AppendMerklePath(witness,
                             "0xe8ae2ad91d463bab75ee941d33cc5817b613c63cda943a4"
                             "c07f600591b088a25");
            AppendMerklePath(witness,
                             "0xd53fdee371cef596766823f4a518a583b1158243afe8970"
                             "0f0da76da46d0060f");
            AppendMerklePath(witness,
                             "0x15d2444cefe7914c9a61e829c730eceb216288fee825f6b"
                             "3b6298f6f6b6bd62e");
            AppendMerklePath(witness,
                             "0x4c57a617a0aa10ea7a83aa6b6b0ed685b6a3d9e5b8fd14f"
                             "56cdc18021b12253f");
            AppendMerklePath(witness,
                             "0x3fd4915c19bd831a7920be55d969b2ac23359e2559da77d"
                             "e2373f06ca014ba27");
            AppendMerklePath(witness,
                             "0x87d063cd07ee4944222b7762840eb94c688bec743fa8bdf"
                             "7715c8fe29f104c2a");
            witness.position = 48973018u;
            notes_with_witness[0].witness = std::move(witness);
            return base::ok(notes_with_witness);
          });

  base::SequenceBound<MockOrchardSyncStateProxy> overrided_sync_state(
      base::ThreadPool::CreateSequencedTaskRunner({base::MayBlock()}),
      db_path(), &mock_orchard_sync_state());

  OverrideSyncStateForTesting(std::move(overrided_sync_state));

  keyring_service()->Reset();
  keyring_service()->RestoreWallet(kMnemonicGalleryEqual, kTestWalletPassword,
                                   false, base::DoNothing());
  OrchardBundleManager::OverrideRandomSeedForTesting(985321);
  auto account =
      GetAccountUtils().EnsureAccount(mojom::KeyringId::kZCashMainnet, 6);
  auto account_id = account->account_id.Clone();
  ON_CALL(zcash_rpc(), GetLatestBlock(_, _))
      .WillByDefault(
          [&](const std::string& chain_id,
              ZCashRpc::GetLatestBlockCallback callback) {
            auto response = zcash::mojom::BlockID::New(
                2769076u,
                *PrefixedHexStringToBytes("0x000000000003b74b5acfd73e70b1a8e8de"
                                          "5f5557e560d524037cf40d3190a804"));
            std::move(callback).Run(std::move(response));
          });

  ON_CALL(zcash_rpc(), GetTreeState(_, _, _))
      .WillByDefault(
          [&](const std::string& chain_id, zcash::mojom::BlockIDPtr block_id,
              ZCashRpc::GetTreeStateCallback callback) {
            auto tree_state = zcash::mojom::TreeState::New(
                "main" /* network */, 2765375u /* height */,
                "000000000112b1bc454c12ba83f30ffbd7a345058025b563c448191174d65d"
                "37" /* hash */,
                1735298875u /* time */,
                "01648bd00a4d5db427655c678d9c26a59e09abcc8c8ef9a7f2a1e297517e70"
                "1257001f017d536e6b7eabbdef129ca34437c8d034beced186c35f5f66cd77"
                "7c64c2ca9b3a016efbc1e91cbd6995a31c5b0897983ab9b2b96c17c662c71f"
                "5fdf65aeade92d14017b331d294c0e337adcd246b21374e64b910cfc517080"
                "09d0e0debe917f15ef03015f7f9d6a0e4a9dc8fba2dee53f1bae480c00c7c8"
                "2cfafa40517f654dbd10d60e01b7bc0e9de66553b16e8232cea0234426708a"
                "5c69579fb86742d0654e812acf5e019fda3522805f03cf00697404f2dac8a6"
                "9c9311ac6acb1f1fc9ca80f8f1012830011cd5b744f9cbf8714ba0eaab9bdc"
                "8cb4e30b7cf3ea8207bf955da49be3c3c9230158722d8666ffd6429f8dac66"
                "8a521406a8c88292f7f55cb15d93919af7d9f655011a932b1d84e39121c34f"
                "316343c12c4d0ff98ca14aea2b445fc86e4f252e773d0001a6ab94f6c08c3f"
                "5631fbff992a0ad6060a974774bb0aaab45feae601b6d20a3e01381db5696e"
                "399cba5c84b9798fc00081c67622074ef6aabeedc8e0dd7478c334017b8476"
                "7ca227e22f8192b542c0688d413316e34c2059335311b1188a9c0e9c640159"
                "ce19fb2d0c0b844f38a48379313529a5a08c150fc62e174d17b00c99d54d0f"
                "018de4b3e39611e40a92dc0e1214110e088110ac37dc21f3f1d9f7a449447f"
                "6a700000017d1ce2f0839bdbf1bad7ae37f845e7fe2116e0c1197536bfbad5"
                "49f3876c3c590000013e2598f743726006b8de42476ed56a55a75629a7b82e"
                "430c4e7c101a69e9b02a011619f99023a69bb647eab2d2aa1a73c3673c74bb"
                "033c3c4930eacda19e6fd93b0000000160272b134ca494b602137d89e528c7"
                "51c06d3ef4a87a45f33af343c15060cc1e0000000000",
                "0186d941a3b8736a930cb42ba0f5401ecf11ed23d110439c94830215614080"
                "4f25001f013797643dbbf53668419a775bcb9e952d48c9e1b3b87dba09246d"
                "1151a94bfa2f01c41cd6f8852d94aa243c8c09123c5fd0272333dc50c5909e"
                "2ec3e31e50226e3d014244cfb4e26114a03190f5a6c98a972b6236a1609e94"
                "b71ee4d34fd61f2e5e2e00017a00225e7a25a7acc77283bf8bffb2ff4b71ec"
                "bca92810c27f15cf9840e1a312000001a8ab4951a8a3ca00d0fb151a18f130"
                "d616a8943849f96e10ab77c3021e6f92250000018c7625575f2bb3a042b706"
                "61de0b01be0795b6590a9f21c504e86acdde863d3a0000010f00a6eccdd778"
                "c92fa1940b57175068e04a57b96ad730db163a948a42baf22c00016f3f63aa"
                "b58e63b6449583df5658a91972a20291c6311b5b3e5240aff8d7d002011227"
                "8dfeae9949f887b70ae81e084f8897a5054627acef3efd01c8b29793d52200"
                "0160040850b766b126a2b4843fcdfdffa5d5cab3f53bc860a3bef68958b5f0"
                "66170001cc2dcaa338b312112db04b435a706d63244dd435238f0aa1e9e159"
                "8d35470810012dcc4273c8a0ed2337ecf7879380a07e7d427c7f9d82e53800"
                "2bd1442978402c01daf63debf5b40df902dae98dadc029f281474d190cddec"
                "ef1b10653248a234150001e2bca6a8d987d668defba89dc082196a922634ed"
                "88e065c669e526bb8815ee1b000000000000");
            std::move(callback).Run(std::move(tree_state));
          });

  std::optional<ZCashTransaction> created_transaction;
  base::MockCallback<ZCashWalletService::CreateTransactionCallback>
      create_transaction_callback;
  EXPECT_CALL(create_transaction_callback, Run(_))
      .WillOnce([&](base::expected<ZCashTransaction, std::string> tx) {
        created_transaction = tx.value();
      });

  zcash_wallet_service_->CreateOrchardToOrchardTransaction(
      account_id.Clone(),
      "u19hwdcqxhkapje2p0744gq96parewuffyeg0kg3q3taq040zwqh2wxjwyxzs6l9dulzuap4"
      "3ya7mq7q3mu2hjafzlwylvystjlc6n294emxww9xm8qn6tcldqkq4k9ccsqzmjeqk9ypkss5"
      "72ut324nmxke666jm8lhkpt85gzq58d50rfnd7wufke8jjhc3lhswxrdr57ah42xckh2j",
      10000u, std::nullopt, create_transaction_callback.Get());
  task_environment_.RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&create_transaction_callback);

  std::vector<uint8_t> captured_data;
  EXPECT_CALL(zcash_rpc(), SendTransaction(_, _, _))
      .WillOnce([&](const std::string& chain_id, base::span<const uint8_t> data,
                    ZCashRpc::SendTransactionCallback callback) {
        captured_data = std::vector<uint8_t>(data.begin(), data.end());
        zcash::mojom::SendResponsePtr response =
            zcash::mojom::SendResponse::New();
        response->error_code = 0;
        std::move(callback).Run(std::move(response));
      });

  base::MockCallback<ZCashWalletService::SignAndPostTransactionCallback>
      sign_callback;

  zcash_wallet_service_->SignAndPostTransaction(
      account_id.Clone(), std::move(created_transaction.value()),
      sign_callback.Get());
  task_environment_.RunUntilIdle();

  EXPECT_EQ(
      "0x050000800a27a7265510e7c8b4402a00c8402a000000000002cd070549e491a0df4915"
      "225357413a184ab90d71e4898b5ca7f875df5326d206f82b81e69c77e1aa9e205cde3ea4"
      "ec2012e7ddbbbce9a6175d85c1b563c33114aa330ebcde9b657ad9ae243ee70996d11b78"
      "cfbe5149ab5ab7156320bc90e3afbb9756c1154de1acb4cba5240b551cc40715bc56e4f7"
      "aa9943ed22662c1d591cfe10eb5336fa9de6fe6ea1d717bcd2338b3c2cfcac82d49134e8"
      "e6f45594b72662f3788511b52fe573730a695a0a8fa83c0104e07ea70fbd322b029d75ee"
      "6d35b10995fad40a9032839e196e3da02f6494a5a618412edc87edf1c5c178a0e021bea7"
      "15bbe897b6081ebcec10eeb56a53a3ad57d820a67fe5da08adcd265ae96cc0a9e9bbd911"
      "ab0fcb6220d19564d7554116cd4c4db477644f2c43d80f8d08e93aedfdf9f4bad1047a3f"
      "574dac8901c90b3b6c43d99fa8d86816eb839d3c354397d61258ab01eadfdfb395018818"
      "5d257d1a88f776683e8a596dffef3c715f15aac436eb83b15e6220c58e78427a5f8a30c6"
      "613e7a6ba0eb7a0f4150b74320adfcf757396fe15d1dac8b416dcdfd14e8e0c8df006043"
      "66b7d7ffcfaeb84a758e590031cdae473aab7dd960bae73cfd6e288950c593a2cdcdec95"
      "6cbf1bbcae306b5c67fbb69a9d2c87e7f40bb8791f673eba3c3ffd157501d15935a5876a"
      "15c732d2796ecb04ee8c86b5adf0b225eaa29e6f77cf7dca359a3be0d8cd9c4cbf7c957b"
      "4144bcb54fd9ebdd977887a41f0eca5f3ce9e0f909612e843990cd8607be7fe1c30f1929"
      "4ccffd3de4518b062076d227627103deda4c2e629832c96f9c264a5920c9dd9694d72ec2"
      "383f9f12e9334b9790462067e5c7e7ea79d08fea0684e84a57b933e370d79125f6b9b548"
      "e028cb36d4c625b29ab3054b7866caef983a9bcdebde9473426158459985b22d6a236644"
      "1b2be20dfbf545eda39a42da52a490ed05512f760fcc431d9af350eaadd737bdebe453e1"
      "0983e36a83270b7774e445f01ce5bf70a649ffe686664801f3f9fb1a3c59ca27964780a9"
      "f121457a7d0a4908b4fe87f0a0efe5b68f2e6d8e1c22d5c107cdd366c43a32ee6e166fa2"
      "b654e382c8c9409eca8cb3185509a327a67f82e992bd3ed8a0285dd7d517daabe703959f"
      "905aec059ac1aafaa08f5ce277bd1ed7f5a14f990c7437812ad7ff9f011cf5dbf7b4eb04"
      "d8179fb67e39841f56556ec42cbd14aeb32cc0a16a9d242875b7001e8421a7585980b618"
      "6f05a73bbd452855b43a85a5bb03cd3233ba5bff0675c05374565d06d2f824d07961d610"
      "d2137beeaf97bd69503da07b4c66af2d1b9beebc24d98ef7296e01f057f62257cb924685"
      "d51229065505e626bf218b005c2efce29359b280f1178521ddec699b22d40cdab39be67e"
      "9c9998a596c1b7539ed3c0777555474bd0113fe3cac6fd5e5af5351bd4337c7ee1edf23e"
      "0836ad89549b289a8680cd7f0320b371fa42f53003d6ee8ffc060114f21dfc15ba94320b"
      "ac087de256f44ca96ec11aebcb2a9bc1b27275601c94b59a70465c99544c1aafcbc3c939"
      "680cfed2990aa2ead83b81d1888ed6976690d6a68192b48df45e81beb3e222e3867088a5"
      "2dcc7e6c3086841a21ba30fc22efa13a86cddf5994c3904cad4258db86d9783827b08dd0"
      "5e5d1beaeb609808aa856d89f2b19c0717f29826cf0c442ddc59b0308b9c341f3ef747c0"
      "fe2bbc84354d14b3bad841fb585e1cd9541af304c7f41463e427430a82d04089fc9bd13e"
      "cedde94bf4ba11ef878ac7fa0b512f1edf002e1b1c85d06e8be3f4fba106d338c31400d9"
      "cc378c8881f600e39b324714afcec825c7cdbef40928746b02895cf6acc8bba36acf97d7"
      "1aa853bf363311295d46e319f457171b133427b130ab418bc40d945c032cdad4b9f046f1"
      "82289132b3cd363ee99dacb5e89f2ce963e380a51b09c2f3ffe171fa4c1282561d9d716d"
      "66e0f6cf1ad050e3cb20567e25639bcdce1bc867d7d1c5f5ad702b567f3105955628d37b"
      "71e337229f69983a2e233be753163224eab3d04ba35ad17e7bf91cd5128919e52bc8ab47"
      "5078fb60aa64e475f9e1e4d35b519044c5b1cc5fcd1d7c84512060ca5c4b2f01d744dd35"
      "ea4a4cec610b0553f44c1cd7bca3a4a67c82e66ea23a6f56927a4abe28ab9e6a5ec09731"
      "71318c27aae5f7048e9d2c84195e74a3f647027288f3ce49e1ac48be4467881e36c0d82d"
      "76d4c6ec95034423f98adbccb208b6ab2d0e2da9003c622526ae46b6c7ea3bb0a56f9425"
      "992792f4ae238c6ba510937e65e727fec71241469475100b060f7e01a03e6766f0489aca"
      "173bfe025274aba1124003102700000000000058706d7f3ef746a43c4d07b29c2ea87e70"
      "78c972b461c72a591b7a633e23e12efd601c657f539571a668851512c305556a14e47d02"
      "f84e655d5f6a6fabe2addbe6730e2a9e53b1515cb118d28d91871d6d27769f9f651bd2c0"
      "b20a883384711077d004471b8bf7dca00fda9f1a379c1a199e0306754c89837aa2206479"
      "3287d5e5730cff5ea24ebf1329b3789449fb1557634cd86b7777d2921a81d916700e7bd1"
      "d596c20ee021c3ca5e94ddc07bf80ab5d184dd0ebce2aaa936356e8b6babc6f2743d95b7"
      "d06cb5eb8e24fceec3a304c1dec5a054dbbd6ce4a9a43d731830e86a699f393dfbd9966a"
      "fbb3b3db66eb895c07f221d897a1c7e56a239c8798af5bf2db0bd27b2efd73219f913a21"
      "6e538d1ba91962f5f33b77c5978600fb90b281221aaa1009143b326aa07e6d8cdca3253a"
      "f33aec42d20f4f27fc74b535112b1c14751e1e56c3648e778b11264174f06e4bd09dcd3c"
      "fbd9b7ac02d689cd166ef65f5f977d54302bd4e9eb222da6433ccddcb9a7fe04f1c09a9c"
      "c5153393c6fd985edeaba2e04be7f26eb2fcf93037b97380be9ac790962d944618530f3e"
      "8383a84327a5cf3c6e140110f9548fd427729500f27a90c835258a044a5bb47dfc2fc540"
      "0da3593f9b4269b5897663322b7f7779f3e0de2943f284308bc9f2ebe1a7e4755221c724"
      "2fd04bb59e775a5d7e94d681a49330d4698dc6c1f449679439aa8478ea1dc3663fcc0650"
      "0c8aa2a33309d0ffd51db1a8710240594f816584257d8de112367eeed7b65402a1750f88"
      "6b1cc27904c18f8b60acef03706085df1ddc5efb44b8535596d49ef5cc3ac43b299d53e2"
      "973b2601c9a0204c829d681fc489c9c3e8ba91530b6060117ce1b0b4e3f5cea50b36e0b3"
      "96c2bfe1b777aad4f74a35288ea88aa627ae8d394023a5efb59b9c5110c01b6e26d09239"
      "66588bf82342d0a91f3d30916c5a2eb02f367ff33b4f325349d94229362519d3ef452ce6"
      "699be6db95ad1521f025483613fe437fb38bf9b780be49bb9df93a768091a3ef210e57d9"
      "4f30bf8dab58d01757461470a36a580c876a879d54516781847ba90691de7432822379bd"
      "662b437b1d74409f06f4dbff1064d0c90cec4ccad5becb8f611f127846253f712ef30f21"
      "ea1395b630d2f3397e9141eab8a0ecd327e5970c87308464101a72ddbc7629d37c829167"
      "673fcc19f063313e67e4d70b19e497f6d2baa1c48f2193e41cf3201e6ba3ad53ebd23cb7"
      "9294ef4a32120eb460e8aaace6775e0fe9922b6e61b018e91129728c54e7673801e92e41"
      "c38aa834aefa8b8ac66f45e0b5a77c91f77a81c6678b78742011e65d50eef059c3ceae1f"
      "5ffcdb51fabd9a91bc1c2a4060b699e0d9aba13f32218267ef0ebbc822d520765dbd21ca"
      "44ead38581228480e8a8028552117ed9c00dce5cc423c1f7b24619a5e7823167f84b8b5e"
      "22a8b0b6207a165ea35a2338ba7f5b33d5d08bf572b6cda42e0be6e3d7b2e978ba9a92f1"
      "57b75f5fa1c7ae26424cd8fbe53bb12b7bfc5917727f58b12e65695536ba53e5760362f5"
      "b106a30109bad91fbca259fb7b1d1f8c1824736e50d8e1a8a9046989d50d2aef68dbb413"
      "d246830f6214b20cdfa030025c8065835fdfaf87ba9a946cb94d6ef6002e609da9db1630"
      "e5d3e9b005eebd041f8c2f235ec58faea9ac846bb6510625b80241749ded3022b1d1e925"
      "c564e95e4fb0cc10354fdb5fe0382af65bd57d9d29d97e09ea5a1d4b972ea2e33e12fe62"
      "fae549a2d1e3c2abd6015d65e31663055b4ecae6320a60e18470d258c9f56b3fa191803b"
      "348ed3de1f936fbe7fd7ad6c5a92a52731e25f5fa6dd9a7cd9e7b48915cf500e257b072d"
      "b03f010d230893dda3a22f818d928ea7030376730fd5b8eeca83267ddb2537671da89602"
      "57de704b8e1fb02fae21d6ed1624bf1e86578d7c0317c7883c1b02870c818ec1e05d264e"
      "dcda97e4a31d130b6ff0c2a2aeb96b2ae44d9b98e4a01253b98faaa718507fa2c4b1dd1f"
      "0b02f7e213399540d4346c5b49f4d982ee2272a6ec9382c2c4c2836016820acdb8b12097"
      "f3d8010da39e4fc09cc7a7b591c475fa29b70382be699636bd1cb7a4aeced6b4e6601337"
      "846c6eeb087568e750e2aa44ad20fa301297f9cbcddbab48d2b21c814de083f13cac5723"
      "7a135d220c03edd95d8eec4157672501b8b06043749defb753742fc518083b6325e6c462"
      "7500dcd3082cedcce004e656b2aa6b8ccdd832f62aa4be3263ad2a816991e1fedfd55c7d"
      "fe0a61c541aa6e59dbacbc767ea3ab2bcd1463dae9094defb33f148263dcdddbba84fb7c"
      "5a901d59bf0ff07902be10fc3b59b7ac3a601381c065ff27c93a39ecdabc2cc5f3e9dfc2"
      "acb36fa70f4af19e6d84c536f09ba627965d03d7e2ff9b096c917a695eb38368d4db2672"
      "ebc2f39e4924de7682eb7aa31e4cf2db336483a09091b68b01e723cba30b98887e22b907"
      "a9641f9660fc6587a403816fbc3b39c1de2b036fd5f9ef860a011f33a0dd3ddfeb1619fd"
      "e98081102d3907c5dca1d1464d23a83607f7c14c89ecdad0d9edae318ef466795978b631"
      "b6115620896cc0a42c398934111a705e957b6eb33f6dda6e0c6f04632158cf9b368eee7a"
      "c0f00fd3cd3bd472271a285cec4bc335f86938a3ce08efaa965cdd0d62dc02e03a05bcca"
      "e723e032ed9435fbeb69c88f2aba3045a60446800a9a0df77bd4bac900e500fdeb04a13f"
      "adc4bc98f5138fafdbf231b4f52ab6e23c2c41574594539b36c1ca24bb2f5955a0c83bcb"
      "9ca0978fa22d241d59511894ff7deaeaf4f57b0a704d3d14b9065e5f576e061b999a6708"
      "14db73c947dcbcc71ff8487d6fa03239383006001f313fb86697ddea300860d9c2e48f26"
      "dd7254e1de955a3ca08ba8be35c05b70f72a02910a49b14e389b337a4f6b761c924fdd56"
      "0218490b7b7ab95e8cb7ab57a70a42ccb910695af50cb2c5356548dabaa7c6d5c47de6df"
      "785a9805e17f5410892247cce1aa814b3ba0318ce75f89fd796a295bfea23a796bf8e537"
      "e7421ba6da2a136251958a1b38e97783159ac89df85d812f6aef0b8396a245b4305cba71"
      "cf3b83b403b266e4dfec6623368a7bea60ded5115a3b9441dd700cb5194042f1203e4106"
      "5e1875ecf588711dd28b53cc455132d0356b698d6cbb39aff3237c9bb03bb909eb2e3ff3"
      "2e1d8ed48e6cce57eee4f9e73574df4c1c2705a8ec4f4a74f100db46eac1c8432d6c0619"
      "f36825b3665e5b8928990430ffed38d3f48ce4373e3d985862c077f7e868b2481b7d5d97"
      "910d85b80152a30c88e1171a64affeabdf21ced883e028f691ab8bc12a665a354226b205"
      "31b9737c6b0972a69bbc86e5aa1d80ca4304d48a97ba2aa7540b45b74a1cadcb3b873c47"
      "cc14ebc48003d1a771032fd836291a213f5ad8534620be45400431ddc6495eed9082e5ad"
      "601276f5f92562722533c0abeeb95171cadf39f443f330e182ae512ad428c2d894b8e249"
      "ab3c336b4c05aa62452b3502778bde65160a79b4b943fdc2c0bf4f8cf2b01d5e4205ad87"
      "6b1d35eb5c84962ae4701afa3ebfb7cfb7c8a3d7016dfd50eb8563c0423c2b5383ab021b"
      "3a3bd0489ef74e5c488329c7b052702f0b94476f6f9d0637af1244c7bd824840de6d0612"
      "0df71cd529d94b36fcf7a51155174c7fcb732bacf006573ad80d655af7637eed8f93b05d"
      "d15ae2a65b556f0f97e69817d50113cb8a1c5ed1e997940775abea4e31a038fa8c62e71f"
      "83831d6dfca8dbb50619f6bc050c611061c0c69ed52b939a33f4195568594e729509764f"
      "131c40c67a1f2ee8313b758abed3c5db5cd56bd15b654e9722da39447968a78bafc4b176"
      "1d99ed127221672001343342aa5235a9a109ec409dda9e529e1d7ee9ad5f76dfe9c47c1d"
      "400ce694e768cc43977da947a827285c3a49268fceed30f96afbed07fd79f0c9413e0ed3"
      "d6bb48787311e70714c690f1d1d1af01a6d5fa86d7875be7ed76de9407351957973b3985"
      "d9f7c239c2ded8defce88221b73bdb24c58ce29edeca93702d2f4d27c689d61ddc07232f"
      "4ef0f87b4f5ca2fca3ec08690d2f6bc22abd208b7c1709aaeb6e1ccb75003b020869dacf"
      "6ee4cebf71d336619ccbea02681ba1231235b5559436d583468ba32ea8d8ad4bea27e863"
      "44fc7e867269f28f9943e831f5375cf7ecaca6aad8313211b1d9bf5f1ca58ca48d1b1fdb"
      "72487525b1131551270aeb6951cbe9f51e569fd463df9f3c6d8c69ad7e40987c8eee20dd"
      "581f43792a15254bcff9250e1ddd78fb4d61e260e38dd8a0fbabf2391729a4a9295daee9"
      "c532cb3aab525c89490129eb077e60a3373f7ff8c69aa712b2efec83fc906b18c83eac8d"
      "f71199d3805d34541e6068b09b713bac9a3ff4231fba7640722316c9b03dbb73ce72d2f0"
      "53eaae2a807f28970d20b8287ecd785345ff196550b9f6f74223f179cf9659f88c1a9c2b"
      "a6714910514e09f546cc75f045864b5a47048c7b6522e172831e0be9ee7144e978d978c0"
      "e2eb9bfda3e223b55812221d8dcaf664f601e19bb2a4f8c7c7a499e0713ae659e19b7147"
      "c0f10ac5972396fa325b6f2c2a0466edfd9e1049179d384e37a7719ae55da55edaea3eb2"
      "5bc64377bdda7233ec277d60530de0fd3a7c69afd3c6ccbccf4117a76b4ee3677e59d675"
      "c733ecf29312cb66d328d9722b9b8240c23642fef4adb5f2c626d0475de012abefa0b2fe"
      "4435d337b85766eeac96600da4302ed9efcf5f4c1c14bdf54266a226b22af2ec681f4d07"
      "3d7fcc978a065a39fc0a64a829ec35597826a47e08b12b7e3e50593ec732d6bbd7ff3b39"
      "354010fcd6980eb36ec67e1d26cb5221ead31dcf0c4a248ab52c09d604a30f54fda5ba6a"
      "bcd45f6d8c9c3986cc6e5ee73adae9639d2958d4333b44a2c74d4563a3ba7a0b0cc25cb3"
      "7356f6022b570e812a079ccf2348baf4ef0f2263cdb16dd29b4f13aa784c285199f5e430"
      "2e55766b2d61f657dde12d8787090c5129b6a1691fe55f3ebd1f40ed7ca07b879fecd249"
      "23ecaa53e13e5ee19c3b6f9b723c0291349d959def23c6ffe6af4612f8e7b2cd4f8a8d74"
      "205ec5d64638459bd03379dec9c45facba605840d64811074f4700e46737eb714a494e14"
      "b12c5e756b278c282daa80b399a040624ba3abde7a2169f6c87a361b5d3333a4631704a7"
      "9f09fb469baf41b7e7cc64e6f6642cdca110623eca8da88388728298f11c4619c6825591"
      "e80dce1cc9a84f3f2755e52da5690b426b6bfe94938bf939ae20632125b6b72332266f92"
      "dd9b2e81c6356dc56ab49d57aeae8b7b5fa15f873c2a268d863b94605437348568ee64ec"
      "96eae0c78279e302b294365c7a785a40b90496275823e678498530c85bf35d91757e4e5e"
      "7cf73fddd3cc01a4ffb4d6f281131c5552d044bbbbd0d295c04e0665e9fe75aa18f535bd"
      "7269ae264d72a62d863a569e00d85eb8739bc21700681a3009406defaa51ffdfc97e3f55"
      "cb20d088563e98dfc8ecf29c9bd263138c69f5ce33afd61feed7c52ca88b855ccb0bb9b6"
      "4f2f6ee42a98970ced49612fb2dcf6c8b66de8a341733e2719354caf4fcda859692b569e"
      "1f1af68716801f7e0ba5bd2e7fccc68731ca47d0b2001735ae2d70057708b90151e74317"
      "37f58f9ce605820813fb3737381451a70dca29af445bc3f2b2369991d8994443318bdd8b"
      "1a0e93a763c608c85fa4b9df024481318a07e8583024cd6e71828da157ad949de4ba1f04"
      "94e8883d68dbcafd541da38d2cfe5a35ec186dca24ecdfc8ebc96fc4c3f56cec61929d2c"
      "17b44ed65c2fdb3b1bdd55820a2c3d85c1784655b0f782fba4299b2fc874d366634fd02e"
      "14167f4592d474f73d0fbe565bc6e4031dffa27b12fb7f94453dbd2557e05a7ce8e4558e"
      "ff978f349221e704d3760b2dac0043cc316dc7a288a3b1e550a552ef08a676a14cd6b325"
      "650c4d4d122de903635d4d0310de82a31290667771ed8036a50272a077fc6de7940b8b3d"
      "4cc083c02255e3f16b4a6a51019a833e0f6b7f0067a4e6f97fa41bb8ec254b54b46db52e"
      "a1b13beb90712af6bb7edae989e40758142309214574dffaa401e47ff6ff471e15015cbf"
      "e9f90f9265a16719a153ca85dc890e761ee14756220b1bff91ad8eca190d21c1faa6a1e9"
      "5e3498d5b0d7a7b2df6f2d351e4d4fe3c0356a0609c61e3c5f6170433565296a78e5ae5d"
      "96938a7047d8be11fe3bf4d9d2038a715afa81909ec8969b893b0df9c6244746cc840285"
      "c058f88a54226f22e6374faaac93aba0a6ea5fe8a67edc4d1fdeccee0372a373677e3649"
      "39e91ce7182ec1aa36b6c91ae05f3664d9611ae7267556c68a5bb07eca1276fa6bdb94d3"
      "ed1de1cd6fe2761f7b939e361b93c335227e50ca1a548abda449a3d2838a878ba50049cb"
      "126567182d360a7d06c3bd9b24687a86135d99f5378e56b6673c106bb43d9df051077987"
      "a79e5d8f88a7a917c4e612d7d70c9aa4fac90757b7b80ef94a35eec58e845dcb7a4ed4d2"
      "e080e1aea0a5fb9a171168552211178e8d8bd107a61296f6e77f495fa1b07ad488371c2d"
      "44f44ff33718f19bffceafe126184c11a8118fe52fc7df98fe90da92c6e69e6095f8dd11"
      "5d54a0a2f010d332c90d8261b9027659c8577472ed4c79407fc5943bcc86d0569131447d"
      "72e9eef5062043cca43984cf3db4f4297b0e9a3f651e0521f1839bfab069b3a762e7a9c8"
      "77acae262f14340c7da3a6fa151ca052ff496b3c39d5f3e852ea36e3f56e1d295ea60eba"
      "cd2265ddc3906b4273144fe29d5c0846b2084c29f9575cfee7bf9df8745b658a773fdcdd"
      "69d2b29046b197e4229ab592b32346fc589103fe971a0468a035f6a24f0a4f16b36d7881"
      "8d18b115fa3f59e96b63be6208f856b0e0777e383ba09cae1e0d5dddb0d2769135102969"
      "9733713fbca650c29eb2f457d50af5b277ab54308734fd4e6493838263bdb590784f5f8c"
      "89b7d99e17c57cc2f93c462a7e246916a43cd36b47f99f2219c44e83c5f03ec4a35d15ca"
      "e99ebb9cf757bce5539aa024f0257cef7eb47e736c2d779ba2cee59ecb12bbed650ba2c6"
      "34e2c05bbb2a8f467f1494cab69ad38d0b5dd3a410bcdeac5e52e139722feaf328ddee41"
      "c8f580fa7f29dd57213a400b120ed334132479360544005452e560e79a9a64b42afdab57"
      "ca1b9f6dfc38ab8eacd715377e5b2b318c898d736c36b4a2538b09189786c89b30088409"
      "7cb8f683118e5dbcca41eb1a100e7c378c62af3af8114a16433ed598bf11a384276b9a60"
      "8d3569df8f82608ab32eeca0ea7a7ebb95e63ae7f6f6dea1ca00412d88dcc6bf47f5f8fa"
      "f9276ca8f26631aea788fd2cf0ddb28942b0a655120c85f696fc9a2860162ae875011c4d"
      "c153ee301f06d499a21f6754f7448cd2e7168a5dc20e6e839d206e477f75c0d9f1f47fdb"
      "cadac7129dccabd4ff7320ad650bb450b7ecb32396e3c91bdcf1de9d721a9f7aeb36aeb2"
      "d63fc8cef9e5c8d59d1fe9660d1da4947867ddd025ac714659382d4066e82c028d867437"
      "c3d89a52ba126c23df49ccc00df8bce39591a85fead39f347cee1bac2bd97be771058050"
      "0938ca82f0a42e20dc22f60c7989707d8050f30ba1600520a497907280c8906188302a75"
      "a4ff69024a1d4b479abe766d7d70014bd0f6dac788696c82b95085bebd14a7d99f74c353"
      "0ad98b8c90066cd8583945274dc0851ffdf11f4ea59e28a2921d3d50ea9d1e5a1bacda40"
      "1880f287fb143e70bba464f14872c95a1ca2df792b30cb194a7ca30215627feb26f948cc"
      "54915cead274fa6d866b02125e1ca5d8e52f6561408f8b10027f70d2b6934cc2266294a5"
      "9c9169f7d949de8665c282603f3a083ae12ff0e52f241a0e353eeea06437b2cc0c9e4e10"
      "c6453983d1b5425ea63ecc05fb6d2c83ff273809778abe0ae3f63a2af7e317a56e5d9489"
      "5612a1acb83121551187cb051c40114d9c31c4c11f13ce8bd35dfd98482f35091dd86683"
      "870755c8c53d552932d8268135a175d54a145b270a13763910a9dc7baa0b6534b70aa39f"
      "793c9412f62d6d06bcbc8e675c97e817a9686b4b5cc6c50b341c945bb1091f5762457fa6"
      "5ed6162a81550b72ffe803ab0bcda423a5c588d41cf7668e683a73133223a245889a19b1"
      "fb1cbe1caf7485bcda2ae1298266c0c612b6c20de51e54e245b93e72c4a90f557eaf4098"
      "693e56cf2a07fcddb7049a33ad2e4cbfea0d5cb61f379ff71f3b88a9405a5f5960c8194b"
      "5ade8b2cf8fbc859963e2594f0164522950880a9eacbc34ba581faaa0a77ccf6826201ea"
      "a5737035be0246c99f032e7a63799abbdff416b96a67ef6b034c1fb66d9be5467c774315"
      "56409f98982e462f112eea01f6a4d5f40cc9242995bd20e05f6c96173f678d9430c18e47"
      "aa1ca5f8bfc859c0690bc04c984d28b9a65e359e4b778ad53d48dd2f2a48ed45782b90ad"
      "27a428e06be67b5fafe9cd9974667d7212000c5962dd5f9293f7e2d79938224bc0f22f34"
      "ff61c94cee9b9d0acc2ad439082f911ab02d3305fafaac19cf3477cce1c9edd1674ca061"
      "7294169eaed2c124a85c8f8f7dfe3f615afc06194030b8dbe49a1cee42387ed44bb3a625"
      "219ecd71c79d2b2a86275a0eec398088f50fae8f4cf2d3bcbcc9659175fc2e902a5f37fe"
      "e0f449cf3705033876af6730971ee95a2278a2a7fb99e4d0e4470a9de5bd739fbd241d1e"
      "0380eba55a6b91e97b1a00bd2bdc0d6f148b4e8eec50ecce55d13d6f2591f7aea93c2157"
      "523dc78c940ba222283d84b53a4ab884a4f0be1a1c057e9fcd41b31c52cb17b4de4dda58"
      "df394c3d00a6f9ee78236bafc7cd29ae3645eca0f08e4fbc51265f82c3bbc2dbcf1964a4"
      "fb9eaed6b239c199e56217ff668400b8b6c7f4e71a911bdcf0418389dc3cc0154d6183a8"
      "5ff4f0ce1e2bb83807ad05ea0730578474f00abfdf7da0ee6b3c2c32959122971e274ab6"
      "774663b4d17f2fecc2f12596f9a53a115d816bb84191f999e5e44117e8cdd75208e951a5"
      "217468885b9edb284dae7049c878c8a26d199d2e0f88a9865d257150c632ee6c92bf338a"
      "008473a480fff34ff162e8d29a2a99ead10d3b49bf369a191052a2e02bec10cb650521b5"
      "110ec827612e225d1c2f6c3ab1f831698e74bd23ad8f4bd04efc5505fdb1c690ba58d2d2"
      "d4a62ff3ce10a400ae7577ab5350d9dcbb91a8447c053cfa7227eb84dd1541ccbb060652"
      "e50dde555b82485d236859d3184af0fae1662a4c4edcb5c5b3419078b826605205145c4c"
      "89b583a2c5a866a723ed4d12062898f26b222c1f34be7853847e312329a40057c45372cf"
      "1d6f421a32953b9038bd66435a828824bae045165c8690f5989dda63b66bec1e6b9de911"
      "8cb0f581559df0a68eeb9094051e9e579c32ed1dc3ac5e994bdd75b325906db616f35a1b"
      "faeb4a6e58cba57b6699c8c7c1d794109706ac7298f0ba1a836ef916ad3058e8448817ee"
      "18907f1180e0d6c1e1294607478096ba2e51f8b5fa608f5e7f5222538216c3e20d8cb87c"
      "9ba8603f73c4daaa7f242968309e8da755d187e70aab0b240cb73ad822184576c41706cb"
      "fa046cbfe4a36d62315fd46fc431369020bdfd70eb7c1fab8bda51d08b183882b3300a38"
      "02b78c4d1bfddf4a5c4696c941bc0b0666bb43f42ef223853c3c0aa4c1e076a90b85ced8"
      "1c6dda01185c77a85b0b73e76b41847e7b213dabc505d9ad9897d7a626b4f2d7506c7d3c"
      "5c6881fa1343d5434d3f1ec5a4fe3d3dcd208dccebedb5c14eb6585649ad18868ab0537d"
      "89576a93905b4ad0f8065e14b2be41036a15b00baf2e2fd146c7d42d2575883abcf70438"
      "eafecf87646793867dd0c0458990c527de15bd858672bd126aa08a77ceaa6c35ec507f75"
      "d3396ca02f8bcbbfdec29c3e452afe39ba3060b47db6524c1ad6c1d561c95511473f1494"
      "37a683ec2c1e23567c18cb63fb096526fdee02b4029da8dc6f1802b9f47cb89cae297616"
      "39f720a07e0567d939a4aec13f621b01a6db64c9a21e0516d5018cbeb5820e983b09704d"
      "1f261b8336c707864c529cdcffdd6d473ddcdec70a1d741c90e2680c7f5568d910ab07b7"
      "00e65e22e5d8975a59360d7bc6f5fed784df0d2b96fa556af134e801a1185ef723010de1"
      "3929fb724c2467c6376e58b6e81ca88096c8307139053d4f4a8194a2ef47335f124da40e"
      "241fb315b7ac16cdeb5f8c83ae6c99ee17e9ae8a13995fabada5761f9e9d5699326ea6b9"
      "aac5c9b0bea0a4b0065aa5269358063eeb05a4ec0becddecd954315243a70f2e1dfb1b44"
      "5dae3ecc346f8382d5e2bf795a3beffb6bd870f823cf723a80a40ad50d14ed5b9e007d7e"
      "ccb741b9868080ec133d81ad26226fafe30b300582ed8ee4c15505f0eb79b4d6d99433c3"
      "609d4d3b8b920298388ff5d9c982fb79cec8163c3bf7d65d72d985f7ef381e6ccb2ee9cc"
      "3337f86152dd6aa2150eb97a2b4964e11b1ee31e97336b864e0cc114025a15156b9b6d12"
      "081aebd862b212293ff3894f9209eee6009f27744faf40da02bbd8da6411f40630bee0e4"
      "8ef65c9a8230702e613b7b26665b93ea1315b040ce60d10de6aa334b6dcbdeefe1df571d"
      "a2d5b927a11e607ce2d6d4f36193977cbdf4ee939512",
      ToHex(captured_data));

  {
    // Cleanup old MockOrchardSyncStateProxy to prevent dangling pointer error
    base::SequenceBound<MockOrchardSyncStateProxy> empty_sync_state(
        base::ThreadPool::CreateSequencedTaskRunner({base::MayBlock()}),
        db_path(), nullptr);
    OverrideSyncStateForTesting(std::move(empty_sync_state));
    task_environment_.RunUntilIdle();
  }
}

// https://3xpl.com/zcash/transaction/99b223198efaa5fb2ea4b10c53f9f9d118753789ce7f9ed3f891c9e2c8da91f0
TEST_F(ZCashWalletServiceUnitTest, MAYBE_UnshieldFunds) {
  // Creating authorized orchard bundle may take a time
  base::test::ScopedRunLoopTimeout specific_timeout(FROM_HERE,
                                                    base::Minutes(1));
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndEnableFeatureWithParameters(
      features::kBraveWalletZCashFeature,
      {{"zcash_shielded_transactions_enabled", "true"}});

  ON_CALL(zcash_rpc(), GetLightdInfo(_, _))
      .WillByDefault([&](const std::string& chain_id,
                         ZCashRpc::GetLightdInfoCallback callback) {
        auto response = zcash::mojom::LightdInfo::New("C8E71055");
        std::move(callback).Run(std::move(response));
      });

  ON_CALL(mock_orchard_sync_state(), GetSpendableNotes(_, _))
      .WillByDefault([&](const mojom::AccountIdPtr& account_id,
                         const OrchardAddrRawPart& internal_addr) {
        OrchardSyncState::SpendableNotesBundle spendable_notes_bundle;
        {
          OrchardNote note;
          base::span(note.addr).copy_from(*PrefixedHexStringToBytes(
              "0x17761cbf97af0ee84d19c767db54494b30623dd7683c8a90312b76b8b1aa62"
              "bb7949a5780454e04c12319f"));
          note.block_id = 3117379u;
          base::span(note.nullifier)
              .copy_from(*PrefixedHexStringToBytes(
                  "0x375f17037d6dba52ede9f06ba6dd5beb630cf917e2cfa5618663e890f8"
                  "667532"));
          note.amount = 500000u;
          note.orchard_commitment_tree_position = 49354719u;
          base::span(note.rho).copy_from(
              *PrefixedHexStringToBytes("0x37d1dff821d1bacb98cc0f2cee17492daeac"
                                        "603e2d51e69a5f9d64ca6d1a832d"));
          base::span(note.seed).copy_from(
              *PrefixedHexStringToBytes("0x87c76573ec9ace879a9b278611df1843a4e3"
                                        "d899850c60d0f4b120cad3745cf1"));
          spendable_notes_bundle.spendable_notes.push_back(std::move(note));
        }

        {
          OrchardNote note;
          base::span(note.addr).copy_from(*PrefixedHexStringToBytes(
              "0x17761cbf97af0ee84d19c767db54494b30623dd7683c8a90312b76b8b1aa62"
              "bb7949a5780454e04c12319f"));
          note.block_id = 3117388u;
          base::span(note.nullifier)
              .copy_from(*PrefixedHexStringToBytes(
                  "0xe64a4dbb4b2098a28420234e2b61af86782e6ffc24d4ea09b68cf5fe93"
                  "651c25"));
          note.amount = 500000u;
          note.orchard_commitment_tree_position = 49354775u;
          base::span(note.rho).copy_from(
              *PrefixedHexStringToBytes("0x8625ac9a9c5c8688efe8f3636f2af858817c"
                                        "a89f5e087439dd945fe0b9bd3022"));
          base::span(note.seed).copy_from(
              *PrefixedHexStringToBytes("0x6a3cedb4fe0860c85a3bb522e60b601c9eb5"
                                        "84fd3d90616c5ab56d2324f6fe28"));
          spendable_notes_bundle.spendable_notes.push_back(std::move(note));
        }

        spendable_notes_bundle.anchor_block_id = 3118554u;
        return spendable_notes_bundle;
      });
  ON_CALL(mock_orchard_sync_state(), CalculateWitnessForCheckpoint(_, _, _))
      .WillByDefault([&](const mojom::AccountIdPtr& account_id,
                         const std::vector<OrchardInput>& notes,
                         uint32_t checkpoint_position) {
        std::vector<OrchardInput> notes_with_witness = notes;
        // note 1
        {
          OrchardNoteWitness witness;
          AppendMerklePath(witness,
                           "0x2187d4b1436b1b4aad4ffd12b7fb5b9ff5afb63c0981f98fa"
                           "890808cb516f30f");
          AppendMerklePath(witness,
                           "0xa0a6e372162d30084a18003486ffcfbb6a8bee4ee25776566"
                           "fc8f0d5e2b7fe2d");
          AppendMerklePath(witness,
                           "0x7c4621c3f38aa2d65a01f00487409592feb5461c89faa1aa4"
                           "58582e126c0533a");
          AppendMerklePath(witness,
                           "0xf1d4882fe69304d717e2c54bec5228ebdd64bf3363b0eb0ed"
                           "9648dbd2eca9c00");
          AppendMerklePath(witness,
                           "0xc04477cdba61d4bea1f8da86e3dc8cfacf6b88743232d10bc"
                           "0bc6372a0f10e28");
          AppendMerklePath(witness,
                           "0x44d117c81ac5802167508984ff7a3a3c6c4ba76368f774b50"
                           "8aadf8068de3101");
          AppendMerklePath(witness,
                           "0x2e6adca7afb5fd52b79facde59649fc93a8fde91bd4f160ac"
                           "c55801d5553ca27");
          AppendMerklePath(witness,
                           "0xf8612f29621c1d9c076e47dca7a2ad88a6383bebac7fc1a91"
                           "0295a90fc12813b");
          AppendMerklePath(witness,
                           "0xa64fee7d929bdab5410f4f9dcb3563a7d90cf0ebc4428e92a"
                           "1c0f549dd675729");
          AppendMerklePath(witness,
                           "0xef85404a53249f188444e2950bb657e2e305f137488b14978"
                           "b66df4fa3b7e10c");
          AppendMerklePath(witness,
                           "0x964490cd4b4ce4163a9c9be5325097fe7aa3cb251829cab2a"
                           "c15091d6df58632");
          AppendMerklePath(witness,
                           "0xcdb15d59374b9a74f9717c135b9cd5c442ae0175203694ece"
                           "69b83d270feb713");
          AppendMerklePath(witness,
                           "0xd1ea1e02be5b883948fc431f9613ea4eebf7de6478eb31384"
                           "699fbc03c4c8b16");
          AppendMerklePath(witness,
                           "0x21cb3470fd7f6604dc565888fde97100a25af5a60577a2db7"
                           "7932ea8ba402536");
          AppendMerklePath(witness,
                           "0x3f98adbe364f148b0cc2042cafc6be1166fae39090ab4b354"
                           "bfb6217b964453b");
          AppendMerklePath(witness,
                           "0x63f8dbd10df936f1734973e0b3bd25f4ed440566c92308590"
                           "3f696bc6347ec0f");
          AppendMerklePath(witness,
                           "0x7852cc0085e68db25ad9fa1b63a310463c148e07140f600bc"
                           "2779971b21d2211");
          AppendMerklePath(witness,
                           "0xbd9dc0681918a3f3f9cd1f9e06aa1ad68927da63acc13b92a"
                           "2578b2738a6d331");
          AppendMerklePath(witness,
                           "0xca2ced953b7fb95e3ba986333da9e69cd355223c929731094"
                           "b6c2174c7638d2e");
          AppendMerklePath(witness,
                           "0x55354b96b56f9e45aae1e0094d71ee248dabf668117778bdc"
                           "3c19ca5331a4e1a");
          AppendMerklePath(witness,
                           "0x7c8ece2b2ab2355d809b58809b21c7a5e95cfc693cd689387"
                           "f7533ec8749261e");
          AppendMerklePath(witness,
                           "0xcc2dcaa338b312112db04b435a706d63244dd435238f0aa1e"
                           "9e1598d35470810");
          AppendMerklePath(witness,
                           "0x2dcc4273c8a0ed2337ecf7879380a07e7d427c7f9d82e5380"
                           "02bd1442978402c");
          AppendMerklePath(witness,
                           "0xdaf63debf5b40df902dae98dadc029f281474d190cddecef1"
                           "b10653248a23415");
          AppendMerklePath(witness,
                           "0x1f91982912012669f74d0cfa1030ff37b152324e5b8346b33"
                           "35a0aaeb63a0a2d");
          AppendMerklePath(witness,
                           "0xe2bca6a8d987d668defba89dc082196a922634ed88e065c66"
                           "9e526bb8815ee1b");
          AppendMerklePath(witness,
                           "0xe8ae2ad91d463bab75ee941d33cc5817b613c63cda943a4c0"
                           "7f600591b088a25");
          AppendMerklePath(witness,
                           "0xd53fdee371cef596766823f4a518a583b1158243afe89700f"
                           "0da76da46d0060f");
          AppendMerklePath(witness,
                           "0x15d2444cefe7914c9a61e829c730eceb216288fee825f6b3b"
                           "6298f6f6b6bd62e");
          AppendMerklePath(witness,
                           "0x4c57a617a0aa10ea7a83aa6b6b0ed685b6a3d9e5b8fd14f56"
                           "cdc18021b12253f");
          AppendMerklePath(witness,
                           "0x3fd4915c19bd831a7920be55d969b2ac23359e2559da77de2"
                           "373f06ca014ba27");
          AppendMerklePath(witness,
                           "0x87d063cd07ee4944222b7762840eb94c688bec743fa8bdf77"
                           "15c8fe29f104c2a");

          witness.position = 49354719u;
          notes_with_witness[0].witness = std::move(witness);
        }

        // note 2
        {
          OrchardNoteWitness witness;
          AppendMerklePath(witness,
                           "0xbe94a601d4c4e72f233b6c6a498a48ac068a1c15074dd1496"
                           "2a4d8692099461c");
          AppendMerklePath(witness,
                           "0xab973402d0aa7d926672f525aca792a644ed4acfdf8b9f194"
                           "6b251df92acc039");
          AppendMerklePath(witness,
                           "0x565267e792c82b0775eba58d3c5e8a3318b39c2f2d484a8c2"
                           "9c0e188f2c27123");
          AppendMerklePath(witness,
                           "0x8ee73e276415fb4f4b57cabf70e20acbada8b253f8053d49d"
                           "477c07d0bc2822d");
          AppendMerklePath(witness,
                           "0x5f906df0aa64ef0b0ae2d0682cdfcac0e2842ff74f8f2d637"
                           "ccae872cd34190d");
          AppendMerklePath(witness,
                           "0xc4e8c930ba20821d02a2d37abecdd7d0eb2f1f0c885f50efb"
                           "0fd077aadd07034");
          AppendMerklePath(witness,
                           "0x988c1e837b2496168cf3e1a4e35dd6db51dc0c8ccdf986361"
                           "d1206a06ca81f14");
          AppendMerklePath(witness,
                           "0x8ecbf078a151552aeaf650f2369a69f9561dd45566de821d1"
                           "3d67b785f49810a");
          AppendMerklePath(witness,
                           "0x48b1ab90d9e80126df5b7708af316b56e084f650baad10837"
                           "aef427b0d191d21");
          AppendMerklePath(witness,
                           "0x2448683143b059f1fc5bb02994b53b6c5436148be8114e8e7"
                           "09fa93b3fcddd2e");
          AppendMerklePath(witness,
                           "0x14ef18f8f96fa6e4d3d8eef551295bd2d0ea0ebe1e95b8b50"
                           "c39f0d94c942d2c");
          AppendMerklePath(witness,
                           "0x63f0923f4027fce970c3a50bc69f7c65cd20ca47e56748168"
                           "c4dbf2385776d06");
          AppendMerklePath(witness,
                           "0xd1ea1e02be5b883948fc431f9613ea4eebf7de6478eb31384"
                           "699fbc03c4c8b16");
          AppendMerklePath(witness,
                           "0x21cb3470fd7f6604dc565888fde97100a25af5a60577a2db7"
                           "7932ea8ba402536");
          AppendMerklePath(witness,
                           "0x3f98adbe364f148b0cc2042cafc6be1166fae39090ab4b354"
                           "bfb6217b964453b");
          AppendMerklePath(witness,
                           "0x63f8dbd10df936f1734973e0b3bd25f4ed440566c92308590"
                           "3f696bc6347ec0f");
          AppendMerklePath(witness,
                           "0x7852cc0085e68db25ad9fa1b63a310463c148e07140f600bc"
                           "2779971b21d2211");
          AppendMerklePath(witness,
                           "0xbd9dc0681918a3f3f9cd1f9e06aa1ad68927da63acc13b92a"
                           "2578b2738a6d331");
          AppendMerklePath(witness,
                           "0xca2ced953b7fb95e3ba986333da9e69cd355223c929731094"
                           "b6c2174c7638d2e");
          AppendMerklePath(witness,
                           "0x55354b96b56f9e45aae1e0094d71ee248dabf668117778bdc"
                           "3c19ca5331a4e1a");
          AppendMerklePath(witness,
                           "0x7c8ece2b2ab2355d809b58809b21c7a5e95cfc693cd689387"
                           "f7533ec8749261e");
          AppendMerklePath(witness,
                           "0xcc2dcaa338b312112db04b435a706d63244dd435238f0aa1e"
                           "9e1598d35470810");
          AppendMerklePath(witness,
                           "0x2dcc4273c8a0ed2337ecf7879380a07e7d427c7f9d82e5380"
                           "02bd1442978402c");
          AppendMerklePath(witness,
                           "0xdaf63debf5b40df902dae98dadc029f281474d190cddecef1"
                           "b10653248a23415");
          AppendMerklePath(witness,
                           "0x1f91982912012669f74d0cfa1030ff37b152324e5b8346b33"
                           "35a0aaeb63a0a2d");
          AppendMerklePath(witness,
                           "0xe2bca6a8d987d668defba89dc082196a922634ed88e065c66"
                           "9e526bb8815ee1b");
          AppendMerklePath(witness,
                           "0xe8ae2ad91d463bab75ee941d33cc5817b613c63cda943a4c0"
                           "7f600591b088a25");
          AppendMerklePath(witness,
                           "0xd53fdee371cef596766823f4a518a583b1158243afe89700f"
                           "0da76da46d0060f");
          AppendMerklePath(witness,
                           "0x15d2444cefe7914c9a61e829c730eceb216288fee825f6b3b"
                           "6298f6f6b6bd62e");
          AppendMerklePath(witness,
                           "0x4c57a617a0aa10ea7a83aa6b6b0ed685b6a3d9e5b8fd14f56"
                           "cdc18021b12253f");
          AppendMerklePath(witness,
                           "0x3fd4915c19bd831a7920be55d969b2ac23359e2559da77de2"
                           "373f06ca014ba27");
          AppendMerklePath(witness,
                           "0x87d063cd07ee4944222b7762840eb94c688bec743fa8bdf77"
                           "15c8fe29f104c2a");

          witness.position = 49354775u;
          notes_with_witness[1].witness = std::move(witness);
        }

        return base::ok(notes_with_witness);
      });

  base::SequenceBound<MockOrchardSyncStateProxy> overrided_sync_state(
      base::ThreadPool::CreateSequencedTaskRunner({base::MayBlock()}),
      db_path(), &mock_orchard_sync_state());

  OverrideSyncStateForTesting(std::move(overrided_sync_state));

  keyring_service()->Reset();
  keyring_service()->RestoreWallet(kMnemonicGalleryEqual, kTestWalletPassword,
                                   false, base::DoNothing());
  OrchardBundleManager::OverrideRandomSeedForTesting(4234211u);
  auto account =
      GetAccountUtils().EnsureAccount(mojom::KeyringId::kZCashMainnet, 0);
  auto account_id = account->account_id.Clone();
  ON_CALL(zcash_rpc(), GetLatestBlock(_, _))
      .WillByDefault([&](const std::string& chain_id,
                         ZCashRpc::GetLatestBlockCallback callback) {
        auto response = zcash::mojom::BlockID::New(
            3118559u,
            *PrefixedHexStringToBytes("0xa5f73a402176de4470575b70fe1e0cda7a2aea"
                                      "8ca045448199782b0000000000"));
        std::move(callback).Run(std::move(response));
      });

  ON_CALL(zcash_rpc(), GetTreeState(_, _, _))
      .WillByDefault([&](const std::string& chain_id,
                         zcash::mojom::BlockIDPtr block_id,
                         ZCashRpc::GetTreeStateCallback callback) {
        auto tree_state = zcash::mojom::TreeState::New(
            "main" /* network */, 3118554u /* height */,
            "000000000043298abfe7f300a7c02ea6f1e92f6372a447f56ad40af9a98440"
            "07" /* hash */,
            1761916103u /* time */,
            "01bb48829ed71af6873ff9940dff0c226ae7485b084568b2f37cbe31f8d11f"
            "d66c01471b3ea27ca9fce56e0ba984538f88c14f24eeeca862603220b43201"
            "63f6d52f1f01bc2a6ef3e3db718b0a4200daca6299478b46c9eb77e9b051ee"
            "386d6a5b086f35000000000128ef8f3ef0b26d96d2aebe349a829a446abbb4"
            "e59a507bb5c5fc5309a794a74d01fb8b4ba331bcf7243a16e8f37497128306"
            "4ac5892de4de25dffcb0fa101db50c00015707fbb044f505d17d910831a892"
            "0c2e567ff4489eb0c5543a8a5bd139171c6e000001fac3b6c4f0946f8f4064"
            "37b5ea0066ceaffc81436dcd66a5cad0d1c87e04625c01fd4c4391cb31054d"
            "9ebea02406bdae95e15579e74bff7e8c75a1ffbb1182283b0128423c48fcf6"
            "6ca650c3aa306cbb3f8c1ae1cbc9ae88c6349bf80790c09db153000001931c"
            "48ea50688eae5e54c24d0df7a6b00ce498f92eb5e1cfd6a2d87d8ebd536401"
            "7d1ce2f0839bdbf1bad7ae37f845e7fe2116e0c1197536bfbad549f3876c3c"
            "590000013e2598f743726006b8de42476ed56a55a75629a7b82e430c4e7c10"
            "1a69e9b02a011619f99023a69bb647eab2d2aa1a73c3673c74bb033c3c4930"
            "eacda19e6fd93b0000000160272b134ca494b602137d89e528c751c06d3ef4"
            "a87a45f33af343c15060cc1e0000000000",
            "015e47ec3dd650d19acf4fbf183d478c4ebb40748466c46d187e51aec21d54"
            "bd0001cdbdce676ae3fe5dd1713f14e87d1ac5bfa89367faf55cdc78ed3f6e"
            "5b06182e1f01e6677145c4406ef28290287c6194ddea1c0df6e5cf45c8f169"
            "0e164bc9d85b30000001ccb81ac716d737138f3862a40cbeb8ef7f57229510"
            "63c8bcaddefe8bfbeace2e01b0dc10d01b484706263972638f10b35e13f0b2"
            "dc74a4c0dc617c1df393cfa60b017c0fd92f88103cfcc36e3796bd9b873f32"
            "205073ddecd12419e9579b626bfb1c01c20d7c086b635a7f1f230dd0e9d25e"
            "73567352d1b0411b32031b85e3dac8b71a000000010d1fecf33f83e117159b"
            "8c2dfc380dcab49bb120f47f74da4a1a72a66f51d12700019fee5cb985ac09"
            "618e2efd0ca273eb36b05eeeb13934ba6010aa74f471950d2c0000017852cc"
            "0085e68db25ad9fa1b63a310463c148e07140f600bc2779971b21d22110000"
            "00017c8ece2b2ab2355d809b58809b21c7a5e95cfc693cd689387f7533ec87"
            "49261e01cc2dcaa338b312112db04b435a706d63244dd435238f0aa1e9e159"
            "8d35470810012dcc4273c8a0ed2337ecf7879380a07e7d427c7f9d82e53800"
            "2bd1442978402c01daf63debf5b40df902dae98dadc029f281474d190cddec"
            "ef1b10653248a234150001e2bca6a8d987d668defba89dc082196a922634ed"
            "88e065c669e526bb8815ee1b000000000000");
        std::move(callback).Run(std::move(tree_state));
      });

  std::optional<ZCashTransaction> created_transaction;
  base::MockCallback<ZCashWalletService::CreateTransactionCallback>
      create_transaction_callback;
  EXPECT_CALL(create_transaction_callback, Run(_))
      .WillOnce([&](base::expected<ZCashTransaction, std::string> tx) {
        created_transaction = tx.value();
      });

  zcash_wallet_service_->CreateOrchardToTransparentTransaction(
      account_id.Clone(), "t1gcfiSPZKnEMeNT4ULeB8Q8RVJX2GiPqyH", 700000u,
      create_transaction_callback.Get());
  task_environment_.RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&create_transaction_callback);

  std::vector<uint8_t> captured_data;
  EXPECT_CALL(zcash_rpc(), SendTransaction(_, _, _))
      .WillOnce([&](const std::string& chain_id, base::span<const uint8_t> data,
                    ZCashRpc::SendTransactionCallback callback) {
        captured_data = std::vector<uint8_t>(data.begin(), data.end());
        zcash::mojom::SendResponsePtr response =
            zcash::mojom::SendResponse::New();
        response->error_code = 0;
        std::move(callback).Run(std::move(response));
      });

  base::MockCallback<ZCashWalletService::SignAndPostTransactionCallback>
      sign_callback;

  zcash_wallet_service_->SignAndPostTransaction(
      account_id.Clone(), std::move(created_transaction.value()),
      sign_callback.Get());
  task_environment_.RunUntilIdle();

  // 99b223198efaa5fb2ea4b10c53f9f9d118753789ce7f9ed3f891c9e2c8da91f0
  EXPECT_EQ(
      "0x050000800a27a7265510e7c8df952f00f3952f00000160ae0a00000000001976a914f9"
      "77bfdb88897ddb888a8aec52cbd0523290637888ac000002d07b5a77d264bfb506136be8"
      "70de410f475589e9d70c11378e740318f3aa8e2be64a4dbb4b2098a28420234e2b61af86"
      "782e6ffc24d4ea09b68cf5fe93651c25e3942eda8d7f371e015f34d29d6b5d02bd35007b"
      "7d4541416b5ff25005f84330dd6830341a41f048dab6204a1acf9b6c3a2d7b961e268ded"
      "a2500e795cbac60cb226affe1156249b61f7262cfb39bbf9a13e80b7b2da9c52ff36b31a"
      "e8c221a5dbba3e37ffa8069ecc23229e04dbf5dbebdaa1b518bcd2b8f471df45ce68f587"
      "d9ba178a35b9fd1da1921c4e35bd7bcf00ed6d94d2003d2a3c502cd236e544a87ff4c4f0"
      "d3e28da4e36b2826a1f38f5875b7924905bde672367c898859421f3a0e0da39a7275b7eb"
      "0adf7ee2e25243d9e6a189f9294a69f29cd1057d24a14260e3a70d04ae92d62d371e1f9b"
      "0e3e7a562e5ed247449fc66bbac18ed33d4b60cc837a8dd593fbfa8d49ae10e983bf58da"
      "28eba11abae8a2e2963f7f0ff91c5ec4200026e34f8a07c12c86e235f484ffdaf191847b"
      "8bc48125628128e8e3fb1a9e31b1b06b752be2611f78e4ede403f13757a282201fbf1ae3"
      "aea4ab67141c3ed7cd6381df89f9c4b3e59d0d6abda8c77ee6483459ff9d7038ebc239ac"
      "9c4a2ea4e5f9c13b4feabf47dc3a24d359925c9ec236f6b00633260afdbc3180f9aefa3e"
      "e68f7dc16bc6ff85f4ff93a47c6c2bbdb0892ab9d6ddad13afba688104358a7a97d831cf"
      "14d888b91112d06c33024a104aa1a9ebc799fde3350e91bb1a19c0c95d2d00083767f5a8"
      "97d3fdb74b795610860c2a551d8f03adef1a0ec95f213e8978ef1ea1c88165ea1d67ff79"
      "0df350ad816d09bfb2ea09ef3e4113d1ef166478d7a642a7a9e714a9a23ae7542bc95b06"
      "7a7ec9e92c55b702ec905d84c0cd5405000ff9573ac6503d558d814da5d0f2dd2e1994f5"
      "d97e9e64f7efeb3e980990ee68486aa217695990a692659e4d1e9435155367d606e65a73"
      "adc3a3eb47dd19159f150b41fb54b331168e7c5b6c68b5bb13897ae891421fb6c7931e2b"
      "1e99af510e263ecaa4a5d34ac78be7c706e2964b1c45ab87b81a5a86fc67d0d042bf1f45"
      "21c93283fab3333496d8b8a951dcc6ba2fdeb4deb9cc02c82d6d6d0eda583508a01b95be"
      "feab525af65cb84f63acd3528e9f6595b9c94409feae1d6a62e63234a84f14f561951a86"
      "e9a0d494d8325d174c9e5f00375f17037d6dba52ede9f06ba6dd5beb630cf917e2cfa561"
      "8663e890f86675325e06fd09cce028cf370ee65010cab12efc7631ce6b33f4aa193c095e"
      "243ec78258fa538e553b890777369dd4d2187216422c7e778532b8584d6b03474569053d"
      "28e3232d7a3bebaf7e0d56758143ec667f75be23930ab90ff61f535dc7ad0687a361c66b"
      "7ae2347062667760be42c6296d31710f122d600b43dc8dfc762577cef26c86ab3cb3bcac"
      "77db1027f5bb9db11dd0e4c8557653cd0394b708456c747bb260119d7ecd2c4d131378c9"
      "84dec7115c202a040daa487d62de67b16345e88e3d104b2466413629bd4d58ebdb536e29"
      "184353f5005dab3e05b38c21f8dbede6c6a97a59f6990d59039a1eca0c2718601b75cc0a"
      "6e2caa285c724301176a72994775a93edaf63313a19396cc09f664988838b1fb0a52dd0b"
      "ba91c900fcc5eaadd0e816905cfcccd798ff4f31e1bf20ed12faeb5863556380977a64cd"
      "90b314eff3f17cb3c4dd659acbf3fb0c990ddf95dc21791205936502b75443903f034197"
      "2fa9c26a72e358633a5ae5523c4872eea817cbaef89994c730b9208a64336d7e8a1cf043"
      "090f5d753235ec005211baebf0dd041f8376be5710fcc75499076cf2c928d9a8adb9e78c"
      "b12840163f01b041acff439d04aee80d9745037ebde0a589d459b6d9b81b72d998865acf"
      "b96a094e3b71ef1c32b9a0243370bd12755c7b249ab06469c345b74f914498392a73c896"
      "395ea493e4f35ebd5f4b17378113754f68bca7dce59dcf179ed2c801ff2dad58f07713a2"
      "2884b4965f3a8f65ef812ed431e8b8accfc0fcce88b2e81e4401388c80a61b7809552379"
      "3c6269439be4554a0ef55933b7c69dd5cf4563c043692c2eb31b3985adcefb92701a8b2f"
      "4c474d772447f7af0667cfca8d389b5a3ca3e43ddf7b21342beaa7deb714c06155419412"
      "6d0c8baf489ba512bfca150baac81205b84376396ba350f219fae58343285fee38e94cd8"
      "ec106838f4137c906fe22a1ceab4e2adcb5a5a73b1b6648394e85dc7b3b720770f9e3109"
      "16ff956bb9f7961d7bf11b3c22a39cb3e5cff2749cde9f6994e86c24b431a49dc8028c70"
      "aefe1716d9e8ddb403f8e80a00000000007698e7b40051b7ed4f63fb95f1d468043dc375"
      "611994fd73a9b5d0bdae4a4c05fd601cc398f8dcfded38d9be17458beed36e311bd85e7a"
      "5f5c0547a19ea1bb9649d21cd47038a38a81c1f3ba067bafba04307c8ed0eb5342ddfded"
      "b7daf3bb7cdee022c46018d614268c347700d38bbd47686f8520e361eebfdc96765c60b5"
      "8ff729aeb3c46cc9b0ec7f2a26e7cea0407ac4384d7657d54a2e42f53380423f95cda727"
      "2ede13e82e67052ebdf7984509c29ec435f10f8439950443a24d222354d4fbb493dfa8fd"
      "c0a9873e27f9b7d7f4f7df65ca5d416bc1d6c9b541f558565fc4911639713d0171c14d1c"
      "263254f305e471861d7c3364c3945141fcdf0a883bf7d81a65f6f480c7c153a9c581339c"
      "b38a3f756303056c7284b23e88ed071a081895a4b770aefd29c306cf482a2b1da55e555f"
      "7d96463b61f4e00bb0a834f7b4c9673544213d1342223b78ffd70ddc1057b5e06e42e793"
      "061d91162c7850538169cba331444975894b3d21908d1ef26085412288303387d9c341a9"
      "3733a1504959f4abd53c9562f4b4b0f10b3765ebf0a4bf62c5d31362a9f1bb0f4905de3a"
      "d53dbe2d69a842cecd1bddf30e67f77f240ce15808c2d34813f15e8a35ea93d448e00b33"
      "2705fd84f37d414001b9b22d593f7749e97e1d48410e4a3d3cbd8e0e29ca3f3ae6bf09fb"
      "e64e0a924f15516bed640b7fe453047f16e1c298bbf272dd4cbf233898874353de80ef8d"
      "ae2250571e1f760309320bfd7575b39c224fa409f62d2b8acee56a604279db775b6ed15c"
      "37966573e7bb7d8547528cce9e7eea5b5746cb99d6b3e80516de0ac0e9e100f80ce4e435"
      "b4cd2ee220e35c81ae9d806dd01a0c8f4930219bfff326209ae1a374ca88fe9b6e3438d4"
      "b70c564bede609fc424ea41294e7fa0c299cb2f70e3c95518414e8412a28e48dcbff26f9"
      "4ae3701fb0c9560686c07bbb66f1f7f5237bd2920f1651c06815b56f2d5c3aadd42c7cac"
      "9b4d0b39be6c8c071621bbd114d79359c9fa2ce402d6fce60b19f9ed3466617928f5ab80"
      "59a8328291e3acfd363f649995d3a79e61ec16400437321a24a7b5136a83d2ba1483efb6"
      "f79aca83e597ce4515c870158312d05f9fa4b670fb7e130c473d9f86ecca85ec58217df9"
      "0e2b8309b2d974b8ab37901fbff0131f8420a7e7995c7ab771173796ba66a409f2bd4a87"
      "91cf83ffdb3072bd06183ec59effac5fd70ad133f636fea477ae6efe2e9e5d4ba494bd85"
      "d9af15cf18a7bde4be9efa438d65c88b388cc4252fcc8d7332893b5ab2dc6b311c2a064e"
      "56e95cd75ad72b0685cc4538eab54f6fc68332b55cadaf2413eb4d8c225c7abcfac27e06"
      "4007a8eb6b6214037e7f36e2be498df1aeb49b876eb85a0cd256d954c240f7943c05a70e"
      "27c51d0db89d29aaab44b1829bb45a5beb269f8aa3293a16fb615805d0913a41a0408417"
      "0cc041c6d0430b5dddbd9668a72de01f0fc1e587ed3d2dad472175468201bd06ebdbcdf6"
      "92280be5af876ecef2ede60935d50bc97c857cdf7e285ef871f3c6b6bb9507a5117f4229"
      "daa2f94889d6986e21d67c1b2320cffb0ae350bc648806a478c5ea7606848388be039740"
      "f302d58f2bcea23ce007d74f0c9842292caa6d979588b728cd2c59db4da256a1c9e9bdfb"
      "e17a3b27d4370e45cd2c3c50419a8d3bdf1fb34d6997d4a41545a8d119d4bccecc964af2"
      "b013063225c0a15f8dca2c39ca08cb88df591fc89296b7ec8e8898616ab0f2eb3882bce8"
      "8472a4c6881d5c904e04a29218f007e3715517ba1aa5c86f5833948c915d4ea9467cc984"
      "880663bb6341158dfb9ad02fcc367bb5be45b397f40402bc6ce1415bc4808edbfe3b439d"
      "8445323a2fb082ef7f38be624607ded67ca607b008ee147ade9a3d66b54f9d971983a96f"
      "d865a048f6a659350c705c9eba39d9b6c17b924795d7c847ace963a10ba897a5013c3336"
      "fa9f7f5ae7f60c5b898fc9573c4dc84d09cb2d46ffb2dc02293475efa1fd73d69b315dce"
      "335b44b3379a7d168e07bc3d5aae02270a4d9f3bc125d9948070cb63cc75a3b32c1ef0f9"
      "8a6f58234dd03223403313fd8f8bbea0bcebcf8047f9ed623f65f50d9512136d55c4f9af"
      "7eadf23dd1e5b42f531e8527a27835049c610ce7389af1b70a82a9303c3c393201f81274"
      "bc43daad0115649669115d1acd5a2129e085bd67a6c0ce9a7ce5a35b0acf871120c3b575"
      "c1068e2db95ff536985b18a9b8b6bf6ba0b8536d2672a092aad004ee5493f3b4be0739ae"
      "c8eddeb7b316058fcddc7bc31912b7f055e857434a870e19041944c41901e1b59a6dc0ad"
      "c4ccf02c5be724acf7f97bc067e5be80d39d03657d704f0bea5e39235d3b882c9223e0a1"
      "69a868fb7cc7a5cbf0d91b635e6fc22b7be29fc2eda2f7251c750f7ac6e4f721aa704f68"
      "4e096157d73477139009c44338d9336ca3ec38936faf5b70b58f8a220080dd3c52253db9"
      "b9c8ab6bcd754e99ff3ebf5e8064872b5aafea37b7b2696b00c15832a5eb139dc0145d0c"
      "db1ece4eb9d017daf7d28029c0f272ea63472f27764009e38817220dd1868ec0d4c12264"
      "90471e0fac743f0289836179d79bc2ba7756267bce5588ece1d7cb83cd8084e7fc21b47a"
      "a8f1cd3886d5ca26f0198c536d3d2e6c03856412b4b1c137154efa4a89d3864611d46033"
      "8925169d2fe5b1b65be5b22d8bc2cdf0b819e68b8c06f9c29cb76d288bf76a3bb6239468"
      "095e3f622f397dd516e6baba0eb9e082a5657a18313b84fa27928007629425bf41b23e2c"
      "59fcfafa5c61c09a85451480856ac2c91ee8e1fae5321e04f290a4a94059b832ff2c3bc7"
      "22a047629e3961e6c79ad4df56830d9485a7fa2bb041bd40b5a341beac9acf30d4ff846b"
      "2d30a3316d56fa0c20742e54b7b7eb0036993e72d8e6162b63a6499b17b584a727dac6e9"
      "ba6de7a788aaae5451b4a80a196af583e7c261382df2eca25baf76484b7f2779180ff20d"
      "9d5ef78347ce9d0765e5e6c4f5d46532431c93ac38b90accac8d3a4bbe867d67ec06d277"
      "48dfe124f10d567ef323368668a85ee7dbb229ea1a256fd0575a71b5f763140e8ca7e12c"
      "f612c2698c12ce76472149c1e1b2828f10ec28f8b9681edcfe9c3dfc8508dc27a1ac8322"
      "5180e10886740d827dcee33ba49fc43670a5957d107ca8df3e6e0a37704b69e7d5bd4132"
      "6404433b12e51acf986b6610733a3fd82fde960cfd5fe2309a2f9f17bde1714e513ed4b1"
      "e02cf2f68009e5598ddfc180fb08faf76a85fb20e46eb009bcc206c20ba9cf722cf7e1be"
      "b536b50ebcde6bd1249215327a7975096dfeef06bc4e385876f486e7c89aa11daeb6e20d"
      "5a9e981a41e9ab0e9791072e1a7646a26ac34a9a23aa8fd3a22703687f730a042aa11190"
      "dd9f3d22904b56150cbea01a7700b5d59abb6cca7ff1abfbf34c722ebc880555f4e0a81a"
      "36d21b3050557784abd1b80c71c30d027ef93403dd940d81d010f917b7c1d3d6827e543b"
      "8f43cb6297a1d13b0003c6652e647836c941663695bb2d0bbc9db4d7e642140065d11e1f"
      "925d8f855d35a9f42fad7bb7732af65b071740075bfecb7590a4e0050957ed5474f78699"
      "01a20acdae64e6df52d6f2134527248ae0cb376f7f57ba0ccdcea17121a5d590cf5250f5"
      "f3a72beba4b97a35ad4a43522693dee411aa971240144a4402e8095672998d530786e603"
      "846efe40a7106b0b14d97ceb08fbb63270ee5660aab5c8b2b43c317bcab008a88f632f45"
      "42f3cb7c521579e3ca0874141d224ae4a664e09c4f70a8d763ff6088e5fd4e88a3225327"
      "40bef7dbddad6316d4938239ab524858739ae67d9be351d50403d6ddfcddb5616dc6fec7"
      "367fff315c3fe482a4385d4d3cc7522993f352803d45f4d5ce2b97004083cf6488f39513"
      "bcaaa9048d64738dd5415ef5db528241d3cec16e59f33e0182bd5b02f2f9742edf1609be"
      "992f9486aff9feb3422c6060d4d313d36e849f8e3ad819940a568a307cb7ab58c68664d4"
      "4c615acdc1b0fc36f6b057f74e3a6444fa08b36dcc1ec2001c0b2cb6d51abe5f73583462"
      "f8c5c99847afbd7f1466c11a9418bac7bacc90299da6c44269cd4c16158dd776efca338c"
      "83ba8771edf2ed09b0985c4f1a0258067c7c406b47488c7b96ef190fbad8b981d31dac62"
      "16158673964f374708b7ea014d7acf4ad2571ba793628c76eddb4aaa30321824f03a172e"
      "26d34973f7128c054c3182c819b343465dd0d14bc6f5f0f21a754f0c56dd8c085dfe78f6"
      "f7cea710752c776dfc6b0f75ca7b77e5869f7b416a5bfdc1e86572081f79dbe31bbde523"
      "00b3375d5d9c4a3b9c57ce008bd0d48cf9baa5189ba250cfcf96027fcbdce9051da107f6"
      "ecfee022b5fef1c7113540a167afb1ec41ada970702e1f7cfd4b18195005984f959ebaab"
      "c0b385aa7f8d1106fd5880f2ca6cfc364be7704f47b3240d18d96cc9dcae12ad82ca6906"
      "d27386a6a1ef50947300992ed4e3d54e03c514398e0a8b48035d18cd98408149e0ec8085"
      "e90a6751ec24df2d4a83316e72a7a21ff93b0a7a344652356db889f31a0c7f767175f058"
      "5b82c79539f2e0acca2fda1c65ff33cc412d3745382e452e2db5ec392b8777436b28decd"
      "806574fbb1016b32ce4bbc0fc9eadbf5314d68173d73b11778087bf0ef996c460b121fd6"
      "f533b6315b1d280347f336b9318aae1c3f4018f6ad31867e97df3b9f99bc1ca81f5ed510"
      "3900b17bcc1137d70c799e222fcc22dacef68c7609e7c5062fdb8306a8f7213df42c9d92"
      "0cd2eb0ae73e06e19c9ccd88d288f90b648bbf3eb863be09c854ad05955a657ecbc716ef"
      "5eacde389d7cc19bf76b2cab4ced6b196de81bdec5e2e61962c83f762eb8b52d0c7b02cb"
      "68c7b12cc6b57294b6f6cfc0724fa874292afc38a1c75d919371442ba74f0118462c58db"
      "bd71cd8068c8f2bdc68c7b404d8b0c2243211f66c8289a013ee3f6d745a20f85d5023212"
      "70807ca1a1d79aa0532e9128e684dd0cc7fae4d850975b777eb5e2603cd1dd834596ddac"
      "06141112ae2a580ab18012bdaec685973d06982b022ff2495eee31b368d7796135b9cf11"
      "679cfa23c5b7ad6a966bc12ea244fdb16c650e60734024a50151c20186a7d80b7d729231"
      "ff98bf632d6174b6c30ff0c3b2d4444a060ebde8faa8d0c6d9b6d9633884f8250efca71c"
      "9e590ba369228d7354697a9517e7d14906cc831a9c97dc99bf0a06295af3f767fe058b0d"
      "a8d7bc18acb154b20249def5db48c18aab8fd116a72824278d893610e3c574e905e01d63"
      "8497aeabf9909df3562422803f31e26514ed431519e5275578e03c9e5a57c2dd7b888ff4"
      "9ed2d3094af9df947f219df807e9783d4ba9c31ff46176c1562078946af3e00ee712a75b"
      "84fad7bd5df3a01bf86a1f3a78678f4f59de1d48129749aaa14760039d8431038af5ac29"
      "1d6691d77b88971b5dbe1531d85e4b08f6f0c3a1e3901648de35648add961f8c17e6da1f"
      "d040e120316d2bfe9b16066c75eda0aa95472a412c5da5d6d14de8b47efa6cd40970c836"
      "a6c4fabe8b8f2f81e8239a26def6a1c6cc7081f857db7ef3ebc2fb6a2b4f691804a4d30f"
      "0f47ea6eedf0326fbd82b06dc0d7f93a498e82170f48a3def7fb2838bdb17ce2fd94bc32"
      "0836ea1fd12063db85508ccbad18ddd7aeee007e9150c1387333a780ced3ed07a8bfbc3f"
      "f7fab8a5af4a99eda8577d09a1e09a9bffbfb61ca9211d6e6569fc6b12ea6313caff48ef"
      "84964177263c5d57d83d146c82b53a0519590a6f66f2594fe551c3dd189eaa987fb04c15"
      "41c886b3681e04b9ab3127267c9714f8c0616a4fd5c94a3093be144495dc9e59546031f5"
      "3b1432d8a25128380ec6b6000989764b856173aa5855a3232cab89cc4cb8d011148ca008"
      "383b0b11680a5e378064b4c2d671db0b4cded21e2d364ddd31925d94cc270f7e4852aa1d"
      "b67554f782d44754c5808776fcfef6e98284b2aa5da1442241b5d2e1cdd89404d00c2e90"
      "bde7cf33f43711e879fd2a392b05ed6c954160ab5c22b3e4ae7487041e3ba763f905b4c1"
      "cee6c9487f6b7db43c2ff831d556bfde390bdb2045fc902c593ff1984061007ea441f7e5"
      "af261337bc2ff5cc6f4805b5a6fd3f5d38eb5e1f47240384be6d6e5662f1383fbaa8a6a7"
      "c15c132778b6cc4e8742632f8cbd8132517acb85c63cecf41f7a3c953377d882de27f705"
      "3a4495652e2c5105f98fc4082125cb06358a45a11f5856b69593122ef00e75b7a918aca8"
      "1af818ad40223e06b994c38c361df9ce7f07106ea39a1682d84e8906e1d84853a0aa5547"
      "efb1f32a620003f3f94ec8a4223be4a5af5d5e159e6ad656ef3bfcad7b14a98f33548d13"
      "2b413a27a6f51f19d34c6da916e85d41d509881b1217b0b5c0ab87400466c91156c7dedb"
      "38525c8a9632aea6a510821c9eb7dff6d98ccbb41454df92768ffe082356b75d77cd11cd"
      "d0fb38fdec1a47c46e372d9a5921f7883d7b90199b7d8f0f3ee441c93ff42165a71bf077"
      "323edf64ddb6cdb4d23460c2f79e7770e599782a504531a856f46364727638b033279b4e"
      "06043a6061bf357467e86a2af4fdd610b1ee11cb037a302a25f44dd0ef7b420ad57baee3"
      "2b41fd1a5ade4c75a8a3cc2824bbc8d5c1124adea71434c93c056583926afc9b0c5ca421"
      "a72273822c2f8a0902b0b54961a51ae4fa0e454908833d211c70136a6579bb6bb5ff560e"
      "b4d7ac273cbb61e47cf246f186ba887e5edf89bee11af845fec9f4692a1bfdd3ecf9d925"
      "f8b9f2d1719cf8f4e8b24d3545acd642d3a0a734e910abba9aa2ea47a61ff4259940c525"
      "842e5c6e37723b1c927b014bb4e8b321aee575f8efb24947b4c8d71d0e074528584a6d99"
      "d60e56c71f3190deb9033e3a610c740de600a0683b7266189a708dc1052f334a54dbdba5"
      "23bd522e0abaf08e1d62dee6769511a7faf6e7147288a4fbb4aac39f4a2683c020a99e55"
      "7936e284eabc9b1ec826db2b996f83088b15f96162d43a4f1342aba5e21be19609a58986"
      "e72f775c4ea2c43d0c1d12235f3fbb26df4da457a19ef4e90997bf48599151ec400c7dd7"
      "8bf62734f090d02db4494235694ec3d11ed96d5cf4e953dfcc3150872a23c0a1bbde99df"
      "541e4636cad6530c772dfa53adb8af85ed2c1a757211c781f7092e1c4c0bfa033124410b"
      "a54539f90058d8a26ac8820adda946274f3e951d1d7ac7531a24afc76ba39620f1e7b066"
      "ed46d9f8dd8e587943ab80b10f160473d1c56842a4cbf02edf42ce01d82cdb7387c40b33"
      "9ce6680c41e1dfc93d096705064733a57a4a461ef2bf103cc60af121fbb0727d4db5e381"
      "c56cc4f668d378dce27d5855fcd02cd2c1bc0d0c85852634f7407ccba94945444283ed33"
      "cbc2b76739ea2dc09839665541fff021a80cf252e594bf74ff11a9d562859e0e8c285ba5"
      "7dbe24e5771591c0d9e80413ec2b09d536c397c215b520c04cec5b17e122d9cad7ca7e68"
      "07e0717e2256d030f68f1ca4e769cd75ee56be9b2822eaccc1917f7a27f567627b0ef52a"
      "dc28f9007b1da960274ee0b6f039886e926fd610adf44cfac9af4dc3d18462692d683224"
      "643ba3873c01cf3e967ec37b3f964946ef4ad529cc34a31eb6591bb99efdf11f4f42ef8c"
      "472baef367631873eb82131d1d97288dd9f251a1e6023803bc6eae3a06d5c8c8e6b98503"
      "dfbf7fb4128f8828ce9ab097716107bfee33428c46070608e50c325dc9070ebcf9ce936e"
      "39cd9e2a728bc9c58ce4f23f2c6b8920fa1d86349fba443f5e904c81ec805439808db48a"
      "279a7e4ce0301c54eb40b1b50c26510f22673cd7859c8762d1fb460c16a68bf77e9baea6"
      "9ec5baf9179798fbc19dc017ff9f3e57539bc5ddcae03358a8450cfb6e9a4774531c075c"
      "fb5bf0440012c822635d90ea2dfc13ff74c79d4e9c5cea723cbd5275a53d25556ebd2cb4"
      "432def00aa3d87377f27a2b0319bf248028a4cec3efbebdf634e568e4e76e1d9470f6417"
      "673e49df0bb59c4ff335adf201f00c16e4a5368a1bc6c5d4c8e80dd2f19add05854d5ffa"
      "812d6693b79f519c3e246b5b8626a303c2e04570acecfc3105ded5210170e1c93274a999"
      "959979d3b5b552024bb224db0c837deb9ab2d010aa73c03f200387f4ae946e2a5f8d25fb"
      "b7ce0144127281564f51e82a2d8edfa1de6e0923cef18d853f3fb59ce7a5d16a0c51d9da"
      "7aaf14e2c45f7dcf177ed3ba952f4f2426436fdf2614c3c02c1cd4e3e1592571af36c62f"
      "d21ce91dbc35bf27770c2f3cd355dbfc0739d024a6ee16321366b50c5aa7051c88f1fc04"
      "0a47ac026b86d323ddbc555f243a5961e0a0b112cf8a7af9399733e459c6a483ed80db6a"
      "4db24518dc54214751139f5b4c02dbaa0dc0587fb05e49cfd9277a29cd4f8b17a1c34f3b"
      "76cbd73236c59ab90fbc2e2a2cca45352c52b9472d97c71a8b93f1e28fec92259a2f15bb"
      "bada52626353b2a1c3f5051e19a2a68577027658f4cd78fddabc63343db8bcfac0f30475"
      "73146211ae910815b518bc21ee0c0be5ced2922539ec8e25632f29e6aef15fd0fb496961"
      "52f2499b0ebe25479468ffdc6a8cb1f4b8e3740fceb205fa350ff2398dd2675393cd082f"
      "e8366a72776fe26d763733ee7202ba30e35650cbf3a06f7264705a857a2ae150c0bf6646"
      "f3e77033cfb2433d2b324c0f3a80836a6672272b2e16422eac18c8e84f56042fc31217ba"
      "5344635f6705aa115bd12b9acdeb144b84cf44f1018467d6b7b59ea72e2ba6c7723f6829"
      "2890962ca6decafed283891f99cd08c5ec31e5bd612fa1fa5c8ecf418d81ed4742807a26"
      "d4c5012a134679cdac6f849f6e26cc8e78cb369d7946f56eb362f2fcd4955c30b19f88e5"
      "b77310de51606f949a8f810497fb730a53b97d799e33d53bcc50441852f63caf1cd28c4b"
      "b01bb6a979cd7d663e0aa4d2d22eea9a26ebfb324c725c2a9be630b8a91b8fabd575273a"
      "2b104c30c83eda8bb630c46d47ee91742d6881805da6035cb634bad4dfdc1102973d4db1"
      "6c665d2d4e594579e586380399a6b70d3254df4c5918932c9136d23f6ba568bc1609c9bd"
      "acfe8eb19b6f09baa66d24176ce076e8ffdb3eedb0efb212c25fd4e76365ba1a98d884ce"
      "463af052e1f2f40fdec2a2c9a531e84861baa643882aefd158bac1dae86f5317564d4242"
      "13797c027c08b957d590a129c5730bf733667bed208ec4e9e41eb3fc07c763fa91679b39"
      "662c49b75b6a9286af8c3e7ec5227b60acc5a212d86927932d6f978826c3dfaca0086917"
      "b42ff6534bc95c2777da60fb78b96650050e113c910d03cd665d21ba11a863b3eeba1373"
      "82785b07a487c93104e96c62405cb6941351b6018f4b2ba77c0e98ca4d703cac112f9a77"
      "3947867391c1be26b2821b90e31c8a7919bd2db59f3c6dc8cb6cd0c6d2abcae187eb1b96"
      "4b23bac02cbcb7b46e6a84cd7129111c4939418c3bcf1453827145d21eddc18cb7ba7e5e"
      "762114465d6ab59d2c3e0d2780d3e38cafee283ddff3f8f2a5975ad57d66494531498c53"
      "c2a3d702a456899068b3d7c4e7555c5ff9cf0d330f43be47da372961870a9969f7d1abf5"
      "43244a8a9a94b6f32189fa5c473dcb278384c199794026c16bc85be333ab81384d85d19e"
      "b09cbaa5e8abb6f19b575d40240d12a5484f382e59235531397a3d1f0e86751a2ff639e7"
      "ca3c906e0971b95d73b1f77de5b1f75a898d85734957b66e9321d98a7d68a001479a0afd"
      "b7cc559c19bd4d348a40c96e437f444d7e445c089566300aa062a29d1a7f6b752778cf96"
      "16ff6e130d9392d3bbc8153002dcac855360c1b47767dcaa0b2f973dbb49cfa3998421cb"
      "72c1344b7826d03f180d64ac4ae5a2bd2de28450c5ef9a57fa437f482bd143c534d7e98e"
      "8e1da9712c73aafd4e044294397bed605314dc8800022a18373141439224bfb6316ffbb2"
      "b193e0f4aafac2bdcaa9b7f4b7ef412bbce45ac630a387592db05ba7a710125e4d63e809"
      "310c7b8c4248a70cb4498c4678195cdf72392fc667f40c4b69497340237f6588268eed00"
      "c4fceeeaf91076fe3a720e113da1f2941f28391f063b51596bd6e9b92247268c1a1444c1"
      "d5825140a1252d97777400ab622fb197f90b9ef155b80364fe2cdba114b2149ddbb3b897"
      "2672327210f63a02dbc6e8689884278f3cb757fe6e35072ba1aaedd2f60a8652cbfaa285"
      "52564663e19a31664d1e1964ff074b4e76468a36135dabac7dc7504048796a036093e8b0"
      "d4397ceb7a492c584298932b35ababba8d447f4e34e12aca1e7f842c8994c1af11663667"
      "9942dc7cfae9e6a9d088d526cff6b509a58bedfcd1ee6f3d481df1f47a525394ba86a225"
      "0e4ad714ab5196119c2d7e84a948569af651ff1e31cefbe887fced8a927c7e69ad8462ba"
      "e17fd695973fade92805a70769bd188bb1795a30922c046897c2a4e283eb51bf72282a04"
      "038517c5e40b6542b891758944fc4679628da70cb708ed8fcee7dc2fc412f1b1a51c2844"
      "48861ccd0294d1197cb537cdb6a24cfd1afcafd4d2bfd7c73d9d893c901a4c5473b7052a"
      "623bacefbf1f26c9e66ffdad45273e8dd7149d5dc81dc4a0f3c8a853ff7899f8ba5cc7c5"
      "d2461ae272eb2da0f170af1d8aa82d43660f541d",
      ToHex(captured_data));

  {
    // Cleanup old MockOrchardSyncStateProxy to prevent dangling pointer error
    base::SequenceBound<MockOrchardSyncStateProxy> empty_sync_state(
        base::ThreadPool::CreateSequencedTaskRunner({base::MayBlock()}),
        db_path(), nullptr);
    OverrideSyncStateForTesting(std::move(empty_sync_state));
    task_environment_.RunUntilIdle();
  }
}

TEST_F(ZCashWalletServiceUnitTest, ShieldSync) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndEnableFeatureWithParameters(
      features::kBraveWalletZCashFeature,
      {{"zcash_shielded_transactions_enabled", "true"}});

  keyring_service()->Reset();
  keyring_service()->RestoreWallet(kGateJuniorMnemonic, kTestWalletPassword,
                                   false, base::DoNothing());
  GetAccountUtils().EnsureAccount(mojom::KeyringId::kZCashMainnet, 0);

  keyring_service()->SetZCashAccountBirthday(
      account_id(), mojom::ZCashAccountShieldBirthday::New(100u, "hash"));

  auto account_1 =
      GetAccountUtils().EnsureAccount(mojom::KeyringId::kZCashMainnet, 0);
  auto account_id_1 = account_1->account_id.Clone();

  {
    base::MockCallback<ZCashWalletService::IsSyncInProgressCallback>
        is_sync_in_progress_callback;
    EXPECT_CALL(is_sync_in_progress_callback,
                Run(testing::Eq(false), testing::Eq(std::nullopt)));
    zcash_wallet_service_->IsSyncInProgress(account_id(),
                                            is_sync_in_progress_callback.Get());
    task_environment_.RunUntilIdle();
  }

  {
    base::MockCallback<ZCashWalletService::StartShieldSyncCallback> callback;
    zcash_wallet_service_->StartShieldSync(account_id(), 0, callback.Get());
  }

  {
    base::MockCallback<ZCashWalletService::IsSyncInProgressCallback>
        is_sync_in_progress_callback;
    EXPECT_CALL(is_sync_in_progress_callback,
                Run(testing::Eq(true), testing::Eq(std::nullopt)));
    zcash_wallet_service_->IsSyncInProgress(account_id(),
                                            is_sync_in_progress_callback.Get());
    task_environment_.RunUntilIdle();
  }

  {
    base::MockCallback<ZCashWalletService::StopShieldSyncCallback> callback;
    zcash_wallet_service_->StopShieldSync(account_id(), callback.Get());
  }

  {
    base::MockCallback<ZCashWalletService::IsSyncInProgressCallback>
        is_sync_in_progress_callback;
    EXPECT_CALL(is_sync_in_progress_callback,
                Run(testing::Eq(false), testing::Eq(std::nullopt)));
    zcash_wallet_service_->IsSyncInProgress(account_id(),
                                            is_sync_in_progress_callback.Get());
    task_environment_.RunUntilIdle();
  }
}

TEST_F(ZCashWalletServiceUnitTest, ShieldSync_FeatureDisabled) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndEnableFeatureWithParameters(
      features::kBraveWalletZCashFeature,
      {{"zcash_shielded_transactions_enabled", "false"}});

  {
    base::MockCallback<ZCashWalletService::IsSyncInProgressCallback> callback;
    EXPECT_CALL(callback, Run(testing::Eq(false), testing::Ne(std::nullopt)));
    zcash_wallet_service_->IsSyncInProgress(account_id(), callback.Get());
    task_environment_.RunUntilIdle();
  }

  {
    base::MockCallback<ZCashWalletService::StopShieldSyncCallback> callback;
    EXPECT_CALL(callback, Run(testing::Ne(std::nullopt)));
    zcash_wallet_service_->StopShieldSync(account_id(), callback.Get());
  }

  {
    base::MockCallback<ZCashWalletService::StopShieldSyncCallback> callback;
    EXPECT_CALL(callback, Run(testing::Ne(std::nullopt)));
    zcash_wallet_service_->StartShieldSync(account_id(), 0, callback.Get());
  }
}

#endif
#endif  // BUILDFLAG(ENABLE_ORCHARD)

TEST_F(ZCashWalletServiceUnitTest,
       OnCompleteTransactionTaskDone_InvalidTransaction) {
  // Create an invalid transaction where inputs != outputs + fee
  ZCashTransaction invalid_tx;
  invalid_tx.set_fee(5000u);

  // Add transparent input
  auto& input = invalid_tx.transparent_part().inputs.emplace_back();
  input.utxo_value = 10000u;

  // Add transparent output that makes the transaction invalid
  // 10000 (input) != 6000 (output) + 5000 (fee) = 11000
  auto& output = invalid_tx.transparent_part().outputs.emplace_back();
  output.amount = 6000u;

  // Create a task and add it to the complete_transaction_tasks_ set
  auto [task_it, inserted] =
      zcash_wallet_service_->complete_transaction_tasks_.insert(
          std::make_unique<ZCashCompleteTransactionTask>(
              zcash_wallet_service_->CreatePassKeyForTesting(),
              *zcash_wallet_service_,
              zcash_wallet_service_->CreateActionContext(account_id()),
              *keyring_service(), invalid_tx));
  ASSERT_TRUE(inserted);
  auto* task_ptr = task_it->get();

  // Create a dummy callback
  base::MockCallback<ZCashWalletService::SignAndPostTransactionCallback>
      callback;

  // Create a dummy result
  base::expected<ZCashTransaction, std::string> result =
      base::ok(ZCashTransaction());

  // EXPECT_DEATH should trigger because ValidateAmounts will fail
  // and the CHECK will abort
  EXPECT_DEATH_IF_SUPPORTED(
      zcash_wallet_service_->OnCompleteTransactionTaskDone(
          task_ptr, account_id(), invalid_tx, callback.Get(), result),
      "");
}

TEST_F(ZCashWalletServiceUnitTest,
       OnCompleteTransactionTaskDone_InvalidResultTransaction) {
  ZCashTransaction valid_tx;
  {
    valid_tx.set_fee(5000u);
    auto& input = valid_tx.transparent_part().inputs.emplace_back();
    input.utxo_value = 10000u;
    auto& output = valid_tx.transparent_part().outputs.emplace_back();
    output.amount = 5000u;
  }
  EXPECT_TRUE(valid_tx.ValidateAmounts());

  // Create a task and add it to the complete_transaction_tasks_ set
  auto [task_it, inserted] =
      zcash_wallet_service_->complete_transaction_tasks_.insert(
          std::make_unique<ZCashCompleteTransactionTask>(
              zcash_wallet_service_->CreatePassKeyForTesting(),
              *zcash_wallet_service_,
              zcash_wallet_service_->CreateActionContext(account_id()),
              *keyring_service(), valid_tx));
  ASSERT_TRUE(inserted);
  auto* task_ptr = task_it->get();

  // Create a dummy callback
  base::MockCallback<ZCashWalletService::SignAndPostTransactionCallback>
      callback;

  ZCashTransaction result_invalid_tx = valid_tx;
  result_invalid_tx.set_fee(1000u);
  EXPECT_FALSE(result_invalid_tx.ValidateAmounts());

  base::expected<ZCashTransaction, std::string> result =
      base::ok(result_invalid_tx);

  // EXPECT_DEATH should trigger because ValidateAmounts will fail for the
  // result_invalid_tx and the CHECK will abort
  EXPECT_DEATH_IF_SUPPORTED(
      zcash_wallet_service_->OnCompleteTransactionTaskDone(
          task_ptr, account_id(), result_invalid_tx, callback.Get(), result),
      "");
}

}  // namespace brave_wallet
