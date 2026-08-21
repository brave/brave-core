/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/zcash/zcash_wallet_service.h"

#include <array>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <utility>

#include "base/functional/callback_helpers.h"
#include "base/memory/scoped_refptr.h"
#include "base/strings/string_number_conversions.h"
#include "base/test/bind.h"
#include "base/test/mock_callback.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/scoped_run_loop_timeout.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "base/threading/sequence_bound.h"
#include "brave/components/brave_wallet/browser/internal/orchard_bundle_manager.h"
#include "brave/components/brave_wallet/browser/internal/orchard_sync_state.h"
#include "brave/components/brave_wallet/browser/internal/orchard_test_utils.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/browser/test_utils.h"
#include "brave/components/brave_wallet/browser/zcash/v5_zcash_serializer.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_auto_sync_manager.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_complete_transaction_task_v5.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_rpc.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_test_utils.h"
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

using testing::_;
using testing::Eq;
using testing::SaveArg;
using testing::Truly;
using testing::WithArg;

namespace brave_wallet {

namespace {

constexpr char kGateJuniorMnemonic[] =
    "gate junior chunk maple cage select orange circle price air tortoise "
    "jelly art frequent fence middle ice moral wage toddler attitude sign "
    "lesson grain";

std::array<uint8_t, 32> GetTxId(const std::string& hex_string) {
  std::vector<uint8_t> vec;
  std::array<uint8_t, 32> sized_vec;

  base::HexStringToBytes(hex_string, &vec);
  std::reverse(vec.begin(), vec.end());
  std::copy_n(vec.begin(), 32, sized_vec.begin());
  return sized_vec;
}

#if !BUILDFLAG(IS_ANDROID)
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

class MockOrchardSyncState : public OrchardSyncState {
 public:
  using OrchardSyncState::OrchardSyncState;
  ~MockOrchardSyncState() override {}

  MOCK_METHOD3(
      GetSpendableNotes,
      base::expected<std::optional<OrchardSyncState::SpendableNotesBundle>,
                     OrchardStorage::Error>(
          OrchardPool pool,
          const mojom::AccountIdPtr& account_id,
          const OrchardAddrRawPart& internal_addr));

  MOCK_METHOD4(CalculateWitnessForCheckpoint,
               base::expected<std::vector<OrchardInput>, OrchardStorage::Error>(
                   OrchardPool pool,
                   const mojom::AccountIdPtr& account_id,
                   const std::vector<OrchardInput>& notes,
                   uint32_t checkpoint_position));

  MOCK_METHOD3(GetMaxCheckpointedHeight,
               base::expected<std::optional<uint32_t>, OrchardStorage::Error>(
                   const mojom::AccountIdPtr& account_id,
                   uint32_t chain_tip_height,
                   uint32_t min_confirmations));
};

}  // namespace

class ZCashWalletServiceUnitTest : public testing::Test {
 public:
  ZCashWalletServiceUnitTest() = default;
  ~ZCashWalletServiceUnitTest() override = default;

  void SetUp() override {
#if BUILDFLAG(IS_IOS)
    feature_list_.InitWithFeaturesAndParameters(
        {{features::kBraveWalletZCashFeature,
          { {"zcash_shielded_transactions_enabled", "true"} }},
         { features::kBraveWalletWebUIFeature,
           {} }},
        {}  // disabled features
    );
#else
    feature_list_.InitAndEnableFeature(
        brave_wallet::features::kBraveWalletZCashFeature);
#endif

    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    brave_wallet::RegisterProfilePrefs(prefs_.registry());
    brave_wallet::RegisterLocalStatePrefs(local_state_.registry());
    keyring_service_ =
        std::make_unique<KeyringService>(nullptr, &prefs_, &local_state_);

    zcash_wallet_service_ = std::make_unique<TestingZCashWalletService>(
        *keyring_service_, std::make_unique<testing::NiceMock<MockZCashRPC>>());
    zcash_wallet_service_->SetupSyncState(
        OrchardSyncState::CreateSyncStateSequence(),
        std::make_unique<MockOrchardSyncState>(temp_dir_.GetPath()));

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

    ON_CALL(mock_orchard_sync_state(), GetSpendableNotes(_, _, _))
        .WillByDefault([](OrchardPool pool, const mojom::AccountIdPtr&,
                          const OrchardAddrRawPart&) {
          return OrchardSyncState::SpendableNotesBundle();
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

  std::map<mojom::AccountIdPtr, std::unique_ptr<ZCashAutoSyncManager>>&
  auto_sync_managers() {
    return zcash_wallet_service_->auto_sync_managers_;
  }

  MockOrchardSyncState& mock_orchard_sync_state() {
    return static_cast<MockOrchardSyncState&>(
        *zcash_wallet_service_->sync_state_ptr);
  }

  KeyringService* keyring_service() { return keyring_service_.get(); }

 protected:
  base::test::ScopedFeatureList feature_list_;
  base::test::TaskEnvironment task_environment_;
  base::ScopedTempDir temp_dir_;

  mojom::AccountInfoPtr zcash_account_;

  sync_preferences::TestingPrefServiceSyncable prefs_;
  sync_preferences::TestingPrefServiceSyncable local_state_;

  std::unique_ptr<KeyringService> keyring_service_;
  std::unique_ptr<TestingZCashWalletService> zcash_wallet_service_;
};

TEST_F(ZCashWalletServiceUnitTest, GetBalance) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndEnableFeatureWithParameters(
      features::kBraveWalletZCashFeature,
      {{"zcash_shielded_transactions_enabled", "false"},
       {"zcash_ironwood_enabled", "false"}});
  auto account =
      GetAccountUtils().EnsureAccount(mojom::KeyringId::kZCashMainnet, 1);
  keyring_service()->UpdateNextUnusedAddressForZCashAccount(account->account_id,
                                                            1, 0);

  ON_CALL(zcash_rpc(), GetLatestBlock(_, _))
      .WillByDefault(  //
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
      .WillByDefault(  //
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
        EXPECT_EQ(balance->orchard_balance, 0u);
      });

  zcash_wallet_service_->GetBalance(account->account_id.Clone(),
                                    balance_callback.Get());
  task_environment_.RunUntilIdle();
}

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
      .WillByDefault(  //
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
      .WillByDefault(  //
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

  ON_CALL(mock_orchard_sync_state(), GetSpendableNotes(_, _, _))
      .WillByDefault([](OrchardPool pool, const mojom::AccountIdPtr& account_id,
                        const OrchardAddrRawPart& internal_addr) {
        if (pool != OrchardPool::kOrchard) {
          return OrchardSyncState::SpendableNotesBundle();
        }
        OrchardSyncState::SpendableNotesBundle spendable_notes_bundle;
        {
          OrchardNote note;
          note.amount = 10u;
          note.note_version = 2;
          spendable_notes_bundle.all_notes.push_back(note);
          spendable_notes_bundle.spendable_notes.push_back(note);
        }
        {
          OrchardNote note;
          note.amount = 20u;
          note.note_version = 2;
          spendable_notes_bundle.all_notes.push_back(note);
        }
        return spendable_notes_bundle;
      });

  base::MockCallback<ZCashWalletService::GetBalanceCallback> balance_callback;
  EXPECT_CALL(balance_callback, Run(_, _))
      .WillOnce([&](mojom::ZCashBalancePtr balance,
                    std::optional<std::string> error) {
        EXPECT_EQ(balance->total_balance, 20u);
        EXPECT_EQ(balance->transparent_balance, 10u);
        EXPECT_EQ(balance->orchard_balance, 10u);
        EXPECT_EQ(balance->orchard_pending_balance, 20u);
      });
  zcash_wallet_service_->GetBalance(account->account_id.Clone(),
                                    balance_callback.Get());
  task_environment_.RunUntilIdle();
}

TEST_F(ZCashWalletServiceUnitTest, GetBalanceWithShielded_FeatureDisabled) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndEnableFeatureWithParameters(
      features::kBraveWalletZCashFeature,
      {{"zcash_shielded_transactions_enabled", "false"},
       {"zcash_ironwood_enabled", "false"}});
  keyring_service()->Reset();
  keyring_service()->RestoreWallet(kMnemonicDivideCruise, kTestWalletPassword,
                                   false, base::DoNothing());
  auto account =
      GetAccountUtils().EnsureAccount(mojom::KeyringId::kZCashMainnet, 1);
  keyring_service()->UpdateNextUnusedAddressForZCashAccount(account->account_id,
                                                            1, 0);

  ON_CALL(zcash_rpc(), GetLatestBlock(_, _))
      .WillByDefault(  //
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
      .WillByDefault(  //
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
  note.note_version = 2;

  auto update_notes_callback = base::BindLambdaForTesting(
      [](base::expected<OrchardStorage::Result, OrchardStorage::Error>) {});

  OrchardBlockScanner::Result result = CreateResultForTesting(
      OrchardTreeState(), std::vector<OrchardCommitment>(), 50000, "hash50000");
  result.orchard.discovered_notes = std::vector<OrchardNote>({note});
  result.orchard.found_spends = std::vector<OrchardNoteSpend>();

  zcash_wallet_service_->sync_state()
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
        EXPECT_EQ(balance->orchard_balance, 0u);
      });
  zcash_wallet_service_->GetBalance(account->account_id.Clone(),
                                    balance_callback.Get());
  task_environment_.RunUntilIdle();
}

TEST_F(ZCashWalletServiceUnitTest, GetBalanceWithShielded_IronwoodEnabled) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndEnableFeatureWithParameters(
      features::kBraveWalletZCashFeature,
      {{"zcash_shielded_transactions_enabled", "true"},
       {"zcash_ironwood_enabled", "true"}});
  keyring_service()->Reset();
  keyring_service()->RestoreWallet(kMnemonicDivideCruise, kTestWalletPassword,
                                   false, base::DoNothing());

  auto account =
      GetAccountUtils().EnsureAccount(mojom::KeyringId::kZCashMainnet, 1);
  keyring_service()->UpdateNextUnusedAddressForZCashAccount(account->account_id,
                                                            1, 0);

  ON_CALL(zcash_rpc(), GetLatestBlock(_, _))
      .WillByDefault(  //
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
      .WillByDefault(  //
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

  ON_CALL(mock_orchard_sync_state(), GetSpendableNotes(_, _, _))
      .WillByDefault([](OrchardPool pool, const mojom::AccountIdPtr& account_id,
                        const OrchardAddrRawPart& internal_addr) {
        OrchardSyncState::SpendableNotesBundle spendable_notes_bundle;
        if (pool == OrchardPool::kIronwood) {
          {
            OrchardNote note;
            note.amount = 7u;
            note.note_version = 3;
            spendable_notes_bundle.all_notes.push_back(note);
            spendable_notes_bundle.spendable_notes.push_back(note);
          }
          {
            OrchardNote note;
            note.amount = 13u;
            note.note_version = 3;
            spendable_notes_bundle.all_notes.push_back(note);
          }
          return spendable_notes_bundle;
        }
        {
          OrchardNote note;
          note.amount = 10u;
          note.note_version = 2;
          spendable_notes_bundle.all_notes.push_back(note);
          spendable_notes_bundle.spendable_notes.push_back(note);
        }
        {
          OrchardNote note;
          note.amount = 20u;
          note.note_version = 2;
          spendable_notes_bundle.all_notes.push_back(note);
        }
        return spendable_notes_bundle;
      });

  base::test::TestFuture<mojom::ZCashBalancePtr, std::optional<std::string>>
      balance_future;
  zcash_wallet_service_->GetBalance(
      account->account_id.Clone(),
      balance_future.GetCallback<mojom::ZCashBalancePtr,
                                 const std::optional<std::string>&>());

  const auto& balance = balance_future.Get<0>();
  EXPECT_EQ(balance->transparent_balance, 10u);
  EXPECT_EQ(balance->orchard_balance, 10u);
  EXPECT_EQ(balance->orchard_pending_balance, 20u);
  EXPECT_EQ(balance->ironwood_balance, 7u);
  EXPECT_EQ(balance->ironwood_pending_balance, 13u);
  EXPECT_EQ(balance->total_balance, 27u);
}

TEST_F(ZCashWalletServiceUnitTest, GetBalance_OrchardNotesOverflow) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndEnableFeatureWithParameters(
      features::kBraveWalletZCashFeature,
      {{"zcash_shielded_transactions_enabled", "true"},
       {"zcash_ironwood_enabled", "false"}});
  keyring_service()->Reset();
  keyring_service()->RestoreWallet(kMnemonicDivideCruise, kTestWalletPassword,
                                   false, base::DoNothing());

  auto account =
      GetAccountUtils().EnsureAccount(mojom::KeyringId::kZCashMainnet, 1);
  keyring_service()->UpdateNextUnusedAddressForZCashAccount(account->account_id,
                                                            1, 0);

  ON_CALL(zcash_rpc(), GetLatestBlock(_, _))
      .WillByDefault(  //
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
        std::move(callback).Run(false);
      });

  ON_CALL(zcash_rpc(), GetUtxoList(_, _, _))
      .WillByDefault(  //
          [&](const std::string& chain_id, const std::string& address,
              ZCashRpc::GetUtxoListCallback callback) {
            auto response = zcash::mojom::GetAddressUtxosResponse::New(
                std::vector<zcash::mojom::ZCashUtxoPtr>());
            std::move(callback).Run(std::move(response));
          });

  ON_CALL(mock_orchard_sync_state(), GetSpendableNotes(_, _, _))
      .WillByDefault([](OrchardPool pool, const mojom::AccountIdPtr& account_id,
                        const OrchardAddrRawPart& internal_addr) {
        OrchardSyncState::SpendableNotesBundle spendable_notes_bundle;
        if (pool != OrchardPool::kOrchard) {
          return spendable_notes_bundle;
        }
        {
          OrchardNote note;
          note.amount = std::numeric_limits<uint64_t>::max();
          note.note_version = 2;
          spendable_notes_bundle.all_notes.push_back(note);
        }
        {
          OrchardNote note;
          note.amount = 1u;
          note.note_version = 2;
          spendable_notes_bundle.all_notes.push_back(note);
        }
        return spendable_notes_bundle;
      });

  base::test::TestFuture<mojom::ZCashBalancePtr, std::optional<std::string>>
      balance_future;
  zcash_wallet_service_->GetBalance(
      account->account_id.Clone(),
      balance_future.GetCallback<mojom::ZCashBalancePtr,
                                 const std::optional<std::string>&>());

  EXPECT_FALSE(balance_future.Get<0>());
  EXPECT_EQ(balance_future.Get<1>(), "Pending balance error");
}

// https://zcashblockexplorer.com/transactions/3bc513afc84befb9774f667eb4e63266a7229ab1fdb43476dd7c3a33d16b3101/raw
TEST_F(ZCashWalletServiceUnitTest, SignAndPostTransaction) {
  {
    auto account =
        GetAccountUtils().EnsureAccount(mojom::KeyringId::kZCashMainnet, 0);
    auto account_id = account->account_id.Clone();
    keyring_service_->UpdateNextUnusedAddressForZCashAccount(account_id, 2, 2);
  }

  ZCashTransaction zcash_transaction;
  zcash_transaction.init_v5_part();
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

TEST_F(ZCashWalletServiceUnitTest, SignTransparentPartV5) {
  auto account =
      GetAccountUtils().EnsureAccount(mojom::KeyringId::kZCashMainnet, 0);
  auto account_id = account->account_id.Clone();
  keyring_service_->UpdateNextUnusedAddressForZCashAccount(account_id, 2, 2);

  // This transaction is the transparent signing vector used by
  // SignAndPostTransaction above.
  ZCashTransaction tx;
  tx.init_v5_part();
  tx.set_consensus_brach_id(0xc2d6d0b4);
  tx.set_locktime(2286687);
  tx.set_expiry_height(2286707);

  ZCashTransaction::TxInput input;
  input.utxo_outpoint.txid = GetTxId(
      "70f1aa91889eee3e5ba60231a2e625e60480dc2e43ddc9439dc4fe8f09a1a278");
  input.utxo_outpoint.index = 0;
  input.utxo_address = "t1c61yifRMgyhMsBYsFDBa5aEQkgU65CGau";
  input.utxo_value = 537000;
  input.script_pub_key =
      ZCashAddressToScriptPubkey(input.utxo_address, false).value();
  tx.transparent_part().inputs.push_back(std::move(input));

  ZCashTransaction::TxOutput recipient;
  recipient.address = "t1KrG29yWzoi7Bs2pvsgXozZYPvGG4D3sGi";
  recipient.amount = 500000;
  recipient.script_pubkey =
      ZCashAddressToScriptPubkey(recipient.address, false).value();
  tx.transparent_part().outputs.push_back(std::move(recipient));

  ZCashTransaction::TxOutput change;
  change.address = "t1c61yifRMgyhMsBYsFDBa5aEQkgU65CGau";
  change.amount = 35000;
  change.script_pubkey =
      ZCashAddressToScriptPubkey(change.address, false).value();
  tx.transparent_part().outputs.push_back(std::move(change));

  ASSERT_TRUE(ZCashV5Serializer::SignTransparentPartV5(*keyring_service_,
                                                       account_id, tx));
  EXPECT_EQ(ToHex(tx.transparent_part().inputs[0].script_sig),
            "0x47304402202fc68ead746e8e93bb661ac79e71e1d3d84fd0f2aac76a8cb"
            "4fa831a847787ff022028efe32152f282d7167c40d62b07aedad73a66c7"
            "a3548413f289e2aef3da96b30121028754aaa5d9198198ecf5fd1849cbf"
            "38a92ed707e2f181bd354c73a4a87854c67");
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
        btc_account_id_1.Clone(), mojom::ZCashTokenType::kTransparent,
        "t1JP7PHu72xHztsZiwH6cye4yvC9Prb3EvQ", callback.Get());
  }

  // Normal transparent address - mainnet.
  {
    base::MockCallback<ZCashWalletService::GetTransactionTypeCallback> callback;
    EXPECT_CALL(callback, Run(Eq(mojom::ZCashTxType::kTransparentToTransparent),
                              Eq(mojom::ZCashAddressError::kNoError)));
    zcash_wallet_service_->GetTransactionType(
        account_id_1.Clone(), mojom::ZCashTokenType::kTransparent,
        "t1JP7PHu72xHztsZiwH6cye4yvC9Prb3EvQ", callback.Get());
  }

  // Testnet address with mainnet account (network mismatch).
  {
    base::MockCallback<ZCashWalletService::GetTransactionTypeCallback> callback;
    EXPECT_CALL(
        callback,
        Run(Eq(mojom::ZCashTxType::kUnknown),
            Eq(mojom::ZCashAddressError::kInvalidAddressNetworkMismatch)));
    zcash_wallet_service_->GetTransactionType(
        account_id_1.Clone(), mojom::ZCashTokenType::kTransparent,
        "tmP3uLtGx5GPddkq8a6ddmXhqJJ3vy6tpTE", callback.Get());
  }

  // Wrong transparent address.
  {
    base::MockCallback<ZCashWalletService::GetTransactionTypeCallback> callback;
    EXPECT_CALL(callback,
                Run(Eq(mojom::ZCashTxType::kUnknown),
                    Eq(mojom::ZCashAddressError::kInvalidTransparentAddress)));
    zcash_wallet_service_->GetTransactionType(
        account_id_1.Clone(), mojom::ZCashTokenType::kTransparent, "t1xxx",
        callback.Get());
  }

  // Malformed transparent address.
  {
    base::MockCallback<ZCashWalletService::GetTransactionTypeCallback> callback;
    EXPECT_CALL(callback,
                Run(Eq(mojom::ZCashTxType::kUnknown),
                    Eq(mojom::ZCashAddressError::kInvalidTransparentAddress)));
    zcash_wallet_service_->GetTransactionType(
        account_id_1.Clone(), mojom::ZCashTokenType::kTransparent,
        "t1JP7PHu72xHztsZiwH6cye4yvC9Prb3Ev0", callback.Get());
  }

  // Eth address.
  {
    base::MockCallback<ZCashWalletService::GetTransactionTypeCallback> callback;
    EXPECT_CALL(callback,
                Run(Eq(mojom::ZCashTxType::kUnknown),
                    Eq(mojom::ZCashAddressError::kInvalidTransparentAddress)));
    zcash_wallet_service_->GetTransactionType(
        account_id_1.Clone(), mojom::ZCashTokenType::kTransparent,
        "0xA4bE3C94e8c1B7D2F9e6Bf3E1D9A2cC45B6F9A12", callback.Get());
  }

  // Eth address, shielded pool.
  {
    base::MockCallback<ZCashWalletService::GetTransactionTypeCallback> callback;
    EXPECT_CALL(callback,
                Run(Eq(mojom::ZCashTxType::kUnknown),
                    Eq(mojom::ZCashAddressError::kInvalidUnifiedAddress)));
    zcash_wallet_service_->GetTransactionType(
        account_id_1.Clone(), mojom::ZCashTokenType::kOrchard,
        "0xA4bE3C94e8c1B7D2F9e6Bf3E1D9A2cC45B6F9A12", callback.Get());
  }

  // Unified address - mainnet.
  {
    base::MockCallback<ZCashWalletService::GetTransactionTypeCallback> callback;
    EXPECT_CALL(callback, Run(Eq(mojom::ZCashTxType::kTransparentToTransparent),
                              Eq(mojom::ZCashAddressError::kNoError)));
    zcash_wallet_service_->GetTransactionType(
        account_id_1.Clone(), mojom::ZCashTokenType::kTransparent,
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
        account_id_1.Clone(), mojom::ZCashTokenType::kTransparent,
        "u1lmy8anuylj33arxh3sx7ysq54tuw7zehsv6pdeeaqlrhkjhm3uvl9egqxqfd7hcsp3ms"
        "zp6jxxx0gsw0ldp5wyu95r4mfzlueh8h5xhrjqgz7xtxp3hvw45dn4gfrz5j54ryg6reyf"
        "1",
        callback.Get());
  }

  // Malformed unified address - mainnet, shielded pool.
  {
    base::MockCallback<ZCashWalletService::GetTransactionTypeCallback> callback;
    EXPECT_CALL(callback,
                Run(Eq(mojom::ZCashTxType::kUnknown),
                    Eq(mojom::ZCashAddressError::
                           kInvalidUnifiedAddressMissingOrchardPart)));
    zcash_wallet_service_->GetTransactionType(
        account_id_1.Clone(), mojom::ZCashTokenType::kOrchard,
        "u1lmy8anuylj33arxh3sx7ysq54tuw7zehsv6pdeeaqlrhkjhm3uvl9egqxqfd7hcsp3ms"
        "zp6jxxx0gsw0ldp5wyu95r4mfzlueh8h5xhrjqgz7xtxp3hvw45dn4gfrz5j54ryg6reyf"
        "1",
        callback.Get());
  }

  // Sapling unified address - mainnet.
  {
    base::MockCallback<ZCashWalletService::GetTransactionTypeCallback> callback;
    EXPECT_CALL(callback,
                Run(Eq(mojom::ZCashTxType::kUnknown),
                    Eq(mojom::ZCashAddressError::
                           kInvalidUnifiedAddressMissingOrchardPart)));
    zcash_wallet_service_->GetTransactionType(
        account_id_1.Clone(), mojom::ZCashTokenType::kOrchard,
        "u187vrwl4ampyxd5m6aj38n4ndkmj8v6gs97hkt23aps3sn5k89a0gk2smluexgdprcrtm"
        "5"
        "6ezc5c7tjwlrnnl79tjtrxmqd42c5mpyz7g",
        callback.Get());
  }

  // Sapling unified address - mainnet, shielded pool.
  {
    base::MockCallback<ZCashWalletService::GetTransactionTypeCallback> callback;
    EXPECT_CALL(callback,
                Run(Eq(mojom::ZCashTxType::kUnknown),
                    Eq(mojom::ZCashAddressError::
                           kInvalidUnifiedAddressMissingOrchardPart)));
    zcash_wallet_service_->GetTransactionType(
        account_id_1.Clone(), mojom::ZCashTokenType::kOrchard,
        "u187vrwl4ampyxd5m6aj38n4ndkmj8v6gs97hkt23aps3sn5k89a0gk2smluexgdprcrtm"
        "5"
        "6ezc5c7tjwlrnnl79tjtrxmqd42c5mpyz7g",
        callback.Get());
  }

  // Testnet unified address with mainnet account (network mismatch).
  {
    base::MockCallback<ZCashWalletService::GetTransactionTypeCallback> callback;
    EXPECT_CALL(
        callback,
        Run(Eq(mojom::ZCashTxType::kUnknown),
            Eq(mojom::ZCashAddressError::kInvalidAddressNetworkMismatch)));
    zcash_wallet_service_->GetTransactionType(
        account_id_1.Clone(), mojom::ZCashTokenType::kTransparent,
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
    zcash_wallet_service_->GetTransactionType(
        account_id_1.Clone(), mojom::ZCashTokenType::kTransparent, "u1xx",
        callback.Get());
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
        account_id_1.Clone(), mojom::ZCashTokenType::kTransparent,
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
    zcash_wallet_service_->GetTransactionType(
        account_id_1.Clone(), mojom::ZCashTokenType::kTransparent, "",
        callback.Get());
  }

  // Empty address, shielded pool.
  {
    base::MockCallback<ZCashWalletService::GetTransactionTypeCallback> callback;
    EXPECT_CALL(callback,
                Run(Eq(mojom::ZCashTxType::kUnknown),
                    Eq(mojom::ZCashAddressError::kInvalidUnifiedAddress)));
    zcash_wallet_service_->GetTransactionType(account_id_1.Clone(),
                                              mojom::ZCashTokenType::kOrchard,
                                              "", callback.Get());
  }
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
        account_id_1.Clone(), mojom::ZCashTokenType::kTransparent,
        "tmP3uLtGx5GPddkq8a6ddmXhqJJ3vy6tpTE", callback.Get());
  }

  // Malformed transparent address - testnet.
  {
    base::MockCallback<ZCashWalletService::GetTransactionTypeCallback> callback;
    EXPECT_CALL(callback,
                Run(Eq(mojom::ZCashTxType::kUnknown),
                    Eq(mojom::ZCashAddressError::kInvalidTransparentAddress)));
    zcash_wallet_service_->GetTransactionType(
        account_id_1.Clone(), mojom::ZCashTokenType::kTransparent,
        "tmP3uLtGx5GPddkq8a6ddmXhqJJ3vy6tpT0", callback.Get());
  }

  // Mainnet address with testnet account (network mismatch).
  {
    base::MockCallback<ZCashWalletService::GetTransactionTypeCallback> callback;
    EXPECT_CALL(
        callback,
        Run(Eq(mojom::ZCashTxType::kUnknown),
            Eq(mojom::ZCashAddressError::kInvalidAddressNetworkMismatch)));
    zcash_wallet_service_->GetTransactionType(
        account_id_1.Clone(), mojom::ZCashTokenType::kTransparent,
        "t1JP7PHu72xHztsZiwH6cye4yvC9Prb3EvQ", callback.Get());
  }

  // Wrong transparent address.
  {
    base::MockCallback<ZCashWalletService::GetTransactionTypeCallback> callback;
    EXPECT_CALL(callback,
                Run(Eq(mojom::ZCashTxType::kUnknown),
                    Eq(mojom::ZCashAddressError::kInvalidTransparentAddress)));
    zcash_wallet_service_->GetTransactionType(
        account_id_1.Clone(), mojom::ZCashTokenType::kTransparent, "tmxxx",
        callback.Get());
  }

  // Eth address.
  {
    base::MockCallback<ZCashWalletService::GetTransactionTypeCallback> callback;
    EXPECT_CALL(callback,
                Run(Eq(mojom::ZCashTxType::kUnknown),
                    Eq(mojom::ZCashAddressError::kInvalidTransparentAddress)));
    zcash_wallet_service_->GetTransactionType(
        account_id_1.Clone(), mojom::ZCashTokenType::kTransparent,
        "0xA4bE3C94e8c1B7D2F9e6Bf3E1D9A2cC45B6F9A12", callback.Get());
  }

  // Eth address, shielded pool.
  {
    base::MockCallback<ZCashWalletService::GetTransactionTypeCallback> callback;
    EXPECT_CALL(callback,
                Run(Eq(mojom::ZCashTxType::kUnknown),
                    Eq(mojom::ZCashAddressError::kInvalidUnifiedAddress)));
    zcash_wallet_service_->GetTransactionType(
        account_id_1.Clone(), mojom::ZCashTokenType::kOrchard,
        "0xA4bE3C94e8c1B7D2F9e6Bf3E1D9A2cC45B6F9A12", callback.Get());
  }

  // Unified address - testnet.
  {
    base::MockCallback<ZCashWalletService::GetTransactionTypeCallback> callback;
    EXPECT_CALL(callback, Run(Eq(mojom::ZCashTxType::kTransparentToTransparent),
                              Eq(mojom::ZCashAddressError::kNoError)));
    zcash_wallet_service_->GetTransactionType(
        account_id_1.Clone(), mojom::ZCashTokenType::kTransparent,
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
        account_id_1.Clone(), mojom::ZCashTokenType::kTransparent,
        "utest1vergg5jkp4xy8sqfasw6s5zkdpnxvfxlxh35uuc3me7dp596y2r05t6dv9htwe3p"
        "f8ksrfr8ksca2lskzjanqtl8uqp5vln3zyy246ejtx86vqftp73j7jg9099jxafyjhfm6u"
        "956j0",
        callback.Get());
  }

  // Malformed Unified address - testnet, shielded pool.
  {
    base::MockCallback<ZCashWalletService::GetTransactionTypeCallback> callback;
    EXPECT_CALL(callback,
                Run(Eq(mojom::ZCashTxType::kUnknown),
                    Eq(mojom::ZCashAddressError::
                           kInvalidUnifiedAddressMissingOrchardPart)));
    zcash_wallet_service_->GetTransactionType(
        account_id_1.Clone(), mojom::ZCashTokenType::kOrchard,
        "utest1vergg5jkp4xy8sqfasw6s5zkdpnxvfxlxh35uuc3me7dp596y2r05t6dv9htwe3p"
        "f8ksrfr8ksca2lskzjanqtl8uqp5vln3zyy246ejtx86vqftp73j7jg9099jxafyjhfm6u"
        "956j0",
        callback.Get());
  }

  // Mainnet unified address with testnet account (network mismatch).
  {
    base::MockCallback<ZCashWalletService::GetTransactionTypeCallback> callback;
    EXPECT_CALL(
        callback,
        Run(Eq(mojom::ZCashTxType::kUnknown),
            Eq(mojom::ZCashAddressError::kInvalidAddressNetworkMismatch)));
    zcash_wallet_service_->GetTransactionType(
        account_id_1.Clone(), mojom::ZCashTokenType::kTransparent,
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
    zcash_wallet_service_->GetTransactionType(
        account_id_1.Clone(), mojom::ZCashTokenType::kTransparent, "utest1xx",
        callback.Get());
  }

  // Empty address.
  {
    base::MockCallback<ZCashWalletService::GetTransactionTypeCallback> callback;
    EXPECT_CALL(callback,
                Run(Eq(mojom::ZCashTxType::kUnknown),
                    Eq(mojom::ZCashAddressError::kInvalidTransparentAddress)));
    zcash_wallet_service_->GetTransactionType(
        account_id_1.Clone(), mojom::ZCashTokenType::kTransparent, "",
        callback.Get());
  }

  // Empty address, shielded pool.
  {
    base::MockCallback<ZCashWalletService::GetTransactionTypeCallback> callback;
    EXPECT_CALL(callback,
                Run(Eq(mojom::ZCashTxType::kUnknown),
                    Eq(mojom::ZCashAddressError::kInvalidUnifiedAddress)));
    zcash_wallet_service_->GetTransactionType(account_id_1.Clone(),
                                              mojom::ZCashTokenType::kOrchard,
                                              "", callback.Get());
  }
}

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
      .WillByDefault(  //
          [&](const std::string& chain_id, zcash::mojom::BlockIDPtr block_id,
              ZCashRpc::GetTreeStateCallback callback) {
            EXPECT_EQ(chain_id, mojom::kZCashMainnet);
            EXPECT_EQ(block_id->height, 100000u - kChainReorgBlockDelta);
            auto tree_state = zcash::mojom::TreeState::New(
                "main" /* network */,
                100000u - kChainReorgBlockDelta /* height */,
                "hexhexhex2" /* hash */, 123 /* time */, "" /* sapling tree */,
                "" /* orchard tree */, "" /* ironwood tree */);
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
        .WillOnce(  //
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
        {{"zcash_shielded_transactions_enabled", "true"},
         {"zcash_ironwood_enabled", "true"}});

    base::MockCallback<ZCashWalletService::GetTransactionTypeCallback> callback;
    EXPECT_CALL(callback, Run(Eq(mojom::ZCashTxType::kShieldingIronwood),
                              Eq(mojom::ZCashAddressError::kNoError)));
    auto account_info = keyring_service_->GetZCashAccountInfo(account_id_1);
    zcash_wallet_service_->GetTransactionType(
        account_id_1.Clone(), mojom::ZCashTokenType::kTransparent,
        account_info->orchard_internal_address.value(), callback.Get());
  }

  {
    auto account_2 =
        GetAccountUtils().EnsureAccount(mojom::KeyringId::kZCashMainnet, 0);
    auto account_id_2 = account_2->account_id.Clone();
    base::test::ScopedFeatureList feature_list;
    feature_list.InitAndEnableFeatureWithParameters(
        features::kBraveWalletZCashFeature,
        {{"zcash_shielded_transactions_enabled", "true"},
         {"zcash_ironwood_enabled", "true"}});

    base::MockCallback<ZCashWalletService::GetTransactionTypeCallback> callback;
    EXPECT_CALL(callback, Run(Eq(mojom::ZCashTxType::kShieldingIronwood),
                              Eq(mojom::ZCashAddressError::kNoError)));
    auto account_info = keyring_service_->GetZCashAccountInfo(account_id_2);
    zcash_wallet_service_->GetTransactionType(
        account_id_1.Clone(), mojom::ZCashTokenType::kTransparent,
        account_info->orchard_internal_address.value(), callback.Get());
  }
}

TEST_F(ZCashWalletServiceUnitTest, ValidateUnshielding) {
  auto account_1 =
      GetAccountUtils().EnsureAccount(mojom::KeyringId::kZCashMainnet, 0);
  auto account_id_1 = account_1->account_id.Clone();

  // Feature disabled.
  {
    base::test::ScopedFeatureList feature_list;
    feature_list.InitAndEnableFeatureWithParameters(
        features::kBraveWalletZCashFeature,
        {{"zcash_shielded_transactions_enabled", "false"}});

    base::MockCallback<ZCashWalletService::GetTransactionTypeCallback> callback;
    EXPECT_CALL(callback,
                Run(Eq(mojom::ZCashTxType::kUnknown),
                    Eq(mojom::ZCashAddressError::kInvalidSenderType)));
    auto account_info = keyring_service_->GetZCashAccountInfo(account_id_1);
    zcash_wallet_service_->GetTransactionType(
        account_id_1.Clone(), mojom::ZCashTokenType::kOrchard,
        account_info->next_transparent_receive_address->address_string,
        callback.Get());
  }

  // Acc 1 -> acc 1.
  {
    base::test::ScopedFeatureList feature_list;
    feature_list.InitAndEnableFeatureWithParameters(
        features::kBraveWalletZCashFeature,
        {{"zcash_shielded_transactions_enabled", "true"}});

    base::MockCallback<ZCashWalletService::GetTransactionTypeCallback> callback;
    EXPECT_CALL(callback, Run(Eq(mojom::ZCashTxType::kUnshieldingOrchard),
                              Eq(mojom::ZCashAddressError::kNoError)));
    auto account_info = keyring_service_->GetZCashAccountInfo(account_id_1);
    zcash_wallet_service_->GetTransactionType(
        account_id_1.Clone(), mojom::ZCashTokenType::kOrchard,
        account_info->next_transparent_receive_address->address_string,
        callback.Get());
  }

  // Acc 2 -> acc 1.
  {
    auto account_2 =
        GetAccountUtils().EnsureAccount(mojom::KeyringId::kZCashMainnet, 0);
    auto account_id_2 = account_2->account_id.Clone();
    base::test::ScopedFeatureList feature_list;
    feature_list.InitAndEnableFeatureWithParameters(
        features::kBraveWalletZCashFeature,
        {{"zcash_shielded_transactions_enabled", "true"}});

    base::MockCallback<ZCashWalletService::GetTransactionTypeCallback> callback;
    EXPECT_CALL(callback, Run(Eq(mojom::ZCashTxType::kUnshieldingOrchard),
                              Eq(mojom::ZCashAddressError::kNoError)));
    auto account_info = keyring_service_->GetZCashAccountInfo(account_id_2);
    zcash_wallet_service_->GetTransactionType(
        account_id_1.Clone(), mojom::ZCashTokenType::kOrchard,
        account_info->next_transparent_receive_address->address_string,
        callback.Get());
  }

  // Acc 2 -> transparent addr.
  {
    auto account_2 =
        GetAccountUtils().EnsureAccount(mojom::KeyringId::kZCashMainnet, 0);
    auto account_id_2 = account_2->account_id.Clone();
    base::test::ScopedFeatureList feature_list;
    feature_list.InitAndEnableFeatureWithParameters(
        features::kBraveWalletZCashFeature,
        {{"zcash_shielded_transactions_enabled", "true"}});

    base::MockCallback<ZCashWalletService::GetTransactionTypeCallback> callback;
    EXPECT_CALL(callback, Run(Eq(mojom::ZCashTxType::kOrchardToTransparent),
                              Eq(mojom::ZCashAddressError::kNoError)));
    auto account_info = keyring_service_->GetZCashAccountInfo(account_id_2);
    zcash_wallet_service_->GetTransactionType(
        account_id_1.Clone(), mojom::ZCashTokenType::kOrchard,
        "t1WTZNzKCvU2GeM1ZWRyF7EvhMHhr7magiT", callback.Get());
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
        account_id_1.Clone(), mojom::ZCashTokenType::kTransparent,
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
        {{"zcash_shielded_transactions_enabled", "true"},
         {"zcash_ironwood_enabled", "true"}});

    base::MockCallback<ZCashWalletService::GetTransactionTypeCallback> callback;
    EXPECT_CALL(callback, Run(Eq(mojom::ZCashTxType::kTransparentToIronwood),
                              Eq(mojom::ZCashAddressError::kNoError)));
    zcash_wallet_service_->GetTransactionType(
        account_id_1.Clone(), mojom::ZCashTokenType::kTransparent,
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
        {{"zcash_shielded_transactions_enabled", "true"},
         {"zcash_ironwood_enabled", "true"}});

    base::MockCallback<ZCashWalletService::GetTransactionTypeCallback> callback;
    EXPECT_CALL(callback, Run(Eq(mojom::ZCashTxType::kOrchardToIronwood),
                              Eq(mojom::ZCashAddressError::kNoError)));
    zcash_wallet_service_->GetTransactionType(
        account_id_1.Clone(), mojom::ZCashTokenType::kOrchard,
        "u1ay3aawlldjrmxqnjf5medr5ma6p3acnet464ht8lmwplq5cd3"
        "ugytcmlf96rrmtgwldc75x94qn4n8pgen36y8tywlq6yjk7lkf3"
        "fa8wzjrav8z2xpxqnrnmjxh8tmz6jhfh425t7f3vy6p4pd3zmqa"
        "yq49efl2c4xydc0gszg660q9p",
        callback.Get());
  }
}

TEST_F(ZCashWalletServiceUnitTest, GetTransactionType_IronwoodMatrix) {
  auto account_1 =
      GetAccountUtils().EnsureAccount(mojom::KeyringId::kZCashMainnet, 0);
  auto account_id_1 = account_1->account_id.Clone();

  static constexpr char kOrchardUnifiedAddress[] =
      "u1ay3aawlldjrmxqnjf5medr5ma6p3acnet464ht8lmwplq5cd3"
      "ugytcmlf96rrmtgwldc75x94qn4n8pgen36y8tywlq6yjk7lkf3"
      "fa8wzjrav8z2xpxqnrnmjxh8tmz6jhfh425t7f3vy6p4pd3zmqa"
      "yq49efl2c4xydc0gszg660q9p";
  static constexpr char kTransparentAddress[] =
      "t1WTZNzKCvU2GeM1ZWRyF7EvhMHhr7magiT";

  auto account_info = keyring_service_->GetZCashAccountInfo(account_id_1);

  auto get_tx_type = [&](mojom::ZCashTokenType from_token,
                         const std::string& addr) {
    base::test::TestFuture<mojom::ZCashTxType, mojom::ZCashAddressError> future;
    zcash_wallet_service_->GetTransactionType(account_id_1.Clone(), from_token,
                                              addr, future.GetCallback());
    return future.Take();
  };

  // Ironwood feature ON, shielded ON:
  {
    base::test::ScopedFeatureList feature_list;
    feature_list.InitAndEnableFeatureWithParameters(
        features::kBraveWalletZCashFeature,
        {{"zcash_shielded_transactions_enabled", "true"},
         {"zcash_ironwood_enabled", "true"}});

    // t → orchard addr → kTransparentToIronwood
    auto [t, e] = get_tx_type(mojom::ZCashTokenType::kTransparent,
                              kOrchardUnifiedAddress);
    EXPECT_EQ(t, mojom::ZCashTxType::kTransparentToIronwood);
    EXPECT_EQ(e, mojom::ZCashAddressError::kNoError);

    // orchard → orchard addr → kOrchardToIronwood
    std::tie(t, e) =
        get_tx_type(mojom::ZCashTokenType::kOrchard, kOrchardUnifiedAddress);
    EXPECT_EQ(t, mojom::ZCashTxType::kOrchardToIronwood);
    EXPECT_EQ(e, mojom::ZCashAddressError::kNoError);

    // ironwood → orchard addr → kIronwoodToIronwood
    std::tie(t, e) =
        get_tx_type(mojom::ZCashTokenType::kIronwood, kOrchardUnifiedAddress);
    EXPECT_EQ(t, mojom::ZCashTxType::kIronwoodToIronwood);
    EXPECT_EQ(e, mojom::ZCashAddressError::kNoError);

    // t → orchard_internal → kShieldingIronwood
    std::tie(t, e) =
        get_tx_type(mojom::ZCashTokenType::kTransparent,
                    account_info->orchard_internal_address.value());
    EXPECT_EQ(t, mojom::ZCashTxType::kShieldingIronwood);
    EXPECT_EQ(e, mojom::ZCashAddressError::kNoError);

    // orchard → transparent → kOrchardToTransparent
    std::tie(t, e) =
        get_tx_type(mojom::ZCashTokenType::kOrchard, kTransparentAddress);
    EXPECT_EQ(t, mojom::ZCashTxType::kOrchardToTransparent);
    EXPECT_EQ(e, mojom::ZCashAddressError::kNoError);

    // ironwood → transparent → kIronwoodToTransparent
    std::tie(t, e) =
        get_tx_type(mojom::ZCashTokenType::kIronwood, kTransparentAddress);
    EXPECT_EQ(t, mojom::ZCashTxType::kIronwoodToTransparent);
    EXPECT_EQ(e, mojom::ZCashAddressError::kNoError);

    // t → transparent → kTransparentToTransparent
    std::tie(t, e) =
        get_tx_type(mojom::ZCashTokenType::kTransparent, kTransparentAddress);
    EXPECT_EQ(t, mojom::ZCashTxType::kTransparentToTransparent);
    EXPECT_EQ(e, mojom::ZCashAddressError::kNoError);
  }

  // Ironwood feature OFF, shielded ON:
  {
    base::test::ScopedFeatureList feature_list;
    feature_list.InitAndEnableFeatureWithParameters(
        features::kBraveWalletZCashFeature,
        {{"zcash_shielded_transactions_enabled", "true"},
         {"zcash_ironwood_enabled", "false"}});

    auto expect_unknown = [&](mojom::ZCashTokenType from_token,
                              const std::string& address,
                              mojom::ZCashAddressError expected_error) {
      const auto [tx_type, error] = get_tx_type(from_token, address);
      EXPECT_EQ(tx_type, mojom::ZCashTxType::kUnknown);
      EXPECT_EQ(error, expected_error);
    };

    // All Ironwood-related transaction types are unavailable.
    expect_unknown(mojom::ZCashTokenType::kTransparent, kOrchardUnifiedAddress,
                   mojom::ZCashAddressError::kInvalidRecipientType);
    expect_unknown(mojom::ZCashTokenType::kOrchard, kOrchardUnifiedAddress,
                   mojom::ZCashAddressError::kInvalidRecipientType);
    expect_unknown(mojom::ZCashTokenType::kIronwood, kOrchardUnifiedAddress,
                   mojom::ZCashAddressError::kInvalidSenderType);
    expect_unknown(mojom::ZCashTokenType::kTransparent,
                   account_info->orchard_internal_address.value(),
                   mojom::ZCashAddressError::kInvalidRecipientType);
    expect_unknown(mojom::ZCashTokenType::kIronwood, kTransparentAddress,
                   mojom::ZCashAddressError::kInvalidSenderType);
    expect_unknown(
        mojom::ZCashTokenType::kIronwood,
        account_info->next_transparent_receive_address->address_string,
        mojom::ZCashAddressError::kInvalidSenderType);

    // t → transparent → kTransparentToTransparent
    const auto [t, e] =
        get_tx_type(mojom::ZCashTokenType::kTransparent, kTransparentAddress);
    EXPECT_EQ(t, mojom::ZCashTxType::kTransparentToTransparent);
    EXPECT_EQ(e, mojom::ZCashAddressError::kNoError);
  }

  // Shielded transactions disabled:
  {
    base::test::ScopedFeatureList feature_list;
    feature_list.InitAndEnableFeatureWithParameters(
        features::kBraveWalletZCashFeature,
        {{"zcash_shielded_transactions_enabled", "false"},
         {"zcash_ironwood_enabled", "true"}});

    // orchard sender → kUnknown (shielded disabled)
    auto [t, e] =
        get_tx_type(mojom::ZCashTokenType::kOrchard, kOrchardUnifiedAddress);
    EXPECT_EQ(t, mojom::ZCashTxType::kUnknown);
    EXPECT_EQ(e, mojom::ZCashAddressError::kInvalidSenderType);

    // t → transparent → kTransparentToTransparent
    std::tie(t, e) =
        get_tx_type(mojom::ZCashTokenType::kTransparent, kTransparentAddress);
    EXPECT_EQ(t, mojom::ZCashTxType::kTransparentToTransparent);
    EXPECT_EQ(e, mojom::ZCashAddressError::kNoError);
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
      .WillByDefault(  //
          [&](const std::string& chain_id, zcash::mojom::BlockIDPtr block_id,
              ZCashRpc::GetTreeStateCallback callback) {
            EXPECT_EQ(block_id->height, 100000u - kChainReorgBlockDelta);
            auto tree_state = zcash::mojom::TreeState::New(
                "main" /* network */,
                100000u - kChainReorgBlockDelta /* height */,
                "hexhexhex2" /* hash */, 123 /* time */, "" /* sapling tree */,
                "" /* orchard tree */, "" /* ironwood tree */);
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
        .WillOnce(  //
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

TEST_F(ZCashWalletServiceUnitTest, ResetSyncStateWithAccountBirthday) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndEnableFeatureWithParameters(
      features::kBraveWalletZCashFeature,
      {{"zcash_shielded_transactions_enabled", "true"}});

  auto account_id_1 = account_id();
  keyring_service()->SetZCashAccountBirthday(
      account_id_1.Clone(),
      mojom::ZCashAccountShieldBirthday::New(100u, "old_hash"));
  base::test::TestFuture<
      base::expected<OrchardStorage::Result, OrchardStorage::Error>>
      register_account_future;
  zcash_wallet_service_->sync_state()
      .AsyncCall(&OrchardSyncState::RegisterAccount)
      .WithArgs(account_id_1.Clone(), 100u)
      .Then(register_account_future.GetCallback());
  auto register_account_result = register_account_future.Take();
  ASSERT_TRUE(register_account_result.has_value());
  EXPECT_EQ(OrchardStorage::Result::kSuccess, register_account_result.value());
  // Prevent auto-sync manager startup in OnGetTreeStateForAccountBirthday.
  keyring_service()->Lock();

  EXPECT_CALL(zcash_rpc(), GetTreeState(_, _, _))
      .WillOnce(  //
          [&](const std::string& chain_id, zcash::mojom::BlockIDPtr block_id,
              ZCashRpc::GetTreeStateCallback callback) {
            EXPECT_EQ(chain_id, mojom::kZCashMainnet);
            EXPECT_EQ(block_id->height, 100000u - kChainReorgBlockDelta);
            auto tree_state = zcash::mojom::TreeState::New(
                "main" /* network */,
                100000u - kChainReorgBlockDelta /* height */,
                "new_hash" /* hash */, 123 /* time */, "" /* sapling tree */,
                "" /* orchard tree */, "" /* ironwood tree */);
            std::move(callback).Run(std::move(tree_state));
          });

  base::test::TestFuture<const std::optional<std::string>&> reset_sync_future;
  zcash_wallet_service_->ResetSyncState(account_id_1.Clone(), 100000u,
                                        reset_sync_future.GetCallback());
  EXPECT_EQ(std::nullopt, reset_sync_future.Take());

  base::test::TestFuture<base::expected<
      std::optional<OrchardStorage::AccountMeta>, OrchardStorage::Error>>
      account_meta_future;
  zcash_wallet_service_->sync_state()
      .AsyncCall(&OrchardSyncState::GetAccountMeta)
      .WithArgs(account_id_1.Clone())
      .Then(account_meta_future.GetCallback());
  auto account_meta = account_meta_future.Take();
  ASSERT_TRUE(account_meta.has_value());
  ASSERT_TRUE(account_meta.value());
  EXPECT_EQ(100000u - kChainReorgBlockDelta,
            account_meta.value()->account_birthday);

  base::test::TestFuture<bool> unlock_future;
  keyring_service()->Unlock(kTestWalletPassword, unlock_future.GetCallback());
  ASSERT_TRUE(unlock_future.Get());
  base::ScopedClosureRunner lock_on_exit(
      base::BindLambdaForTesting([&]() { keyring_service()->Lock(); }));
  auto account_info = keyring_service()->GetZCashAccountInfo(account_id_1);
  ASSERT_TRUE(account_info);
  EXPECT_EQ(mojom::ZCashAccountShieldBirthday::New(
                100000u - kChainReorgBlockDelta, "new_hash"),
            account_info->account_shield_birthday);
}

// Disabled on android due timeout failures
#if !BUILDFLAG(IS_ANDROID)

TEST_F(ZCashWalletServiceUnitTest, ShieldFunds_FailsOnNetworkError) {
  // Creating authorized orchard bundle may take a time
  base::test::ScopedRunLoopTimeout specific_timeout(FROM_HERE,
                                                    base::Minutes(1));
  base::test::ScopedFeatureList feature_list;
  feature_list.InitWithFeaturesAndParameters(
      {{features::kBraveWalletZCashFeature,
        {{"zcash_shielded_transactions_enabled", "true"}}},
#if BUILDFLAG(IS_IOS)
       {features::kBraveWalletWebUIFeature, {}}
#endif
      },
      {}  // disabled features
  );

  keyring_service()->Reset();
  keyring_service()->RestoreWallet(kGateJuniorMnemonic, kTestWalletPassword,
                                   false, base::DoNothing());
  OrchardBundleManager::OverrideRandomSeedForTesting(70972);
  auto account =
      GetAccountUtils().EnsureAccount(mojom::KeyringId::kZCashMainnet, 0);
  auto account_id = account->account_id.Clone();
  keyring_service()->UpdateNextUnusedAddressForZCashAccount(account_id, 1, 0);
  ON_CALL(zcash_rpc(), GetUtxoList(_, _, _))
      .WillByDefault(  //
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

// Shield*Funds tests are disabled because they exercise unsupported
// transaction-creation paths, including Ironwood stubs and ShieldAllFunds.
// TBD update to support Ironwood.
#define MAYBE_ShieldAllFunds DISABLED_ShieldAllFunds
#define MAYBE_ShieldFunds DISABLED_ShieldFunds
#define MAYBE_SendShieldedFunds DISABLED_SendShieldedFunds
// UnshieldFunds (orchard->transparent) doesn't touch Ironwood, so it's
// re-enabled with a v6-format test vector.
#if (BUILDFLAG(IS_WIN) && defined(ARCH_CPU_X86)) || BUILDFLAG(IS_IOS)
#define MAYBE_UnshieldFunds DISABLED_UnshieldFunds
#else
#define MAYBE_UnshieldFunds UnshieldFunds
#endif

// https://3xpl.com/zcash/transaction/821edadc1bc51e7dc7a57c01eb766292a88d3836ad7f83f4e3e505100cef2300
TEST_F(ZCashWalletServiceUnitTest, MAYBE_ShieldFunds) {
  // Creating authorized orchard bundle may take a time
  base::test::ScopedRunLoopTimeout specific_timeout(FROM_HERE,
                                                    base::Minutes(1));
  base::test::ScopedFeatureList feature_list;
  feature_list.InitWithFeaturesAndParameters(
      {{features::kBraveWalletZCashFeature,
        {{"zcash_shielded_transactions_enabled", "true"}}},
#if BUILDFLAG(IS_IOS)
       {features::kBraveWalletWebUIFeature, {}}
#endif
      },
      {}  // disabled features
  );

  keyring_service()->Reset();
  keyring_service()->RestoreWallet(kGateJuniorMnemonic, kTestWalletPassword,
                                   false, base::DoNothing());
  OrchardBundleManager::OverrideRandomSeedForTesting(993284);
  auto account =
      GetAccountUtils().EnsureAccount(mojom::KeyringId::kZCashMainnet, 0);
  auto account_id = account->account_id.Clone();
  keyring_service()->UpdateNextUnusedAddressForZCashAccount(account_id, 2, 2);

  // NU6.2 consensus branch id, active at the captured block height.
  ON_CALL(zcash_rpc(), GetLightdInfo(_, _))
      .WillByDefault([](const std::string& chain_id,
                        ZCashRpc::GetLightdInfoCallback callback) {
        std::move(callback).Run(zcash::mojom::LightdInfo::New("5437f330"));
      });

  ON_CALL(zcash_rpc(), IsKnownAddress(_, _, _, _, _))
      .WillByDefault([](const std::string& chain_id, const std::string& addr,
                        uint64_t block_start, uint64_t block_end,
                        ZCashRpc::IsKnownAddressCallback callback) {
        std::move(callback).Run(false);
      });

  ON_CALL(zcash_rpc(), GetLatestBlock(_, _))
      .WillByDefault(  //
          [&](const std::string& chain_id,
              ZCashRpc::GetLatestBlockCallback callback) {
            auto response = zcash::mojom::BlockID::New(
                3372956u,
                *PrefixedHexStringToBytes("0x1916aabd2578659f9bfb1c6ddd6dfd722e"
                                          "24ff8cab661911723dae0000000000"));
            std::move(callback).Run(std::move(response));
          });

  ON_CALL(zcash_rpc(), GetTreeState(_, _, _))
      .WillByDefault([&](const std::string& chain_id,
                         zcash::mojom::BlockIDPtr block_id,
                         ZCashRpc::GetTreeStateCallback callback) {
        auto tree_state = zcash::mojom::TreeState::New(
            "main" /* network */, 3372955 /* height */,
            "000000000093cc0d6befd348426b65998720605aa84f04670312b854508e5404"
            /* hash */,
            1781088485 /* time */,
            "01bd187803efee449ab7125c6ec27ba3369a6ffad3a3631cdd52104edacfad2e28"
            "001f0183bfa5cdbca07b5a2501700ed0d198b79a2ebb9fd5e761bed04f69c0237e"
            "f93801b0dfe9b9a33d724098563c4bd9fe6cbcc52e38c4f078937bf97ba5b9700e"
            "7b1c01a35d374167e6cd37f6050a00df74c72a6e3cba417e1d0bc6616c273e1f58"
            "dc6d000181fa5c90ba5d60a10d465d2a783683a57f5a2caa76070115348267df7d"
            "824f700161c6ad3ccb29b8d229fc9a2d09ef40be37f39070135a0483f59189a2fe"
            "fb786701d40b0f68d706717296d5c6b0b33f1a67a56eb24d2e7b245c60d832a3fa"
            "4b502800012f948132821e835359858f90bfb7b787b1e9339765e5fdb988e483f9"
            "2fc3672a0001e25e784a9d4deea96347b2723195d1111cd3530aa7705b69963cb9"
            "05e170c71100017fff53008e35fa8fa40542e41b3ae7040142750024e47b96b2be"
            "7db7355ea9440190e323f6e45d107d4c5cbe2c4cb4c18a466d71c43fb4b4613d36"
            "17ed45d8f456011d85b015e936981d3a869c16ed2e03aeeb2444206933acc3f7b3"
            "84aeb963256601929f0d44045c58428bd8daca3e2c9ca5a0361b5573a4158251db"
            "e9f63a84e94901931c48ea50688eae5e54c24d0df7a6b00ce498f92eb5e1cfd6a2"
            "d87d8ebd5364017d1ce2f0839bdbf1bad7ae37f845e7fe2116e0c1197536bfbad5"
            "49f3876c3c590000013e2598f743726006b8de42476ed56a55a75629a7b82e430c"
            "4e7c101a69e9b02a011619f99023a69bb647eab2d2aa1a73c3673c74bb033c3c49"
            "30eacda19e6fd93b0000000160272b134ca494b602137d89e528c751c06d3ef4a8"
            "7a45f33af343c15060cc1e0000000000" /* sapling tree */,
            "01484eab650e2b5058c184c67724723e376afb406fb06fb560fda37770661d913c"
            "0137ebd78a183a29265c420acb76daf6ef3816fd25a392626de45b8843be985b13"
            "1f014249e9ae085a07773111f253e1eacb1bd2af26a3ba9961126b832ec0de4890"
            "2c010886994961992b94da6b65b58a6a2b91b382cd4d440d24a8d065c3c92650bf"
            "3800000175ed3b75a222546fc87a29ae4c61cf6e23c2d02fa3180a9b404f1902a4"
            "fe591001038eb720114006be158a0dcd398b1e1e2dc2403054e054426c735e84ea"
            "5d270a018324187ce59ecddb29ae3bfee52634b9a8786a47d552b005c66ddc1a20"
            "0d422c00017c2fc71855d67cd7f787315d10666f63f6ed924507bb56474e1ade91"
            "53c19c0100000001bccfea84e6372bc58fee7024a266768c077cb432a7137bc71f"
            "8a6a21f13ded2601d16ce2138bce884c09007675df57a32eeec505e5ba468ce197"
            "69ddbe554bc41d0000000150fc4bd01275d506ffc3b8391dc5dc9cf837cacfb2a3"
            "412d7907cda594d8633b012829e8aacdf1501baaeb5cb6e189d4e7182228e3d4b9"
            "acf54713595241e97f21017c8ece2b2ab2355d809b58809b21c7a5e95cfc693cd6"
            "89387f7533ec8749261e01cc2dcaa338b312112db04b435a706d63244dd435238f"
            "0aa1e9e1598d35470810012dcc4273c8a0ed2337ecf7879380a07e7d427c7f9d82"
            "e538002bd1442978402c01daf63debf5b40df902dae98dadc029f281474d190cdd"
            "ecef1b10653248a234150001e2bca6a8d987d668defba89dc082196a922634ed88"
            "e065c669e526bb8815ee1b000000000000" /* orchard tree */,
            "" /* ironwood tree */);
        std::move(callback).Run(std::move(tree_state));
      });

  ON_CALL(mock_orchard_sync_state(), GetSpendableNotes(_, _, _))
      .WillByDefault([&](OrchardPool pool,
                         const mojom::AccountIdPtr& account_id,
                         const OrchardAddrRawPart& internal_addr) {
        OrchardSyncState::SpendableNotesBundle spendable_notes_bundle;
        return spendable_notes_bundle;
      });

  ON_CALL(zcash_rpc(), GetUtxoList(_, _, _))
      .WillByDefault(  //
          [&](const std::string& chain_id, const std::string& address,
              ZCashRpc::GetUtxoListCallback callback) {
            std::vector<zcash::mojom::ZCashUtxoPtr> utxos;
            if (address == "t1NB9RWCY4JCvY4rFGXyeAwk9pvXvjEC5Ge") {
              auto utxo = zcash::mojom::ZCashUtxo::New(
                  "t1NB9RWCY4JCvY4rFGXyeAwk9pvXvjEC5Ge" /* address */,
                  *PrefixedHexStringToBytes(
                      "0x0c495c101b8b535c3a5475cab5dae4e96ec8df70e672d9f664d1a7"
                      "b6b2e9a964") /* tx id */,
                  0u /* index */,
                  *PrefixedHexStringToBytes("0x76a9142f31ccc175b8c73923aa36da57"
                                            "d754cf3b8e1e0788ac") /*script*/,
                  270000u /* amount */, 3372940u /* block */);
              utxos.push_back(std::move(utxo));
            }
            auto response =
                zcash::mojom::GetAddressUtxosResponse::New(std::move(utxos));
            std::move(callback).Run(std::move(response));
          });

  std::optional<ZCashTransaction> created_transaction;
  base::MockCallback<ZCashWalletService::CreateTransactionCallback>
      create_transaction_callback;
  EXPECT_CALL(create_transaction_callback, Run(_))
      .WillOnce([&](base::expected<ZCashTransaction, std::string> tx) {
        ASSERT_TRUE(tx.has_value()) << tx.error();
        EXPECT_EQ(tx->memo(), std::nullopt);
        created_transaction = tx.value();
      });

  zcash_wallet_service_->CreateTransparentToIronwoodTransaction(
      account_id.Clone(),
      "u1dl9dtss80tsutx3xfje4vlndwhc2f2pernhhpxfsz9vw6nr0zzlkw9p2m22xjcn5588fp"
      "3tnta9uqhpk4nh06xumwvt8ff7w653g5pvk",
      100000, std::nullopt, create_transaction_callback.Get());
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
      "0x050000800a27a72630f337549c773300b0773300010c495c101b8b535c3a5475cab5da"
      "e4e96ec8df70e672d9f664d1a7b6b2e9a964000000006a473044022069051fac98deb4fb"
      "f9bc5661e355aaca3da57f8477db70fd1a884cd85b4d041d0220387915a4ac9f9ae7369e"
      "dd577edef7a6e476d6f86738fb0af473e3dddcf415f10121034ed29dfda30be25a3fa0fd"
      "48aecc3646b050d1d8f8c3ea5b8b0a96595b1bf409ffffffff01785d0200000000001976"
      "a914fd2c04606d4f7f1f95c6815f2d07a97807edae0488ac0000020ef0418e2250ca2b2e"
      "4bc24b42658632745b910918d9249a86854a4ff2d3b3a83fd01b2263392d79173cf9b0b8"
      "084505ce244c51b0a9106b9248664329f8fe2dd7bf932e1cd4e43e0517440acc2bce3318"
      "36245495da323bbf6aa12726ffbcb86cb032651ae726e373f0648d82768506f8ca9851bb"
      "bac68ed42983351553161273b6c9d06a17255c20478b5daeb108b136a46301a4b7fa39a7"
      "efa453c925018c01ced100ca63b7e4225729fc701a24137ab0d625e7fdc6569ea066c1a5"
      "d982fccbfe65ce11c650913c7b80a3677b5a14e06a904b4c4bb5473b67e3588c1d608e8d"
      "d0c981952219544f737fc0f197f02b15db7aaba906be628e2a85fd78f4a56e7db1f0311c"
      "c64e648984f416ff7bdc182d13b198863bcf2ac0a6d2e10e69b5c236ddccc0f3a44a3f5e"
      "2d9d9232267cf92f4940018db7cb614dc2f94ddadc6d80b1cc30e7369304fd3e44400db9"
      "ed601120e1f57d91f0d75cae70334c9fd87d625b04b976de704c1ecf3b681b6a677aef3c"
      "219c3be6f202db550e5a4378ada6056b7980424799612d2fd443918e9a868e37b7ca1399"
      "fea3632d825a9471ee801f663e72a91ac5154061d7743289ce1fd72646386e7eea9edb6a"
      "b090f4bd954a737b1072ed76b1e8874e4fa14fbdf6f70332c9aab57427ca68851235a02b"
      "8246d6690957cd98fbd820be1cb53e9a12d252e4cc54249242397bf4c88b2f75b9356037"
      "291d1943ef9c81fa54f7951f3f7f5d792b317d4b67ad90450a1d38ae1e0d157c17657334"
      "9d5c81c84ef093ca4e023c80157166de58b6279949c8c954219092ab30129a99ba654ba8"
      "f645e2de41720b77aa563a323c858ee5fdc96daf54b9eadb2ac9bfea2eec265338f29afc"
      "31ac8c21762c0c1197738887f387150ec0214386958a25eb18f3caf265b594c1717435df"
      "8ba3c30fedebd5dc167efc52be6c415dc3344682898bd0f1cdfbc41d412c6e0651860aeb"
      "1ee3752e495661550dc41ba3b497664201fda468eaf294b098987692843b81633775f852"
      "f84940ba1993481b46e2920382c07b2eeae766cf64e0f72300ba9b2485b194d773b4c20f"
      "d6298597205880edb71149c744e6859fe07db83fb23cd96e825c0212733bfb34fd33ecb6"
      "316657b21ca78b5799f40594c0bea02517c817803d6f13f5153b9d80ed38c5aa0574bfe7"
      "f439984ba557d2b3f877835d16edbab8573aeb58630d8f48daf62a2be4474fd1f0a04926"
      "f8ff9ae3a8af68fcf6bb39d2dcddb5a03587b46f99d1c669e35692192e110c5600d5ec9a"
      "89f1e061c81a0d6b709cd25dc6595b848ad9e2f48df4691d4a239e94717c8ce435d46ecb"
      "3cbc3ac909f3bf317289a03ff202e21743437add7e6d061df806c1b59d7551d8df423b91"
      "aa4e6742fe8c636d95a03be372fa19c42fc6583b4f256ee4de1eb6367474317cda1c3ac1"
      "04bdb15595d75f1486c220c08820d2caed7f5a0476ea65f30e3ac2afcac8f00a2701e462"
      "cfabfebbda0e8368bf558c06311b68cca2517eeb2bf5e22181be3b516df86f3bbc9a0188"
      "8ddd199d74a52b907cdcdd3c13b6c29d58b0537445c9f830dbfcdabdb9f4dc251d2269e4"
      "f7e9cfdafbc58c23ce79f4560c21a04dc70d748a9990bd3c1b7b87d2e3dea8a6ff9b026e"
      "134e631fbfc1e28f2f3cbe57c577261e7a004360a6d151375a055215d46ff53fa4b4051b"
      "a0b838a85726920766b385dff689c72fa5984effe88184b61c49fdef7e28babb6298e82a"
      "7fc208745541df6a4f1eb1a7c32b6f8f35457050e554540beb1bce94f2f3c8f561ef1bb7"
      "b2197f322f4d81cc617ad615fdcaefc05f46fc3330d210c02069e6cb21a3bda47ee627b6"
      "9bbc5d8d578f263a460442c528daf615035f07864c7ea4d9b23bb33118ea3bdb723339ea"
      "39c3774e9affc69654313bc240c1bf22a7da0663f4ca0c9445a8dd3011331ca67e829654"
      "45f5a74df7f51c60ff9f7a3e417de9a2fd0f2da0452592ea0d87cea30749c3420dd4772f"
      "f6884b38f5e6c51a3a75a963ad97fc235f225afabaf87059f3a92b5240932a0279203f63"
      "b8670e31260730b3ae19bdaea1201c15ac7cd775a26b3f128b5bdec2ce329c58ac63c9f3"
      "e5b833ef624d77f6d4487b5b1579c675d69d1b135e08a22dd36f87eeed6f554081edfa20"
      "1916e43b299bdc217f145294cc88e0865e5e38be6ab8088213c5c49cbe83c1c09841846d"
      "4761fe39bb2762d60f0b5c188b29010fbbe30225c74db73cebbef819f034f717838de15e"
      "29fe22fb516a0d3a3414337ac96c4aa166389aee86ce0a9c7089448bc31a7c6c41574e43"
      "195a64e0d19546b5e07616036079feffffffffff63139eadcb47f2dd637cd32fa3b6045a"
      "6698261e021ca0b9999792201212c713fd601cfdbff6318b4758bc3f01a7ccd1027af957"
      "c8471dcc915ac965428d58bb1e32256742794c43e5c4048c9d1d657f05e4ca467cf49a75"
      "bb0a6f9eeb5e2bda2c3f91c959c6bb8ae60139f7a9003002ed03d8cf3b460008fa427ffb"
      "d531308f92163a86fb65ba8c2c6677d526ba626ae5de97a6b1df1dc911474f95b31c5a23"
      "93a82cf0e32953767f1deb516571b1c2b15193c689bf3ddeb699a2775788ebfd957eac83"
      "39b7fe215691e883c12727b3a5580dd077474e5ae6f930d53f3d33b176e43cc6413728c5"
      "4bbf7ed21f2e9e56e37cbb24ef1c485604292f0532ed7bc5771e06d447f6dd25db01f1f7"
      "c501e84af098d9035643a0ddc994e8a23cc0e2c52df212079d81fc3f492ea0ef794d1295"
      "121df837e1f6f7c0611c1957285ba3dc47c71a39fc097fbf8c5867db5bf0691e72c1d6c0"
      "02fdcaae16ebe4e3929bd2956cd2b85ce12353f4c6cc94800e3cf81e42497044c6318489"
      "024307394b4e2e7a4e8226530c089d68be71ab1bf46b3fd4d935e3442e429ac06c981444"
      "dd7be5d92c773edfbf7549865753d99e43e575d6fa62626176bc5437af0fa148c551cdd8"
      "f15d2218036da2aa79ab106430a3924f3ef9048622f70a868a71bff621a51fd9167d2bc9"
      "2dedbfe99dff73fd7175835b4fa97e4a0ba57f56e3f95b5ee0dddbb7efc2000e4bb9fac2"
      "e4506eadb02a0721a85f0d22925ac4de0f92de52befa45f4b7f2bc779011535d0a3fff2c"
      "b4d86a411faecd3d560ad3cae6fc81a49fbfb68cb58b25f8d7e45cf2f68a47444ccf3295"
      "ed11c66afc660a213c1bf9902d57ab8b78531781170a05cbdbfa5d67abc10e201322bd72"
      "a0d74b9a7a3c6ce78e7b7a0bde0f3b856f859903d0c1bdbbb1c670de57e6ad3e5f66fbca"
      "1a7bca7a9f60108fb697bf929ef2c7495e487fb40642690024f07758e72ed641b1f12b84"
      "435ff49503b2a7d339876b1b5a447997d48b1b0208f1acf79dbe89bdabccbe7377ef6761"
      "f54327883dcec4a78c1809d2c9d8f583c76087881d5acf4a2bc236634d7126fa96218d15"
      "22677a5759ca22244e48121d0e9015e7b32a40bf667b082e8534093f8ce49041d1e09635"
      "723a2bda71dd077d31aeb21ea1f92ff793e7e44fa5284a5ca9cda9c445825f9669624477"
      "f0f28fbced2b7da5fb932b1e2ed6978637318d71a4aabeedc20d1f9e0c90495c3c12312d"
      "4a76173df0068c54419b6534df63771978d8bbdcc488c777fe7a8e8443f65b99ba3180ee"
      "d2085a91f8c12e4ee5937f5a4b8b937e58f29fecc3134b230a17100cfa2d4362cdaf6dcf"
      "e143167cf3357044f398870d9109eabccd6fb70ff82a57444d52d64ca308649b97355252"
      "2929c8c6dee387905dd2f0d9622f8e2e403a0101532b615ef60fecfae4dda83e82c87e8f"
      "51cfb373c234e62fd4fc43fa6f3e827337761cb563f75492a1169c77861e197a1ccb0378"
      "707083158b30040f3944a545258b242d4081ff1a5f37cef88440628f57151e1434202dd5"
      "cc54e16441cc896f5af6c5df105e83c53e3789dcde853a64e410000803219837e916f34b"
      "386bd097889c8416d749194a653f11e091c9044f8e893ea90b2635ee518f3fea417a80d6"
      "d077984c1ad53c549bfce3946bc820c463882b5b6df074ed9572f844d9533f370e9ca27d"
      "4f301ea7e3068cb3f8482caa12758717d055ea251cc5ce2070c544f77532d3477a280590"
      "742a502c9231055f05af86abdc51cd1b182d9ae72c2bf24e17c0d47992a8d5b99d605c2a"
      "719ea3e15f7e8d0f4fe4e6578f6f65b6c7103ebb6f220efa683e6fd8eb462c3d5d421394"
      "e59b315af45983112e16bb310044325ed80260365d085ce8a8aad8dc1d978ca0326e1f1d"
      "dc73947a8b9105cc4479ed3c93cac0f40db7a5289a59eb86de0c1cd7395dbae4d032c2c2"
      "a21e7ae5ff39bb9c7e1fbff9a4d2e297d4d7d012aa959f6020bf3e4e063ede98e6d0d84b"
      "1759a7f6626a61ef634abbe3de1ae0aea7ad94365ca603709f95df25a57b52a0e08b33c8"
      "5740d6d1cb86ef30792462a2cf503ef99937a0edcabd83998e206edd5d09dffa697b5108"
      "2b74f4079ff3321869898465950d21df836287ad58ce48e9bbcec17a210de6b51a89738a"
      "9fb0b5729205ae396c6f39d872d396d93877355816e69ac431da4e164ca3eb0cefab3c73"
      "3bd93c3de1711446cd5ae02545c98fac657b34d81071ef784fef209ff52f617b335dedc4"
      "042c9bde4d0dd42936e083624c4855a110395958a9dd2d782d57c87b9fe976cbb2eb3811"
      "39adf0f40dbb57cd5d7672c829862629c338eebe3f57f2390b066c1821ca8ad4b4b017fa"
      "e308e82319e7fbf7c69cb1f9d612da93e341eeaa7a688ec29b2a136b854325baccaeb57f"
      "591ac9dbb466e39873f0b460b2f6c93527d39c34eaf11f47f092a9790bcf4029501a9647"
      "bfcdfc9716eb650a7abd46ddf53548622db52e7f4875fad5a5bd2c4db2985bb7f532d8fc"
      "41ec93ef6ba607d5714ad647a38f255c79fca64a67f30535515bc11017f58492891cebd7"
      "8be557e6ea31c9679cfd3434bbbbcc57d4c57e50dc3f365e033aae9c7e219ce129ea5bab"
      "1db55ff9844839bbadcb158e60fa086ee51914fdff0bfb545fc7c72b8f09f4a1d8fe9f53"
      "cdc52179b4cf313a507aae09d30efcfcd6dd86d131913a78e9a418299c9ad89d95140b9b"
      "120607aabcdcc3989d2292c3005584af5604e3f8e36955f0a3ae6145513713d2301db2cc"
      "5444669d78540a7246d0b910d8092450668d573f68148d69de42034241aab082fd322be7"
      "c395a34e2e6d84692dc884f19ff08c4bae31644d3220066dcd0392dc48d4c7584db1c9de"
      "bed7ce1f7b98d2f28f1b9fa9bd1efba233783d91d009faedede4e3ad2e42ebaaa5597a99"
      "29983fecb48ffb6ba26badeab047103ae71df7e251cca3ba9438768aa63f7194825f5a91"
      "3a7ff35156908cc7e95a12e1f50e89d2e24d8146c17f0c0f75e1ef30930d001e7d35da7b"
      "2b4bcfb8460a34f2a5f7c5ec62f68e728e10625393b0051b9882aaefe9057d746386a34d"
      "63283ee0c708841e01d18f00a959dffeae372b52250f67ae4ac38cae20dd85f9f8b82888"
      "7d9570e95697e0cc99c78d9f339a36bed3644334e883c5226d14551efda01ee7bbd484c6"
      "bbe6d9caa29e43fdffd49b6ecb04e55c6bf9992eb38d8041b13510bac802b85e1ced3014"
      "219ad074a2447a4597d8ed0094fabcccbceb69876449050ac1900442b09e7d145404cf2a"
      "60540e136acae1138611467a47f1767d7bcf2b86d9ff2880d035db9c86924b21761eed3f"
      "8b01a28aea2365196e46e69f7870328233f944607e97b92131345dc8c72d789f918f755d"
      "48f1941d8a6edb75af8804a5487de2cd8d7408d115e70c7061b428193f7602668d8f3f7a"
      "63f5e6d3024607c26d2e78d1e0445fbf23f3a092058ce71468b2abe4903d3728600d1489"
      "35d833998ed8c5ecf92cbc77177850346cb5fdddd3d58a3f78f40104e6d5c86027bd3500"
      "e2e47d1199800bd826d207596f28da58c5e34d2f58adf84f4e562d7e208a0fd70cd45237"
      "a42047f944e42c250bb1b6c3b3d5243675c0a959b762f6dde9381a55c416a1b360ab9ac7"
      "7467080320ee6521ede038049f8641b39abf544c88a802f47d3283d97cd3348ee5c10a13"
      "698040675384ced9215f88ca6b205689cc283a6b4b32169bd7b03b316b6dcdd426bc3551"
      "dd7c2e2c153d80ac49516cf9041536d23a0b036a9bed6c76ac5f98c3767403be65ba5889"
      "19d545aa9e246a01a4060266dbff30d4417139608422aed670e1ac5e482c6082cc4a5bb5"
      "6500100e3585028171e480944ec1309a603f864775e068df863087b29fd554897d4a39da"
      "06921356af5b9f9f3dbcf991e8bdeebb9febec777ac1b4527e39891bd9e3ab14f1881ec2"
      "78eb5457afa82f8bc7606cde6fe0bfe040220b74b90db477c6055292519a294a6eb064bd"
      "b4f9abbfa2e0ea196d3ff71f4455e52ca76a90252481ec8b14290f36361b11466deccc20"
      "48037819f632b2a29a572e61a42fa26b084b828f5cbd26bceec21a68a411b122379deeda"
      "fffd25f2ede974d17f583c0270e76ce0611f38737e811f3e33b3fcb4aabef0eaf526d75c"
      "deda651d23d7dff318687d67b2bd2aa6d461c1604ade524281916b5e8f626bb068266eb3"
      "aa8790e098c05950e91c04af5c099e7b223147426bd0623ab4401eaa5f280ee2c0223459"
      "648e0665872b366202ee28a648457959931518372db7520768f5c6e6d0617fa9fc73e14e"
      "ed123e42568d8c57e39047a17fac9330fbf8b4c7edd9ed02ca103e918dd53a4b82ed1aa6"
      "997e32a5a3c8aaadaa232b973098e1467aa63043c59ebbd6664adb134cdd3b2cd50239b6"
      "2f4b0ac66f7bafd80529246e9cee35b5d54213454ec58866e2f42c78282a82979f4c062e"
      "ee09160e9b23e1b88d7670d2bf9b1582c66548e6cc9d3716805f75c9ef13726842bb3b96"
      "48ec502f2dd8893c6d91f78902d560b9f4d70afff8f9855322e06ed658d3e8aa6b7537ae"
      "afbfdb8d6435edce4227c3fa82ac2f41f1dd5d4ef28201e0acd4837f28eac5b816f8f719"
      "2a72108b9883d0f78ffb15aa7297a6e005bde5cf461e6ba4046735aeb88d77cbc8a7dd19"
      "7b7e9acb503e313975a371031541745dfcbc5e38c245a405272dee05f71cc549836e286c"
      "d2fc33107500b8ed31d3b6d19a80484efbee6d996e01827fe9a31dbe891ff02cecff0adf"
      "e8aa1ee3ba8a33f09caa40984c07c78fac2fe30f0a604f3ec4c69a28f35722c6d6ae7175"
      "9c28b58d0bd2afb0dd210d6165cf289434b3703eb0037f6283a9212d5daa329c36e8e0dd"
      "bd67db823fee87d07d1064ad93a00a57b8febd710674178096cc146bcec20bad170e09bb"
      "f7fc1cd4a69455413806abe3d28d47e976e52a73c3bff75eecefc35e7f01f3d523e71edb"
      "9821524211896bb928690110574d23a77800fe7a60ea66a900353cc9987bbc2423ee263d"
      "696456fbb3dd44c411193d9075f04309f5c0b0d156f46d01c775806d64a7bb87381b26a9"
      "b5292831d7b03e9622c7a121614b684d1237d8287e97fdc8a48595f215a906ad39d8288e"
      "55513d1e5865b52340f0fb3aec4a72f8ddb102e714a832228676c7bada324b2aa8580cc3"
      "770a96b459b2e681d6929247560bd20877cccda9aa69f6af9281c828891620bca1d6419e"
      "217701142ac04a39422582e88f7770605bed98714f7f18eec2312ac59c12d91f393c8908"
      "c6a2915b15e7c158216febcb7be1788ce002b5fc92a436f682afddbf0a90a00d77f4fc7e"
      "2074b575453d624e0f7990a08b5b736a4d6e21934451a0806d3014f179d9c68b4c60de0c"
      "c666f6acc889dc7980363d45a3620871d9cc7836c0b9db6d6cc56fec455fb6a398b92b28"
      "333e79a8480f7cea655f004fe8cf5f3f682bafba0b703542f9dc9923f0ab1f5884d82435"
      "a3840c1143df097be28d5e9d8795fe979244b609cf00dbc072eb0fc3a03f563be64921f6"
      "adc815cb841d37a7f8c9f66dfd1abb0eae8ee32d0e418bc4d3239452041244c965e32f96"
      "46f422fb00f8b2b71ef1b6b3dd8877b4b6a53da29266387c18772b8ba10008dc425e152a"
      "54c464d4b97643a2ccfd3566b6744b06b6238593013cd5676ab32fac958c6a4df4c7a48c"
      "c4c7f38292df44ef4e544042e709c0aa8ed54b72b9423af7fa4f987134970d3e8c008c98"
      "f95c3f522197fb453cdc3a9309d151681cf20c47c5118e64691135ea00e8e4745e76bfc2"
      "00adc442178283125320130701f91f81449322fc5f9f298a3805918cc60afafd071123ae"
      "9af118e5cc2ba2da1c290187db28ff7fd0202d22edba9cd900a8a72d347e39fcc37bd427"
      "550ecab6cb4d18ee6277ce952ea6d03c0afa6b550afdde55736460e48595819f7442218f"
      "39be363ecc8dac9e454104ca16865805d0c8277ff7c314bf7c10c0f839da27e6e7243f81"
      "02ff112217d9fffd062a36aca774a1f6286c3e6783c8c19dd0bf3501b2c929b9826cca0c"
      "17ea57ab3c1e7ecf9873e15a3266f5ef74ae5e9ae5a07cd212f93c97b8f3ba7cf183becf"
      "f281ab6ce94babc8bef4bdfeed6da5ce89d72f721d2302604b645439cb6bac0a9ac9c7f2"
      "02d7d84e16e40566069525e552a5c751f60f3f36c0882e9cd5130c327aaea50fac5c3ef4"
      "15baf972b700da2ad1a1919d08b509dc10134294da166513645c78729f59aa83fa8fff62"
      "c6d0536f91626e0b418b3cbba25256d7ee6a16ec2a5a6864926f4d829d9a089e899c6db9"
      "4b095d23362e07eaaf174eb27e50255ebfc3cc7d341f8202582b8fa2578b3157511b5b0a"
      "14143c5c09b3bd63006cc60c74d8897841ed4cfcebe33d2c816b9c2a6506dc9a6f1c2ad2"
      "a136b33ec4a4057fc399449b1c71df354983751e3f24ecbd0062bef176ac0e22597f4854"
      "69c8ca7e44b98ef9e4db5043da635f5dcad3ce84ef2cac43c3db2ebe9abbbdf041326348"
      "4f840062430e52deabcc079eb349eb635fb338485fa80cba662aa90209ee1b5ef93415f1"
      "8ca80a61d78c9d509b3629156066a6bf7de408f71d0cb7b2ddb4a54b5444925a5f2ecc00"
      "9286217c6cb35177db0803a4f825196f80d4e5feb534201b947e2e2ec22923b2ba0303bf"
      "df62f59dae574a90445527588e833c7612c58036cb7775892c830ea8e87f36a570bdcd62"
      "3bfd3aea2a452c0848b0cc47e1e497f5558d8089fc3f6ff6d334fc25935b073b225c453c"
      "e4d626fdf8216c47f913cff4c67949d0cb59be3d970016866c1ae43819b39552a47c2c11"
      "b688e5418a83c2e8b0dbdb1be0ada71e2a62e8402a1be76cf2d1199ba123262869c6fccb"
      "522b547b19851b79c1635596a8eb6a8b00ea6a9aefc3b7d8dd080e66d5163706a43a93ad"
      "054504c2f57deee11e1c3f17b945c5e90c1d9cc70ffd1dd42459b777723d8c2b6db6f62e"
      "60622eff31034d9c873af774101b90b416c232ebb7d2f518ff7eb62e047dd64a98c73de4"
      "ddfa60252d309d7ee3bf890bb1e3398b65b1dfe2336189d861a422baf13f06e09ef497ca"
      "05e6ab8825680b1573cc38cdbffe2579a14e6d883348eae5bda9eaf1e85d600f57519a8d"
      "f2b7bd4766df21ce360ab130de75247c66cb468aaa5fc9e18edb398a50968ee6626de81b"
      "b01c21cead42e5c18f5b7b1084cf77a9fe74cef8dd70b40c2d7ee2af232aa861595e3701"
      "2b8b1fc8f71292a984df25bd5e7b0e6ef537512804bb63519ab81978f42b1cb2817cb59b"
      "7645be53019690089c29a9633ac95d5f3d828554e60c5af25c8c31b155a7773405ee3fa4"
      "cbc3e6013a751881eee35b03eed54d92fa0252e267ee1c68e8af457a27f1bcb2f94c992d"
      "ec43e2540eea09099a5b4972693453825e2d25f876df9966f4654131cdc32b8005b20d8a"
      "374bbee3f142bb8d3c2dd3f9d7ae1bfac09ec875a1e4d2d195eaab5081afe3368617b2ac"
      "53dd7fb4bf990e74f46f0fd2ef3172fb736f835a105c2f61013cf516abb404258d989ae0"
      "d5a840d91d8d3728b0e1601a63d24669041ef3dc0662e4b776e657f784ef4f419603470c"
      "5af704de51a6ca8a55278d65f3d9da9a789aee466da089116fbf3306a7a177c5245c18ad"
      "2d562aa31f42e860c2f6931b429b916c3a60cde946f07d8bf17ea55dc08c18938d289b9a"
      "12bd869f68b5686fbd1e9e9a705b2d6a217ac8c05810036dbe882e21c9611d94f49c4a78"
      "17857ace3c037d70a5f760046bd3e588f24ca96d62a01c7f11e1ae75e2b9b7fa7713648d"
      "12268e041e615763aee1207ac5ae810651d51c2a80b78cd3e0b44c72b75a4812fac49966"
      "35d37f56fc45ce69c900da27e3b62fd311069df77cd1d4ad46c9bcd19adbf893023cddb5"
      "702a7611f266989294c613a3bd8d948495384827d1d091de12ff85f58db4be53c3e5b24a"
      "1dfd9ac9dfab152efcc610ce5e7f1919134ff31f570a15c341028bccd57dd470be994a5e"
      "0fe0270725ec0520e6df8deee6c6684b46a6af0c565afdbcd5d8c05f7607c455a45d2c60"
      "3e7b1976694e961070306845c3219cc4971bc063c340616eb491aff6331037c75a87c969"
      "1f88a2a37e5ebd8c9212464fbdfd9a068c53b896aa0657d75cf10d660c4c55574813c264"
      "39f98484f53134710517d6320260546b24c97d63fc6b355f5ec5e7ba4a76f43a23727664"
      "d5420e01931b4ed6db84f89fe5306983e4aa12899582bc985f2d1e3507bfc39f0f02ca64"
      "0cc258322fac0001068c7110aefc08c381c39c35cd9378f397a056ab67c14584979395c4"
      "a07113efe1830e5dd7c83e8fa19e0c488a92154670107b915f38ea8917cfa34c5d7016c0"
      "1fa2a8843b8b343ceff12c4a4518647da1f15537e43586053a83821487571c562d418651"
      "ed532da5f12eaa6cb853c65e14a6bbcd8f4eb61d28b46da81d86cc0204320a5e3c981fd5"
      "401af96531ba11cc1710f9962eee6c25ef144d02cf9c7002054f80b9209d08014f5eb3f9"
      "b413f2f0b0f017656798688fea51e87806da40491eae8988f2751f8820b43f716c51c7bb"
      "cbed24e26695c74b4747a2b7a8a153f4ffb36e57ce3b3ac1096ce11ef0c28883d4a47e4e"
      "e22821c35d7c2671150623878addca73a535179cc0c36deb636f49590bbf4c465b57beaf"
      "dc87aae1915cf62abf2ddbfde60f1db399097969bf26998791b205b7726cd37322ef6c6a"
      "0fbf375c19405761bced2af711320596a2f1988def9d41f578e0b8b9ea4277389490aabe"
      "2ff9986521b02c01158a960b46e72e8736be7bf22b1211e332a52f5aad94c08013b7bffc"
      "bd8e3156af8e3cde8c4bf2919b66509726ac25332fecb7f9196f20bd1cfd0d2a82fd1b0e"
      "b029270452b7c904ec489c7c4c8cb2ad54991d50cfe4a6e70bdcb852b35f21d0436b8eb6"
      "bde32b8b297a6e30c2214c33a54840fdbef990818f2060a0a17e3720e0de14110efe18b2"
      "2b8d2fce58b21e68c73091ff499b5e8c3c2d4e6928fcaf42089c59bb05424d66a8a3ef41"
      "c9486caa9df2f71797aac16811dd695d1e173907e876d8a8fccf590679779852ed9781b2"
      "1f9e359b1a3651da64ed8d08cfdc12121f73dfc3d575163ab0ac30820c2ca4676d63d4a8"
      "12be5cd6598fd6a6148f2955b31ee7f95bfe2705354e43b0aa212c68a5f9da67a3cba6ca"
      "f4710c2154452b00a85e777fdcea7c880556aaeef9c04d721138d8128e6349e9f71b9896"
      "ad0d36690d8e2534c520cfaabbb5afa9edbf05a298e18c6e082210c4b644a5c28fc03cdb"
      "c8aebc8e2c23605390cbd371c3e7a27782adec5556776a8741bb4d93f6278d85185647fa"
      "fce6105d142d761b683c9174a14e7daea5d2b9ee6502e7b8194fadf157ef5d03936bea7b"
      "2298dc58c811e6988306cf3a58b347e7f7a50d8673619f93de3f5bf8608231b6189e8f22"
      "4d35debca2266d9b4e79e1ff86b7c99a81a105a9d92639d161f52f847ab0782bbd65d494"
      "7481cbd682be5ca98d2b61a15254b6b4a986dec98c734248e2a72b003e5e9f021b4099fd"
      "da559fe9c59da54a5ebab733178e86be8ec98a739441127043a83634b8da6fd8f2c83354"
      "9a36f1cc38dc9eb5911b5365abebf992c46c1fa3096eb51f1c3e99a466c275809eb08cfb"
      "a6e6a35694c66e3ffcd2dc6b0eee2f2dd7f2f73a6b9d69b4e772221fd1c37a53f8ae9e46"
      "beda9a9c79d488c7a7535eb356140aa7a5c551139b8c4218ecedcad12b1699dd75f754f8"
      "d8322f3c5428f3a0825c0d2ae54f6ae6fa1c9f38ce29b4409b921bb989f9b16e03081d4d"
      "f118889457605be94984c1a7fa5a749a32bc0b165c2e2550490b50375369c4ac7fb77dc4"
      "1e8668eb7e8725d8c76e9957d13a949ab72aad448eb121583dccc2fd98751d3e7e877330"
      "5df7bc2bac78cd4561c0c9b642c4964c3c66582349f67e6b64c2634a364a587f735e4292"
      "5e38add40ce980eebd773417efc11f5c469e672e71182c1e356d3e43ee66e73722adba6a"
      "43f00ced3ce11f3366408b01943dde29448ff99185ddc5be860d8ff765faad7fea15a6a3"
      "b4e6b91982cb8256af8c7fdf187559167014a3a5139e23f33ec53c1e36861f8614e506f5"
      "746973f7629e8d5fe783dab6be776cb2b4e2b7c0ebfae0d101311d00e2cabf5982df34f3"
      "5a730a9c883206790bb5bf25bb2144bb4bebf7ddb8481822c0f83ad1cdf0c5f429b65c45"
      "c308c0ccd3dba7fab29311029688672545f71f1d88c111bddb4f9809abcb658c46498a9d"
      "f0a4bd70dfbc3334f275864b5029a080ca8a8a5d32f821cd3e383e6a910bbb41eea5e2bc"
      "c3c57833147214b9e70888faa7062a2652ec455d33291d2b9eb23a2c0d89a169eb41a83e"
      "dafe99f9cc998b6d7c7f1697451d2c93f2f27331a33e9ab830d08bfa41bdd7f1d2cd3075"
      "2192b343e36f9dd321c82254ed4385d35479981b86af8f762810c4697b3c3278e87c45b6"
      "948b00a16edf504ff0565e09629e353ea615ed476692c2cb0b96094dd5c10b9f5fbcaf0f"
      "40c2695b587a15af4069d93c0ab4466bc819ffa1575aa1acd74f50b1747c2ad55c3e7279"
      "50c20949737195d61347ab93502b5b76a8de18a52a234d52e1212c6205fc266ce47b7310"
      "b934fa6f0a10a1ae6d6f4d06979900c2065b3569b22a2a",
      ToHex(captured_data));
}

// https://3xpl.com/zcash/transaction/7aa77162f1a2ba09222136abcf6fcb11dbef9584dd62dca79803111eff285b96
TEST_F(ZCashWalletServiceUnitTest, MAYBE_ShieldAllFunds) {
  // Creating authorized orchard bundle may take a time
  base::test::ScopedRunLoopTimeout specific_timeout(FROM_HERE,
                                                    base::Minutes(1));
  base::test::ScopedFeatureList feature_list;
  feature_list.InitWithFeaturesAndParameters(
      {{features::kBraveWalletZCashFeature,
        {{"zcash_shielded_transactions_enabled", "true"}}},
#if BUILDFLAG(IS_IOS)
       {features::kBraveWalletWebUIFeature, {}}
#endif
      },
      {}  // disabled features
  );

  keyring_service()->Reset();
  keyring_service()->RestoreWallet(kGateJuniorMnemonic, kTestWalletPassword,
                                   false, base::DoNothing());
  OrchardBundleManager::OverrideRandomSeedForTesting(993287);
  auto account =
      GetAccountUtils().EnsureAccount(mojom::KeyringId::kZCashMainnet, 0);
  auto account_id = account->account_id.Clone();
  keyring_service()->UpdateNextUnusedAddressForZCashAccount(account_id, 3, 3);

  ON_CALL(zcash_rpc(), GetLightdInfo(_, _))
      .WillByDefault([](const std::string& chain_id,
                        ZCashRpc::GetLightdInfoCallback callback) {
        std::move(callback).Run(zcash::mojom::LightdInfo::New("5437f330"));
      });

  ON_CALL(zcash_rpc(), GetLatestBlock(_, _))
      .WillByDefault(  //
          [&](const std::string& chain_id,
              ZCashRpc::GetLatestBlockCallback callback) {
            auto response = zcash::mojom::BlockID::New(
                3373048u,
                *PrefixedHexStringToBytes("0x6064d8ecaaa146f7137ee760f8b4de32d8"
                                          "957783093c91017154360000000000"));
            std::move(callback).Run(std::move(response));
          });

  ON_CALL(zcash_rpc(), GetTreeState(_, _, _))
      .WillByDefault(  //
          [&](const std::string& chain_id, zcash::mojom::BlockIDPtr block_id,
              ZCashRpc::GetTreeStateCallback callback) {
            auto tree_state = zcash::mojom::TreeState::New(
                "main" /* network */, 3373047 /* height */,
                "000000000015fbb09b09ae28f6e54a2c3015a7e31154b3eb4f67e374878bab"
                "06" /* hash */,
                1781095163 /* time */,
                "019ac629bf6cdef1dd91aa5aedfd2425047f883b1056603f77fc73c11f1e27"
                "cd4c01e6a834f7b7f8625bda488f7bc9314ecdb53e97b60a93505142cde028"
                "5900b0091f01ed9badfc366c7b910962da19459b1c26ab5794453debea1707"
                "c444ed78eccd0d018ab6a667003d3b68ddfa4aa87e4788655ec035c147bf31"
                "6b26f5747e7211094f017f09f2a57c77e4b3e66406ddabee7a92ad7345831d"
                "1403d0f6b4f3ec6755e56700000000013738a72a6019f71a51eecddc13c3c6"
                "dee1aafee2f7820b7abe8a858d0f2b3a39012f948132821e835359858f90bf"
                "b7b787b1e9339765e5fdb988e483f92fc3672a0001e25e784a9d4deea96347"
                "b2723195d1111cd3530aa7705b69963cb905e170c71100017fff53008e35fa"
                "8fa40542e41b3ae7040142750024e47b96b2be7db7355ea9440190e323f6e4"
                "5d107d4c5cbe2c4cb4c18a466d71c43fb4b4613d3617ed45d8f456011d85b0"
                "15e936981d3a869c16ed2e03aeeb2444206933acc3f7b384aeb96325660192"
                "9f0d44045c58428bd8daca3e2c9ca5a0361b5573a4158251dbe9f63a84e949"
                "01931c48ea50688eae5e54c24d0df7a6b00ce498f92eb5e1cfd6a2d87d8ebd"
                "5364017d1ce2f0839bdbf1bad7ae37f845e7fe2116e0c1197536bfbad549f3"
                "876c3c590000013e2598f743726006b8de42476ed56a55a75629a7b82e430c"
                "4e7c101a69e9b02a011619f99023a69bb647eab2d2aa1a73c3673c74bb033c"
                "3c4930eacda19e6fd93b0000000160272b134ca494b602137d89e528c751c0"
                "6d3ef4a87a45f33af343c15060cc1e0000000000" /* sapling tree */,
                "01037737bc2fc2853959b5207bb96a580f09c6a619b5497be6e496721b781e"
                "9c19001f01c43aa7e72ff00b04df2aec90108152eeeaf6b0b22826f4baab5e"
                "7789de078134000000000000000001aa112d9d3fd2d02ca29f82e87a5fe112"
                "e7e70e5d1cd14d1a2e5a89e3be9aca1b000001bccfea84e6372bc58fee7024"
                "a266768c077cb432a7137bc71f8a6a21f13ded2601d16ce2138bce884c0900"
                "7675df57a32eeec505e5ba468ce19769ddbe554bc41d0000000150fc4bd012"
                "75d506ffc3b8391dc5dc9cf837cacfb2a3412d7907cda594d8633b012829e8"
                "aacdf1501baaeb5cb6e189d4e7182228e3d4b9acf54713595241e97f21017c"
                "8ece2b2ab2355d809b58809b21c7a5e95cfc693cd689387f7533ec8749261e"
                "01cc2dcaa338b312112db04b435a706d63244dd435238f0aa1e9e1598d3547"
                "0810012dcc4273c8a0ed2337ecf7879380a07e7d427c7f9d82e538002bd144"
                "2978402c01daf63debf5b40df902dae98dadc029f281474d190cddecef1b10"
                "653248a234150001e2bca6a8d987d668defba89dc082196a922634ed88e065"
                "c669e526bb8815ee1b000000000000" /* orchard tree */,
                "" /* ironwood tree */);
            std::move(callback).Run(std::move(tree_state));
          });

  ON_CALL(zcash_rpc(), GetUtxoList(_, _, _))
      .WillByDefault(  //
          [&](const std::string& chain_id, const std::string& address,
              ZCashRpc::GetUtxoListCallback callback) {
            std::vector<zcash::mojom::ZCashUtxoPtr> utxos;
            if (address == "t1dQ2Zho7TkCLc51bmeCjFymRt6GRXqHCJD") {
              utxos.push_back(zcash::mojom::ZCashUtxo::New(
                  "t1dQ2Zho7TkCLc51bmeCjFymRt6GRXqHCJD",
                  *PrefixedHexStringToBytes("0x0390d680464760da73fdaaf0a07180e9"
                                            "bbd8c39f24ca36890f3b320b5012d088"),
                  0u,
                  *PrefixedHexStringToBytes(
                      "0x76a914d62b5e6986b08eeb0bade2b80ab2bfebd9776ef488ac"),
                  100000u, 3373035u));
            }
            if (address == "t1gxFhEJTuAr8YUwWCVyf84uXHB1kefV5EV") {
              utxos.push_back(zcash::mojom::ZCashUtxo::New(
                  "t1gxFhEJTuAr8YUwWCVyf84uXHB1kefV5EV",
                  *PrefixedHexStringToBytes("0x0023ef0c1005e5e3f4837fad36388da8"
                                            "926276eb017ca5c77d1ec51bdcda1e82"),
                  0u,
                  *PrefixedHexStringToBytes(
                      "0x76a914fd2c04606d4f7f1f95c6815f2d07a97807edae0488ac"),
                  155000u, 3372957u));
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
      "0x050000800a27a72630f33754f87733000c783300020390d680464760da73fdaaf0a071"
      "80e9bbd8c39f24ca36890f3b320b5012d088000000006a47304402205b77d80f83c76a81"
      "e122fa728cdaf8de78eaa819ff533b580a82d116e5ba210502200dd958a9dfbef72728e8"
      "65a14df6c879614188ad39a74933870c0a5ecd6d101f012103000f086f622ddffed99158"
      "8ccb24c1d15c390284950ef4935ee37a12d1f6748cffffffff0023ef0c1005e5e3f4837f"
      "ad36388da8926276eb017ca5c77d1ec51bdcda1e82000000006a473044022011fe60ad70"
      "92d532b486daa85c56815841890067b423270f531389b19f9a68530220542da0bbfe3582"
      "a659dc93e0b6dc90b4d29cdf2ef5037aa34f15691e45633ad6012102951dce68f0a8ea4f"
      "49369151e856d9a174d5ad06510a2b780a2c5080d8f4782effffffff0000000219b9b98d"
      "2428f7b1426ed3a9ef64f260a0e066fa658d70e4063ab70daa03771e1d516eba8caee84b"
      "d92b00c9f926eddae80c37f40a7e8a86f8e83584a50bb50e3a0373f0298b4857c21c62b4"
      "b11c454c75b957320b3568582cb84db704940e05a381e591c4b0e844ee701ec3e9606c7b"
      "dd0bd520b9eadd4a6e06ddb8d9b1e60ffcf70077b311550d716794b41449e5a36c06dcc9"
      "5b9da43e8a53769ff6713b9e9b401dea5fabfeff035675967b852b1d7d8d01be7ec7c1ec"
      "05b6336da7a63e7d2627f954455d2c7572865311ea203558b6a0123851c46c62b481c8c7"
      "4495b33ca42a4c3de6d257724d8d943fff41b3874302e76d83888e8233b12ba6aecbf61e"
      "ba6d8be4d5419e53a2caa1db4cb126f51ecbcfb9e97802c59b4508f6dd7586448914c78e"
      "8294a69aca100a8f2d5827c1edc5226913e04f164be185188c511bfe2e904e5573de1f9e"
      "a71ad2ea8cf88ebed5683b95fde541afdf16d1543df83425d5f4de9b8b95b9cf8ce32db1"
      "78d466301261882f294d585e378b8e8964f7bc1a22932b1d8129ed970cded9fd8ccd8b92"
      "5366e62d4826ebd8ae307a2212f12af3957f4c947953b188878d0e99d0358b8d41721a77"
      "245c186e6d95e3f8422a749d2756b6d505e6449c2548c31bb52e0046ad0eed9b562447f7"
      "9b1d690d97705545ffd6d14b5c125bb0bed2634ff26d39a82da72b326b54971b16a32b71"
      "0f9efb6dbdb5bc0cf8ba9676e2e8be55cbd997e0355573c2e2731b5e1a01b7bc7cf36b31"
      "4a54f3ee42070bd49be234b0a7c283589730d1bad8c840e2d0ca7b24b339a732c7370a57"
      "406c80e2925246ba3565bf0b768cac41631eff7a1025c938e69589afd46c5c74de17d135"
      "56ecb9a084633739bfdda241c9105a0981056810cddcdd44c5823bcc2aac65c2b14771c6"
      "42b4bdeb34d07582439127330646b60b056404e629bf2ab7f78cd5449b5b6ffcb9be7a5e"
      "ab2b504a49bb56af296917f7a9b42f98cceba9045a8db350b03a9e38e1392333470edf14"
      "2243dedee552eaece6974d414e05b1e2ab43fcced94a573357f1fbf752b5dc50e757d9f1"
      "4666bb9573a76a8687a33dd3a7ccaa32e11fdcf35284fa9b0675887d35607822fe7c50ca"
      "d9891181b0a80e5fb472bf3302901f7dc66cf284de8ef6311f5d7aeece8275bd3b02d77f"
      "89e6420d36f69ea279a31bd35f7681f5845c5ea3b26b49adaaa07da63f1c666efb625519"
      "14809157b8f2e5ac4f5f83b9a8c36d06ad6b6168ccc1b375d089d423b652f3484909a2ee"
      "577d74cdc5ce8fdf3c7ed836cd9ce19dc7e35589d81cf034700cfbe8b09509d169ce995a"
      "71b8c6ec365d87095afb068f59c7634d63419327c00ae8f85f2a87c67045bd460ff237d0"
      "0f9f88b1c7374ea3639f6a442a415701cd7e7c30d52067527274a0a42d11d45c49effca8"
      "5010327c59a20055cb353dfd53ab5a71707fb063eace1f5f71a7bad2b55d2d63b5ce6d2a"
      "007dae3d8d3a24527c44dd768078c65b24b43a8f41367ea4afd83153fae98a8fc66383b0"
      "546c8f5298b931c9122fb90ebbdb6304b76b236bb9f46a9d249601fa8ea4822db8f923be"
      "fd6c9bedf86b62945ebbb09ec2086c6678f5e60c5b6564e19a555955bd00c53b9537d4be"
      "09128c9cef0a4e9c80337883b59885cddbbbca4f2adc4c095484642b824ae5fc47c5beee"
      "be4dc119f52c79eccbd4a932ea8697a20125915e9d856a9a88ac30e0defb04aa64c69450"
      "968f3ad8e05755f6104e0404d7cd7be2143675cb81572e0f3270c1c82f278783444c1812"
      "1adb56762afdde57f65505f9a0389d9ae7e6abc6d76a41e1afb748c15c0d85de38c9c94e"
      "a57ad7af95d73971629562eb1049e03f91974fa91151470aab09ee5fdafa32415fb9e1f7"
      "81dfd64b13fdc02b059a6f39faa9465d99bde17cc23ddb4f0bfa2e929309b6a043cdabd0"
      "1645f6a6824bf4fe2e6ddce3f90e776076de7068c62e7bcf66b4a1d7b57ae3ef415283e3"
      "34b90e431bb7358e6b0d2041b3e903bfebbf15a04ed9897a47ded823a9252005e546cdd2"
      "d931c33bd00559b262e639439dfbdde8633ccca471bdb22bba22cad4609730d5c71a38da"
      "5d025b19e3c37dbf493ed87fa1c1d5ee7be0fcff78fdb773d569e089721dde7483051c46"
      "787ec18f8668893d4d5d49f558cca28dc9662bfa9b3e1a0966097287e8dcd10d119c6987"
      "b8512087805863d63103aa6fb33e606b2148f3cf9d582a04daf06781536a31b9794632d0"
      "6aecb46e94eb6d4b53db4a673430ba3f22dc6e402a2b6e0b8c023977825aeb15b907ddd6"
      "a30a98b8c3a2029e6761e8300a6ff56e03086afcffffffffff1a92e2c8bf2ca817e9f7fb"
      "7e8f098be20dce176629962f360b335e6fb18ebd34fd601c654561d1072feb2f61920ab8"
      "6e249f04f0eeeda898de9e0c6eab6de3eddc00385dfb4b97e5eb05ccbd3806251c5c1130"
      "4410d94dec1c775fa4c3f5bbe2fdeb916fb59d55059f1f2dc53b24eb76a4a27e59a821e9"
      "f3c06f6ade93741182fdf9387fecb911890b5051c4cf00473590a6057ed013c4412058bf"
      "7af1738fb381f5911a3c6ac8aee6edd1dfc11e8cceaa8b2f5ca22668ecfb846ee3594c94"
      "6b433302ec2184ba4ad66f83bc3b139e61e7753a64bf923e22845f434944b9fa2a5bd783"
      "81651f0fbfd4cf9c11109b7cfbd5ad07cd46c5368351ea2863617298d59964a98919ab42"
      "566302f880e18468125c800d33072764f383a886ff507c00172bf623673fa522e4e7f0f3"
      "9e6d799931e969a6a65046dff92822bfa3708343ee714a852eaaf02319a16e426f295ab7"
      "0d7ea527e086c9592c6f2d3a5f9c5c526e9e8232965d974b86d834a5573f1cca227663d0"
      "791e2af5eada4d5e97580711f404d30fbef163a3fb5a4b4f61278a51f265cca584a9e116"
      "7146655cc3bc8c8f1ce0f60f16321e735b5b8b1e49210efddcfde963ee2079588a9d4834"
      "8c84b8b74c949f0be4d68e0d19f0d2df75fb798270276393a591e25758cbec457c25f73a"
      "ba5c4230cc63e9a7c4e2e9b0ba01e7b85ae5e12a814448e44162add74356041531e08785"
      "eee0bc3604908f4d3a11767e02dc57c4bec1e3f32e1e50cbcb699f7417ab6334c95a4d8b"
      "356fc93ec9bf391e1912dc120f948f4411d9bfd24474bd8ec359ffb7c3cfd1363512fc3c"
      "519d3ac29e216d2b1cdb37bb27f87039bf9d0ffec49f449bf1f7bf86952f3ed4635eb205"
      "fbc375e918aecbd097ea28710bea553de8d109a1081ed961a8c47d3e9516ae2936f3613e"
      "18477041de5d886d3015ed1ecc6f52827eb3512218e1acefe1b2e1403f5d8b246933e15f"
      "126173d031efa18acb2c5c27156792b67bdc0fb77bbc8b59974312dbf3f5f287d5f98aa1"
      "e3430ada9b43313fbc7c5673e3e53d2eddf430ec54f0fe1088597474b18b9ea26e82bc67"
      "d9a4aaa9ece5ad43226541e8b20f6f070400ecc0669c43c97508c7f0d23eadae084ae981"
      "fbddbf13b3c825b5da4747a09e312b0c3567c2fde54aa7132a52d57e705a4a8305527298"
      "edfba32745b7539a277e4d17680fd44a4a328c4c6a8d1fb4f983352c12bfcb89ca0e9647"
      "7776728bae6d0335ef42f6f8eb47e5a3b270b56bc50b698dd0bf25aab0fb665522036153"
      "a3cdba9d67fad9260c5b80855497ef33424bac9359461daa3249fc8ac139cefa7df3f75c"
      "8e415ab058e0885a0b1919dde88d313a3f21efe14127be745f951f3476d10e301d685152"
      "a022ce1fe937dcb16e5f6b82a75a93041b821cb168170c7a778a485bd550289ea88f7336"
      "31b6f14f6523b692e6abbf097378ca63efa544b21f5527be4c9e14b7360ca9ad2c0da35b"
      "481d3200253827874b9dc574b7e7b0660b5921595e245ee316f690e43d7f977d2e3c170d"
      "611af4e124dec2e7f56514786709a0415d9fef9d161a5b94ec1b19394a9d77b1cb5edd2d"
      "c4d3bd09e7dadf0dfb3a8504cf2000b0793a4b566da2146032ace919549d7a912318ae16"
      "f0133fb69e475be8cd28d6b36f03da5243e1b82e3e3244ada9f5f56cdbfbee9a09ea03b6"
      "1e2a7e51f9ada9ec64760f7dd5c5ee4fa3647222a46449eaa3fa4bff1bc40e2da1352dc8"
      "45523aec24f6987265ccc86a8acde5b13979d38771560eb81b3310ad4dad55b4d3371748"
      "d79d2561fb275812011f2703cdc23ce8c18d3db0195456f7224418c080f14d240b9c04d4"
      "565304ea2a50aa87be24de18e52dfccefe783c981191e5a16fd1f4c6751b67087f0a9289"
      "dfd5373273f8f3356982a83b1ac10faf70721e014cb5d22adc131a589341d3f91788868d"
      "1dcfa09a838c5af0ec45d427a2f209d96d523caed76decdc4df7f7ece874ec06c0f5ff51"
      "7a7e168552312dcb5ffa9168970c77d4fe93c80bb57249da6be84125444f4915c723bf7b"
      "6108582ad726dbbe57f6f9ab4a54dfad3c78ede6a4d7999077d21a0f47d21e11022565a0"
      "ee4cea1a589afe1747eee2ce148edd859eea4c341c10ddc79e457b425829dec1c5a99a24"
      "8188b78078cddc0ba7e36d11d89e0e920940006fe76136cafbfcf950f7faa017af279703"
      "71cec59842cbc1db00f5fb1afe812270d50f7590eb472cf1f94af28687683d683baafb53"
      "cc5b384428b61b222ef5601c15a91546c1a3ac318a10f65b3fa65d15b4593b6abc4839d0"
      "22f7b78e5c82756346895e8c5153b6ac08a46d9f45edf108ae260bffa89d125415e1c78e"
      "eb0fc58c0c5bb2e2d697cff52ab3601ed88321bb11abd0a15ad7bed04828bd0de4c46a69"
      "e5190587fc2acfb338cef56540f96ae1797ea966197f60f8b7fd989aab398cb48d17242b"
      "7e3d02e11e89284bc57aa07088964143581d006dd5c57337e565ca42059c7b1e99aa5a93"
      "49d0dd57d9b274f177aa72ec9f8e34f53cdb34369fcd39de994fb0341e025edc3ec531a3"
      "d55e1e444c7f34f035a6748085a101188b4a870956d502c996bfa94751afc4f0bb96a145"
      "417f48e5bf111cf701f566079d74614e7604b870608e95cd0e13f5ef804f57faa6339de4"
      "43667e8f4485b1021769ddcbc78ba0b290b64850737259e679d80f8b58a818f0c9fe7c91"
      "1a3b78165eb7fb573af8ca8a7c021208203b182ee5786c22498795ee6fb7f74adc94f814"
      "f5891eb48635a96302e74d05b1469e2f217231ea7b40c572938c04761060280c72ba3c85"
      "0dd213563c01a13dd4bfb1207fd114fa272737c9c5eebc5f9b084e373f5d0720c02b5d41"
      "db85e4d0c899a3ee9cacf2024fcba448ebe3b0ed62574d2d5e1967b0256095e02db42434"
      "140ff139f347fe0c4e9fa10f55e28eb049c0271ff36881490aa97ed10eb6955f8949b0d2"
      "3a6dfc675bd898fefd2ddc143fb8351f7bba718ae9ed3d11d1044284a54729557a547757"
      "c3419e989200679607e1e51e1d1e71abee683ac0c26a7b9a74de386ffe6bad72b7fdf3f7"
      "8f858d3f4b68731d8e14e0e4295a417960a89259285a396d8e079665c6374246630bbdae"
      "27224d1f2997b5b23e33250a765afe1e99912387eb52c089fcf0e9c67376b639c7533332"
      "f302703643c267f365fe92c07615554cd2cb2316e1779ec31e3b4d8c78949129ac4d3797"
      "2e6304c9763aabe862a77012b4ff0c397272a06d19287a94207a6f1ef40a8fd8377ec212"
      "2690b6926e554ac98db1824d00698dc14de237e89c1326136fb6bce17fc85ccdad0406e1"
      "7ad19b3ce64d29b6aa98a8cc18abdc73c3fb5f16c626eb8bd60faef8a21ec59ddf6121c4"
      "651edb09d9820c6d9706ac2cd1334f3e5b713d79434808350b4fb8372aa157dcd933c0cd"
      "ff87d933b3517054c8c1461b7c46eca3d26f13c92896af27dbe0abef5f7e7030f914b4be"
      "7dc4bed52c4a64364b6f09cac4148c7f7594d50005be14fc507b6343a291c92d11504f36"
      "dd214e39075dd926457e47d62bad375e5eda054cf6e27f84bc36f0ecb0f5d25178b7a33f"
      "b968a9a2a02d6b943dc433653fa33fdc517ae465dec647f635bbb040c598ee303b7e37e9"
      "c275a5db7062c0ac607bbad3ee9980be3fa0a77bb0ea864c354ad7252a09a9f2191f2c01"
      "8fe24c7b7d377ebd24613fa398e93c5885267a484f0b1a311259f53faa993520141a0e25"
      "de34bb31a31d87f76e16606776f2d812db2edd1bdcd7177e3ca42b1d254865a68ff4a419"
      "031e7ab631f7a85167e5e9fc93d8ed1ceb73c1298ebd3eb169fdf92a219df5aebcc4b02d"
      "ffe18eb9cbd863f95e0dc335fd7e450b733f16aa28283a68aaefe3bb6c80093c05c8e930"
      "66da5f09421a5c0a95f398fcc50eee91f037d1d997f276036e9e096d09c3317195fe5f52"
      "31060a2e13744a35a4c6fa947b0b2aea9205a1897e7175bd09b547621c9de5dfae1cc014"
      "ef19336ee2463c8aea2a5f9e689c0b07859a0207af65922019dc65a77b3bcf2f1042d266"
      "17c5346ded42bd9c35b006571b13005720268a9fbdb2f78c635cbd12f227e3b932fd7d1f"
      "ae871be3fcb39e21b6b3eb40991e63c61a87493f441c291a5039e74928ffdea6654b850b"
      "2b56cd08c672dd862a5f82d06913a576f5c90401f73fbeea42e690725d2a3beb405f43c0"
      "2c36fa077ed6abd92c40dbc23c96bb0bb79dcdfd3d95a41470bd06421d07a1537ce3b3c5"
      "a2e84430e78b90cf96d64f213be19fcfb04ea33c1218d81a4adcee4c0a078d4b3a9f589a"
      "870c29ad6fd84d29837235a32ad11c263b4d1a77b08cc5773be7cf1ea6fe10993965f366"
      "481b9e3ee92b96fbf3374b17f5eba81147c76b63e99d18d18d7de6ba2b443adbf740a70f"
      "d7045613a8dc31bd4a9cbe933c5719d4f3c40802dc54fb344dbf8986cc85223b0b64ec86"
      "b6522c00ffeca4cd843e039e89fd6f0fcfde9aa5046c8744b385211bd4931ea9b229fb71"
      "2041c70b48828eafe88e422ff9bbd072ebdcd8edcf41073808a5501762479dbccb10a54d"
      "dd7b78efe42e95329ea0130c6f1df112e263090bec07f09b72341654866ac3743c87bafa"
      "a65d08fde6be6c45fad04634b315312ae2a7405424f7f8501f3b36e67db9d3f7e9065652"
      "318e5c1865791b1a6532c7059c16a085d2a288066b80481d768be0a062c4565add44221c"
      "fd682163a7fdd001eb767242b552529e2347d296102153486f1e5c3eafafb37a161e0c16"
      "396308282e29230fe77e96ba14c59b2efcae0d683d81b6f62dbe796776f2e849e2a96a24"
      "c05b45b5c8aa020cba5d16f3a02f66e75d2a30ae7d80bc9d4c41f47bcfc17d0ce75d5ca4"
      "f93463f24f63959e3da76413a452140194995b3f60381a330991d60634da43e68f60b357"
      "7995f07fb3c62938b536fa4074e1e79a23c717f81596bc354ff46ee2f13e8a8b12b97f9e"
      "ec8598068d865924b3d49d3ac3f7f8bf0a9a4b011eadf411865e39852265021a504910d7"
      "f4bd7b29e63dd4ed1261897458b155149eaa746dea144af3a56e26086ae5f1ad895a3668"
      "622d67c3f804e442bfdd23112ee526c2bc86ae1ee84ee8a9db4c14ce79ba0743cac51406"
      "55c425f931101e2308f1573c9c54ca5964020d6289c913f1e3b601936065d64222336246"
      "2e567b2ee59062873ae3e76325cfcd606d9fc35931602dd738ff31741b187ebb81dfd40c"
      "023b364086890552b5470c080f371d35c28a9c51f3be2118c551ba1a1525490df80472c8"
      "0ee53fc4e347b8efdd61da6ddef48192ab77c925be3ecda2a2ff330d17fba0ee28f89019"
      "b0231489d210349c59c635e6f7796a4a4d09d59485d9a5267d0182a783892421f4eea1c6"
      "3d08c8b083a8fc46d90b59485c7a63894b55b71bd9f15ee73f6c02cd48a943caa8980ce9"
      "531cdc1412d2c0758d6721d9e1a74727d738d74084ef1ed2763f1e622ec48446a0ba027d"
      "586f552bff71bf3533eb9031a1af7a1793af9acff031420426c17cb27fa2457a06db9ade"
      "97bf65c54a8bb63dd64740624bfa03e111fa95b844087018f48bb923b90d4fb87b0e6c38"
      "e5936820a355130f94eff6c80d3b5925f31a047295bfb77c38f3252919197162a91cfb08"
      "2017c1c55ceb9e15e0ea72bf97aca44c8beee8dde268819c482473d3a9830a1f52eb7ee6"
      "342f71f4f9a5323a2fbc361034fc689f6176ff1776855a2d018b2638017d1a88bc0fa5e9"
      "af1850b2b17dc27b402a06df0d598ab45b1551622744e909f67def9f941a6319b40dbd33"
      "08c84a046338402216cc81087de33f4fec4ab4340f96d60047d205e761f43551c8db06d0"
      "5478717864aedaf589fde2a6e1e8d30b36631fd76a26837c1ed6dc0f55a97ffa5e03fa06"
      "75cab98c68ad35df6c59b42306dc6b387336a1186d4a02fce9174919696004f7d6219d92"
      "263d707fab85021bdfac2fdacfd17c82b936a6989c9680cf02ce92b5663f5c23a6d3ed88"
      "b8872b0bc4001a28473fd4e0690e1d5fc3c984fd291a36f16241de01a752866f105ddc2f"
      "35000dd0ce3750963815eb225f57c17bf7554bc3ef7006d7dc14109f0bae951aa9b9cca6"
      "3f3dc70491a75fc562c4f1adba63d26621860bbe6505d7cc32d65d1f095e3cc977b45cfe"
      "78e4e958533b6fa0eea105e46305eeecb3fa97351067d623b9c846416f5693b46886b1c1"
      "8967a3dd71b0032e06b8fe3d0bbe4addadb7883ddc8b93fd1e002a1f52a582b9cbcc6228"
      "2b949dbecec24559bdeaba231a6301149723357fcc0fbdb1b90ced1dc992486c5508b005"
      "df1fcbf845402575f9bd1d068ec5540f9ff0a8fa9522b271291468c5abbbd2fc525c766d"
      "e1320a3b995c5b3f1b9b2d3891d88aa445b711ec53161860b7f6bc086773e37b34cccba2"
      "85ed172a3fe651b8be173c410abb91d9504bca15fc80836c52e45458b4c588a7d91d5e1e"
      "0bd46e414c8ea9d4de147ce2005cda637d4f31bee7c13b1dc777e35eebe61d33557fbf8a"
      "2577bedc4fba464aeb714bb41b3a8bbee7c602e3f46131f82c6c7c1f39e7990eaf8800a3"
      "0ef2af2f8655b6b06715c7e335faeb81708aab6ccb40322c1ff9aba6e3cc38d591c53342"
      "edde11a8a4c746ba6498da0523aa7458752bdc0a1e07ca9a7bce3040d4208e4a10320c63"
      "b9634726b46b603bcd9fd0f666fca70c7df5d2725eba0f3c028d162b0975db8b5612174a"
      "0cc8c321b1b89ab603f3892a71a4ca127a91a0b783a97a623dc65ee1aaa0d9f9302d1625"
      "e75a2e45aaa35a0a69c5f3e215a4d0463b80469e70f444a47fb9070710b702237da0b3ea"
      "1e012c0489b47211d8be6102f066994a090b105a33d1df085d4bb788db2df85dd12d910e"
      "3c257c3ec40f514dd80ce1c66e31f5ad45ebae7855801efc2dea24bf05335228028bed52"
      "38047c342c7c664b7adef758b8fcb3effc6c29d95c7b0a0f0201ad251d60c68e4c82dab6"
      "8c77e1e8f6271e96a839d4df553dc7f44e24fdae3c2b1709bea744a6ca7ce5fb9fe12db4"
      "685a0eb77471fb2b1a2a92724a3401866a78d323ad95ce47335d1c1c9d57a58640498979"
      "fa521e0fc971edaeec265c5e407bde06e99dd4c287699cd253ebbab40e8d99324adc4668"
      "9baf832e5c02a61c135ca810f91ac82397f107bb3f5fe233254128ae58f19adc5fe0e3e1"
      "9af73ed2f7db712581f217203170649c14f8eef60f15a24fee767c18e9d4d0dcb9e53066"
      "b3ad3d24f625916de1a7dce1d02956d6dfc1c3a03024c747c1b89fcb985bdac31cfe570b"
      "5f8d460f312818cf9ec1e6c4695a6d61ea5aa1936c063bd3ffc96de14a0fa01396e38133"
      "3bef339c1cd583ba9d03b4592ace7fd00cd441a524600e8cc773d427d828adee2cb376f9"
      "045c0845018c9f41cf489c496cf4a3e1b93e029b1af1931e06eb867d83cae98e7e130112"
      "04a7dd0a90471fa378e8583e152ea9194aa5b42502c25368334ffda7b67aa9ef34678dba"
      "68d92859cb2e399c86a700e2a4c04c36dea53e77ae02ee494a826055abf11d4013cf7f57"
      "74f5319687aa22ce1d1c29303e658babc305579795e106a3867a61fd2b476bc3763814f8"
      "6f6dc251529a15382e736d81723761b7fbc80f7d2b6e4d3d1f77ed2daa22e02f80dc7eea"
      "6db869079326c2305e4b60ad0843f9fb7b39daa1d22409c90caf7f68cddf10acf9e0013d"
      "3d4343bb29cb2db5411ffd86143fab9032213ea2d6450c912bb7e600b818541c5ca2e665"
      "2ac9f4ee8ffbc272c8aa5756112ba369c483c94afde60ae193b06c3c3d3d8bea522ab0bb"
      "50dbcc4493dc15e5ee92a439b057a3eea59aab04a1c45502d8d9a2a957e305874173e2ed"
      "4388b68e5268564529e3e9cd60843cb28b40f618db617d3f2cef7821e1eca674a997545d"
      "437ccea0ad2e80094f83a0b8e1d7c30d2898e51032bfa642924e34ebba8b41330654a03e"
      "ae142279889f47c0fcceb60f40cb0a0c1c785efaafc5dbc2c03df3c159a3a493dcc14e34"
      "9d6d3f8f8ff35707c4bf2775ea03e6ddaa313afad79fdb45bb5a72b110522e945a7c432a"
      "4c794600abc6d2a51cc5cac33ec993829ec8bc11b5b12b1019c68271bc55bbbc5ba00d08"
      "da7282232e72267a4fd65806584996483bc7ed96dcc9a2aff94a70515b532d3addff2cb8"
      "6d4ec5310c7009347f589f76cc8f685d82205d0b8e12a7b1d33d302c1f4eaead423746fa"
      "6bf3db841d2c7bdc4373a8398c30f81de1951ea615844a01ed7a565c5b3823dbe53e7dd7"
      "4082bf207edb37c5293393905c9de596b4f9ac1c5b4d48f146a8625f07b042153b22c102"
      "d1031e9e29661a2415ef37a8bd81ac2a8c4efc66fa1adf66bb9ba3071193d0a43315cc1e"
      "8da3bffcc9ff35ddbf0179208b8e4c30db72ee2d3b356a4111208d2c7c9a9428e79a027d"
      "9f33fb921c577307d1d6e72bdb49cdbb52646b1e8016bd37f6d11289315d5fdebc284b20"
      "52a74721a7d5d3113f287ee4270371257ff05d116698dd48f625028e13ce0e97256a673c"
      "6ef643764df4d8cb89834590e515c8c1aeae0e0e29d0199f53b7b0b1939fa51f75b2ae80"
      "b184403a562478c0808475ab4ede7c2d8767c2ee2d65dd0d8a1d292d7b85fb100f4cd589"
      "1de72521a5cd893e1c5333a63b589e55cd87f01ec4a6b9376a6ab8fd4a5eb416f012acc1"
      "2bc3ce3bd605b3403c28fa74fd918c3c44db0a2da8bd3833d54d6b2430257fcf2ab86d61"
      "3fa2f7a898d7f8b66040947afcd1841b20de6d6642d2dcd4c1955948d2b0263a05824af0"
      "b4390e2d647c9fce6f240b22b76a3d1b1948c0bd406edc45da01524ce3cf764c69ae1563"
      "521aba69184b3a354333d3cd5f513bc6a071d78df1c0fef9bf78203cbdf0fac66daeafc8"
      "40822e108dab42f1d755bf364721fc5fd334465faa75100aae1da03b7a7cb29a9ee9e236"
      "0ebde1bc33e8609272a8d5132af65b5d5f324d42e1d2010f149aa145ff218b06fb8fd72b"
      "272298c50b8fbd91701a29b88252cab6ece18e047e1c99195e05280dc27f56092d3633bf"
      "859b9096a2ba23b2ca652c7d5b245c6a4d1499fcce3b3e007e3f5271c0119fb11a547ac4"
      "c5a4b47bbc2e2e636537b5a1b456afff2b6d4f3fd2660513532e4f6e5b4891f249aee090"
      "c364f2f8d6e0b9ce2a82348d33f686308c9a0d78c9d3d040dc7843b99bba6f88d5976dbd"
      "f698d3b58bc27c1bd9da48340fcd4bcd37ce015d0bb1ee004c162a271003bc9bfc02c516"
      "cc44a5e1cccb2d168614cbbf68fd8b13a1a2e2e97eedd95a50af5ac2941a77cea8ac652e"
      "4fc9d61e05e22b22a82221640da7061aa5c9f6e24a990cc4321be265261cbe8d4f01b333"
      "eda12dc44d421f1cf44aae74cb42db04343ac070d0add0056e8486b5ad410c1e18fb6a7d"
      "9a24dfbb615a1d80830d7d8e8c5e82bec580b00812e9b1491777dc8f06419126b21f4717"
      "ddb92b32edf9b7091c7eda38054ae026b16f02f0df3596a74430bbd75d8f2ebe4f90b98b"
      "5ab0f2a3f0573c395bd9f15e3d64dd2a03548515fa613c204e8df8cd706b07f8af679671"
      "e2ce4fb001858fed425dd9692728e52f4f2876c8daa8061a45bdd6aa8f5a1e6b44ded2c9"
      "868fd1094d495193d057829730d6d5dbad1dbb88d9d578c3dc0f0b1bea920d65e0d0f00b"
      "56f07235b80de53aa21c7d48163f60b0cb278f94007aa463bb4e2edb26861f1934cd72f9"
      "50a9fc8d332973847ddd13ef5c8c9dc33c40b840529f6b72b514f9d9b1ca2c628d6e9ca8"
      "56eb5b48ca31160d7655c79f80677d12d03589351010a2d25622b63d79669da1c0f45785"
      "657b7ddb68c543cd565f1e26ddbaf4771be194dbc7a5517dc8acea8aedb8e69ae077d113"
      "0af87ddc629bc620c7f0219632b69be32e46b30641fac9a1bb3ffda1ab287f7c417022d2"
      "6f7e8c96bbf4c2354c46f5a18ee6ad372312eeba3ce7f7616f6093be6d9a005c11a635e4"
      "d02dd05019afae7327fa750a5739d9309aafd80babdc4685be23015a7f25badc356da5b6"
      "6ae625000bfed7c393b07a9b5f0fd73deae4fd7a3ea0bcd3b17c124435b4776fd89e6dc7"
      "2b1f350cea1702b6537e0ef6235979583134df08b892bc44fae8f332a321f780d39a501b"
      "c171abb561d3e25d6771f476bc4bf2a88d12e1f791e6f6cddc8c2b03438d978db0b40d27"
      "75bf3881172ced5f0251bee7828d1aad2fe78550d7433eeb53dc6bae985a7d8abe363ddc"
      "d31998eab92263269543c1fc3544e7df2a924baa9c1b481df28ba594f749cf04a3b11913"
      "265fc4c9be44bc08a64700c3365ad28a11232def5214caaac9782d23bd016b633eab1ffa"
      "c29913153a008768ff3877e8d62d09907ed1be1282795129f1da326812105a95b0366cac"
      "9bd2ca23f24a42a5ef96fbab48948011ed07c7f3a3bdd70c322c7e83de12927ae22aaea4"
      "621dc3ad2dec2c0e0cdc4f08935797f3fb4bbae0cd274c3c113719c6f23c3bebf4f57c54"
      "c146b45354b32a12dee51e3f6f1e590f3fcc61df5318b845e25097bcbe84338a6f0ebf2b"
      "03dbd11d23f252beb88abdf999ed10efae7f60b1c48bd33952986895b5dd92909bdaa331"
      "2c9a061d1fbb3124ab3ad26e08a1eb04ebe00f875f470a743b5f845ab18e883c022f55fc"
      "222058424dd9fa6ad497af340692856dfba69de2f66de97ad1a2bd11",
      ToHex(captured_data));
}

// https://3xpl.com/zcash/transaction/2718c2dea2427f05c0120da629f150bef8e2f566abda02f530eb689e51a75ac6
TEST_F(ZCashWalletServiceUnitTest, MAYBE_SendShieldedFunds) {
  // Creating authorized orchard bundle may take a time
  base::test::ScopedRunLoopTimeout specific_timeout(FROM_HERE,
                                                    base::Minutes(1));
  base::test::ScopedFeatureList feature_list;
  feature_list.InitWithFeaturesAndParameters(
      {{features::kBraveWalletZCashFeature,
        {{"zcash_shielded_transactions_enabled", "true"}}},
#if BUILDFLAG(IS_IOS)
       {features::kBraveWalletWebUIFeature, {}}
#endif
      },
      {}  // disabled features
  );

  ON_CALL(zcash_rpc(), GetLightdInfo(_, _))
      .WillByDefault([&](const std::string& chain_id,
                         ZCashRpc::GetLightdInfoCallback callback) {
        auto response = zcash::mojom::LightdInfo::New("5437f330");
        std::move(callback).Run(std::move(response));
      });

  ON_CALL(mock_orchard_sync_state(), GetSpendableNotes(_, _, _))
      .WillByDefault([&](OrchardPool pool,
                         const mojom::AccountIdPtr& account_id,
                         const OrchardAddrRawPart& internal_addr) {
        OrchardSyncState::SpendableNotesBundle spendable_notes_bundle;
        {
          OrchardNote note;
          base::span(note.addr).copy_from(*PrefixedHexStringToBytes(
              "0x2de7d627d11cc71e2fec164e83b75fad82321c35e5150f2c79949e70d75a35"
              "d0ecb5f0b04eaacd05f73ba6"));
          note.block_id = 3372837u;
          base::span(note.nullifier)
              .copy_from(*PrefixedHexStringToBytes(
                  "0xa9e915b1e94c953e59c3a5635c565daade75f88f2186899d5598bdd0fd"
                  "751824"));
          note.amount = 100000u;
          note.note_version = 2;
          note.orchard_commitment_tree_position = 50094446u;
          base::span(note.rho).copy_from(
              *PrefixedHexStringToBytes("0x827a4a0d2e3035ea1775e13bc30b3bfe5bad"
                                        "69dcda312911349d20adefca3811"));
          base::span(note.seed).copy_from(
              *PrefixedHexStringToBytes("0x8a2a0f00000000008b2a0f00000000008c2a"
                                        "0f00000000008d2a0f0000000000"));
          spendable_notes_bundle.spendable_notes.push_back(std::move(note));
        }
        {
          OrchardNote note;
          base::span(note.addr).copy_from(*PrefixedHexStringToBytes(
              "0x2de7d627d11cc71e2fec164e83b75fad82321c35e5150f2c79949e70d75a35"
              "d0ecb5f0b04eaacd05f73ba6"));
          note.block_id = 3372940u;
          base::span(note.nullifier)
              .copy_from(*PrefixedHexStringToBytes(
                  "0xb8462634013463dbdb60a4531f6547baa1de7dbde99cc669a165812748"
                  "d1a218"));
          note.amount = 100000u;
          note.note_version = 2;
          note.orchard_commitment_tree_position = 50094790u;
          base::span(note.rho).copy_from(
              *PrefixedHexStringToBytes("0xa2d159c97b0e5b5f9977c58b9157ddb9c101"
                                        "d0a79afab3271e182d634fe36c2f"));
          base::span(note.seed).copy_from(
              *PrefixedHexStringToBytes("0x672a0f0000000000682a0f0000000000692a"
                                        "0f00000000006a2a0f0000000000"));
          spendable_notes_bundle.spendable_notes.push_back(std::move(note));
        }
        spendable_notes_bundle.anchor_block_id = 3372996u;
        return spendable_notes_bundle;
      });

  ON_CALL(mock_orchard_sync_state(), CalculateWitnessForCheckpoint(_, _, _, _))
      .WillByDefault([&](OrchardPool pool,
                         const mojom::AccountIdPtr& account_id,
                         const std::vector<OrchardInput>& notes,
                         uint32_t checkpoint_position) {
        std::vector<OrchardInput> notes_with_witness = notes;
        {
          OrchardNoteWitness witness;
          AppendMerklePath(witness,
                           "0xb9e6c18633676666078f2da90e00f847f9d35d17b43e3230d"
                           "52f0f7c1d4f480d");
          AppendMerklePath(witness,
                           "0xfdf8c35a4a5e4b6ee4547aba2ed7734f28fa6e81248a0df87"
                           "f39e5e4462dbc06");
          AppendMerklePath(witness,
                           "0xc6ffab3e87bbbeee82e477b47bffb0bc6ba3e885c97de1d64"
                           "65e652e16a10e00");
          AppendMerklePath(witness,
                           "0xf3e8c87b98950d757e3db46d90ec2f8a6d5f28b27fa3cc0c0"
                           "04f74ae83ea4835");
          AppendMerklePath(witness,
                           "0xbf08d35f5441cec05cf24f54d12db1775e96db5ee47eed917"
                           "bdfda575709bf3b");
          AppendMerklePath(witness,
                           "0x3cf8c626e8a04ded2a01714dd8b2c0579bc04e83b1223b0f9"
                           "80e52da6754b11f");
          AppendMerklePath(witness,
                           "0x0990442082945abda30adf925a05eb3b816cb205d91a8fcf0"
                           "f123389c42ea108");
          AppendMerklePath(witness,
                           "0xcdbca21ccd4d60977432f9fede62ae262b8527ae15c42df4c"
                           "79d19c771288906");
          AppendMerklePath(witness,
                           "0xbbfa1266b18f1cb98746940dd38256789b5dd66527a3e35fc"
                           "084b3827224e820");
          AppendMerklePath(witness,
                           "0x00156bfb4806755c71575bf07975ba53bd91b3e86e24d92b8"
                           "b3a07f11c9cbb0a");
          AppendMerklePath(witness,
                           "0xa3c02568acebf5ca1ec30d6a7d7cd217a47d6a1b8311bf946"
                           "2a5f939c6b74307");
          AppendMerklePath(witness,
                           "0x3ef9b30bae6122da1605bad6ec5d49b41d4d40caa96c1cf63"
                           "02b66c5d2d10d39");
          AppendMerklePath(witness,
                           "0x22ae2800cb93abe63b70c172de70362d9830e53800398884a"
                           "7a64ff68ed99e0b");
          AppendMerklePath(witness,
                           "0xbccfea84e6372bc58fee7024a266768c077cb432a7137bc71"
                           "f8a6a21f13ded26");
          AppendMerklePath(witness,
                           "0xd16ce2138bce884c09007675df57a32eeec505e5ba468ce19"
                           "769ddbe554bc41d");
          AppendMerklePath(witness,
                           "0x63f8dbd10df936f1734973e0b3bd25f4ed440566c92308590"
                           "3f696bc6347ec0f");
          AppendMerklePath(witness,
                           "0x2182163eac4061885a313568148dfae564e478066dcbe389a"
                           "0ddb1ecb7f5dc34");
          AppendMerklePath(witness,
                           "0xbd9dc0681918a3f3f9cd1f9e06aa1ad68927da63acc13b92a"
                           "2578b2738a6d331");
          AppendMerklePath(witness,
                           "0x50fc4bd01275d506ffc3b8391dc5dc9cf837cacfb2a3412d7"
                           "907cda594d8633b");
          AppendMerklePath(witness,
                           "0x2829e8aacdf1501baaeb5cb6e189d4e7182228e3d4b9acf54"
                           "713595241e97f21");
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
          witness.position = 50094446u;
          notes_with_witness[0].witness = std::move(witness);
        }
        {
          OrchardNoteWitness witness;
          AppendMerklePath(witness,
                           "0x29e5cdf91e9f981a55687f14eb8877a7ad68dd99ad3021775"
                           "be61611fb405536");
          AppendMerklePath(witness,
                           "0x6dee77bb7d2e16f65e244639adc15e18dbde33d9397410347"
                           "2f37ead485c6e29");
          AppendMerklePath(witness,
                           "0x7d73448cf21f5b4a89e88411a035c0544fe609e12c807eb52"
                           "d323d58f3d11a14");
          AppendMerklePath(witness,
                           "0x5f51d666690492f84fb87892c2cea3de247da660bb7c2a50a"
                           "131e6172ea0a109");
          AppendMerklePath(witness,
                           "0xe3cc3491ea76602b336d50ad7c72bdc9cf6ff277716c6eff5"
                           "d32449582e38e17");
          AppendMerklePath(witness,
                           "0x7159bc999ab6db050157c55f4f5ee6731dd55f6f59a82913b"
                           "18d9f19d838890e");
          AppendMerklePath(witness,
                           "0x038eb720114006be158a0dcd398b1e1e2dc2403054e054426"
                           "c735e84ea5d270a");
          AppendMerklePath(witness,
                           "0x8324187ce59ecddb29ae3bfee52634b9a8786a47d552b005c"
                           "66ddc1a200d422c");
          AppendMerklePath(witness,
                           "0x68185872cc9ab1750e3ee26594ececca07dd8976cab911317"
                           "6662080bbc8680e");
          AppendMerklePath(witness,
                           "0x7c2fc71855d67cd7f787315d10666f63f6ed924507bb56474"
                           "e1ade9153c19c01");
          AppendMerklePath(witness,
                           "0xa3c02568acebf5ca1ec30d6a7d7cd217a47d6a1b8311bf946"
                           "2a5f939c6b74307");
          AppendMerklePath(witness,
                           "0x3ef9b30bae6122da1605bad6ec5d49b41d4d40caa96c1cf63"
                           "02b66c5d2d10d39");
          AppendMerklePath(witness,
                           "0x22ae2800cb93abe63b70c172de70362d9830e53800398884a"
                           "7a64ff68ed99e0b");
          AppendMerklePath(witness,
                           "0xbccfea84e6372bc58fee7024a266768c077cb432a7137bc71"
                           "f8a6a21f13ded26");
          AppendMerklePath(witness,
                           "0xd16ce2138bce884c09007675df57a32eeec505e5ba468ce19"
                           "769ddbe554bc41d");
          AppendMerklePath(witness,
                           "0x63f8dbd10df936f1734973e0b3bd25f4ed440566c92308590"
                           "3f696bc6347ec0f");
          AppendMerklePath(witness,
                           "0x2182163eac4061885a313568148dfae564e478066dcbe389a"
                           "0ddb1ecb7f5dc34");
          AppendMerklePath(witness,
                           "0xbd9dc0681918a3f3f9cd1f9e06aa1ad68927da63acc13b92a"
                           "2578b2738a6d331");
          AppendMerklePath(witness,
                           "0x50fc4bd01275d506ffc3b8391dc5dc9cf837cacfb2a3412d7"
                           "907cda594d8633b");
          AppendMerklePath(witness,
                           "0x2829e8aacdf1501baaeb5cb6e189d4e7182228e3d4b9acf54"
                           "713595241e97f21");
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
          witness.position = 50094790u;
          notes_with_witness[1].witness = std::move(witness);
        }
        return base::ok(notes_with_witness);
      });

  keyring_service()->Reset();
  keyring_service()->RestoreWallet(kGateJuniorMnemonic, kTestWalletPassword,
                                   false, base::DoNothing());
  OrchardBundleManager::OverrideRandomSeedForTesting(993285);
  auto account =
      GetAccountUtils().EnsureAccount(mojom::KeyringId::kZCashMainnet, 0);
  auto account_id = account->account_id.Clone();
  ON_CALL(zcash_rpc(), GetLatestBlock(_, _))
      .WillByDefault(  //
          [&](const std::string& chain_id,
              ZCashRpc::GetLatestBlockCallback callback) {
            auto response = zcash::mojom::BlockID::New(
                3373000u,
                *PrefixedHexStringToBytes("0x1e600cd2b2f8832c9a3eae4064b5945fda"
                                          "e6e96c266d76335e7f550000000000"));
            std::move(callback).Run(std::move(response));
          });

  ON_CALL(zcash_rpc(), GetTreeState(_, _, _))
      .WillByDefault(  //
          [&](const std::string& chain_id, zcash::mojom::BlockIDPtr block_id,
              ZCashRpc::GetTreeStateCallback callback) {
            auto tree_state = zcash::mojom::TreeState::New(
                "main" /* network */, 3372996u /* height */,
                "00000000008ed7214463263a9d462593f8fc7e860d93960941726c4fe2af06"
                "cf" /* hash */,
                1781091331u /* time */,
                "016082f8ee6423502f98ad4280c96006fc2c24373ca4eeaad1e72df713294f"
                "b05a01a8f9c8a6af30d0f454ee828ee84b052107c9fc48ec0094c3ab6189f1"
                "67e6d9051f01da23cb2bd75678a1748d4c31c907fdf4dd0ff2eeb51671fe18"
                "d3eb77e646d834000160844b6b2a71bf19c30a5febd6cbc93b9c39a2c5e279"
                "c6614a8b0fd80447e62701a640a657861c14161da92759bfcbfbec96700f1a"
                "7721ba2fb9b8c02b51f9c83d0181fa5c90ba5d60a10d465d2a783683a57f5a"
                "2caa76070115348267df7d824f700161c6ad3ccb29b8d229fc9a2d09ef40be"
                "37f39070135a0483f59189a2fefb786701d40b0f68d706717296d5c6b0b33f"
                "1a67a56eb24d2e7b245c60d832a3fa4b502800012f948132821e835359858f"
                "90bfb7b787b1e9339765e5fdb988e483f92fc3672a0001e25e784a9d4deea9"
                "6347b2723195d1111cd3530aa7705b69963cb905e170c71100017fff53008e"
                "35fa8fa40542e41b3ae7040142750024e47b96b2be7db7355ea9440190e323"
                "f6e45d107d4c5cbe2c4cb4c18a466d71c43fb4b4613d3617ed45d8f456011d"
                "85b015e936981d3a869c16ed2e03aeeb2444206933acc3f7b384aeb9632566"
                "01929f0d44045c58428bd8daca3e2c9ca5a0361b5573a4158251dbe9f63a84"
                "e94901931c48ea50688eae5e54c24d0df7a6b00ce498f92eb5e1cfd6a2d87d"
                "8ebd5364017d1ce2f0839bdbf1bad7ae37f845e7fe2116e0c1197536bfbad5"
                "49f3876c3c590000013e2598f743726006b8de42476ed56a55a75629a7b82e"
                "430c4e7c101a69e9b02a011619f99023a69bb647eab2d2aa1a73c3673c74bb"
                "033c3c4930eacda19e6fd93b0000000160272b134ca494b602137d89e528c7"
                "51c06d3ef4a87a45f33af343c15060cc1e0000000000",
                "018be7788c1df1a500fb34470988bb1942095846f27746146a9723830f57af"
                "162a01bbf7f1e56fa477749c12b3e9cbb152420fde2ebb66466afbce849d27"
                "32385c1d1f012f8f20768eae13c1caf717f214fad64dc114dde8dedb3c4629"
                "c50b981c0d2019000001b94b7ab374fd0461baba22a7c49b0bd18314fd3073"
                "fa8a0b3ae217dd28a25a21015587d607319bbf2388ac35eddd98af13820396"
                "c17a50aaccf0ef763d2288780901c453011e5614deafc7b095d15e94aef3e0"
                "5284831c55a779eff25e3f5a2e131e0001408b7b113f634fc73680112bc61b"
                "8028985cd65434fc8a98aef4a65c8279260f017c2fc71855d67cd7f787315d"
                "10666f63f6ed924507bb56474e1ade9153c19c0100000001bccfea84e6372b"
                "c58fee7024a266768c077cb432a7137bc71f8a6a21f13ded2601d16ce2138b"
                "ce884c09007675df57a32eeec505e5ba468ce19769ddbe554bc41d00000001"
                "50fc4bd01275d506ffc3b8391dc5dc9cf837cacfb2a3412d7907cda594d863"
                "3b012829e8aacdf1501baaeb5cb6e189d4e7182228e3d4b9acf54713595241"
                "e97f21017c8ece2b2ab2355d809b58809b21c7a5e95cfc693cd689387f7533"
                "ec8749261e01cc2dcaa338b312112db04b435a706d63244dd435238f0aa1e9"
                "e1598d35470810012dcc4273c8a0ed2337ecf7879380a07e7d427c7f9d82e5"
                "38002bd1442978402c01daf63debf5b40df902dae98dadc029f281474d190c"
                "ddecef1b10653248a234150001e2bca6a8d987d668defba89dc082196a9226"
                "34ed88e065c669e526bb8815ee1b000000000000",
                "" /* ironwood tree */);
            std::move(callback).Run(std::move(tree_state));
          });

  OrchardMemo memo = {};
  std::ranges::copy(base::byte_span_from_cstring("hello"), memo.begin());

  std::optional<ZCashTransaction> created_transaction;
  base::MockCallback<ZCashWalletService::CreateTransactionCallback>
      create_transaction_callback;
  EXPECT_CALL(create_transaction_callback, Run(_))
      .WillOnce([&](base::expected<ZCashTransaction, std::string> tx) {
        ASSERT_TRUE(tx.has_value()) << tx.error();
        created_transaction = tx.value();
      });

  zcash_wallet_service_->CreateIronwoodToIronwoodTransaction(
      account_id.Clone(),
      "u1698hg659yl5tq0lycfsjt2z8gfy2v9q0n92qj4ju0hmvzzp0yvmjwg02qmt7xezmc8qu6"
      "r6amun97r25jxk4dvyv3ykaccrzxzqrj6rw3f2ut8xd22zxp0udmvcccfgqfc3muem8z7rx"
      "yw7dzh3qcnf9wgfrrw8c7e3wayl80gan5hlk",
      100000u, memo, create_transaction_callback.Get());
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
      "0x050000800a27a72630f33754c8773300dc77330000000000022d895b63027dfe929306"
      "89d32588d677d1b0feac134dfe63e95c94f2b54ce594b8462634013463dbdb60a4531f65"
      "47baa1de7dbde99cc669a165812748d1a21879b73e50f8735f95fa680072f52d91b891cb"
      "390d71b91ea1a6c51bafad591d054c4e51cc643d6119d304398604ab3dc0838c97569b10"
      "bc5a4a8c1f4bc6540814493fcea7ad4e79dd873d7fd015eda69b5865b1c06ca9c963b8c9"
      "42dda682f6a337ce16781874dea2e38852fc07f2a78dbd50d6f80a5a47794a22744f85a8"
      "194cfad7e6478a7d4c4d784d590c93442576ca6e4cc0cca74b7b2916e8679e14989e61e8"
      "20ef5b5d90eeffafd9f55eaf6b4b7334f90754804168190d75eae69c7b902437ea667051"
      "e9256f1b45b43ef13416da87afd639a4409233c0d9cfcda8490468320c7d6fec9f4cdcbe"
      "77a8545f9e48dc55457488c36120e95f92b963cf1ec6993e3117709c54212d78af82f248"
      "edc7a137b431ffd1a99b626151e7fc5f25147a4005e53b276b02f57945111e5b17a1a185"
      "09fa870ac629b8c36a4e90ffd258403e624fe2d0d39a808f2798359de68b383348943b22"
      "8e97ff0c72859f5454b85d047bd75984a060706d5447076259d5c38d136a78ff6eb3afcb"
      "0acde0eff43792e3ce43482bacff08f294507764f7341d410b1837a16036721546efac06"
      "43db35be7cf69a18180908bc552e9b0d8b4aa605858e28396a38305a334ede3200359826"
      "fd2592457132a1ffdc145f7e51f6185fcabb09b6ea79b540b7fc693d380618f58609d3c7"
      "2f2595679b6d5a3f21d79ede6f4eb7dcd67fe8e47b9a34ff003f3e46ab985d635f03a409"
      "2e595c7f68ed622b2efeb94185d75a35003b83a3c10ecd834ec5e6dc648d94a5e2da3531"
      "3d97b54be3ebd77f9c2bd357d85f10921596ba27ed31f683f9a9a99f553dd49e8a429b1e"
      "49c1ac88b95f1f8121748e6a9b58bac1cb5dea1c061afab901306305b904399635c49317"
      "8bb3018a8158eed70a8088230ef315eae6294a685d60e39460c4815a77ad5565d894c8b1"
      "a8521ac3ca2cda15c97b1146e7f8be897f82e2ebd60dbe766acf1e8e5173bd349df63ab4"
      "b2f501629c3bbfd6711d79a3e12022d09aec315da8dee7a67dc7c7b1655614ee9f334c50"
      "a55774fb70bd409731c93885edf5699adc83270d3f39a52320f042a39bae300016f494b7"
      "58391ab6b94710704d3e7df0a030a9e915b1e94c953e59c3a5635c565daade75f88f2186"
      "899d5598bdd0fd7518246dd9414a3b3364bbc807032c3604649a736857eaa88951491c27"
      "e035966b569fa6a2e8235659439f28607dc597920d5f2168d5ad1ff079b90db2b5ea7f4a"
      "492bffee95c161fe17dbd171a36e352e1eaf5dfb7a1675bd678383e0bd0af23fa7304794"
      "4c6997016fb1fd578722df9c9fbeed0e443f0cb44017e1c4aac218f12c0b4adeb4c1f926"
      "f38941c362be96ba2a98a6d3902e9aa867945e7d60a77b3508d9a953e6b65fde08275fce"
      "3086381aa0e1aad5e2818975974dea33275301a7319dd0518a5bd584663d48d95f222025"
      "2716982fed060bd0e39431a391c7ab75f03ca76b152240d6b1bf1a97e6131a6030e2eee3"
      "7d931988bc6ebe92207d4d437500e0d8a48f05a1c36e252be6706b9c1b8ad2eed7390bf4"
      "0fafbb2f5939d5798fcdea5e0708726107a00a28e4574a3cac8f3ed5fe9e832c50bdeddd"
      "2205e063dd4009d744ceb9e78375782ddd678aba84e8dea862c93553db329f85da0a6b8d"
      "88941c13b4dd61b1a92a7aa646a415e8e349acd14d9cce852bcb3b3e9e653068c8a84534"
      "9e753b94e55a75b3aeca106cd93b0aac0e860af922fa7bd3d74cf6816f118d0373dc3c2c"
      "32d88939cef945199b8de7679969ba5f4de496564560e7488136fdd1a7ab3d6d4b5f3ce8"
      "8d2be0c7e1361375828fc68fe397208c7115173e2102c860672711df188c0f1b83ae3b94"
      "befe8d16ce32a3e6c318585553361df557f76311544672e031db2eb196c48d63ddd9fd1f"
      "d79c929da982cecce634c57d08d4c07cbd0f5bcfcb74849213815394e22a8e1bb6f18095"
      "683ff6f93ea075dab5e8007687151dafe9bb126d8e2bce34ef8b663ca37873f082e358de"
      "9f38ff063fc36d13c7d8b0fca263fbd4d114639929d10c38e16b32dc69a0867a1540f04b"
      "7f22f84b893cb02e649987aaeb637b105ae93be3e0285d44e902d9978b7bc7c687696d2d"
      "040f82dd4bf334ef286ead91eb2593f6825f8e36a1da567e1415588149cd06cddc95e82f"
      "96cd0bb3693a2238caae39ea6772954c49c2ee2c0bd7d8068261257bc6c12dc4cda7d55f"
      "9ebab1a73734a4933c63031027000000000000d01b4faef4c7fc573aa42fbcd2f64fc5ac"
      "f7c97e5c3ca63fa1263fc3a1badd22fd601c8817d4aa11d26c3fd1e884a12ac969a4a28d"
      "0fc237956a8cd38832dde9390b2fe30a8fd4762b49a4dd73492293577049c5be29a0e921"
      "cad937b4c01f5042982de67204b5368c3bda26761c3e11ba285dc957edd6721820c5d5ed"
      "baa0f2d777b9bacbc2e91229009db79c8258e9b81f1fda6a8a8507a686681ff925b9e08d"
      "7b89c2f18eaf032998c2c03a502f3fc27fe9c840b8fd32eafb1cb427cbecad59c00d8e31"
      "0387c22e4b8d8e4fc379d5588383bd789a60a5f635c91c9efd9284438aae59a85fcb8670"
      "e88857a12d7b66110b4f48aa27abf6294308b0eecb4404e6e90b01b53f042ef211622cae"
      "0ca974364ffbad5c1d45201a4300df2fc3dca90ff73837cf0b6918d04750f45d3491be03"
      "7cf8d6711b3064cecf7daa4ec5179da859016560b41bf5e5538a0db29924438a417c9d35"
      "5b8dd35433c82f6fd0c9045d0e9acb6c55a03511daf88db6996ad3fc9bc52c9a464544de"
      "8b4303f7d988199657aa80466a78da0558df1437d7741a7c07ad2bece3ad4ba9cd3517ad"
      "0eea07b8323a3105b5caeb6062f3b208253336b50e29214b19a658ec3835106e679c545e"
      "75b0c4b74e5b4ede0e0381e998e6424f72790acd9168fc56ab63c52b0b6502a5f18f007c"
      "e3820401595248515c447cd547773c6c5d8af95ff37b032a09146f11d508a346bea4c3c6"
      "4e7da7b62592712b8a18703a5df3fe8461475a4764e464014aa76780a2fe64fb89ec6368"
      "28c98914ff0cc0a830833bda3307eec826f784978397c5de2c6f9c828c899af6827b66c0"
      "6f771c8f1061b39e0d94f72a9ce7dc570c93f4f337e96eeeada580ab39eff3c1209aa2cf"
      "2d7b1f69d15dafc46d3ebfbfd51c3918dd1064a73c7dca3a262a5eefc2905aec74a859b6"
      "f377dbf8c6b803b8563f975da261eb8cfe062522f064eadd10f4a00b22da41c5549cbbd7"
      "891dc00dad2156808aa5d5717eda70105f959c86537c53c87240295666866828e495507c"
      "da1a2205846365e87f16cd91a88e9a2ee4ace70c69796858fb13b5c7d1fe053b2eac1348"
      "6df82ea233a12809361d99e538ac1113e759c8f592aca121da461018d1138e1ee843a65a"
      "309de0987b811ebde71cd4e4a85aef87aca70c4639aead31372566d324b41626fec14432"
      "4d095da60803e4d8d1c72e69972a5461ecf49750b8be8cc6a1ed965e16b3b1950971fe64"
      "972a48adb43e96826b6f06eeeba597c299b1f3b26104bcc65eef0ac042ceb8c276172ebf"
      "2d7569ec1949a333b693e17d97985bf1b24300f73e5dec49ece468ffe5a51ced44814af0"
      "12797d9765b24c0fbebeac0e5a4956a5d3f4507bd24253e448cde6dbff8f9985a8165d38"
      "943a279c4a20edebade6753693b12aace394e485960822be3d83e2e5663248d52b16e8b3"
      "038a3628f0259fd385ee81f138d085df184aae3079e3960cf009efb7e884fd2e393d0f8a"
      "cca5275898a0e6b3113730eb3c46cc1fa30daa4e29b0514a684500dff8b24f141bc6b356"
      "8630975f2bd0fc7907b5796ba250d1c4b51e9c8749ce1e48ca35b3ec29bf55271f8d1f6b"
      "f12cf2eff079121a4506222e7c74130f9ccae33c7217bd9692e8d6f5804ee4a9ce057a28"
      "c5e57028ce32abfb70156b19d1383d072eb7f002f765439a1d80abe28a87b1fdb38b9c1e"
      "51e5256e3f2ece69248c844e4d851821798ee0c384819e66f20334567ffaf3eda39061f6"
      "081fc7b94d77f3b3ee8345376ba93a564d676b9a2c781a9297b64072a323c9ec4b2723a9"
      "27e8af655094b3dc6a1dcda8360a6e33e44a747af1701669bfa41f5a704132c52ce4fe71"
      "1431cafc58a85e3f4f9ab10d011737e7c456c693fb7169eacb73a9006d7a8376fd341bd6"
      "c04e5fc3853f685aa56908e6a130661668dde0d627cbe8988c7d17ec6c2f4a525641e9f4"
      "e973c06c018875279ed12a6c008fd362edb1f23b32ff0991a71f3db1c090c8c18fb6e074"
      "f2b40e13058193c379abfaec982e5549cb2d9451393e8be59831d7a774c5852f963122ba"
      "147715bdf387e7b1f2cc885d16653341bd314047c73e76cf99ee3372381ee80798a70973"
      "e1e79999b9afdff19dd4cc77c29de198d30686173204679ad88e3caaee2fc97279f748bf"
      "43db763faddd40a2a83c1e5b28195759d652dcec4c88102e23ea01fb9de98a29f1092f23"
      "ba37fb420cb9975f67e7a02bdb3e030cb8078ba9126a6b614f95681c35d306fed7bbd844"
      "1326d4d4e805b48c4d2e63379fcb816e8b5ca64af45132341bcd63805cb2bd85f3b32541"
      "90901ec3a72a8888318904203f75a4238cd64f418ca495a4af53a98aa603a49cb6cdb571"
      "88387e91778addb480f8bc28a42ac63080e02479ffbd9496849ddc969be0d34d125cee78"
      "66f59cabb49de88e81f9d9de681fe1bd6fd38218bb07e890d425446c28422f030f6679d6"
      "8955aaefa4b7019bf859c14f8235b27e621973286a5e40903253771768d6ad719db3b334"
      "7cea28f763c2c7b40998a7ad480d1e0cca3feeaa18756ce718286d8ab5654f60005cba9c"
      "601486b1a5fb9b148d225f251c3b787486b50d46b7ad4e1210879b013d2d7ee6d0493c6e"
      "7dd7ea269d00a68186423ae04153e5199f1f3ab19fbc666c9eb1fe83049b045d578d8361"
      "572b52a83f9c3d94c4b95d474108d920da40de38dd5ddfc44ed966d3669af7431809ae6c"
      "4b2e8205ddb8f9e51f2fc6e2749e0b62dcef03db1dbbbbcb825c325e3a35c381ba9a2e51"
      "dd677089d2dc459a58b8b0542d01a0e55a845e75ac21d2d1103424805a2f82d4a068b330"
      "765fcb4342d54e30792001c6f16e172736d8f667bb0e94904826b78bc0841476c6ed1a8c"
      "5c69d95da8082146434a7f2d3fe8e5f68e0b3f1586e736b3ac2cc547e083a611951df593"
      "850c983b0645528de637488dbf3bd24b616a3e20bd3eb9e066a0f3bf3ddb4de3c18eada2"
      "9b4cf43a07230476563c395095f19be893219bc9726805837f7af739ce248c73d7536978"
      "f147b643221aad89da6888043c1d40421f186d33c84ce3547cc8c87770565e998a52fb95"
      "8613a66379eb9f02e397001f8aa60dad693dc3d97ba3567de5991f33021bdc0797123a22"
      "aabd12a8bd27550730eaf6eec020a18627f5d98a422054b5377af838762e6be6dc237bfe"
      "8ad7629362ce6a96cc75d985202d89c152bafe7c467021d18534f8e015d7a7e4db329db2"
      "9d759a2c44f0eff6942761e7ef98d0a733c0627269058c817bc5afae7c1d2c3e39995a55"
      "a8c48d3dd192ef73248972e6334ff83c3317b374c1a44513bc0ad88df01662eafe9503c0"
      "085f5d10713abab311c47bd38b2ceb66cba3044f5ca3e68fb46672889acd27290bd63f32"
      "6faa405d886533e96f13e9e5a2c72dd69a6591343f9ea9e43be45d4f5a20558b93fc1446"
      "1b3555e82c023e52730ab233f7595b1bb7e9420d7890f0e5f9e7b85222dc702c92958f78"
      "1222908f7552b63b0a1c299014017fa386fb6748f685bfe027693a939670f0d72c3d6122"
      "97cb0b011469235cf1645eaf8017ffc84640c1d4576643a030065962603d9dfdaeb55a71"
      "1cad1bdae8b5df1a3cc4f4a39c7f20c2fc218e95d2fbfa74bd1dbf14abda2c3e6f54b5fe"
      "6d1183c8a2b116fcc20d86bea242a807a489efb29b27fff4ef35e8d5f0d1d246d9c15f48"
      "9df92fe63197c1ce1cf32dcd8d587f451238c30897eaa4050c89527a97f053285a09af8e"
      "5aa09de0db74dbc3014a43c95c092746955dc103e35689a55a291f61de0ceeebe44fa8c7"
      "67d8a3f14cbaf21eba1334523a52861735f56769237db3f05b0d1d070e6296dfa5ee9d45"
      "8b65b6ab6707a973cb246d9488c02f9a7c65ab4de4f0903c88aeac5f7ccf00f8158584c8"
      "182f9082e25c71d889a769b931a42c620f80b9691bb6274a8bd08eb56b4ffcc8351db0f5"
      "95179131ed51e70a0f2c4cae956db3a4852fd6587674fa1e93e5888c2926fa9a5325d83e"
      "14474a1efc9dfdb0b30f3625d1e2dd7e08ed8160b5fbae31fc200cff6a5c38852e928177"
      "7ad3d8ff99445de5fed30710214631703dcef2b71d22f564eca712ca1c1b99a02cb2b482"
      "4d56b338a4fe592761c90094eb1e9529382d02eef22e27ef41153293a149e6cd23311536"
      "bffafc9adc96783832c44001b51a15db1458e4d992c6af0709f7a215041f68f725a3eb75"
      "aed45dbe521fd7dae227bc5befa8cb3cf6791e0fab8b86b37a413f48766754cde49ea740"
      "7ed0ffda6d1e08ea68d07dee30ea5df19c179e029f8a4984653b72c7760c5ee6253684a6"
      "0f145952dd1d089f3c27f0e5375f285681c7b75d063c791c5da8e96b4976e4f57e0a3e18"
      "427c2f6b2b2e2cc4ac5ebc0f4bc705dd07859101edca83998b06444dcc0317de6efd036b"
      "8fb871a4a23e7daf0fbbba018240d30851da1408091649ac1238affca158009394d2860d"
      "cda051c682b491627c5122b0aa6c950cb76ded18643fba52a8d7db71c3443485a0e35f80"
      "aa6fcd9e0ae6584f847d77f996b8cc107c370a95a4543d72ef24f9a687b40696776a2b65"
      "88adae63aa8770d99a847b6dcd3f4111e627e71d1f06a0f1997cf1c256554729897ad6b1"
      "697832d11c873723ca1221ee326a8e76ab621f8a222cb55b46b021ec7ea44843f2c8f512"
      "6d48b5c57c1c7d9c1122e81adc96811055bcb63c44be9f109f025d4b5186843533dd47ea"
      "770de0396ef4f6037adee574c91a10c459e313a87f7d1c7a4c19db14beadb5445a33cce4"
      "908dffb6c401b60bff2e3a2ef5b5f2200e000743ad248511940a1808db03556b776c15ea"
      "818c2074b3c3e947bcc8b75b1133483f83a795fca6f2a1e3fd3ed0c675a017ab6b27f502"
      "cfdd7076330ef2a50f386af878db8d2f81c33f25ca30dd145002c4227278eb0033614e8a"
      "4f1c0a54eed81fc11c3b528f5c38c6caf33b418fcae6bb9e7d80fa0178a62f8c99ff85b2"
      "21f51a183ee3de045a6b71ebc2347fbcffd1333a8054001bada9459c751e68af78b7fb19"
      "0f59ff84f5970ae0c73699a07777234bf9e43a4d68e10822565decd670770f3b2c849acf"
      "e93e1236892c2616f8a47a641e219aa1b348ecb755a65427ce869ac6f9fd07c67f25cc38"
      "ff16df1a2690dcfdc8d31d3efe2efd37351df5d6642ca8f8878a4e477868e88bd8249f5f"
      "cf1ffdecf0c6f9767150e09dc6c417462ac77a02f98f7af85cacce72ec338cd27af4fca2"
      "d505f10d7813859d1d43c8e577e26c50e3dfc75d987dc7530c273ae20d2371a13e3db59c"
      "c87ab04ffe46230688cbb20d8584a8ab333f905a2b2228af201fd3ad37a721a3b49960b9"
      "b442c29d2c7bcb384c1efc5008be77e8d23b12d6119a6eb14a6525e1d9344e8c6e56e0ee"
      "1109481b12b93d7e6eaad89b3c0cdbd86349055896529e5ecca0d9803e17dad938c3fa49"
      "eba65626c9b6f1d29c309edf10b5569d74ec6f1d70240cc5e733ea2f1afd80371fcce981"
      "43aa6601361667fbe882740a7ba62d1696fa56ffcd76a45494f63b4cd8dfc76344cb37cf"
      "bb1417cd1941dfd0053960e31d30833c6f96776c0a1b58f823e3ed1a82a4b485f80236fe"
      "07ea66aaee0a628477a22a0f269e5d167973a19ec63ecb87ab7062ffdd0cbb73252cc9f5"
      "8443d6259b3c83b61f787608a2b8f8940ddcd489e59a03cdb12f770277392eda0780ab4b"
      "19fe7943dca0e6f3603c4f45a2f44e947e8651b88c385811009461f2e8bc59f69aacf667"
      "91518f98ca135d1384bbd3a17643efddc82abc83ba3de05d3ca24c59e13138aa8fa017cd"
      "756b9fef116e209fa8fcdc51b020b3af0e0954c7ce7e4eb49978477208ca273f56d639a4"
      "474fc1d9ed6f49d0631695c2c4cf472cd050a7696cc2654ed5ad982917a67a49efa94788"
      "7443d8c87c3369aa88bc811dfd507b9851f8ae540ecc2204a7ceba496bcfdbd702c78457"
      "252b6b8b5af58f3057c6b590f247c8e3c586a528aead4232819d881fe37a7c4ffa3d584c"
      "6f31e241ade4a2e77144c624effa9d5ff60e8e993231812e3aee0764f81e9fe79fef7a86"
      "6c70d9e93d9583245f42410b1bc6460db40e37effab8927908218e0d583ce33a7885861d"
      "95be27b457703d91977a55a98cadc5239aec13ae3b2f2761a46dbd6ccca56a7ce120ee16"
      "2e5811b83c23643d352f1b0124d199579f07ac65a84a01e83b23e741c7e0d1dede66875c"
      "f3426ce7cbddb5e9deb46daeed30e0ee6ed758c9a93a268bedbf4e67dd094d96623f29ec"
      "d44f1463f5f6de26673e0643779e4be7d4acb3fc5f84873879588fe918a78345087200e9"
      "1a252d52e33425ea6307aa659c89c7d77fc39f22bc5192c5ae5484b7dd4177e882d1daee"
      "1d2272881fe7e527847de7152134d4d5998597d335b4a5a7d665e20330a22074cc371bac"
      "f1673c2452376c393d412c889299b53587b89bcb1cef629e16094c59aa2c56119c3b13e3"
      "83ac0d49d40438505448c7612f6dbdb340c488be37dacf810f2d0fd596bf96889a07b734"
      "07844f3c386eaa73812769aeefaab92d1686e0e8aa1c6891e834dedf73bcf26abb2d4d7f"
      "e5a4233375bd6768d96898ffaea20fd6a4287c1fdd6e53e5ed1a87dacdb1d32bf64aa229"
      "47bdbf2cf73f2f25cc277ce1fd3f9606be2e57d1ce2159934cc478f8157877e5b6c3e13b"
      "ad1552f64a2da242bb0f9862a6f31602653033c132021f65491938be21716c3139d816c5"
      "b895aec3341df202c63a4abbc42beb5553a32be50c7664d62e464358ab02f1eb573c2cde"
      "59305cbe583c5a3bf79e2ea21946ebca8865fd12ea41bfd48d4b721dc99d252ddc31bdad"
      "0a196841bf8c19488dc3b293814c574a405dde8fe2c0d18a663c30b62d12ecacf35a5c9b"
      "890c0295c6ce07dc842747f1a74b497388c1a5c25edccc76a437d193da3d2d0cb8ef2871"
      "3d9da8f7fd6a79a2ede977ac17d6c1fa349cb1adae00b651fd4bbc67f650fb0e4057578c"
      "a70683f05d4a9f8c0d64104871cf02b6d43fda4db24c8380098502dd474e027176aa8718"
      "a8b9000eb835e527c349eb318a107492c066ba644b90e69f90a7a67ee2ee51e88a0da4f4"
      "0ae8ee4055842f0aea067e3e6fb65c5c9d145b77413e631b7d4c85ada4ddb6e96cd6e4c3"
      "490702c99f1b0d108098b4e9e685b0170899c59c45ad4cbf459fa34c52c71b336f78a536"
      "6b146b403f8cab864e74496fbc0b5e1ee3f102b513000ffcb88a2d2f36e20d44fb15cfd9"
      "2c9f2c937ded0094625caa50e7f1262f74c4396452b399171f46e9b4f40922226b23c875"
      "a8c867c31b8bca97a4f2d0407919206fa72b81c860c4f34b582c29d07aede11174c33b8a"
      "a906e4b60f05ddd52966be979171ca1176f180a0d119060366d7e4194bc5b2f464719879"
      "8dd73598efbb71715bd2e2bb987f7251492fa61d3b34c906b4a5ef4d681c234ab5ed1f43"
      "52ba5426f718ca95daf1319bc31185da6d36b6b72360fdb8653f64964af77973e8f624e0"
      "f2e5d83d9f6323357c21f68ccf4541cd48a5bd4e1ee097d6b4ca80872d8f2c715a677d99"
      "597e355f9e0bff139ffefa5576923ea52c1e3920389a19b27dc6ac0201ffff474c349a29"
      "4b0eff633cbbd759bf3167f4a190ebbea495a79759987706d70aa5fbddc37c1a322b604d"
      "a4ae997205909b6f140b374dd09d20d69dbf084260954b235c65d34a4e342170fc3a3f23"
      "38ab564bd510f20951ef90b73eae1bc0eb481731ca104eb7e03b52d3e6f0b4b0d68bcb56"
      "2ca199f18aa8228d3ebcfdf5ea019ca6e31cb23b1f1c42ea51bc5038c66bc35fc42c224c"
      "06d741249d0fbeae8b85bce37cc3d73ea03c085a863062466fcedb60751563a9fd6970de"
      "c0dded30c4d410e7d2ce794f5108091d2901d971c49d40ef738cacdb9aac3b6296a67dfd"
      "01ce8664ec2c92edce1b8ca8595847aa41a01ad14a7852f90c84217d919c48476f40cdc0"
      "dd2e694ee1047a76498245fe98e5c0a4d375ecc81fadd28a37510e83258a910be794ae2b"
      "9e2c5db551247ffe9a6797d0b56fe987d3f3a201310f1a0518a9093ce40c936c952920e1"
      "0ae9b36c052751b8bfed6e69783df5c653e541e485e7b75b38e3f0fd6d3afa1195812f69"
      "4a757ff89b34958cd349a92d0997bdd9c55df6c6dbbea8161d117a17f589b3d7f8ba414c"
      "3d0567fca7c9ca5de4bf8a648fe1ea340fcc459ada215997b1f3b238b4f33cba8aa5cd7c"
      "2e2eba49f49b51ed89c11159d780b577221408e7b0fac8b6b2c6686ebb27f9e3ffbb788d"
      "d48d762b8e87f4b071ace7c4550bc1fade081afb2d111314c4f689886c0748c763f76c11"
      "ceb7d0bf6987d00cdb2f69e4e8a15ac3cefec25ad2a5bc620238851b0def3905c6eeb470"
      "be60a1e18f31478c547072d484bc7dd807e969658e31f5760bd6183aaad7b84aa80f271e"
      "71142e9469741891c2a6cdd076382ce999102b473f0c1c63e1821608d13cdf61fd1e384a"
      "e709955ca51c1bba97848724bab1dc6c177866458293a281f851d822b7081b73cbdf4074"
      "70aab3e17d383255a2de0b14552d7a59c8500cb6cacca5238d119547c0d093c41f1fa4bc"
      "7f6e632fdc7fa07d629d8d7f4eaaf6d5c81d9fbbdb3defd6cd9463c6ee010ab476d645d0"
      "ed2aa724d24b0c1debc831be38f0dbbfaa0de9ad786297dc94a4b5c45d5776ee43e7f677"
      "d02d73d66b74da4299b19bb1b1313087918a83c160d84ab16847290dbe2f97ed6a50774c"
      "c4a7f12920cb97ae0b3bc6e2ec321d47f4f00b6e95d320c4dcc9aabd7961a80be6bf88ee"
      "856d84aebb09a827b89fce339b248fb9a0df904651d3bd5e300530f52c09ae4eec67c480"
      "3922b0ce830c8a641d0a569fcb377bfe31be926827f72165d3d5d9ffa7cc9478c72d5ff7"
      "fc4e7e1dfc3d4582800f1aa2afba42cc8678ea70bc0ccf5dedfc02c52f1c086f449281d4"
      "88b8fd22dc4ea3d3002a09c722486b0ee6ab3ba8566348b8e62cbdfece763b1744aa84f3"
      "930b2b72d38dcbfdf0a1c75272cee7cd449ec818f5363dd1b1e05b8f731d96b294b00571"
      "27c3a10ef0140f100e09a162fc2e1b204c1d59034bfc61ef3e8af33c261cdca91f681141"
      "4f97583de71131b55ceffe96dd39f582dd81dc8efbc1ab685b6494c07d6d53047b88a463"
      "d9e428465234f79fff34b66f235382d923381d09e7abd474dc391ade2abf152a26fe80e8"
      "1090422da8043a4025967abdcb1a8d9244dfd1a4c847d00be48f7c3a6119cc2790945b1f"
      "14277d991e44ed1f5546d41608706973b2df39409a2a5c00015cbbf83431463f811f0f22"
      "ef76d295051e0f9550616a35efb25435a44b1dc4d5fee1ad16595d7805ae330705babfc3"
      "f51930aee17aa032934f5e9fc96c13cb3309ebbd1e5cac5363165252f7f4ef8731a86d5d"
      "2181ac2f2b2142269961a2a114d556d600b1fe29fd9665807c743a4ad140934a5cbe13d1"
      "5d9c5f4901d5d678250c4a86e2ccd8b782b1f0bf06ea3c299443e782faec6cf7fd570bb3"
      "b13ec153ffe3ca62f8d7482dec29cb5956f5e51bdf97183326bef12b93e89bc635a93755"
      "b02971cbf9966f7d8e9f8f0d8481e3cad1e47787b16e0bd9a039971de48f39a4623d779f"
      "b8665f9b2437424cad16b7388c0d48662e9c073c080181d142007b527df833eb597ce38d"
      "dc313be92ed07722a503e2a0a3198d39c53bb7b0e3d9e7302e414e13a28b236c7fafa8cb"
      "e11f9cbae4989d47134c14fdb91d6333e85cfad43a7601abea620791a133e9535b494740"
      "684b03d40d7fd63af9bbf35c7692c2a3f1c4468ba889340b5f17b641c6591e93aecd135b"
      "0aa90b7bf9603fb54fba8fa08d7c409a6ffee5806fa336e91566092258c006eeccc7550e"
      "a80671547506fd48e61e7b47d9f2643ca438351077c686097fcc0b2a51139db1f80ef987"
      "969ac7796378479ca0fa88b106bd6019359a7cb97e39dba46d02079789a29f09e1516a87"
      "6d0d2e2353bc4389393204c288a2e8272415542ef7eed2e507f9548bbb94fea4278601c0"
      "262f08261dbeaa9ce563adfe62745b080b0a37c114ffc94d6ccd9b6ccf22053f300bdb76"
      "d081156a4707178567663511aec8bf9841f9e88d503d6db6ec9d81068fde607b5a1294ea"
      "a8b397175b00f465ef8710175f243fab6f6f080371054227081241e3c497129cdf7e96b3"
      "51c67a40021bf9991a73bc2cde391f039368ae142c92af62423fd2adbe0c274ed03cfea9"
      "e54f33f0facfc2bd77af5ef51eb0883cdb9a6bf23da77ec083e0ba20120cc37f1a914882"
      "fd9ef253e30d212b699a50819ad73b8d26a9490becd8097ee535eda60cc164e9812d3ea1"
      "0ea46d5d699761151da156e046273ea56f8186069d9dc71a77349bf1fc67b8c26843e3d4"
      "91a1be03ddadd223252792ee8a4a65c487de242bee47737f8bb5927320f6e1046ce12183"
      "79c048baa2abd13f74c6abb6ea942063e39f94a9f85b89c909777be38744fe5cb8d7a50e"
      "f01b9f31f1f3be685b811f794b61fdd78b0c053ac217fba76af94ca026fc0c36e01cd700"
      "c815b05fe67e43179d40805275e89b2290465ed2c3712727f0dae8869532ba9cc8e928c8"
      "cf6360fb9ab15d3887afae763ef7e2c07aa53d054a3182e6e63612571e30c02a832a9f35"
      "a34ec390048b952aa5a0f297dd2b729c052e2d91f03e",
      ToHex(captured_data));
}

constexpr char kLuxuryReformMnemonic[] =
    "luxury reform inner vanish palace addict alter control casino bean "
    "metal two banner fatigue type sponsor sun plunge hotel shadow host "
    "health cabbage tomato";

// https://3xpl.com/zcash/transaction/2295048f00c2264cc068b85a6695c4b89cdd06f1e686c69c990cecaf74db0937
TEST_F(ZCashWalletServiceUnitTest, MAYBE_UnshieldFunds) {
  // Creating authorized orchard bundle may take a time
  base::test::ScopedRunLoopTimeout specific_timeout(FROM_HERE,
                                                    base::Minutes(1));
  base::test::ScopedFeatureList feature_list;
  feature_list.InitWithFeaturesAndParameters(
      {{features::kBraveWalletZCashFeature,
        {{"zcash_shielded_transactions_enabled", "true"}}},
#if BUILDFLAG(IS_IOS)
       {features::kBraveWalletWebUIFeature, {}}
#endif
      },
      {}  // disabled features
  );

  ON_CALL(zcash_rpc(), GetLightdInfo(_, _))
      .WillByDefault([&](const std::string& chain_id,
                         ZCashRpc::GetLightdInfoCallback callback) {
        auto response = zcash::mojom::LightdInfo::New("37a5165b");
        std::move(callback).Run(std::move(response));
      });

  ON_CALL(mock_orchard_sync_state(), GetSpendableNotes(_, _, _))
      .WillByDefault([&](OrchardPool pool,
                         const mojom::AccountIdPtr& account_id,
                         const OrchardAddrRawPart& internal_addr) {
        OrchardSyncState::SpendableNotesBundle spendable_notes_bundle;
        {
          OrchardNote note;
          base::span(note.addr).copy_from(*PrefixedHexStringToBytes(
              "0x037f9054dc0deb4c01dfd4b6c47aec0d2b57f9260b6d35f70f2cccbeb39b0d"
              "ab9b253cfaa51f12fd428a83"));
          note.block_id = 3011828u;
          base::span(note.nullifier)
              .copy_from(*PrefixedHexStringToBytes(
                  "0x91a6a8efd7d2521c718e26f23c73a1ea601d704be18d49295bc4decb51"
                  "e4093d"));
          note.amount = 5000u;
          note.note_version = 0;
          note.orchard_commitment_tree_position = 49149412u;
          base::span(note.rho).copy_from(
              *PrefixedHexStringToBytes("0x6b5284a86b7aaabe97f97f053663ff92fe"
                                        "34eb9364f0a8633d06c95372abc73a"));
          base::span(note.seed).copy_from(
              *PrefixedHexStringToBytes("0x3ad7d24582e4d5ad21d5dfffc727c0adf9"
                                        "6a62822bf7a53a2be78c6cbfe5681d"));
          spendable_notes_bundle.spendable_notes.push_back(std::move(note));
        }
        {
          OrchardNote note;
          base::span(note.addr).copy_from(*PrefixedHexStringToBytes(
              "0xe188aa79805343f5edf9fe0fb247f1811aebc31d50787bdeb5b5c978579d03"
              "affbaf35e6751e74d05abfba"));
          note.block_id = 3373001u;
          base::span(note.nullifier)
              .copy_from(*PrefixedHexStringToBytes(
                  "0x2077e07a39f7c7d46bde478f7ace002be596b3ab2be9178dcc07636df0"
                  "461d0c"));
          note.amount = 100000u;
          note.note_version = 0;
          note.orchard_commitment_tree_position = 50094972u;
          base::span(note.rho).copy_from(
              *PrefixedHexStringToBytes("0xb8462634013463dbdb60a4531f6547baa1"
                                        "de7dbde99cc669a165812748d1a218"));
          base::span(note.seed).copy_from(
              *PrefixedHexStringToBytes("0x20280f000000000021280f000000000022"
                                        "280f000000000023280f0000000000"));
          spendable_notes_bundle.spendable_notes.push_back(std::move(note));
        }
        spendable_notes_bundle.anchor_block_id = 3446634u;
        return spendable_notes_bundle;
      });
  ON_CALL(mock_orchard_sync_state(), CalculateWitnessForCheckpoint(_, _, _, _))
      .WillByDefault([&](OrchardPool pool,
                         const mojom::AccountIdPtr& account_id,
                         const std::vector<OrchardInput>& notes,
                         uint32_t checkpoint_position) {
        std::vector<OrchardInput> notes_with_witness = notes;
        {
          OrchardNoteWitness witness;
          AppendMerklePath(witness,
                           "0x3fdf35e01cf8b4d8af3bcd"
                           "fbbc223f4282f573d38450910df90841f7f6a3281d");
          AppendMerklePath(witness,
                           "0xd84e1f1ab06c76b9af41e8"
                           "de6b516cd4b015e78d29b01af241e1941a44ea7d00");
          AppendMerklePath(witness,
                           "0xb1c4a3ca826e6538520f01"
                           "a4f48d8e59555273ec661c5386f000eabdefce332a");
          AppendMerklePath(witness,
                           "0x1165edf78780c70369a682"
                           "025035240622f35a942b35f7319235ed3ac535293d");
          AppendMerklePath(witness,
                           "0xbed3aaebdd6981b130d1e1"
                           "90fc50d6d99db5e88b990931bfdeb692997e127e32");
          AppendMerklePath(witness,
                           "0x4712811b685d505503f141"
                           "3d22a51564c9fa403bff63bbda5b79438a92a48402");
          AppendMerklePath(witness,
                           "0x3365ebe17c786368a6097b"
                           "2a6c0dcb137ead609096a959fdaf1a315fb93caa03");
          AppendMerklePath(witness,
                           "0xd0869d5c129e05f1ed7e22"
                           "57a0ed32f63e6e2a005a5813ff5f5e3cfd54278737");
          AppendMerklePath(witness,
                           "0xcb761c3feb97fe19b40f38"
                           "51099fa22b3d5a17f33d787348bda688c592db4b34");
          AppendMerklePath(witness,
                           "0x5a0aec2da13f41a5f1a345"
                           "49e5398eb8b769d67e721751fb18c780ffeba91321");
          AppendMerklePath(witness,
                           "0x8355eae66213e50fdb48c4"
                           "4c823397caca4c8c8007ab61c688393ff7bde1a528");
          AppendMerklePath(witness,
                           "0x8fbbb93b5cbc41abb339c9"
                           "9e5f86bd6ae431de20c669116df85cfab28490cd34");
          AppendMerklePath(witness,
                           "0xb71849308f20312a246eb4"
                           "056c132c7847151470210cd119a461ffc08ff94b09");
          AppendMerklePath(witness,
                           "0x7a7bd62cf54ed2ed18d1b0"
                           "e56f209a169a9b047b425c7e5038c22bdc82dcf017");
          AppendMerklePath(witness,
                           "0x63275c8cf2223bdb4aa14b"
                           "1c86d140c5e30db04bee30abd21c94d08524850126");
          AppendMerklePath(witness,
                           "0x965f70049519600185d1a3"
                           "ba0c418edc1da0bb2d978da1e3ceec93b8851af419");
          AppendMerklePath(witness,
                           "0x4c093ed9408ece3d125981"
                           "e3d091e9c811ba0472a15ed8a14eca311642d7ec31");
          AppendMerklePath(witness,
                           "0xeba4580410c682affa045e"
                           "43c5c34a2e6a1a20b01f9ffdcc8d330b66d2595830");
          AppendMerklePath(witness,
                           "0x2d113bc8f6a4f41b3963cf"
                           "a0717176c2d31ce7bfae4d250a1fff5e061dd9d325");
          AppendMerklePath(witness,
                           "0x60040850b766b126a2b484"
                           "3fcdfdffa5d5cab3f53bc860a3bef68958b5f06617");
          AppendMerklePath(witness,
                           "0x834a5795c6edb0de014c19"
                           "92743622c75f352afeef725033653cc70138ffed18");
          AppendMerklePath(witness,
                           "0xcc2dcaa338b312112db04b"
                           "435a706d63244dd435238f0aa1e9e1598d35470810");
          AppendMerklePath(witness,
                           "0x2dcc4273c8a0ed2337ecf7"
                           "879380a07e7d427c7f9d82e538002bd1442978402c");
          AppendMerklePath(witness,
                           "0xdaf63debf5b40df902dae9"
                           "8dadc029f281474d190cddecef1b10653248a23415");
          AppendMerklePath(witness,
                           "0xf76156ca12fa931d9e93d6"
                           "8d62432db9b94afc41e5c554e1e2fc222299aa5211");
          AppendMerklePath(witness,
                           "0xe2bca6a8d987d668defba8"
                           "9dc082196a922634ed88e065c669e526bb8815ee1b");
          AppendMerklePath(witness,
                           "0xe8ae2ad91d463bab75ee94"
                           "1d33cc5817b613c63cda943a4c07f600591b088a25");
          AppendMerklePath(witness,
                           "0xd53fdee371cef596766823"
                           "f4a518a583b1158243afe89700f0da76da46d0060f");
          AppendMerklePath(witness,
                           "0x15d2444cefe7914c9a61e8"
                           "29c730eceb216288fee825f6b3b6298f6f6b6bd62e");
          AppendMerklePath(witness,
                           "0x4c57a617a0aa10ea7a83aa"
                           "6b6b0ed685b6a3d9e5b8fd14f56cdc18021b12253f");
          AppendMerklePath(witness,
                           "0x3fd4915c19bd831a7920be"
                           "55d969b2ac23359e2559da77de2373f06ca014ba27");
          AppendMerklePath(witness,
                           "0x87d063cd07ee4944222b77"
                           "62840eb94c688bec743fa8bdf7715c8fe29f104c2a");
          witness.position = 49149412u;
          notes_with_witness[0].witness = std::move(witness);
        }
        {
          OrchardNoteWitness witness;
          AppendMerklePath(witness,
                           "0xa6a2e8235659439f28607d"
                           "c597920d5f2168d5ad1ff079b90db2b5ea7f4a492b");
          AppendMerklePath(witness,
                           "0x35de8591ae4d7f39cff9a9"
                           "e1fced7244a94e51453fc31cc425e117ea96954104");
          AppendMerklePath(witness,
                           "0x0734b899442f7fe0a2f3d1"
                           "834869e5bd2ac719a129c876b8381377d0f80a7b14");
          AppendMerklePath(witness,
                           "0x93cdfc5e4a42bd53f6c205"
                           "1860d36f39d4f3943c40ba7e4e695a0e65deefd418");
          AppendMerklePath(witness,
                           "0xb94b7ab374fd0461baba22"
                           "a7c49b0bd18314fd3073fa8a0b3ae217dd28a25a21");
          AppendMerklePath(witness,
                           "0x5587d607319bbf2388ac35"
                           "eddd98af13820396c17a50aaccf0ef763d22887809");
          AppendMerklePath(witness,
                           "0xc453011e5614deafc7b095"
                           "d15e94aef3e05284831c55a779eff25e3f5a2e131e");
          AppendMerklePath(witness,
                           "0xe9aa7fc99c1cf338b68a17"
                           "043fba2cf6819ac9a078284d254d2597457dab9638");
          AppendMerklePath(witness,
                           "0x408b7b113f634fc7368011"
                           "2bc61b8028985cd65434fc8a98aef4a65c8279260f");
          AppendMerklePath(witness,
                           "0x7c2fc71855d67cd7f78731"
                           "5d10666f63f6ed924507bb56474e1ade9153c19c01");
          AppendMerklePath(witness,
                           "0x1804cb54cb43587a47db4d"
                           "277642cd9aafc81719678de0afac9354ef9b25fd15");
          AppendMerklePath(witness,
                           "0x6168d1be3f5764813e09cf"
                           "16500a1f6749dfd50e292e381bd8ac2408bc763211");
          AppendMerklePath(witness,
                           "0x7c0fcb30f7ca409b3bea0f"
                           "b82fbd7dfe0fdaaaef7bfdc8d1828bd9d39b94db0d");
          AppendMerklePath(witness,
                           "0xbccfea84e6372bc58fee70"
                           "24a266768c077cb432a7137bc71f8a6a21f13ded26");
          AppendMerklePath(witness,
                           "0xd16ce2138bce884c090076"
                           "75df57a32eeec505e5ba468ce19769ddbe554bc41d");
          AppendMerklePath(witness,
                           "0x7ab52d2df61b6a336dda3c"
                           "1b41bf51b2f3590579040d690e678d0cbbe931440f");
          AppendMerklePath(witness,
                           "0xa1e9566bd694ac7b4a82b1"
                           "69597f761a5095c4914e86828ddc8054e026a1593a");
          AppendMerklePath(witness,
                           "0xdbb7779a4beda752a3a525"
                           "89a1bb22170df4d0db86de8bc99adaa3850ea01006");
          AppendMerklePath(witness,
                           "0x50fc4bd01275d506ffc3b8"
                           "391dc5dc9cf837cacfb2a3412d7907cda594d8633b");
          AppendMerklePath(witness,
                           "0x2829e8aacdf1501baaeb5c"
                           "b6e189d4e7182228e3d4b9acf54713595241e97f21");
          AppendMerklePath(witness,
                           "0x7c8ece2b2ab2355d809b58"
                           "809b21c7a5e95cfc693cd689387f7533ec8749261e");
          AppendMerklePath(witness,
                           "0xcc2dcaa338b312112db04b"
                           "435a706d63244dd435238f0aa1e9e1598d35470810");
          AppendMerklePath(witness,
                           "0x2dcc4273c8a0ed2337ecf7"
                           "879380a07e7d427c7f9d82e538002bd1442978402c");
          AppendMerklePath(witness,
                           "0xdaf63debf5b40df902dae9"
                           "8dadc029f281474d190cddecef1b10653248a23415");
          AppendMerklePath(witness,
                           "0xf76156ca12fa931d9e93d6"
                           "8d62432db9b94afc41e5c554e1e2fc222299aa5211");
          AppendMerklePath(witness,
                           "0xe2bca6a8d987d668defba8"
                           "9dc082196a922634ed88e065c669e526bb8815ee1b");
          AppendMerklePath(witness,
                           "0xe8ae2ad91d463bab75ee94"
                           "1d33cc5817b613c63cda943a4c07f600591b088a25");
          AppendMerklePath(witness,
                           "0xd53fdee371cef596766823"
                           "f4a518a583b1158243afe89700f0da76da46d0060f");
          AppendMerklePath(witness,
                           "0x15d2444cefe7914c9a61e8"
                           "29c730eceb216288fee825f6b3b6298f6f6b6bd62e");
          AppendMerklePath(witness,
                           "0x4c57a617a0aa10ea7a83aa"
                           "6b6b0ed685b6a3d9e5b8fd14f56cdc18021b12253f");
          AppendMerklePath(witness,
                           "0x3fd4915c19bd831a7920be"
                           "55d969b2ac23359e2559da77de2373f06ca014ba27");
          AppendMerklePath(witness,
                           "0x87d063cd07ee4944222b77"
                           "62840eb94c688bec743fa8bdf7715c8fe29f104c2a");
          witness.position = 50094972u;
          notes_with_witness[1].witness = std::move(witness);
        }
        return base::ok(notes_with_witness);
      });
  keyring_service()->Reset();
  keyring_service()->RestoreWallet(kLuxuryReformMnemonic, kTestWalletPassword,
                                   false, base::DoNothing());
  OrchardBundleManager::OverrideRandomSeedForTesting(55595u);
  auto account =
      GetAccountUtils().EnsureAccount(mojom::KeyringId::kZCashMainnet, 0);
  auto account_id = account->account_id.Clone();
  ON_CALL(zcash_rpc(), GetLatestBlock(_, _))
      .WillByDefault([&](const std::string& chain_id,
                         ZCashRpc::GetLatestBlockCallback callback) {
        auto response = zcash::mojom::BlockID::New(
            3446639u,
            *PrefixedHexStringToBytes("0x00000000006f3ecd55d028513d2e8438520"
                                      "afba943b9b08c7a2f1a8d60a09b5f"));
        std::move(callback).Run(std::move(response));
      });

  ON_CALL(zcash_rpc(), GetTreeState(_, _, _))
      .WillByDefault([&](const std::string& chain_id,
                         zcash::mojom::BlockIDPtr block_id,
                         ZCashRpc::GetTreeStateCallback callback) {
        auto tree_state = zcash::mojom::TreeState::New(
            "main" /* network */, 3446634u /* height */,
            "00000000006f3ecd55d028513d2e8438520afba943b9b08c7a2f1a8d60a09b5"
            "f" /* hash */,
            0u /* time */, "" /* sapling tree */,
            "01402e998bd87610213bf252de2486f2b1fee9bc0abadcb874c5bcb1a288f2b"
            "20d001f016013236dab911d31fca5e1b8e3f91978a9a961d2a3bceb9e6df3c2"
            "012acc521b0149f1a3b1f33ee8f31b0af63b05e9c9d2872ee18f6420e7b6d95"
            "e1c98ebc3b82d0001910d11f215f2bbed7d8ab93745c1fb4936d98c3a27b707"
            "b4623856517e22df3e01af345e1420ea4164e1482c45131e27d99719345e9a9"
            "79b65a9365e9c6d07fa2001bb016db508b4d735447b6d3e8b657d8529e2304c"
            "f15091346bb569672f8bce1101b57679e11e026ff0c52d93d7bb2a12911c323"
            "d6acc1b357f1b4facd14fd8bc1401559356c3cd59408f21a491aba96293feff"
            "bd04a392e894fe4d4a0abf7812080d0146928a2747437d53dadfc360f9a1aa7"
            "efee8999ddb7f4f433925a23b2fb5eb3c0001e66fa600f841db699816579631"
            "5a885f31668979ba7335170af2e0481ffda4180164935f5a70dccda0ed0963b"
            "96e1e2eae4c401545df04a3784e572e47f95b50030000018f37179e8204e48f"
            "ed138154f5ac888a307467595feea345556d3e3ee7cd6e2d000000000000000"
            "0013f3ddc746e57791a2cf8900143b86b9ff7b82454626f0ba633404f9305b6"
            "c32701e2bca6a8d987d668defba89dc082196a922634ed88e065c669e526bb8"
            "815ee1b000000000000",
            "" /* ironwood tree */);
        std::move(callback).Run(std::move(tree_state));
      });

  std::optional<ZCashTransaction> created_transaction;
  base::MockCallback<ZCashWalletService::CreateTransactionCallback>
      create_transaction_callback;
  EXPECT_CALL(create_transaction_callback, Run(_))
      .WillOnce([&](base::expected<ZCashTransaction, std::string> tx) {
        ASSERT_TRUE(tx.has_value()) << tx.error();
        created_transaction = tx.value();
      });

  zcash_wallet_service_->CreateOrchardToTransparentTransaction(
      account_id.Clone(), "t1dYrsWYsYMMMfm85AkRcvpPw8ieiTGCpab", 10000u,
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

  // 2295048f00c2264cc068b85a6695c4b89cdd06f1e686c69c990cecaf74db0937
  EXPECT_EQ(
      "0x0600008098b684d85b16a5376f97340083973400000110270000000000001976a914d7"
      "d7056"
      "9db3474c588f952a198562a7c24affeaf88ac000003a105e927611d4c4b87b53ee54adf4"
      "ee0"
      "a406805505871e989821d839d07b420d2077e07a39f7c7d46bde478f7ace002be596b3ab"
      "2be"
      "9178dcc07636df0461d0cef940238d87e53f25a71a490e30720a855bb9865545684a3ebf"
      "8cc"
      "c82da866be712e7d733524e23a388f545a998da6adee4aae86bd4fe4915298de70c928c1"
      "3a3"
      "bb29cfb3325d266a9eaaae0a49199164c5c6088aae2ffb38f444f2a07cdeb21a8da00000"
      "000"
      "0000a9da000000000000aada000000000000abda000000000000acda000000000000adda"
      "000"
      "000000000aeda000000000000afda000000000000b0da000000000000b1da00000000000"
      "0b2"
      "da000000000000b3da000000000000b4da000000000000b5da000000000000b6da000000"
      "000"
      "000b7da000000000000b8da000000000000b9da000000000000bada000000000000bbda0"
      "000"
      "00000000bcda000000000000bdda000000000000beda000000000000bfda000000000000"
      "c0d"
      "a000000000000c1da000000000000c2da000000000000c3da000000000000c4da0000000"
      "000"
      "00c5da000000000000c6da000000000000c7da000000000000c8da000000000000c9da00"
      "000"
      "0000000cada000000000000cbda000000000000ccda000000000000cdda000000000000c"
      "eda"
      "000000000000cfda000000000000d0da000000000000d1da000000000000d2da00000000"
      "000"
      "0d3da000000000000d4da000000000000d5da000000000000d6da000000000000d7da000"
      "000"
      "000000d8da000000000000d9da000000000000dada000000000000dbda000000000000dc"
      "da0"
      "00000000000ddda000000000000deda000000000000dfda000000000000e0da000000000"
      "000"
      "e1da000000000000e2da000000000000e3da000000000000e4da000000000000e5da0000"
      "000"
      "00000e6da000000000000e7da000000000000e8da000000000000e9da000000000000ead"
      "a00"
      "0000000000ebda000000000000ecda000000000000edda000000000000eeda0000000000"
      "00e"
      "fda000000000000f0da000085c15ae920590a9bd5a5e0b48e1f6bf9d978edfbc333d8d22"
      "9f2"
      "270490436655a1abc353056782e3d389a7ca0a351e8a9cf535b8b26da8045609d7a7ee54"
      "93a"
      "05ed8bf33769ae9b3a7e78305c615da062504978b964d654003f3b3a7f27f89957401c18"
      "cc4"
      "fa559ffd9a5462419da30c77d24b59daed34f3ecfda1f5360ecb8d1927f135bb20809ad0"
      "501"
      "27d30410c087406f092c02e61cfe38e67567a3339fb74a524d945252deeaa190a0a8276f"
      "d2b"
      "83a1c4bb258b6e25932414dd9b09fcaa54bf873c587493bfb3e15bedd1f55602130d407e"
      "1c5"
      "f7318f8c0dc4106a8d69d2830b67e8ba0adffeecaf64596849c1fc1991451a21283e2e9e"
      "6c0"
      "27b9098edb980ab4de124ffa5659c1fe24fa9ecfda0df50d86678409e270887b10ad8155"
      "5ae"
      "263a1a6ff31bff66eb1057d4ce98b6ab4d6505f75f77efbb8362a74d38c3a1c8df00a36f"
      "a63"
      "732e484e6caa1d859dda52af4938c8d708a971f2d0f23b5654b76afa91f833cea0dadda1"
      "22b"
      "3d10b2b6f0c5b892b27845a3239e3aa85387c447eb1e852e030dc2d328356dcd468828fe"
      "007"
      "295960b2e6145d527829c7354f7469ab001567848a145309cae051ecc7ae5389ac22546a"
      "b34"
      "734328a2048675f86bf1a781096dac0b84b5415ca0dc72bc25cfef1b25601bd1380b4d3d"
      "51a"
      "6a8f3593aef435dcb38de125a07e89f2ff3d42664d002f952134d1acf5f237f181bbcd0b"
      "3e4"
      "da7ada2e8d907f34973d365bf232f611a5376da2f4280f08b49e5ecf24dd75843f5d1b35"
      "2fd"
      "8db5660656de980d47cbf525e8ec3881c212fe0cc2c0c2ccb5857576de827ff2f665d5f9"
      "c3a"
      "3c95f3f45306fa2bbdf82bafd8144fc966e286252efe43e29f8892da06ac6592a0d3e707"
      "579"
      "b35c8c82f579b2295463ac09b2815d1f159933a6abdc3ad54c7b5cd05fa18db8f2e0c760"
      "d6d"
      "313f13737655d382a41c238c67b6bcdc82692c91b25d6f19607b0964ade41885c79ee1c1"
      "2d8"
      "1faba510fed8cfde09352e0e3b7a016671bdc82332e06f9f9c26af2df621fd3a1a774326"
      "0d7"
      "e3f6e4e7812c69256d52b1f5ee2339b564a57bd242ceae8865ecd8f8fd2d3db7ee631c1d"
      "c29"
      "bbb911acf473ef3bb35291b47e315a5cab3f7beb853350d6486f6edea80d892aee8b389a"
      "df2"
      "878856353cb165c69cd8d531819a2ecb92e57c93a4fa4fd70d73dbd5e615f083b4d2ff61"
      "4a1"
      "9e4afa5d5c3d3e9f159e1b7091d01be2b8af3e07e67445dd3798a63ea93465c04f1b4aee"
      "eac"
      "e87a54b36abe864ae3cd0e12f347a7126bf95dd9aa6e097b11d543d453c981a6a972df4b"
      "aa1"
      "52d0858fd90f91a6a8efd7d2521c718e26f23c73a1ea601d704be18d49295bc4decb51e4"
      "093"
      "dcf0c7dbcc1bb8f45fe85c8b3fa1e112c9b28bb39a7a4250bcd6af5fb6be60cbc8aca9ee"
      "bd3"
      "85b153fdf99339bae76ca83cf9e74ee42a7f0bbd96b6853cc36806bb79d3379ac47c697f"
      "225"
      "bb0eaa46b67b56de8ffccf077ba4627163f8c33ce1efd4fbbb1039df975fba21de46fa90"
      "7c2"
      "b825dcfcde0857339af4dc4f2b8150b60b51d9743ca5110569a4f35360b5610f831d880f"
      "08b"
      "f8f0c293dc19cb9d1f7319edff169dc76d29746f62046f7ddc57a1c0816f94dc26778819"
      "9e4"
      "c9918be952e2b2306e987b2c27b21a4cdcd6358428b9891d65fa3211461b0b9c2a5521f1"
      "5ea"
      "e4192211dddcbdab566ccfef6ca2cf6d11c13de3736a19fc76b6567708fb58edb08376ef"
      "ec0"
      "7de66d08b4debccbad0108f7d70ff27b036bee22a2c14292b39ffb58bc7f31a96987be42"
      "e30"
      "8ec4d5764f62e56c180926fccc7effd1d676e9babce099709bc709f8da1be3ba64e9c0bc"
      "713"
      "d17d03fa8a2937c3ded9311a9679f9f8f96eea7cee9ed19d31e2ad2b2c1c80f02c793281"
      "413"
      "7635d54a7e6828e3494c9bb7f2fae76316ed069173aea7f99be2a90026716595916c66a0"
      "7ce"
      "0a750e2e2fa86958ecde25285e6b30b4e6f6a9251ecc4a6af7a9a814ac7da53240d775b5"
      "632"
      "5afc627157d175e4bf5d73132f827673c7cd8273ed30ff4e4066e8f62d1cedde25a7eccc"
      "7c1"
      "1882bdd41615afb85555337e1b8dc96bb057611f66fbfb46d41a392ae35550d08a30696c"
      "9c4"
      "06cf7725e0a13d28aea3cc2a3a65fef134565781b21a98ba452ce604c850d30b18778c4b"
      "b98"
      "53a7273b5c1fe4d9269973723f2f7cfa665a34d1f50b2925fdfe07e5286efb0e072a8470"
      "ee6"
      "a5829dedeea8e30102d2030b0caf1a54a4644a0eb3ad91279c4b07078efc1590f4cfeb00"
      "b3f"
      "a3dfe64e8b42dc3b40ad16985473308e6ab40c53575033e29d472cb0ee1b8488e9739857"
      "f56"
      "c9c4ff41f32a3b5f8cf02f927a3d20ffba2dd183b09cc828cdf0ea8db765ce32aaea9d9c"
      "e0f"
      "48bcfafc46b49849aa6d5308006da83a46215eec2e73cb39276c0fe3d5dcad56a4f4b7f2"
      "393"
      "dd171878b9749033075000000000000722a7a4378d3240dc294d2ec9a3347472820e070e"
      "aa6"
      "2bcf21e397a016c83b2dfd4025b6c7c0754fdefa1a21d14401e9cbb4dc82a5198781e5cd"
      "8bb"
      "531173e3fdac8bae38548b806ee2fc22826b37bc58f102ace062316eb53e0a64a57846f5"
      "837"
      "979629df601a5c1931a4b030134b19439a5e7454602e87c36d7147dfdfb655d782276afd"
      "458"
      "fd0d80038b33c67a49bbf0b0db6535f829e4520a6cf377b4dba338e07fe210b12bfd0a2e"
      "5ec"
      "0c510453e8310cb3a0666ebe36f9b81f5f7d2ab19903351282ef0d88fb4180af27cf52c7"
      "7d3"
      "58bc337374d8279bcea64899fd6437c3399b8c0a7394097f476f3c0478452c269ad29c4f"
      "530"
      "8d4373152e92a5c6cb143ba38e93bba1f9ce0653d029136e9f04f34e312d09806c620500"
      "6fe"
      "3bb92327b2d812c3d2dbb0b7a6a891c6ea0b69fe30cc984c47ce9c9e50a9aaec301799de"
      "b8d"
      "1d4f9ea452a031931347d7e491151be2cb7190e33b62bb868d0d8d30d7925667129b91d2"
      "368"
      "1f5e1e3bda8c2ff79a3da03b360b6134b3e8aefd788bbc122142931b858aa641828fb579"
      "2e4"
      "953ff376497b6195198324f035043f10584388f1569fb12e2018cfe8be9a06fedaf0361e"
      "a33"
      "b4167658cefdf542e431e9d1bb79a86174a5d5490fffdba37be5252bcc9647b2291d33b1"
      "b07"
      "14a70b2dac72d234d533185057d58bb996c688b458f06226f59462266770aa9e85015b15"
      "06c"
      "693acae66844eea6ae71ab0b9c0bbf3f162a20ca9a31da2c60dc87620152953ff57ce6fd"
      "c37"
      "4f533f07c59a487b1a7df43407e885d4fb5e1afe50a5d29c59217eced26e8e0e44ff5bd6"
      "58e"
      "45e5f1e049e7e0b469dec46b6f380c66e0fc3fc20334c9a1590a4a241c44ea63faf4fa94"
      "879"
      "28188939f928e8968b2bacb3cd2b6f7f57de2bd6355242a10f07bc13b56087a4970a5b25"
      "6ef"
      "a7c6ff8f521cef839e492a8d56f7703f37520184489807705ebc20bbba6b30f86e0f8125"
      "77e"
      "50ded33189a8e2df332f3262445bebf94da95bd4c99095306c2a6eef9580f82a51631b9c"
      "6c4"
      "73f09a92649431f991ef012539ba19294a976906f67bfc88e79899955491ed91c85da2d0"
      "ed5"
      "60f1ee51e3cbe8756c7ea7fd20fb3cb765ad04c9f5f223751483dc4ab20026a9deb5c033"
      "636"
      "fad87d94a8a1d64236262bcb690196c9564bc4d307c6f310faa2f17f54af00addfec2579"
      "d1f"
      "2d9e65b16af43d17f106516905aa113bb2f1d62dcfd1ec5d243a087775bd1f9bb9816f78"
      "f32"
      "383d9fc3e3c743a5f2ba4576cf79ea52999f59c13e9c2584ea120c64593967fecce83249"
      "2bb"
      "6a95c9286f8760d216b9b9cadb8d9b9568ec91adcfc018f153ba20cfb59ba338f960057a"
      "b5a"
      "c16a482c524a1a934284b985a864a74d83f5b8efa10feba8367c32400f8eb3cded773047"
      "0f7"
      "9e9d8dc5290fb4c0970ed10036b474e3e74fb84d89e1492267340f38cd97deab43f747db"
      "2a1"
      "a6e9d67ffeda81d87b788b28887d7ece2309712b397cc1e3c156e352eee98bd3a5e7444e"
      "c1c"
      "432fbab5f9824d1b8c644a2ae7f4e544940617546d004efbe71884d9293e1c6207fbb8dd"
      "b8f"
      "aa75e744920dfa2d1172e912e93f7077667dc32d90aef1b4e2ef5032d11a57c073a02e78"
      "7a6"
      "10466d49d7a5d5047a058f21855ba9e2c685c4e5057319c4fec3d572bca88e40a6e420d1"
      "921"
      "00125f87beef000bfd503c1639b1c9c6788d58dc6efcb0f05edbfeba794af12f1dd0ee1b"
      "4aa"
      "a8c43b9a4e4161bdb71847e8a374ff47879f7cebe7f1d59011bf138725a67c47be322647"
      "fcc"
      "b31833aef7cdd899f2ec0321a837f2ebed48ab6536cf4b8fb29b2106990fd1a4a90a4e77"
      "5b3"
      "e7fc947ed1481c11a48ce7d5b3824c9541b64ac0a661bad22b6ef7a6f10caaab99d2daef"
      "9e2"
      "bc4ccb4a781a754a68683e5316eeba77ffa73be1e883bea3dbe1b0047d553c3eb4d7ff06"
      "680"
      "7acf9a2d9dd2b6ff5be3127daecd633374e33f92946531fd62c6421be4d97ebb7705b8e4"
      "0bc"
      "fb037d3c82134f83537be923e8094c05923bf43d7c30355207acfa9af992d25b203f14bf"
      "d2d"
      "6dd80b713a4882b8fe59e5f7b26da690a065e67471db59112e634e033c2d8bbb83f03311"
      "a1d"
      "cc17695f5b703c893b10e84f6db3acf4c4005fcea8e89c9fb4fcecd4adfc9065ffb6cdf4"
      "201"
      "decb2951c90a48d5b272921a9f194ea011a867cf80ba7c3f3688af287c9bde45c48e56f1"
      "7ff"
      "6a28f6455023cac1f3c242591553b4a959d23f8efc4588d6be4dba16c15d6730459990c6"
      "b2a"
      "d55b6dfb749f784022a1a84d29dd09a8673fa965d2985aff21a279405bf7ec747440acc7"
      "ff0"
      "9f94681495535a93196810c6e8aabba75659a3775b037fd7544758014a5970a0a503d05b"
      "fc0"
      "8ba8323bf34974730098bcc377c84243b276a985a2368d18242808772b52909e7b686ee5"
      "f64"
      "28172b2844dc95192a7f63498e96bcbfd673e521d9b14cd414621960618269645f713c5b"
      "b0c"
      "f887f3cbe7919b179fd82c0ae47f410a41cf6a0aa59a6920b1103b20c5efeb979f315c07"
      "3d2"
      "298bbda62afcde3dd6493e91494a2898f690899c707c8a01a8f9fe2578f09c64a5b81a7b"
      "0c1"
      "eb4ce0488c365a2007e783d32636c45068645d22868b8dc82f414aaa63704db66bad5138"
      "8b2"
      "d3cfcba96ec841343eadabf18e5b8bb5639dd98fecbbf28635dcfa5cbf54b262ffb845dc"
      "516"
      "f4aba9e4191460836dcf073c4c90960f46c863b7b8e5007162bf44cc445bf8048ff6477f"
      "390"
      "71dabf494bf7600e3b1956d6ff685fa82d2fe6929430687839f606581095f507f9932694"
      "d26"
      "ce965afadc90ef7395f07deeee612359b2f5bffca179ba09822504738f410d71c612be31"
      "c73"
      "f1243b6c183390f8e275a00fde27f3757f892e2364e9413244b6963bdecb93cb81c29137"
      "d4e"
      "51b7ea91c3fe4f690ab354f5bc3881df58636a7f09b996c39a71aa8cb327654fdf154125"
      "5af"
      "71b75f8ce835acb450fe308865ca36ca6cb0b9b3d3cd1f9abf10691d6eea57b10ae3ad62"
      "793"
      "015377f91437aaa811c8684d8f4b759cdfb5a9d7d7d70d78316b76dead228667370d9904"
      "fac"
      "1380dbca778b47f509d3e5ec494f332ad5b451184cd44c21252ab83d2960a616741136ab"
      "64c"
      "f6f47085c32aaf507b21bf50d5210f5c2c389ee2ba72626ae07e7466827490f367b760c4"
      "58c"
      "d12eed082e7ff8b881dd4926202ae54822f4dd910985003cebc44a73c325ccfbe302e3ff"
      "8d1"
      "145e06af186d52c47388b4586ac93ba528ec3f254a7f95b4e226b1fb2d0f3c026835b261"
      "124"
      "b1fa07dab70ca214750218916af1b4037b3fb9b9b162dbad1172ae713996c21f62749d77"
      "15d"
      "18fccc606afda784b0e1e64a21626121320d62832d0e439099a30b2927e40aabae26c5dd"
      "98d"
      "cca1d8c3a19a2163c85bc974aae62c2d95c066b14d6335cad2b2b76cd92cac8ef8bc92ef"
      "5a3"
      "fce6dd491a19d47dae5319f00847540078952c7f30246f27a60de186619507be8de26029"
      "34f"
      "794b43532e58e1f5da9d0ea63d272c2a3004989ec18782766490696b40616b25611a10bf"
      "08e"
      "133a0d508af145fc54b894eb0eec205d33a7ab1c27f37a55fa6785d64853970d432d9595"
      "e93"
      "57bec00972c56f0f04d24c351a7c5eda638540001d886113b07aaef1ee95269a6359317c"
      "7a7"
      "de0b921fd8bf3b9c009113b2d3f76deef6a2914ecef70624f2c5d36768d17f55132b5b06"
      "410"
      "413841250feb5a4c9175a090a446e8d8e065e51491f46cbea4b16a395e6ff82dd4179b61"
      "f6a"
      "4cf17c4250614752f815e40d8bc32b800f6f864b3cb51f5fafdcb19574091372816a1717"
      "7b4"
      "6335303740c374ce1beccfb3b1b4a78ad4430f473d1264d7f7f81bea1c0332cd8793eef7"
      "917"
      "35a89aaf2820f475ad03f0c7e9c6560befa76593527da8e830b3f469194f9fff2f2bd109"
      "cfc"
      "e3212b4755455c27d1ba78a11f12c04bd047fcabb7efc9d679812393414582f7aa29c221"
      "2d4"
      "cff89b33481d422e2069c507564b2dea1e94f631c5385774d687d5bbf21a64f643491425"
      "bd1"
      "e8cb6058c73a4e9d050bf907da1490f1e3db72f5929bd5f90d5d503d86f8faf18c8b09a6"
      "9fb"
      "2d5640ab38b2e0100216017100f698264452752d7319c0836b354dcc6e7e9fe3f0b2e28c"
      "70b"
      "964925aa21c478511afe1dca7281ec38b73d6595776057457b089cbec59eed5ab3a6227a"
      "c17"
      "8d00d9c4918af1c19fc9cbc5076c062731fe6ce5c3d12d1bb4d56c0669d9da169cfbc256"
      "aef"
      "60434ab70551b3528469263bff8f81f6b05e281fd7157f6bbdf16f5f7e70ec530be043d6"
      "355"
      "2b04cfa3ca8cccdd4087d3af4a98aaf5005cd5713ddb393eaac3b6ec5813f8a6d74e3e2b"
      "2a4"
      "6a23f614f7ff1df495ebcea0b3f4f2c228a1571984b28564ab4d8d439fa278471aa6a721"
      "d88"
      "e6bd1414db2d9f454b1b27e5aed3c7c8e848622a5d6f61ef5e509f5b2dc06755abcc2bc4"
      "b55"
      "c085adb1bf6e61d8f3717876a0047753889cd8a03bb04ea630a3a1b1478a510405257260"
      "2cf"
      "e7d118811d0be8eb25f3368520b142acfb95be0698fdbe389aa8015f388b27bceb4837d5"
      "941"
      "4481ed3c218e9461a472e055712e2d67f4300e231b73856113cf8566cf2f90dc1ffde294"
      "24f"
      "242527d31d7fbb669cfb5a3b7e38f84001a8a41835f776c9efd274f3979ae5b7ae34ea4e"
      "c95"
      "db0a6e30e07a953c48c23ed3ecb2204e6015339e0eaa5ee53da9ec6b7d916ab691b12f01"
      "c2e"
      "518ce8cf17477fef15c6a5bff6eb1bc87043848b9d042c2662dd61a587078bd2d6bc6cc6"
      "149"
      "70b5b14d461120e5ccec765ce6890760eff48f7e62b6097649f80e96ccc5810458250230"
      "2f8"
      "deeeb0538fe8e86f2cc4c7620d683eaf79cdf7dc13986d0aa28cd40bf42db3a60dced3b2"
      "bf9"
      "cad29ef5949177db54051771717101ae5a3c97afd9c459b20c1707b62867e3ae125ba99e"
      "526"
      "dd688e9c6d341101a36c585be559da3c76901cd4fa5e05f3f4a02cd2f8c6871f920c72d6"
      "657"
      "df27a25df7d2695d45d36b2d83943f465028a98b344caf3beb3148b23c8aea019302a294"
      "152"
      "f68664e485d92824740e026da931af865a93f22ccd69cd1a511eb96ebea8792a85f1ba66"
      "087"
      "8c036286816c2559c90cf8e08424f2c6c6b1acda0f246abcd76637555470bce2f2d27a59"
      "59d"
      "9b63ecd4f0a0b2ba2e6fc1c063545c6b29d50fdf45e5e35c6f0ae367217f435e71f447d7"
      "fcf"
      "ec4408b65d319cc7587e8f847ff71440f37df8d1b3a5b103ac4ffa5d8cc47ca6c23dbd04"
      "c5b"
      "bbdcbf075ba6653d2e0f454068b9fef9a230f9364919b4f0feb1aa9b1741be7aa448eabc"
      "95d"
      "6ef1df1831c67276d5fa85f2b58ecc691263ef6235769bc13580b45bc4c043776d0563f9"
      "a19"
      "2ce1b87b784f3b8d36e66c5e6f92c7723025be88c88372ea2f5c313ec3229affcfe71ca9"
      "662"
      "f3aa9d62eae0c2189043d25947edb063d4851abf8c683ead60eae0e102a5e1f3d2fb4d10"
      "ea3"
      "c67bacab875c298d3f32362f69b0266a7777d5efdf5c7d844ae0732b8956076329350188"
      "0a4"
      "27563ec80ea3b5b56cba949bb4510e28bff798ad46232dca6eb7fe1eefec831a8c2bb6cc"
      "ea2"
      "497980e39ac951ecad379157bb36083232e0889328f51143250baa8f9675f2927bfc6790"
      "1e1"
      "83eaf74f49ad3590c8a5a3e802396ca22d9eac5659491c0eeac157754db944db6fee48a3"
      "16a"
      "59cedac34b32a3d4b14232ba80e9293a8b23eef3231929d1f53816c5bf572ad56c62645c"
      "1e2"
      "6b5efeb93c74cf11adcc669de0ec3a976d0736ff9268ed7e8df0f1f5f846c6f241331e70"
      "149"
      "43305f129fa012d584644e2d641078e37d6cc1cf77a8f63d98a5386964aa72b2c1f0a112"
      "0bd"
      "4240db4c2744e3eaeec1298485a7bddba7ff940551241b9e106fc3413ee217786cc6b104"
      "952"
      "6532b8126956350df16ebe3337bf5b2baad20844a71d3bfedd7bc618981b6e545fca8edc"
      "00e"
      "716b97a2ec61ad505ac1ad724b07752e7e29d1347550c6a03619fb7bfc9754f2410c057d"
      "fa5"
      "bca218df37f3cb47503ca81a7936836d304e295c46e4e7749fb2882d86faabeb0c7cffb1"
      "021"
      "f71c199d3898539d094fe1503559bded25c70d01cc8b0230afd69e16d7dd709495ead889"
      "61a"
      "fa25c7c07e37801b85784e68d29fec64f230cbc09f2e57b34f0bf1485e59902fcd1ac7cf"
      "926"
      "ce23f0bdd2b637fdbb7cd611cf5996788a5bbf9186b43572fa7dd2cc8eb5e5845b2e0754"
      "bc2"
      "7cdae93d04d0701789c5eb32adf33a229b061658c737eb299c82086ea35c16d757b80f32"
      "9da"
      "26b494e6f16dc7d692f6d24ed23419a36ad0b2eeefd84587e0365beb7c6101f5a90d5eec"
      "438"
      "58038102d5ad6aac827a9dd40c690cd6a6c5be4e460d307047dda203647ea5c2a8ac8d05"
      "4d4"
      "e1b83ef920040a0fd86e63eb884ea75be43f9af2325d4f217ae84614fba397a55613c659"
      "e2d"
      "278173f0d4c21268c3fdfcfa241bd521041edafecd881edd0fc6ab12234312730fd655d8"
      "048"
      "31e656c33e036381e4ec8e803e3f67d8d4021179946dc5cbccec97c3a36358e52f4e7d02"
      "013"
      "8e063cbe1e7a1ce4c77aa247bb37fec142b018a28a20863b5f92dadf635498e9bb1c5697"
      "358"
      "26221e31151e9331dbc7d573da78712e16a371fd91252634c3e2d6b72f3843a519192828"
      "acc"
      "c83f023f2b567b9b31794b6102824adaa10721e27ec92fa7fe05958b2c4b74e7a71411cc"
      "27f"
      "2c73a44f341872e30e0a7565f84a3af75b37af62252bb34e75546d76a35ecc629ccb2505"
      "017"
      "d95832bd0e55201b4f4ee4430d3ca9626ddc55abf9e60d3e60f631dea9440f7c10592aab"
      "1f8"
      "2873313de46d20d0dca1bc651e580a8d3a23f46e76a27b17859fd71f4c7a7047df8b94c9"
      "9a7"
      "13e9a8e447d0724805cbc71413ae6b06e0d2006b06c972bf9d1dc323a4b9c474df0793e4"
      "db3"
      "a89c0156610381f1fc004147e725609f7f37c7de94aa2651dbf339c13d6d10d5154f3c64"
      "ae0"
      "7904af16e2a151f5c8a0a097fb666a6a83a059fda803ee8dce6119dd5936aeb18c395f5c"
      "153"
      "f195e5c921ffffc6b8a6aa36bb91c0b4f63bd42454ca677ad530bb23228dceb30c96fff1"
      "2d4"
      "b89192419c718413521604e506a6bb49dda858ecf5660bb4e81879c1eee6c89c35b90fc5"
      "4ac"
      "23c1d70bd4a6dc7e86e016ee7113d72ce2abd8f6607bd174da2b3b0e1a932bc01f227527"
      "3c9"
      "4da9c0466b43b79a8887df00a56c2b52e7b5a936029f5a50f7a821616b8a06db87be2ead"
      "d94"
      "12c4d70ab110ef21cccccc02f4a138df59b93eb088af9e68474e45186b4515bf62dce37f"
      "708"
      "3c43879c2a6fefb0c004cdcf25c2a870f41663e53ee8a893441a4aae80921a1016d18ed2"
      "432"
      "b5dedafbada6910ce0205d068c285ac61bcbf0037270072da3e6b3329a5f9e04c8c323ba"
      "a25"
      "a87be46a5ec80af46150a23c01b980fc2ffa1f16142dbbf7eb4ca9d6138d7c747a281e6f"
      "5bd"
      "491fbd64e358c826b1479198dc387bd2f573013b6ed83a6a7d8358f7a77c9e7bdd78cf04"
      "51f"
      "069bfd77ff209c0e10960078f7a8bc12b2c1474930f511ab756705249e0661cd16ab3bc4"
      "717"
      "cf6d33c730d2859614f6b911e4bf20bd8fd4927d59dd1a5244288162e9907d29e3a6557d"
      "829"
      "252c5602d081748cb3d358ccd77feb694fb4bc51ac5ca777dba0d5a593577f0609ca129f"
      "863"
      "d6cbc4d0057fb62763beb3bcefb8e7c74315528f1888655e2d3299691510dd32c2b6bca2"
      "36f"
      "62b79cc67a5559b191f68454c821e3fc5734ad450449abd27665ab8fe5c591304a11c99b"
      "2a8"
      "fa6f1931516bc97aaf0d4f30a968920fa5220b1a872b4c1f3ae105a79d873dba08a1fefb"
      "3cc"
      "a8dd9d4355d7a5ff9c3f6689e3174f7a54bac8a204b7114009706ad6a7b896485acf96a2"
      "040"
      "a43763c0cd6916ce331934ef9964570b6bbab6d080375a5df8960fa8b3ecba1a36cabe90"
      "0e6"
      "290014ce948fae8f1e884ffa081da87731e05b94b1b32c5adbfb688f281804985aeb4d7f"
      "113"
      "c275ce10f9c6021cc5b964c32502a234779f4660513c2366009a7f403accb10546ab3197"
      "23f"
      "0d0ff96f8de3d3b853d80c4d86ba97abf184592dc3bed5ba227aa72eece9e67932d7ec1f"
      "4ee"
      "9386967e1ee28bb4573d17d563303a980d9a2a7ae5cee77b1e1e0a241983be639fb1c640"
      "cb8"
      "2418af56853266e00e233f704ec84a52e677e217baad38ca12ccaa7d8f1cf5bcf8bb99b5"
      "d22"
      "7edaf3fc370109b8268585d046d8d6ddc6b47de5401ef6c5af7558231aeb2ce920e1a2df"
      "f72"
      "57e0ab76312f52ba352792563198d074da9dc17835b1488d4bd6f1088e16b0983d4097b7"
      "4a5"
      "5104843775a7d2ea0cf0a3ada5b707234f3328c337d4f553fda1eca3bd0c50a4dc2fb5ff"
      "6b2"
      "27bef388593e10c443cf9817006b73250294102bcbbeb61fb741f017fe21755964971985"
      "22d"
      "a7584c406de6be6e9ef173c5410b14a74a961de1f3109ffc546464f9269f3fed0b47d042"
      "d36"
      "cc9d02eb66681b04b08e385b2b7c5e90f33ae3e8d000aa1ee74b1c8f0d93975a83108df5"
      "e43"
      "85f282f6706968f4ae5ce7cc9144ced47a6a8d0155fbd4d1a58a207bd39537fe6d93b1fe"
      "506"
      "a56857903cca217bfc27fd0b0aa27c5cecac3326eceac2704cc3f6d913edbfe61f4cbd9c"
      "ec5"
      "8b0d2f71d8f2ad42f12949146483b59b5c1113a213a7937646344cfce972a0691df5fc0c"
      "7f9"
      "28963209f78729638a743c52e5980f29311cdff4852a6fdcd588524daf0d5b41d8de544e"
      "963"
      "da05def5ac271aa14fde5841991927c01d69f7d11ee3bd37b745a2bac8d1accaa266dd9a"
      "081"
      "b1a404c1124d2c5ea6cccacf41c69da28158142b12d832face96f37427eb79debd43f1de"
      "5f0"
      "0379e8f7de5a0505d5eba831eab2850c6b46b39fa3838f348f0509737d5b45b020b6c3dc"
      "2fc"
      "130ac044a4a7841aae98adfb12d48a0e1c5b33d1d9598f0b02cda93f88847b405562f9ee"
      "e55"
      "c3b357b2ba224a5ea483380f57cf38149099299caa407153693f05c1ad926ac1c0f13028"
      "7d8"
      "aa413a5468ec071c94850b0eb9028bd916d5e59db0d90c8f42283030ae2e4547a01dc415"
      "dc4"
      "013c1dc7d1b9b3dcfb7c304c5b1e328a225edae9c27c5ecda3bec135e97a1f70aeb99167"
      "0c8"
      "9fd41dfea5028f9609c6618c05a43e281230f3e2da7914c59f062ae73fed3b1c5d2f3134"
      "6e0"
      "aa72b583e62608090d692c7dd56b3950a12312677b0d880e31a37a8a87b3b02157ab6ace"
      "9e1"
      "c389f8a8074f7e743c9a4ae3a72be7e40f9124155f2a5013b83617b0f16e40d4e3c5df16"
      "36c"
      "c0d5f5afdf6e80095e3dd870e977fe8859e2a46490f9e3e60e78189460e3002952eb2502"
      "8d4"
      "34b9082211d7f4f84879236abebecb36a0b3ee63683a7996ba2db078e806f72aedaf8123"
      "424"
      "6cf050f409062a0411f34fe9ef5fc671ac77a2c4931f06653871306df37c768bcdbd3166"
      "9ca"
      "c0aff322195c953b9b1abc483c785bd17d59da91aa18ca4e3830673a52b61e17f379c824"
      "1d5"
      "50be7bbc4904fdc7f4fe31d87f0873335b0a58653e6c6fa59abe38729708b38c9693d881"
      "a42"
      "c518174ea969cd1654a899098d9908382d77fe36c4110c56a20558f91c89722309b1f13b"
      "175"
      "4421ada7d55e80e546537bbda1000011f762c1c0db55c42047eefb7d13ee961e9e0201e8"
      "d9d"
      "5a767d4039bb4987eeebaafe6d7e1dd93d131ce95d65c115fb1031d1551906bdaee111e2"
      "4ee"
      "a1a1cf87ec1551166339983e4d359c050d6ff92f9f95d5476b5621482a21115d7418c196"
      "efa"
      "d2f4c021229b60010c4e341bdcec9a13a245875dbce0b00b2a3c8483abf453fd03608932"
      "76e"
      "3025fe89859172d26b0f91b2e80b3312e062cfcaf3848382f5c9d4f22c92975d18f7bbfb"
      "197"
      "16086a87451bc477a7006e031c420947c03a1e53ebd1aad0206eb0e883c87c4e2ebd5f9b"
      "e56"
      "9d910f74db8ee406efdec47066b59dd02e6900fe42041d0b5cd1ea5c76c56b4da33b1cbb"
      "91f"
      "aeaedc4f20fecac08c110018fd2d64dce11a0f1487438668f26ac52af8bdc992284958b2"
      "8ef"
      "5922020aa8e79273ff23ada52c022c2b42597d324cf864bbcbf54dfd02a7552b799fa64f"
      "296"
      "1312389253da434b46281645f8427bd3a1f53fd8788221e1e3de7d09f34b9205a8620f45"
      "97c"
      "b05c35074227097b72de19e408572e22950ebbb2c334080dd3958ccef0bf89d67dfeaca7"
      "1ef"
      "5ee3502c9f8c0819a79379a1257a8a31833d7f0cc737020e87d7669f5926b09a31600ddd"
      "e28"
      "82b91baa97ea7156b11cab9883c6529b1383abcd87c1b08fba52e05d05ecfef02d4f7704"
      "725"
      "ad7bbb2f57ff5e898a5adf2a0086ab8d3b759d105575bcbba72a00f56bee876b67ff9cdc"
      "fa7"
      "996ca9a4e042c33674c9afee4b42d395a1357c63aaaa4d56ab63cca8319d7492402a2b86"
      "be9"
      "552886cd10441066d87b540a751cf7d637062e54f9cd53dbfdb9a0deaf1ef5f8f60c340e"
      "734"
      "ea28d76a8d2e5d96c552cac3a156643f8a2d1230aa24a27612ebd510f6995b3e1fed4cd7"
      "0e4"
      "f2de46cfaffdeffff3bbcd21d0b983446aae4156753f342784d673086748a156bb3e79bb"
      "dea"
      "b879ab230309349cce779565bfa5d1c2231dee8f60dd22e774fb0ae5f5c84600ac72337e"
      "5a8"
      "b0552551615463617a25490fe668625b4a656f290916e02a99967c807230b1ed14012d94"
      "0ce"
      "4022d2a7ab20eef9fe138edbdfaba532fdfb988962d369a1100496267a9b6e074cc10272"
      "294"
      "16cc6a439860a93bf27849a0ed0fdc446b8d25162083bb2ba37c75d8eb5380b412446a3a"
      "aa9"
      "b86b0a6a8cbe47fed87aecf81995bb808f91cc7af01fe7bf95e7c1a1d4fe2b7c9bd99bc7"
      "bde"
      "0ba6ec4664b2e76500fb97a3e79356b22a6ac31cd42505d43d22adc5b28616b29307678d"
      "408"
      "0f5a7a7000b91fa46d66da67282f35d1b0300311212210e725190fd03ee41e39b71c7b5b"
      "bab"
      "11ff3910c0822437ceb318c238430a1c58789d264370b1c104a663e9b67056177b821e1d"
      "31d"
      "2074054bb085eb35c6c0d23416542188c4a8ef6fffcebf1e567dc9002416adc999662937"
      "b16"
      "1a01b4f9cbe35938da06ffb0a411e71d5f4e9b465eaa36c45185892516616f076896345f"
      "fcc"
      "53ec71bbe18916c90905f563669a570d2188c30855449500baa250d54c5e21724830319e"
      "e0f"
      "18d80fa3b17059016fa63db29993f1b2e60f075f32dbfa57f68de8880c73959fe6118647"
      "5e2"
      "e74a4e25c7f1097f662651fd367d23602d3c6ccbc71eb34af296245ca138d0aaedc8e4c3"
      "592"
      "24adba43c4b2ad96aa2a1d3e13b39648ec3de311fa32ba717e6a46296628c32d75c4148a"
      "f7e"
      "f6ecb911811081095af15cc704c01d160a746310674e1adf5a1bd7076f20abf087668261"
      "1fb"
      "4034e9140b9cdaf32038ea763d9df26a4f403ef41852e0683c80f78fbfce291e323d093e"
      "82d"
      "a820c3e823646447dd35ac06034b262a965dc315820af15af0911d43122c6072be641f87"
      "6f5"
      "bb251b6d83ed7494bb7836cf59e5be66f722920e57e99af48672dd954042b93d71f863e5"
      "c9a"
      "8ce2eb7df6e88aff0770165d8723befd573e0c62a49297387e184654fad24cbdfd19b39a"
      "025"
      "e2246a678d29563f39dcb1873857d36c81aa0551950ea08925dc3a1a1425854e1666bcf4"
      "b45"
      "c3a4304ab645a6630b8a2766da7821d584b09f5aebae6d96ddd110b82d998a2ad4b3d322"
      "da4"
      "ae336ce7a97429f9eca8fb7dca9b07feca4887c0ce875d6da3ff3fa6631592ac92997f73"
      "d28"
      "f89c59dba6bc6bedc76d1a6fe8e2c22783afb4685e392e51389bf726c14e2f42302bde58"
      "b58"
      "9c83520915fc63bf67fef4694bc48bdbf468161abe9e36288b31f1264055e3d218e231de"
      "ccb"
      "609392230187d6db04b69e90309528732689553e0e0a409485fa04cef9f5fa216098a346"
      "fd8"
      "657141f5bdfc8602c324da79764ed12785086486f3c13c648d34a74f9b75aed3c01db68e"
      "000"
      "7372898d141259ec21d9a4150eb52573256cca2bbd34e6af1a65bb7b78803fc2d19311a5"
      "540"
      "7d47246b36d21e009062e3808d689b479c8049b596327f54b6d0d83223b911762d72c5a6"
      "022"
      "81c31eef61badec9bf2101309781b515a3cf4136370ac17ec3c3f720e5a1c2a8dee8db1c"
      "0e7"
      "b959684332acfdc296fe587b3e734dad260e5833064da14980958ddd062228d5f7d28ccd"
      "b4d"
      "396b302b606d4edbb2544123cedf5ce1ad462c5787cf31a209fe4191b49a1f49285aeeb4"
      "d68"
      "3001b60d599e09582ee964e3ca8ee38c0554d62e3cd68ef62cfa86dc3a56a4944f9f4172"
      "351"
      "328c1e93dc200447d689fcb355ed7fcab3ad360386902f74031bab91352200b9e344b350"
      "88d"
      "48d98a512a618227955ff1ec604900e21262d829450102aec1cea80ddc34f0828f1e0b38"
      "561"
      "5df92852a97fb7cfb0bb45f53b1ab00f83e1fba59a41edf2599f69f4535e6e26b8c702ef"
      "6f5"
      "ea2e964a1c6304d6cb59fcc3471ba5db1b4c03f17e85cf2b697a6dbf9a2db33ca0c03732"
      "979"
      "28fa572b919044660ff0204d6043c1bb41b6e9f4b52e75944608b5b7c1f2e7006b4654e2"
      "f89"
      "d7804d5911a181e073d570dfb923510ca477b6d38744e81962b3206ac22d05183f5f305d"
      "a63"
      "dd75120546f88af56224564c8666e4e3044f22d540d6ca684ed9a2ede9637cd341bb5a84"
      "5f3"
      "540e0d251f96f7f0214bda42d73873b5e2b4c30b3ffa4f6e5b2b283fe388d491e1620331"
      "74f"
      "777ba31d54efd84772fd98e1c779f9d23dba1bf3911de26b6fc29d5075c7cdee4dac9bba"
      "b0e"
      "0f34a43eaabc764a05b77d24c2b87c36b921b32e770679ba99289033a1599ee31f338637"
      "8b4"
      "09c81faaeb718dee4c93b7d9573fa21f3c3fcf9c4f1320c7ae31b22eb4f06924c74bc864"
      "d9f"
      "ee349c63b01788b4f3761803cdb11a8464ec78bc0b9e0c3244f36bc02028de0505f15a44"
      "0d9"
      "13de3ff96f79220add424125c3d8ca3e9b8a338a88caa25084bc8d205404616f8196d6ae"
      "4cd"
      "2b8cdccdcae0e736f834c35b78aa22b528fb132747fba1671ebf6e955372af0311d797b4"
      "a4f"
      "a2f215d7c6b082dcea5fd80c7bb86713567e1748ad1626de6f2e8ff621179fc97fed2bd2"
      "cb5"
      "58f10c559c09a41ecf0c499e7f74b033f189e77402bc60c0e07c3c97948047bf47665723"
      "cdc"
      "6ae1da6b5b6e60dcfa0c9567694ad6844567dbd66f8fa7a6c92c6bf5d6e995b25fce0d6f"
      "86a"
      "98bcb1279003c55c0de3b36648f244cd400e6a3bc809566290e8edb4efa94c808ac57082"
      "254"
      "f69074fda62fdd2a14ee56d91dfdcd1c78e28942521309e38155adc4b98b8ab54242d7bf"
      "6a8"
      "621cb5f7a6ff0679b063bc94e95c3a922e09e2b4085e7eac3a0547e0154fc82ed2102e6a"
      "43f"
      "b8a61a27aee71d96cd687636caf236a0e9dcf163b9375f6a46e7626a425aa4e02d48bfdf"
      "0c4"
      "911c3b43726cc492b732ea2e9535ce8fa2d7365cd3634a58044a21992019ea380e38a111"
      "66e"
      "ac23d385fea2a97b5dd15f468554c6622a7a937a753b8271a614adf3580eb0c4df2c0d19"
      "084"
      "436ea8c1ce66a9fd5195b46fb65f5f1905a26e8d4dc953d5272ca81ab4e30db090e29fd9"
      "2ab"
      "d58b736666f2e2d6cc91301f3ad4cb9bdac1f54b3dc07a7f742d56fb52b51a167da0b791"
      "488"
      "47fe4b438642bb8ee2e72bc4dd07096fb07b82dc40e88521bdc3e4412b2ac1b23b8f229d"
      "f35"
      "ae9e045017a58b37e9e8c0c57bbeaccbeb68d086b02143696c877029ed6c64734c709cbd"
      "a28"
      "455cf66635cf767744cbe61409992b4a807cd349e8b6f7223362d25f3fd074533b6a1a64"
      "b7d"
      "6a61c09651e071674f72b8422ea095d73cd483d82ef85805d32b4f55faf07564decbcda3"
      "5f2"
      "cf24c0d76263442891faf5d3dfe5b5534bf7aea4c2d314ad05ee2bce967d5425f8053ce0"
      "bb8"
      "b8c79e657d5b455cac0d3bc0ae4392d29298fbe3a93aa1fdab48e0b08dd3813e2c60dde7"
      "8f4"
      "ac9807ede6ea1500dfc136fdb44a1f4982079b5d5ac32fc2265648254ef6e0a8b0b8c518"
      "62a"
      "4eec29929be5ee71ce85bb13f98852df90400",
      ToHex(captured_data));
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

TEST_F(ZCashWalletServiceUnitTest,
       OnCompleteTransactionTaskDone_InvalidTransaction) {
  // Create an invalid transaction where inputs != outputs + fee
  ZCashTransaction invalid_tx;
  invalid_tx.init_v5_part();
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
      zcash_wallet_service_->complete_transaction_tasks_v5_.insert(
          std::make_unique<ZCashCompleteTransactionTaskV5>(
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
      zcash_wallet_service_->OnCompleteTransactionTaskV5Done(
          task_ptr, account_id(), invalid_tx, callback.Get(), result),
      "");
}

TEST_F(ZCashWalletServiceUnitTest,
       OnCompleteTransactionTaskDone_InvalidResultTransaction) {
  ZCashTransaction valid_tx;
  valid_tx.init_v5_part();
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
      zcash_wallet_service_->complete_transaction_tasks_v5_.insert(
          std::make_unique<ZCashCompleteTransactionTaskV5>(
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
      zcash_wallet_service_->OnCompleteTransactionTaskV5Done(
          task_ptr, account_id(), result_invalid_tx, callback.Get(), result),
      "");
}

}  // namespace brave_wallet
