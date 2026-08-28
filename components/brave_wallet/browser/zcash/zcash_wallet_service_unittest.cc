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
#include "base/test/run_until.h"
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
#include "components/grit/brave_components_strings.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/l10n/l10n_util.h"

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

constexpr uint32_t kDefaultCommitmentSeed = 1;

#if !BUILDFLAG(IS_ANDROID)
constexpr char kLuxuryReformMnemonic[] =
    "luxury reform inner vanish palace addict alter control casino bean "
    "metal two banner fatigue type sponsor sun plunge hotel shadow host "
    "health cabbage tomato";

constexpr size_t kShieldFundsIronwoodRandomSeed = 989841u;
constexpr size_t kShieldFundsNetworkErrorRandomSeed = 70972u;
constexpr size_t kSendShieldedFundsIronwoodRandomSeed = 4437897u;
constexpr size_t kOrchardToIronwoodRandomSeed = 7657542u;
constexpr size_t kShieldAllFundsIronwoodRandomSeed = 4437897u;
constexpr size_t kIronwoodToTransparentRandomSeed = 23423u;
constexpr size_t kUnshieldFundsRandomSeed = 55595u;
#endif

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

  void MaybeInitAutoSyncManagers() {
    zcash_wallet_service_->MaybeInitAutoSyncManagers();
  }

  void NotifyWalletLocked() { zcash_wallet_service_->Locked(); }

  void MockGetLatestBlockForAutoSync() {
    ON_CALL(zcash_rpc(), GetLatestBlock(_, _))
        .WillByDefault([](const std::string& chain_id,
                          ZCashRpc::GetLatestBlockCallback callback) {
          EXPECT_EQ(chain_id, mojom::kZCashMainnet);
          std::move(callback).Run(
              zcash::mojom::BlockID::New(1000u, std::vector<uint8_t>({})));
        });
  }

  void WaitForChainTipStatus(const mojom::AccountIdPtr& account_id) {
    base::test::TestFuture<mojom::ZCashChainTipStatusPtr,
                           const std::optional<std::string>&>
        chain_tip_future;
    zcash_wallet_service_->GetChainTipStatus(
        account_id.Clone(),
        chain_tip_future.GetCallback<mojom::ZCashChainTipStatusPtr,
                                     const std::optional<std::string>&>());
    ASSERT_TRUE(chain_tip_future.Get<0>());
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
        "56ezc5c7tjwlrnnl79tjtrxmqd42c5mpyz7g",
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
        "56ezc5c7tjwlrnnl79tjtrxmqd42c5mpyz7g",
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

TEST_F(ZCashWalletServiceUnitTest,
       IronwoodTransactions_DisabledWhenFeatureOff) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndEnableFeatureWithParameters(
      features::kBraveWalletZCashFeature,
      {{"zcash_shielded_transactions_enabled", "true"},
       {"zcash_ironwood_enabled", "false"}});

  auto account_info = keyring_service()->GetZCashAccountInfo(account_id());
  ASSERT_TRUE(account_info);
  ASSERT_TRUE(account_info->orchard_internal_address);

  static constexpr char kOrchardUnifiedAddress[] =
      "u1ay3aawlldjrmxqnjf5medr5ma6p3acnet464ht8lmwplq5cd3"
      "ugytcmlf96rrmtgwldc75x94qn4n8pgen36y8tywlq6yjk7lkf3"
      "fa8wzjrav8z2xpxqnrnmjxh8tmz6jhfh425t7f3vy6p4pd3zmqa"
      "yq49efl2c4xydc0gszg660q9p";
  static constexpr char kTransparentAddress[] =
      "t1WTZNzKCvU2GeM1ZWRyF7EvhMHhr7magiT";

  {
    base::test::TestFuture<base::expected<ZCashTransaction, std::string>>
        future;
    zcash_wallet_service_->CreateTransparentToIronwoodTransaction(
        account_id().Clone(), kOrchardUnifiedAddress, 10000, std::nullopt,
        future.GetCallback());
    EXPECT_FALSE(future.Take().has_value());
  }

  {
    base::test::TestFuture<base::expected<ZCashTransaction, std::string>>
        future;
    zcash_wallet_service_->CreateIronwoodToIronwoodTransaction(
        account_id().Clone(), kOrchardUnifiedAddress, 10000, std::nullopt,
        future.GetCallback());
    EXPECT_FALSE(future.Take().has_value());
  }

  {
    base::test::TestFuture<const std::optional<std::string>&,
                           const std::optional<std::string>&>
        future;
    zcash_wallet_service_->ShieldAllFunds(account_id().Clone(),
                                          future.GetCallback());
    const auto [tx_id, error] = future.Take();
    EXPECT_FALSE(tx_id.has_value());
    ASSERT_TRUE(error.has_value());
    EXPECT_EQ(*error,
              l10n_util::GetStringUTF8(IDS_WALLET_METHOD_NOT_SUPPORTED_ERROR));
  }

  {
    base::test::TestFuture<base::expected<ZCashTransaction, std::string>>
        future;
    zcash_wallet_service_->CreateOrchardToIronwoodTransaction(
        account_id().Clone(), kOrchardUnifiedAddress, 10000, std::nullopt,
        future.GetCallback());
    EXPECT_FALSE(future.Take().has_value());
  }

  {
    base::test::TestFuture<base::expected<ZCashTransaction, std::string>>
        future;
    zcash_wallet_service_->CreateIronwoodToTransparentTransaction(
        account_id().Clone(), kTransparentAddress, 10000, future.GetCallback());
    EXPECT_FALSE(future.Take().has_value());
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

TEST_F(ZCashWalletServiceUnitTest,
       ResetSyncStateToIronwoodActivation_NoOpBeforeActivation) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndEnableFeatureWithParameters(
      features::kBraveWalletZCashFeature,
      {{"zcash_shielded_transactions_enabled", "true"}});

  auto account_id_1 = account_id();
  base::test::TestFuture<
      base::expected<OrchardStorage::Result, OrchardStorage::Error>>
      register_account_future;
  zcash_wallet_service_->sync_state()
      .AsyncCall(&OrchardSyncState::RegisterAccount)
      .WithArgs(account_id_1.Clone(), 100u)
      .Then(register_account_future.GetCallback());
  ASSERT_TRUE(register_account_future.Take().has_value());

  base::test::TestFuture<const std::optional<std::string>&> reset_sync_future;
  zcash_wallet_service_->ResetSyncStateToIronwoodActivation(
      account_id_1.Clone(), reset_sync_future.GetCallback());
  EXPECT_EQ(std::nullopt, reset_sync_future.Take());
  EXPECT_TRUE(
      keyring_service()->GetZCashIronwoodSyncStateReset(account_id_1.Clone()));
}

TEST_F(ZCashWalletServiceUnitTest,
       ResetSyncStateToIronwoodActivation_RewindsWhenPastActivation) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndEnableFeatureWithParameters(
      features::kBraveWalletZCashFeature,
      {{"zcash_shielded_transactions_enabled", "true"},
       {"zcash_ironwood_enabled", "true"}});

  constexpr uint32_t kActivation = kIronwoodActivationHeightMainnet;
  constexpr uint32_t kRewindHeight = kActivation - 1;
  constexpr uint32_t kPastActivation = kActivation + 57u;
  constexpr char kRewindHash[] =
      "0xe99a69a926bd0d078d39445fdf237c08ddfd3f7a59f7dc266aef610000000000";
  constexpr char kRpcRewindHash[] =
      "000000000061ef6a26dcf7597a3ffddd087c23df5f44398d070dbd26a9699ae9";

  auto account_id_1 = account_id();
  base::test::TestFuture<
      base::expected<OrchardStorage::Result, OrchardStorage::Error>>
      register_account_future;
  zcash_wallet_service_->sync_state()
      .AsyncCall(&OrchardSyncState::RegisterAccount)
      .WithArgs(account_id_1.Clone(), 100u)
      .Then(register_account_future.GetCallback());
  ASSERT_TRUE(register_account_future.Take().has_value());

  {
    std::vector<OrchardCommitment> orchard_commitments;
    orchard_commitments.push_back(
        CreateCommitment(CreateMockCommitmentValue(0, kDefaultCommitmentSeed),
                         false, kRewindHeight));
    auto result = CreateResultForTesting(OrchardTreeState(),
                                         std::move(orchard_commitments),
                                         kRewindHeight, kRewindHash);
    base::test::TestFuture<
        base::expected<OrchardStorage::Result, OrchardStorage::Error>>
        apply_scan_results_future;
    zcash_wallet_service_->sync_state()
        .AsyncCall(&OrchardSyncState::ApplyScanResults)
        .WithArgs(account_id_1.Clone(), std::move(result))
        .Then(apply_scan_results_future.GetCallback());
    ASSERT_TRUE(apply_scan_results_future.Take().has_value());
  }

  {
    OrchardTreeState tree_state;
    tree_state.block_height = kRewindHeight;
    tree_state.tree_size = 1;
    std::vector<OrchardCommitment> orchard_commitments;
    orchard_commitments.push_back(
        CreateCommitment(CreateMockCommitmentValue(1, kDefaultCommitmentSeed),
                         false, kPastActivation));
    auto result = CreateResultForTesting(std::move(tree_state),
                                         std::move(orchard_commitments),
                                         kPastActivation, "past_hash");
    base::test::TestFuture<
        base::expected<OrchardStorage::Result, OrchardStorage::Error>>
        apply_scan_results_future;
    zcash_wallet_service_->sync_state()
        .AsyncCall(&OrchardSyncState::ApplyScanResults)
        .WithArgs(account_id_1.Clone(), std::move(result))
        .Then(apply_scan_results_future.GetCallback());
    ASSERT_TRUE(apply_scan_results_future.Take().has_value());
  }

  EXPECT_CALL(zcash_rpc(), GetTreeState(_, _, _))
      .WillOnce([&](const std::string& chain_id,
                    zcash::mojom::BlockIDPtr block_id,
                    ZCashRpc::GetTreeStateCallback callback) {
        EXPECT_EQ(chain_id, mojom::kZCashMainnet);
        EXPECT_EQ(block_id->height, kRewindHeight);
        auto tree_state = zcash::mojom::TreeState::New(
            "main", kRewindHeight, kRpcRewindHash, 123, "", "", "");
        std::move(callback).Run(std::move(tree_state));
      });

  base::test::TestFuture<const std::optional<std::string>&> reset_sync_future;
  zcash_wallet_service_->ResetSyncStateToIronwoodActivation(
      account_id_1.Clone(), reset_sync_future.GetCallback());
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
  EXPECT_EQ(kRewindHeight, account_meta.value()->latest_scanned_block_id);
  EXPECT_EQ(kRewindHash, account_meta.value()->latest_scanned_block_hash);
  EXPECT_EQ(100u, account_meta.value()->account_birthday);
  EXPECT_TRUE(
      keyring_service()->GetZCashIronwoodSyncStateReset(account_id_1.Clone()));
}

TEST_F(ZCashWalletServiceUnitTest,
       StartShieldSync_IronwoodMigration_RewindsWhenNeeded) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndEnableFeatureWithParameters(
      features::kBraveWalletZCashFeature,
      {{"zcash_shielded_transactions_enabled", "true"},
       {"zcash_ironwood_enabled", "true"}});

  constexpr uint32_t kActivation = kIronwoodActivationHeightMainnet;
  constexpr uint32_t kRewindHeight = kActivation - 1;
  constexpr uint32_t kPastActivation = kActivation + 57u;
  constexpr char kRewindHash[] =
      "0xe99a69a926bd0d078d39445fdf237c08ddfd3f7a59f7dc266aef610000000000";
  constexpr char kRpcRewindHash[] =
      "000000000061ef6a26dcf7597a3ffddd087c23df5f44398d070dbd26a9699ae9";

  auto account_id_1 = account_id();
  keyring_service()->SetZCashAccountBirthday(
      account_id_1.Clone(),
      mojom::ZCashAccountShieldBirthday::New(100u, "hash"));
  EXPECT_FALSE(
      keyring_service()->GetZCashIronwoodSyncStateReset(account_id_1.Clone()));

  base::test::TestFuture<
      base::expected<OrchardStorage::Result, OrchardStorage::Error>>
      register_account_future;
  zcash_wallet_service_->sync_state()
      .AsyncCall(&OrchardSyncState::RegisterAccount)
      .WithArgs(account_id_1.Clone(), 100u)
      .Then(register_account_future.GetCallback());
  ASSERT_TRUE(register_account_future.Take().has_value());

  {
    std::vector<OrchardCommitment> orchard_commitments;
    orchard_commitments.push_back(
        CreateCommitment(CreateMockCommitmentValue(0, kDefaultCommitmentSeed),
                         false, kRewindHeight));
    auto result = CreateResultForTesting(OrchardTreeState(),
                                         std::move(orchard_commitments),
                                         kRewindHeight, kRewindHash);
    base::test::TestFuture<
        base::expected<OrchardStorage::Result, OrchardStorage::Error>>
        apply_scan_results_future;
    zcash_wallet_service_->sync_state()
        .AsyncCall(&OrchardSyncState::ApplyScanResults)
        .WithArgs(account_id_1.Clone(), std::move(result))
        .Then(apply_scan_results_future.GetCallback());
    ASSERT_TRUE(apply_scan_results_future.Take().has_value());
  }

  {
    OrchardTreeState tree_state;
    tree_state.block_height = kRewindHeight;
    tree_state.tree_size = 1;
    std::vector<OrchardCommitment> orchard_commitments;
    orchard_commitments.push_back(
        CreateCommitment(CreateMockCommitmentValue(1, kDefaultCommitmentSeed),
                         false, kPastActivation));
    auto result = CreateResultForTesting(std::move(tree_state),
                                         std::move(orchard_commitments),
                                         kPastActivation, "past_hash");
    base::test::TestFuture<
        base::expected<OrchardStorage::Result, OrchardStorage::Error>>
        apply_scan_results_future;
    zcash_wallet_service_->sync_state()
        .AsyncCall(&OrchardSyncState::ApplyScanResults)
        .WithArgs(account_id_1.Clone(), std::move(result))
        .Then(apply_scan_results_future.GetCallback());
    ASSERT_TRUE(apply_scan_results_future.Take().has_value());
  }

  bool saw_rewind_tree_state = false;
  ON_CALL(zcash_rpc(), GetTreeState(_, _, _))
      .WillByDefault([&](const std::string& chain_id,
                         zcash::mojom::BlockIDPtr block_id,
                         ZCashRpc::GetTreeStateCallback callback) {
        EXPECT_EQ(chain_id, mojom::kZCashMainnet);
        if (block_id->height == kRewindHeight) {
          saw_rewind_tree_state = true;
          auto tree_state = zcash::mojom::TreeState::New(
              "main", kRewindHeight, kRpcRewindHash, 123, "", "", "");
          std::move(callback).Run(std::move(tree_state));
          return;
        }
        auto tree_state = zcash::mojom::TreeState::New(
            "main", block_id->height, kRpcRewindHash, 123, "", "", "");
        std::move(callback).Run(std::move(tree_state));
      });
  ON_CALL(zcash_rpc(), GetLatestBlock(_, _))
      .WillByDefault(
          [kPastActivation](const std::string& chain_id,
                            ZCashRpc::GetLatestBlockCallback callback) {
            std::move(callback).Run(zcash::mojom::BlockID::New(
                kPastActivation, std::vector<uint8_t>()));
          });

  base::test::TestFuture<const std::optional<std::string>&> sync_future;
  zcash_wallet_service_->StartShieldSync(account_id_1.Clone(), 0,
                                         sync_future.GetCallback());
  EXPECT_EQ(std::nullopt, sync_future.Take());
  EXPECT_TRUE(saw_rewind_tree_state);

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
  EXPECT_EQ(kRewindHeight, account_meta.value()->latest_scanned_block_id);
  EXPECT_TRUE(
      keyring_service()->GetZCashIronwoodSyncStateReset(account_id_1.Clone()));
}

TEST_F(ZCashWalletServiceUnitTest,
       StartShieldSync_IronwoodMigration_SkipsWhenFlagSet) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndEnableFeatureWithParameters(
      features::kBraveWalletZCashFeature,
      {{"zcash_shielded_transactions_enabled", "true"},
       {"zcash_ironwood_enabled", "true"}});

  constexpr uint32_t kActivation = kIronwoodActivationHeightMainnet;
  constexpr uint32_t kPastActivation = kActivation + 57u;

  auto account_id_1 = account_id();
  keyring_service()->SetZCashAccountBirthday(
      account_id_1.Clone(),
      mojom::ZCashAccountShieldBirthday::New(100u, "hash"));
  ASSERT_TRUE(keyring_service()->SetZCashIronwoodSyncStateReset(
      account_id_1.Clone(), true));

  base::test::TestFuture<
      base::expected<OrchardStorage::Result, OrchardStorage::Error>>
      register_account_future;
  zcash_wallet_service_->sync_state()
      .AsyncCall(&OrchardSyncState::RegisterAccount)
      .WithArgs(account_id_1.Clone(), 100u)
      .Then(register_account_future.GetCallback());
  ASSERT_TRUE(register_account_future.Take().has_value());

  {
    OrchardTreeState tree_state;
    tree_state.block_height = kActivation;
    tree_state.tree_size = 1;
    std::vector<OrchardCommitment> orchard_commitments;
    orchard_commitments.push_back(
        CreateCommitment(CreateMockCommitmentValue(0, kDefaultCommitmentSeed),
                         false, kPastActivation));
    auto result = CreateResultForTesting(std::move(tree_state),
                                         std::move(orchard_commitments),
                                         kPastActivation, "past_hash");
    base::test::TestFuture<
        base::expected<OrchardStorage::Result, OrchardStorage::Error>>
        apply_scan_results_future;
    zcash_wallet_service_->sync_state()
        .AsyncCall(&OrchardSyncState::ApplyScanResults)
        .WithArgs(account_id_1.Clone(), std::move(result))
        .Then(apply_scan_results_future.GetCallback());
    ASSERT_TRUE(apply_scan_results_future.Take().has_value());
  }

  bool saw_ironwood_rewind_tree_state = false;
  ON_CALL(zcash_rpc(), GetTreeState(_, _, _))
      .WillByDefault([&](const std::string& chain_id,
                         zcash::mojom::BlockIDPtr block_id,
                         ZCashRpc::GetTreeStateCallback callback) {
        if (block_id->height == kIronwoodActivationHeightMainnet - 1) {
          saw_ironwood_rewind_tree_state = true;
        }
        auto tree_state = zcash::mojom::TreeState::New("main", block_id->height,
                                                       "00", 123, "", "", "");
        std::move(callback).Run(std::move(tree_state));
      });
  ON_CALL(zcash_rpc(), GetLatestBlock(_, _))
      .WillByDefault(
          [kPastActivation](const std::string& chain_id,
                            ZCashRpc::GetLatestBlockCallback callback) {
            std::move(callback).Run(zcash::mojom::BlockID::New(
                kPastActivation, std::vector<uint8_t>()));
          });

  base::test::TestFuture<const std::optional<std::string>&> sync_future;
  zcash_wallet_service_->StartShieldSync(account_id_1.Clone(), 0,
                                         sync_future.GetCallback());
  EXPECT_EQ(std::nullopt, sync_future.Take());
  EXPECT_FALSE(saw_ironwood_rewind_tree_state);

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
  EXPECT_EQ(kPastActivation, account_meta.value()->latest_scanned_block_id);
}

TEST_F(ZCashWalletServiceUnitTest,
       StartShieldSync_IronwoodMigration_NoOpSetsFlag) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndEnableFeatureWithParameters(
      features::kBraveWalletZCashFeature,
      {{"zcash_shielded_transactions_enabled", "true"},
       {"zcash_ironwood_enabled", "true"}});

  auto account_id_1 = account_id();
  keyring_service()->SetZCashAccountBirthday(
      account_id_1.Clone(),
      mojom::ZCashAccountShieldBirthday::New(100u, "hash"));
  EXPECT_FALSE(
      keyring_service()->GetZCashIronwoodSyncStateReset(account_id_1.Clone()));

  base::test::TestFuture<
      base::expected<OrchardStorage::Result, OrchardStorage::Error>>
      register_account_future;
  zcash_wallet_service_->sync_state()
      .AsyncCall(&OrchardSyncState::RegisterAccount)
      .WithArgs(account_id_1.Clone(), 100u)
      .Then(register_account_future.GetCallback());
  ASSERT_TRUE(register_account_future.Take().has_value());

  bool saw_ironwood_rewind_tree_state = false;
  ON_CALL(zcash_rpc(), GetTreeState(_, _, _))
      .WillByDefault([&](const std::string& chain_id,
                         zcash::mojom::BlockIDPtr block_id,
                         ZCashRpc::GetTreeStateCallback callback) {
        if (block_id->height == kIronwoodActivationHeightMainnet - 1) {
          saw_ironwood_rewind_tree_state = true;
        }
        auto tree_state = zcash::mojom::TreeState::New("main", block_id->height,
                                                       "00", 123, "", "", "");
        std::move(callback).Run(std::move(tree_state));
      });
  ON_CALL(zcash_rpc(), GetLatestBlock(_, _))
      .WillByDefault([](const std::string& chain_id,
                        ZCashRpc::GetLatestBlockCallback callback) {
        std::move(callback).Run(
            zcash::mojom::BlockID::New(200u, std::vector<uint8_t>()));
      });

  base::test::TestFuture<const std::optional<std::string>&> sync_future;
  zcash_wallet_service_->StartShieldSync(account_id_1.Clone(), 0,
                                         sync_future.GetCallback());
  EXPECT_EQ(std::nullopt, sync_future.Take());
  EXPECT_FALSE(saw_ironwood_rewind_tree_state);

  EXPECT_TRUE(
      keyring_service()->GetZCashIronwoodSyncStateReset(account_id_1.Clone()));
}

TEST_F(ZCashWalletServiceUnitTest,
       StartShieldSync_IronwoodMigration_FailsImmediately) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndEnableFeatureWithParameters(
      features::kBraveWalletZCashFeature,
      {{"zcash_shielded_transactions_enabled", "true"},
       {"zcash_ironwood_enabled", "true"}});

  constexpr uint32_t kActivation = kIronwoodActivationHeightMainnet;
  constexpr uint32_t kRewindHeight = kActivation - 1;
  constexpr uint32_t kPastActivation = kActivation + 57u;
  constexpr char kRewindHash[] =
      "0xe99a69a926bd0d078d39445fdf237c08ddfd3f7a59f7dc266aef610000000000";

  auto account_id_1 = account_id();
  keyring_service()->SetZCashAccountBirthday(
      account_id_1.Clone(),
      mojom::ZCashAccountShieldBirthday::New(100u, "hash"));

  base::test::TestFuture<
      base::expected<OrchardStorage::Result, OrchardStorage::Error>>
      register_account_future;
  zcash_wallet_service_->sync_state()
      .AsyncCall(&OrchardSyncState::RegisterAccount)
      .WithArgs(account_id_1.Clone(), 100u)
      .Then(register_account_future.GetCallback());
  ASSERT_TRUE(register_account_future.Take().has_value());

  {
    std::vector<OrchardCommitment> orchard_commitments;
    orchard_commitments.push_back(
        CreateCommitment(CreateMockCommitmentValue(0, kDefaultCommitmentSeed),
                         false, kRewindHeight));
    auto result = CreateResultForTesting(OrchardTreeState(),
                                         std::move(orchard_commitments),
                                         kRewindHeight, kRewindHash);
    base::test::TestFuture<
        base::expected<OrchardStorage::Result, OrchardStorage::Error>>
        apply_scan_results_future;
    zcash_wallet_service_->sync_state()
        .AsyncCall(&OrchardSyncState::ApplyScanResults)
        .WithArgs(account_id_1.Clone(), std::move(result))
        .Then(apply_scan_results_future.GetCallback());
    ASSERT_TRUE(apply_scan_results_future.Take().has_value());
  }

  {
    OrchardTreeState tree_state;
    tree_state.block_height = kRewindHeight;
    tree_state.tree_size = 1;
    std::vector<OrchardCommitment> orchard_commitments;
    orchard_commitments.push_back(
        CreateCommitment(CreateMockCommitmentValue(1, kDefaultCommitmentSeed),
                         false, kPastActivation));
    auto result = CreateResultForTesting(std::move(tree_state),
                                         std::move(orchard_commitments),
                                         kPastActivation, "past_hash");
    base::test::TestFuture<
        base::expected<OrchardStorage::Result, OrchardStorage::Error>>
        apply_scan_results_future;
    zcash_wallet_service_->sync_state()
        .AsyncCall(&OrchardSyncState::ApplyScanResults)
        .WithArgs(account_id_1.Clone(), std::move(result))
        .Then(apply_scan_results_future.GetCallback());
    ASSERT_TRUE(apply_scan_results_future.Take().has_value());
  }

  EXPECT_CALL(zcash_rpc(), GetTreeState(_, _, _))
      .WillOnce([kRewindHeight](const std::string& chain_id,
                                zcash::mojom::BlockIDPtr block_id,
                                ZCashRpc::GetTreeStateCallback callback) {
        EXPECT_EQ(block_id->height, kRewindHeight);
        std::move(callback).Run(base::unexpected("rpc failed"));
      });

  base::test::TestFuture<const std::optional<std::string>&> sync_future;
  zcash_wallet_service_->StartShieldSync(account_id_1.Clone(), 0,
                                         sync_future.GetCallback());
  EXPECT_EQ("Failed to retrieve tree state", sync_future.Take());

  EXPECT_FALSE(
      keyring_service()->GetZCashIronwoodSyncStateReset(account_id_1.Clone()));

  base::test::TestFuture<bool, const std::optional<std::string>&>
      in_progress_future;
  zcash_wallet_service_->IsSyncInProgress(
      account_id_1.Clone(),
      in_progress_future
          .GetCallback<bool, const std::optional<std::string>&>());
  EXPECT_FALSE(in_progress_future.Get<0>());
}

TEST_F(ZCashWalletServiceUnitTest,
       StartShieldSync_IronwoodMigration_RejectsDuplicatePreflight) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndEnableFeatureWithParameters(
      features::kBraveWalletZCashFeature,
      {{"zcash_shielded_transactions_enabled", "true"},
       {"zcash_ironwood_enabled", "true"}});

  constexpr uint32_t kActivation = kIronwoodActivationHeightMainnet;
  constexpr uint32_t kRewindHeight = kActivation - 1;
  constexpr uint32_t kPastActivation = kActivation + 57u;
  constexpr char kRewindHash[] =
      "0xe99a69a926bd0d078d39445fdf237c08ddfd3f7a59f7dc266aef610000000000";
  constexpr char kRpcRewindHash[] =
      "000000000061ef6a26dcf7597a3ffddd087c23df5f44398d070dbd26a9699ae9";

  auto account_id_1 = account_id();
  keyring_service()->SetZCashAccountBirthday(
      account_id_1.Clone(),
      mojom::ZCashAccountShieldBirthday::New(100u, "hash"));

  base::test::TestFuture<
      base::expected<OrchardStorage::Result, OrchardStorage::Error>>
      register_account_future;
  zcash_wallet_service_->sync_state()
      .AsyncCall(&OrchardSyncState::RegisterAccount)
      .WithArgs(account_id_1.Clone(), 100u)
      .Then(register_account_future.GetCallback());
  ASSERT_TRUE(register_account_future.Take().has_value());

  {
    std::vector<OrchardCommitment> orchard_commitments;
    orchard_commitments.push_back(
        CreateCommitment(CreateMockCommitmentValue(0, kDefaultCommitmentSeed),
                         false, kRewindHeight));
    auto result = CreateResultForTesting(OrchardTreeState(),
                                         std::move(orchard_commitments),
                                         kRewindHeight, kRewindHash);
    base::test::TestFuture<
        base::expected<OrchardStorage::Result, OrchardStorage::Error>>
        apply_scan_results_future;
    zcash_wallet_service_->sync_state()
        .AsyncCall(&OrchardSyncState::ApplyScanResults)
        .WithArgs(account_id_1.Clone(), std::move(result))
        .Then(apply_scan_results_future.GetCallback());
    ASSERT_TRUE(apply_scan_results_future.Take().has_value());
  }

  {
    OrchardTreeState tree_state;
    tree_state.block_height = kRewindHeight;
    tree_state.tree_size = 1;
    std::vector<OrchardCommitment> orchard_commitments;
    orchard_commitments.push_back(
        CreateCommitment(CreateMockCommitmentValue(1, kDefaultCommitmentSeed),
                         false, kPastActivation));
    auto result = CreateResultForTesting(std::move(tree_state),
                                         std::move(orchard_commitments),
                                         kPastActivation, "past_hash");
    base::test::TestFuture<
        base::expected<OrchardStorage::Result, OrchardStorage::Error>>
        apply_scan_results_future;
    zcash_wallet_service_->sync_state()
        .AsyncCall(&OrchardSyncState::ApplyScanResults)
        .WithArgs(account_id_1.Clone(), std::move(result))
        .Then(apply_scan_results_future.GetCallback());
    ASSERT_TRUE(apply_scan_results_future.Take().has_value());
  }

  ZCashRpc::GetTreeStateCallback held_tree_state_callback;
  ZCashRpc::GetTreeStateCallback retry_tree_state_callback;
  EXPECT_CALL(zcash_rpc(), GetTreeState(_, _, _))
      .WillOnce([&](const std::string& chain_id,
                    zcash::mojom::BlockIDPtr block_id,
                    ZCashRpc::GetTreeStateCallback callback) {
        EXPECT_EQ(block_id->height, kRewindHeight);
        held_tree_state_callback = std::move(callback);
      })
      .WillOnce([&](const std::string& chain_id,
                    zcash::mojom::BlockIDPtr block_id,
                    ZCashRpc::GetTreeStateCallback callback) {
        EXPECT_EQ(block_id->height, kRewindHeight);
        retry_tree_state_callback = std::move(callback);
      });
  ON_CALL(zcash_rpc(), GetLatestBlock(_, _))
      .WillByDefault(
          [kPastActivation](const std::string& chain_id,
                            ZCashRpc::GetLatestBlockCallback callback) {
            std::move(callback).Run(zcash::mojom::BlockID::New(
                kPastActivation, std::vector<uint8_t>()));
          });

  base::test::TestFuture<const std::optional<std::string>&> first_sync_future;
  zcash_wallet_service_->StartShieldSync(account_id_1.Clone(), 0,
                                         first_sync_future.GetCallback());
  ASSERT_TRUE(base::test::RunUntil(
      [&] { return !held_tree_state_callback.is_null(); }));
  ASSERT_TRUE(held_tree_state_callback);

  base::test::TestFuture<const std::optional<std::string>&> second_sync_future;
  zcash_wallet_service_->StartShieldSync(account_id_1.Clone(), 0,
                                         second_sync_future.GetCallback());
  EXPECT_EQ("Already in sync", second_sync_future.Take());

  base::test::TestFuture<bool, const std::optional<std::string>&>
      in_progress_future;
  zcash_wallet_service_->IsSyncInProgress(
      account_id_1.Clone(),
      in_progress_future
          .GetCallback<bool, const std::optional<std::string>&>());
  EXPECT_TRUE(in_progress_future.Get<0>());

  NotifyWalletLocked();
  EXPECT_EQ("Wallet locked", first_sync_future.Take());
  EXPECT_FALSE(
      keyring_service()->GetZCashIronwoodSyncStateReset(account_id_1.Clone()));

  base::test::TestFuture<const std::optional<std::string>&> retry_sync_future;
  zcash_wallet_service_->StartShieldSync(account_id_1.Clone(), 0,
                                         retry_sync_future.GetCallback());
  ASSERT_TRUE(base::test::RunUntil(
      [&] { return !retry_tree_state_callback.is_null(); }));

  auto tree_state = zcash::mojom::TreeState::New(
      "main", kRewindHeight, kRpcRewindHash, 123, "", "", "");
  std::move(held_tree_state_callback).Run(std::move(tree_state));

  base::test::TestFuture<bool, const std::optional<std::string>&>
      retry_in_progress_future;
  zcash_wallet_service_->IsSyncInProgress(
      account_id_1.Clone(),
      retry_in_progress_future
          .GetCallback<bool, const std::optional<std::string>&>());
  EXPECT_TRUE(retry_in_progress_future.Get<0>());

  tree_state = zcash::mojom::TreeState::New("main", kRewindHeight,
                                            kRpcRewindHash, 123, "", "", "");
  std::move(retry_tree_state_callback).Run(std::move(tree_state));
  EXPECT_EQ(std::nullopt, retry_sync_future.Take());
}

TEST_F(ZCashWalletServiceUnitTest,
       StartShieldSync_IronwoodMigration_RejectsSyncBeforeRewindFinishes) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndEnableFeatureWithParameters(
      features::kBraveWalletZCashFeature,
      {{"zcash_shielded_transactions_enabled", "true"},
       {"zcash_ironwood_enabled", "true"}});

  constexpr uint32_t kActivation = kIronwoodActivationHeightMainnet;
  constexpr uint32_t kRewindHeight = kActivation - 1;
  constexpr uint32_t kPastActivation = kActivation + 57u;
  constexpr char kRewindHash[] =
      "0xe99a69a926bd0d078d39445fdf237c08ddfd3f7a59f7dc266aef610000000000";
  constexpr char kRpcRewindHash[] =
      "000000000061ef6a26dcf7597a3ffddd087c23df5f44398d070dbd26a9699ae9";

  auto account_id_1 = account_id();
  keyring_service()->SetZCashAccountBirthday(
      account_id_1.Clone(),
      mojom::ZCashAccountShieldBirthday::New(100u, "hash"));

  // Seed orchard state scanned past Ironwood activation so StartShieldSync
  // must rewind before it can start.
  base::test::TestFuture<
      base::expected<OrchardStorage::Result, OrchardStorage::Error>>
      register_account_future;
  zcash_wallet_service_->sync_state()
      .AsyncCall(&OrchardSyncState::RegisterAccount)
      .WithArgs(account_id_1.Clone(), 100u)
      .Then(register_account_future.GetCallback());
  ASSERT_TRUE(register_account_future.Take().has_value());

  {
    std::vector<OrchardCommitment> orchard_commitments;
    orchard_commitments.push_back(
        CreateCommitment(CreateMockCommitmentValue(0, kDefaultCommitmentSeed),
                         false, kRewindHeight));
    auto result = CreateResultForTesting(OrchardTreeState(),
                                         std::move(orchard_commitments),
                                         kRewindHeight, kRewindHash);
    base::test::TestFuture<
        base::expected<OrchardStorage::Result, OrchardStorage::Error>>
        apply_scan_results_future;
    zcash_wallet_service_->sync_state()
        .AsyncCall(&OrchardSyncState::ApplyScanResults)
        .WithArgs(account_id_1.Clone(), std::move(result))
        .Then(apply_scan_results_future.GetCallback());
    ASSERT_TRUE(apply_scan_results_future.Take().has_value());
  }

  {
    OrchardTreeState tree_state;
    tree_state.block_height = kRewindHeight;
    tree_state.tree_size = 1;
    std::vector<OrchardCommitment> orchard_commitments;
    orchard_commitments.push_back(
        CreateCommitment(CreateMockCommitmentValue(1, kDefaultCommitmentSeed),
                         false, kPastActivation));
    auto result = CreateResultForTesting(std::move(tree_state),
                                         std::move(orchard_commitments),
                                         kPastActivation, "past_hash");
    base::test::TestFuture<
        base::expected<OrchardStorage::Result, OrchardStorage::Error>>
        apply_scan_results_future;
    zcash_wallet_service_->sync_state()
        .AsyncCall(&OrchardSyncState::ApplyScanResults)
        .WithArgs(account_id_1.Clone(), std::move(result))
        .Then(apply_scan_results_future.GetCallback());
    ASSERT_TRUE(apply_scan_results_future.Take().has_value());
  }

  // Hold the rewind-height tree state so the Ironwood rewind stays in-flight.
  ZCashRpc::GetTreeStateCallback held_tree_state_callback;
  ON_CALL(zcash_rpc(), GetTreeState(_, _, _))
      .WillByDefault([&](const std::string& chain_id,
                         zcash::mojom::BlockIDPtr block_id,
                         ZCashRpc::GetTreeStateCallback callback) {
        if (block_id->height == kRewindHeight &&
            held_tree_state_callback.is_null()) {
          held_tree_state_callback = std::move(callback);
          return;
        }
        auto tree_state = zcash::mojom::TreeState::New(
            "main", block_id->height, kRpcRewindHash, 123, "", "", "");
        std::move(callback).Run(std::move(tree_state));
      });
  ON_CALL(zcash_rpc(), GetLatestBlock(_, _))
      .WillByDefault(
          [kPastActivation](const std::string& chain_id,
                            ZCashRpc::GetLatestBlockCallback callback) {
            std::move(callback).Run(zcash::mojom::BlockID::New(
                kPastActivation, std::vector<uint8_t>()));
          });

  // First StartShieldSync begins rewind and parks its callback until rewind
  // finishes.
  base::test::TestFuture<const std::optional<std::string>&> first_sync_future;
  zcash_wallet_service_->StartShieldSync(account_id_1.Clone(), 0,
                                         first_sync_future.GetCallback());
  ASSERT_TRUE(base::test::RunUntil(
      [&] { return !held_tree_state_callback.is_null(); }));

  // A second StartShieldSync while rewind is still pending must be rejected.
  base::test::TestFuture<const std::optional<std::string>&> second_sync_future;
  zcash_wallet_service_->StartShieldSync(account_id_1.Clone(), 0,
                                         second_sync_future.GetCallback());
  EXPECT_EQ("Already in sync", second_sync_future.Take());

  base::test::TestFuture<bool, const std::optional<std::string>&>
      in_progress_future;
  zcash_wallet_service_->IsSyncInProgress(
      account_id_1.Clone(),
      in_progress_future
          .GetCallback<bool, const std::optional<std::string>&>());
  EXPECT_TRUE(in_progress_future.Get<0>());
  EXPECT_FALSE(first_sync_future.IsReady());

  // Completing rewind unblocks the original StartShieldSync.
  auto tree_state = zcash::mojom::TreeState::New(
      "main", kRewindHeight, kRpcRewindHash, 123, "", "", "");
  std::move(held_tree_state_callback).Run(std::move(tree_state));
  EXPECT_EQ(std::nullopt, first_sync_future.Take());
  EXPECT_TRUE(
      keyring_service()->GetZCashIronwoodSyncStateReset(account_id_1.Clone()));
}

TEST_F(ZCashWalletServiceUnitTest,
       MaybeInitAutoSyncManagers_DoesNotPerformIronwoodMigration) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndEnableFeatureWithParameters(
      features::kBraveWalletZCashFeature,
      {{"zcash_shielded_transactions_enabled", "true"},
       {"zcash_ironwood_enabled", "true"}});

  constexpr uint32_t kActivation = kIronwoodActivationHeightMainnet;
  constexpr uint32_t kPastActivation = kActivation + 57u;

  auto account_id_1 = account_id();
  keyring_service()->SetZCashAccountBirthday(
      account_id_1.Clone(),
      mojom::ZCashAccountShieldBirthday::New(100u, "hash"));
  EXPECT_FALSE(
      keyring_service()->GetZCashIronwoodSyncStateReset(account_id_1.Clone()));

  base::test::TestFuture<
      base::expected<OrchardStorage::Result, OrchardStorage::Error>>
      register_account_future;
  zcash_wallet_service_->sync_state()
      .AsyncCall(&OrchardSyncState::RegisterAccount)
      .WithArgs(account_id_1.Clone(), 100u)
      .Then(register_account_future.GetCallback());
  ASSERT_TRUE(register_account_future.Take().has_value());

  {
    OrchardTreeState tree_state;
    tree_state.block_height = kActivation;
    tree_state.tree_size = 1;
    std::vector<OrchardCommitment> orchard_commitments;
    orchard_commitments.push_back(
        CreateCommitment(CreateMockCommitmentValue(0, kDefaultCommitmentSeed),
                         false, kPastActivation));
    auto result = CreateResultForTesting(std::move(tree_state),
                                         std::move(orchard_commitments),
                                         kPastActivation, "past_hash");
    base::test::TestFuture<
        base::expected<OrchardStorage::Result, OrchardStorage::Error>>
        apply_scan_results_future;
    zcash_wallet_service_->sync_state()
        .AsyncCall(&OrchardSyncState::ApplyScanResults)
        .WithArgs(account_id_1.Clone(), std::move(result))
        .Then(apply_scan_results_future.GetCallback());
    ASSERT_TRUE(apply_scan_results_future.Take().has_value());
  }

  EXPECT_CALL(zcash_rpc(), GetTreeState(_, _, _)).Times(0);
  MockGetLatestBlockForAutoSync();
  MaybeInitAutoSyncManagers();

  EXPECT_TRUE(auto_sync_managers().contains(account_id_1));
  EXPECT_FALSE(
      keyring_service()->GetZCashIronwoodSyncStateReset(account_id_1.Clone()));
  WaitForChainTipStatus(account_id_1);
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
  OrchardBundleManager::OverrideRandomSeedForTesting(
      kShieldFundsNetworkErrorRandomSeed);
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

#if (BUILDFLAG(IS_WIN) && defined(ARCH_CPU_X86)) || BUILDFLAG(IS_IOS)
#define MAYBE_ShieldAllFunds DISABLED_ShieldAllFunds
#define MAYBE_ShieldFunds DISABLED_ShieldFunds
#define MAYBE_SendShieldedFunds DISABLED_SendShieldedFunds
#define MAYBE_OrchardToIronwood DISABLED_OrchardToIronwood
#define MAYBE_UnshieldFunds DISABLED_UnshieldFunds
#define MAYBE_IronwoodUnshieldFunds DISABLED_IronwoodUnshieldFunds
#else
#define MAYBE_ShieldAllFunds ShieldAllFunds
#define MAYBE_ShieldFunds ShieldFunds
#define MAYBE_SendShieldedFunds SendShieldedFunds
#define MAYBE_OrchardToIronwood OrchardToIronwood
#define MAYBE_UnshieldFunds UnshieldFunds
#define MAYBE_IronwoodUnshieldFunds IronwoodUnshieldFunds
#endif

// https://3xpl.com/zcash/transaction/184c1965c913a56898bad3f35e460dc39687fc6db1a0ab406fe18866530ac191
TEST_F(ZCashWalletServiceUnitTest, MAYBE_ShieldFunds) {
  // Creating authorized orchard bundle may take a time
  base::test::ScopedRunLoopTimeout specific_timeout(FROM_HERE,
                                                    base::Minutes(1));
  base::test::ScopedFeatureList feature_list;
  feature_list.InitWithFeaturesAndParameters(
      {{features::kBraveWalletZCashFeature,
        {{"zcash_shielded_transactions_enabled", "true"},
         {"zcash_ironwood_enabled", "true"}}},
#if BUILDFLAG(IS_IOS)
       {features::kBraveWalletWebUIFeature, {}}
#endif
      },
      {}  // disabled features
  );

  keyring_service()->Reset();
  keyring_service()->RestoreWallet(kLuxuryReformMnemonic, kTestWalletPassword,
                                   false, base::DoNothing());
  auto account =
      GetAccountUtils().EnsureAccount(mojom::KeyringId::kZCashMainnet, 0);
  auto account_id = account->account_id.Clone();
  keyring_service()->UpdateNextUnusedAddressForZCashAccount(account_id, 5, 5);

  ON_CALL(zcash_rpc(), GetLightdInfo(_, _))
      .WillByDefault([](const std::string& chain_id,
                        ZCashRpc::GetLightdInfoCallback callback) {
        std::move(callback).Run(zcash::mojom::LightdInfo::New("37a5165b"));
      });

  ON_CALL(zcash_rpc(), IsKnownAddress(_, _, _, _, _))
      .WillByDefault([](const std::string& chain_id, const std::string& addr,
                        uint64_t block_start, uint64_t block_end,
                        ZCashRpc::IsKnownAddressCallback callback) {
        std::move(callback).Run(false);
      });

  ON_CALL(zcash_rpc(), GetLatestBlock(_, _))
      .WillByDefault([&](const std::string& chain_id,
                         ZCashRpc::GetLatestBlockCallback callback) {
        auto response = zcash::mojom::BlockID::New(
            3458969u,
            *PrefixedHexStringToBytes("0x0f641d09e9026dab5c0d549a77bea74b"
                                      "b0c5df1d043f5efdf16c18a0000000000"));
        std::move(callback).Run(std::move(response));
      });

  ON_CALL(zcash_rpc(), GetTreeState(_, _, _))
      .WillByDefault([&](const std::string& chain_id,
                         zcash::mojom::BlockIDPtr block_id,
                         ZCashRpc::GetTreeStateCallback callback) {
        auto tree_state = zcash::mojom::TreeState::New(
            "main" /* network */, 3458969 /* height */,
            "00000000008ac116dfeff543d0f15d0c4ba7be779a540d5cab6d02e9091d640f"
            /* hash */,
            1787570257 /* time */,
            "0105df4fda879fb1fa597295cb8f25fc29b0539a5acd6970fa0b10e65ea598ec"
            "5f001f0000013b7ad6313d13383e042b50aaae0f5a850c228f6b1f3fb61ccebb"
            "ca029e6c860d0001731e2883dd924192dd3994294861232671f6d7d07b0f42e6"
            "967b6557caaa946e0001a34c997133a6cee45be906ffcb1dc7dee5fee6523ce5"
            "601d94ffad96a01b631e015cf0b94bb3dec1deb91f5d88b20b318de5ea5da22a"
            "928560d3fb51835bb4f435017dbaf53f9acd6da7132919ee0e61c8e006eb14d9"
            "72e7ee9012750a136a38fc610000000103d1187b8d91f9b17135eaf11ea773ac"
            "8f2226a7037227c9c24546d63776c65101ab65114517839c9df32720a9f91f5b"
            "e6604f91ffd2193b3a9d16c4ed2a81df73000000000190eb9e2bc82b8b980aaa"
            "63ba44db65328553ba840c38c5011a465efd8b233b2200013e2598f743726006"
            "b8de42476ed56a55a75629a7b82e430c4e7c101a69e9b02a011619f99023a69b"
            "b647eab2d2aa1a73c3673c74bb033c3c4930eacda19e6fd93b0000000160272b"
            "134ca494b602137d89e528c751c06d3ef4a87a45f33af343c15060cc1e000000"
            "0000"
            /* sapling tree */,
            "01abcde966d111d377ad9eb242e6c6ca5911eaa56a1a16632d453a826c600285"
            "1601407922c14a38a770a53db8d62baffdc4e017fede76ab7b6f22c275f70c89"
            "d32b1f00000126742b16a71a1b4d2f24af2d972359290ef4e87a4fe13a3ddf9a"
            "1b935c85323d01e4ecf077121eac8303022c330d2e3b338f27f17ef9164b07b9"
            "5a69ef986203240000017e76c4f5dd9a3b1809cbe4e2195831b82c586120de80"
            "2413432243ab43132a0b0000017abdf6c56ab611ba2bb8ab806c3169955bb8e1"
            "83411efde5e464fae51b8fd8050000013dda000e1a152209563e47d7dedd8ece"
            "9caf5f808a9b0bd841709ad46eccc81501e0828ebbc5dcb2be8011aa76b77c3b"
            "7e4a630c587f42fb350b0b4f570344f3270001cb129832ff83e5ab567f159e7d"
            "0c58a04c11074a9d29d2f21b908fd11dddc41800000000000000013f3ddc746e"
            "57791a2cf8900143b86b9ff7b82454626f0ba633404f9305b6c32701e2bca6a8"
            "d987d668defba89dc082196a922634ed88e065c669e526bb8815ee1b00000000"
            "0000"
            /* orchard tree */,
            "0136214254c951b32c48db1e38fb2c1357cc4bb47f16c2d16adb3c895b132ade"
            "2d01188c3c624c1eabc4fb2468fbfe077a9d84e57d95d977bdeb7d6820ff8f27"
            "593a1f000183007c8ddd936c2d7a1f4652712af54be7ed191e824518d44b9592"
            "59ba084c120000000109f19344bb1d1c8f1859b84d26ac57615a159c0d18ce08"
            "d20b404939cdeea33d000000013522c475ef9b7f5927937246982aa472585315"
            "3693cf12f856856557d9b7ec0e0000000001f1294ce1ba5cd0625efe133008fb"
            "b34b61f8fee5fd47e84ea1e3aa6faf72d039018e1e8c93493395a08a8d30ba34"
            "9a6c858ca13c4bb34b4f214f99afe7e420452f00000000000000000000000000"
            "0000"
            /* ironwood tree */);
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
      .WillByDefault(
          [&](const std::string& chain_id, const std::string& address,
              ZCashRpc::GetUtxoListCallback callback) {
            std::vector<zcash::mojom::ZCashUtxoPtr> utxos;
            if (address == "t1KgcafpY8SQ9prSaJaLWDNjMK3jZRh462t") {
              auto utxo = zcash::mojom::ZCashUtxo::New(
                  "t1KgcafpY8SQ9prSaJaLWDNjMK3jZRh462t" /* address */,
                  *PrefixedHexStringToBytes(
                      "0x6a201fbe4f47633371559f550fad9f08f092aef8ec5fd71c7faa39"
                      "931ebf2a91") /* tx id */,
                  0u /* index */,
                  *PrefixedHexStringToBytes("0x76a91413dc27df4fecc40e45a619efa0"
                                            "c4d81a64fdebf188ac") /*script*/,
                  100000u /* amount */, 3451109u /* block */);
              utxos.push_back(std::move(utxo));
            }
            if (address == "t1S8CizjZXtyEXbusiYhp96zkLVrQjLm1QU") {
              auto utxo = zcash::mojom::ZCashUtxo::New(
                  "t1S8CizjZXtyEXbusiYhp96zkLVrQjLm1QU" /* address */,
                  *PrefixedHexStringToBytes(
                      "0x8168e775c95d7ddc05deddc37ff728d2cb0809029ba4d442e1426b"
                      "29e6495399") /* tx id */,
                  0u /* index */,
                  *PrefixedHexStringToBytes("0x76a9145a83cfa45e4899770f22d52d97"
                                            "b7adfdcb10325488ac") /*script*/,
                  10000u /* amount */, 2757588u /* block */);
              utxos.push_back(std::move(utxo));
            }
            if (address == "t1Xgp6z3tPYgsAtC6aA87ntNeTTGtKgUJvB") {
              auto utxo = zcash::mojom::ZCashUtxo::New(
                  "t1Xgp6z3tPYgsAtC6aA87ntNeTTGtKgUJvB" /* address */,
                  *PrefixedHexStringToBytes(
                      "0x3ed75ebecf3fb2f0d71d1e6d0ae0ef712d55d1858236c78b763a2f"
                      "a790055646") /* tx id */,
                  0u /* index */,
                  *PrefixedHexStringToBytes("0x76a914978748a668db7815805585c56f"
                                            "981a316e220b0b88ac") /*script*/,
                  10000u /* amount */, 3447696u /* block */);
              utxos.push_back(std::move(utxo));
            }
            if (address == "t1a3UxbMRzhhNRHDmCYGKvzfjGBdmyiRJZW") {
              auto utxo = zcash::mojom::ZCashUtxo::New(
                  "t1a3UxbMRzhhNRHDmCYGKvzfjGBdmyiRJZW" /* address */,
                  *PrefixedHexStringToBytes(
                      "0xf61a9b0c003399d1a6051007944485818642ff4d13cd02438f9d38"
                      "3400aec256") /* tx id */,
                  0u /* index */,
                  *PrefixedHexStringToBytes("0x76a914b1604a14ec8a526556b324f6ad"
                                            "c0ca1d0d2bd2d388ac") /*script*/,
                  10000u /* amount */, 3455353u /* block */);
              utxos.push_back(std::move(utxo));
            }
            if (address == "t1dYrsWYsYMMMfm85AkRcvpPw8ieiTGCpab") {
              auto utxo = zcash::mojom::ZCashUtxo::New(
                  "t1dYrsWYsYMMMfm85AkRcvpPw8ieiTGCpab" /* address */,
                  *PrefixedHexStringToBytes(
                      "0x3709db74afec0c999cc686e6f106dd9cb8c495665ab868c04c26c2"
                      "008f049522") /* tx id */,
                  0u /* index */,
                  *PrefixedHexStringToBytes("0x76a914d7d70569db3474c588f952a198"
                                            "562a7c24affeaf88ac") /*script*/,
                  10000u /* amount */, 3446640u /* block */);
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
      "u1gvf5rrg0e7kzcvwue2fpdrdus6xc8zn024p4te927sq438wrst99evna6chp5xxkv4rh8"
      "mvh03z07exwk9yvp9vduzuvzrll3up49g0s",
      10000, std::nullopt, create_transaction_callback.Get());
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

  OrchardBundleManager::OverrideRandomSeedForTesting(
      kShieldFundsIronwoodRandomSeed);

  zcash_wallet_service_->SignAndPostTransaction(
      account_id.Clone(), std::move(created_transaction.value()),
      sign_callback.Get());
  task_environment_.RunUntilIdle();

  EXPECT_EQ(
      "0x0600008098b684d85b16a53799c73400adc73400048168e775c95d7ddc05dedd"
      "c37ff728d2cb0809029ba4d442e1426b29e6495399000000006a47304402206c"
      "2bd5174e23d751b531b94db29bef0998090b1d08fe49e67c8bb03b2c559f4302"
      "200edfa8d2e4fd93b85a1d4b259bf5c007d0a7f955b252e6b9761988f2f5c07d"
      "9c01210373846e01a350819b65451715a0f0c669b108e43164583ae8a011ade4"
      "d3cfbdfdffffffff3ed75ebecf3fb2f0d71d1e6d0ae0ef712d55d1858236c78b"
      "763a2fa790055646000000006a47304402202ffb7c62efbcf7d7ccdf209df194"
      "8cd0e5956fd259ccfa84be3e7fee9f5a4766022059fac1508a94847965b915e1"
      "a1109ce460c510ebfab0eaee3791d37d73cec3a801210386f887d387339770e3"
      "b28d869b9c5d73fe7bedb1ea5ddd9ff00b142c29ce9df3fffffffff61a9b0c00"
      "3399d1a6051007944485818642ff4d13cd02438f9d383400aec256000000006a"
      "47304402206fd044314dc7daf6db035cb233d92cb58c572abb0513dc84f04a83"
      "ddfcef7b0602205e7949e9471916eef638ef8a078fc8b65a9f3b705457742678"
      "775d5b1272e4790121025391244702bab9ce576d093cb25886ca9dabb92a7e83"
      "e041b7f54d4bb656af24ffffffff3709db74afec0c999cc686e6f106dd9cb8c4"
      "95665ab868c04c26c2008f049522000000006a47304402200689a5793a97f995"
      "6c780dd9e247b3035d5622e527017c92c3515b6d9c11cb6902202d3cdccf8a2d"
      "a747c826b06c9539394c53c73922a6e4d0e2ce46ae26798bd8ba0121037e9205"
      "79f464b9681b99f020aaba1dbd8b11d9fb270a7c503874dc1088761556ffffff"
      "ff00000000022d22981d7e8cd945bd474cd10bf8dc939d831b14c616ee880a44"
      "e2423d7b3f88221c40c5d925f06fe7416590fc771d7a9bfea5ae206cad2b9f0a"
      "a37e13ee5d09382636c199cd3fb84ccce3fad607f92e3787a7a1a3b1166f6821"
      "482b1ea8c1232c72d60307f71f9f83e5cbf18c712d3d4f1d0e59d0ab3a73a5dd"
      "062dc7d2183ade1cdc1df0a0fe57cb02f23e0f321585d6c96cb1be9add91d07a"
      "e4168df0f58d15380e9522734811c9396b7f30ff6af51e8c3d11eaabd42d6e0f"
      "28389a6e27e43b301426a38c10bccd88642ce32a8ec4cc76758f9a0d5d7a065e"
      "9bfb949e6a3757c9777b2f8c6553125b3ff463487994962938144d5f2428f918"
      "92863a977020ede0ce1a566fd4e70832ab99229c5cea0e3479f75605ce0209e1"
      "443385a3f3440fcb64a2faba266ef644d7c5c7825bce1d967ef32c843ca5f112"
      "e8953195063b691e5fdd0180dead5e5ac2ef5d8b530941b13dccc0b870916e0f"
      "9292d3d51943bec33d6f28055235ffa20cf5597df6ded13af57c147eaa343fdf"
      "979922dec4eb5f3a1dc64b378e1f3746676ad004fca0fad681f54a59d74a799e"
      "be1cd7b92a8226809777caa6ae9166560f6a7d9372eaa3b501f640dbe5a6d150"
      "f711805f10f964b6e14b4c30379c88126e98a149c91a905372c1e17971d49f7a"
      "fcfd884519697e22e33563bf13f343efc66cc16449e11f8d8ee7010b97052d4d"
      "d3de2c3c25da7cf1f8f4bad186c62f01d4e0b0a2c627152a60dd171c1886bd9e"
      "7eb61b14906cc4c1404d6627bd54afabbeda27d71eeda614fb95c11a05aa9eea"
      "6b0a338816b18981c9d5b6f0c94ed4c5726df1e84eb9f39d9a4e709fb547795a"
      "55dad88b1749d698324f7800af6b66d7cdef512482636f59a17440931958b85a"
      "1dad561956bbf30e02ec9f5ed2e53a34ee9754e757542b80f946d532fa0a2bfc"
      "2f63440c18bb6c95b223831a9d074c6837bb5009a3e62faac89d7e072f6f1ab4"
      "057bd0d77df75d16adabbb2d3792eb551b88352f7e7ffb3ef505f62de41dc1e6"
      "f701acabaa924f5564e17c39c649fd0a4575db01b1172e2947b40d0a8f53ea59"
      "bef27dc30b86ed8056735a03131bb0ceb8249a6533ea93dd80deb2be4dd2d59b"
      "69e04eb8e4a7a27f3e615e8349d8e03eebecd0b653e346643e0e1947cd7e4f36"
      "fd468e3f41a0b061d11ad35315b7be6f915e89c977b4960f0a032d6db1ea0159"
      "44d79b9c530fdd30f7d8dd6c9d2cb16372107f5b9cf54740e429bdae3389ffe2"
      "54929e7187f47e60e8d2ebb28552f96467e6ce353909b3fdfd9d09c02e346fa0"
      "db28bfe0b6952279e304ab8e7da3d982b79bfced63306e795d18d752ac912142"
      "aed8854d20bf9bd3b78c3a0d3be4790a61cd7313603598e7451885970ae2009f"
      "c56acc92b8b3e4860fa077a149bb54b7752a4ab55ecec3dbb8d89c9bec1a6ea1"
      "fd86f6975e2d4e24f00ef3b47adf8524b87dc58036f864cc04b1f08f68f200ae"
      "d55f247a13be290e7fa67c372ca578b55edca13d65b70edb334567cc533b3304"
      "cb4dc4b9e94d88b6ef3deec767f5af47fb189c746ff2570569e550a610ff0425"
      "5a29bcb31039e6de8f38ef3b9a52544d6cfcbd534a24482845ec990a571fdf8e"
      "9302d9667067236be22cf4dfb934191a7a1a9b247b5d1e7f43d8ce7d5e323cd0"
      "05cd8d9c297150b832521e1e9b8f1750e05c75c3ecb1145750686ed88abbeb37"
      "c0c075ee34c457716a387972f33b1697d2f74a5ad7d22d8985812b68c4e64c5c"
      "16aed08c14246b8890709fc7411e354b34eab6b6bd344cb4a6d1fdda93029e66"
      "fe91f15feeefbd35d7fa4e1400b21bd508e0def8c0a89156a737e5f429422864"
      "09d954902c39b6e7f562ad422f4809dad3eede4ef926cafcd9468b8efc1bc9b1"
      "34133da8e040e68984c550047935bb1bf8ace8cb1233152b79247156ac06be26"
      "2ff0b189dfeb3e0bac9b6e61e874b3b456614692daeea93fbacb3d55933100af"
      "2bfdccd3a0cd25b387f576a466bbf66ff3b2c53adb54219d386dd185ed906e01"
      "d6c4bb59dfc68841ebda50f45a634579e261cfac2b120994edf2900b3e0cbcc1"
      "35c8daa02c2c0a75df22bbf4b1bbd45b49b3811e004631c655d6a52ec3aca177"
      "4539eafb91ec054374f3362ba0c46fd02407a8edd4c263e5724c8f533b516d9e"
      "32f83301de0adc871ff9b58df6081af6c46e339a981afb799174bc9ce9cf52ce"
      "30e38da7e464063711d37f283b1ea7b77f8f8379068e9ca4bd44c7a0a44deda4"
      "272bd98d92ab40223c322220dbdea002ec3a3fada46fd9746da7751917fad8e5"
      "1f7c506431df2a91f01d8b27b80907f0d8ffffffffffff0c50a79e318e120996"
      "f1cdb0154d72ed1b34210b3afda68facd8472c8dc6bd33fd601c033279e79852"
      "ebae152789b26bfa94fa935df2d57292443e18133dc5c8511a0c873712c046fa"
      "5185307a4b8855ae958027c187040d6c3508abae409db276699915accdcde5d5"
      "1a68387ada2b7228a1132bb9ec2170217f442fabdb91a0cc7c265b7f57158e11"
      "21e4a7e0aced4fc95cd5bfc348194a75ede0b7f38f0778c09d27d9e43bb2d3bf"
      "813aa89009e9564c077e4a5ed7def37a8ab47235ef777192c5bf0bc62a8a2739"
      "45dd0f926c29f05fb4724ab680d03e4d515ae7098b39c696ca0942da207a7b70"
      "d0f5fece76091e5a7d7e9bdba15aaecd5f928d444666fad3231c5f4e011aed62"
      "bf167a680a8a3fc43db1c644e78d5e9168dfec4839757346269114741d89b774"
      "d6b2547d8b5eb9fa6dce75dac61f49089bc88d40b876b77d5436616a58a0667f"
      "ea8c26547d9533231eff08ccde600054305b2f6ace7300db630f17004a04a5c0"
      "d228be39ca5d02105b65ba8eba10cbe813e21195614effe27e126bf264fa6d48"
      "810a8882ad4000aa29721c6c8837050e582e5e8e1c65ca8e1a3896fc6897222d"
      "d2dd3e6c3efc109b2dc27bd3012d17a34e6ede0bba444364678ae05a923279ae"
      "d0e667e0f8957ecbb483a7ea558869f10c4f935a083e49f7c6b17b4524700519"
      "31430f231bf35acddeb9d0ef35abf05cd0e75eb3f5f8c8c2302197751457098e"
      "614a4152f16c351046ef94f1a5a03146437f74bc21af3b0c8205bba1a18e5779"
      "de839fc86a0b566c14b4c393e42a9ce28cf34820b83a1ef960a508d0ff6f0a58"
      "43cdb9cdf9dc6627984ece769c78783c2fec0c0395f08b699c085bc9199bd126"
      "99b9b925ca973ff3f9d883e1e184bb3f2e2a5a880dfd4d6bb9331a38907db4c2"
      "24fb85018e895113a63103d72a1a8cc872b159dc0d038ea6e91fb6820ffca219"
      "6a96df593d22e27708f993b69eb3eb4316a058c10511b14eb9803e6b390e850d"
      "7a90ff88b433f02961758ae45e7c26107810d284011848f168a05f13a20d15ad"
      "d0000c8a975d2bd3af5d43964ee1513e581629b79e9215147e88cbb444a56e24"
      "6b5ae48cc0f290a1db9dfe3dac9e00a5a731954e0a960de7d018f841ef51adc4"
      "140f29b9bb6ea415e8ad45264d8706022cbc041f21ca19f2958934d67c391b01"
      "f015032366b3dcef4eafc29d3fb8b2e355fd2323c0baa50c481f3ab626c35325"
      "ea757534f4dd9139091841d01f0390eef6ef41a175b39b48ba81f9cc7d746cf9"
      "815033745f649e17337dce4843bb15cedacca920481fc50d2a3280ba75729d3c"
      "288dd5eb1636dd380fda4935ef89dff1fa47addfd67c7cd8e014973d0d738e81"
      "6e365e28f9ceb90a072467a003f84c29383728710d2c5ba79e203b0e7fcb55fe"
      "2178891183303a30e2f0a8c877692038102c2ffb100f480656901394627b0bd7"
      "25690ac663941e0b941c96afac256268b8dec24fde8a4afdc702f22639e5d4d3"
      "452def55316ed438dac66d85dfcb8e0da4a83766cdbce4b39fa29ccf7d7a95ad"
      "371415fe91c0a221c6ed305031079289adf9a16f59f366024b3d1aa63bda6835"
      "23ee9c2ab6afe3f068249674c0ff1b26a7cc40fa143c0a1ee00badd3dbbb211e"
      "aaedea3f13a2fb9f21c79055449fae6b35b50fade84e9b75443c3d8240a6df21"
      "4f41b12a599962d890cc02206a774d876da0ef5bf680de8c9e24e6a25dafda3e"
      "fc033f17f529ad76f1bbf04313856dd22ef6cbe1adf6d49dba36177037854a95"
      "4b6f10e1cfbcbcc4d2a6053a54d817ef1529c1677d1845fd7a348cfa9998126c"
      "2c5f0fea79b5d0df1e4019554bb7c141d5fe0dea46741ba13425b463a6844c12"
      "18d69c335f10ebbc459939fb6ebf51c63c3857273a0d770f468862df228a90f2"
      "d76cadcf5832438a61ea9db95e61eb263bf1d11949f41b5f03a3802e034e2709"
      "4b8e5d740eb9a909762d24ae9ae4a2e63b21e88eae04fdd8e58e95de98cf314b"
      "d7a8c323645f28ef43058bdce400eb2a8c4ccbeb4367874849bed5617525d4fb"
      "69cd9bdcb1c581cd16a44dc358e987a444af11da1becaf92ff95894606c4cc2e"
      "b1514e2fc48d9fc122b24a40f8993cbec6a20a81e0918f96b9a8c7a61eb6e046"
      "b4c23c2b4ab9f7fa0bf2033e9c23c872cc762c75cd40b44e3faa7c33ba3f1f34"
      "b0b8be9af7ec1502ef59afb6a8bf6e484638789f4afcf62801b6e5cbe2117160"
      "2f5322750f430b85a3a1cc058ee333b15fa5caf7ec0d9cce4f82aee85a2fa2c2"
      "cf739f62cde54c7e23b6e8dcc358d5f2feb69945b7556ea31b868001e539cdce"
      "0d4771c422cd07e29b02619b6697bba1f2de2b2e4b6cc3e7691032392216760a"
      "90934317518a3cc4ab73d2ab5a5b616d73e5a77199272e6d8038237136a89262"
      "bcfb002c1aca43f7936367bf2f6336eb3d32b3daf5fe9bedb194f6f17242aa14"
      "9eebe455fce8a7f73ab4d412637fedc677b0d2fb4cd3256e8d297ce9e1af35ed"
      "4b2345739d8ef3ad26d2029edbca3c0ccaa4ab7d6997d7dfa52fdf9d2e3f1d63"
      "290c83cd34288c65f8c3820ad55de46e3482a7ac59037b0d8e20c4b83f73c9b3"
      "f747e82b2d27d9c8f08826a685194f7e0a986690a9bd16d727340a2301a47825"
      "79c98f25ba17249703121afaf8f0fa76e05e853183049b3dc527e82ea61a5a3c"
      "b0b1a078ada0072d81039299ad371622ccdc7b8eeebca8284912f4a69248f4e8"
      "8fea3902f7dd6de2b189c8073536f006d3ae9f82e2b434fdd22473dcef2e049b"
      "a4451546a3b14e7b2860669bcfa7d5e58f214c541779f54ee537cd6eb09dd2b7"
      "e7212ecac9183e270fe897640f0aeaadfbdc90ad914f3a104d2955be2f70f6da"
      "3ad54f3eeb2b10eb7866171eeceb85371d54774c2c60e761b535e81baa3ec4e8"
      "56eb35b02f7790bf536bca2c6310b59e0ce7e4d5d427a1a47c0151d6234f6000"
      "879fcb87f98869089f66b0fbf0ec3da772b835d23e2824a81312025605335005"
      "0eedf2c13c4a8e18e7429a4767cdacda112220396679fe81fa1a4681b4c93367"
      "02791f3d1071237a52747701780e49ff28334669256b48929f1a9cc11610b298"
      "e5eac80306e9c6546f31530216129fec21144aac00f8df32bc263bfad407169a"
      "c7d8292ec533aa5bb59cba323638a29b9de140377ae54073c9330e590dfbc616"
      "3b4559257d67a955c11ec104c798eb78fff5cd5dbea7fa08d537e02516967a3e"
      "4784431606af8e0a825344f6908a0ddf1f548017c869fd03d806fb04747dc57f"
      "4098bd6a79b6c0b3ddecf246b7904f773097fd0f3733739dd0307ae87ff91a8e"
      "cc1f82690c995b9ba10fa43b5cd51211a9f28be21e3ba4519b06e9177e214581"
      "22646e1d3d78b6f2c31c33a14f7d5fbda1f3c4f8636058a9d8278fa3c103d1ef"
      "18debec8679f3087ed536f7aa48564f099a746e5f7cd0128343312319bcd4f0c"
      "4876efc067e94c5b7cf93e4268d45c72a5e62f4cc4db7375cd0f7ec2ab4f0e3b"
      "7c43cb5e15630a3241a132e6b4993dc2cdc0b932d2ec9936363b116e3f7743a2"
      "9d2b434f5dc606ec89b0d00234c77e7f589f2c88704823c2590939fe32228e90"
      "342598b2377a44e78a03e69c7ebae1b48008939004c8f8bee813b2ea4d8a3250"
      "c241b22a0100159d2a37bedeada1f1cf8624e77371656650eb20cf6dadd50e57"
      "3b1722d7dbb4fe6e5af723c7162caf23c6deafa6ed485247281aed3df5be34bf"
      "fe3ce2004fef8f256738339c41518fee973069292206f2b4fc176140ede7d557"
      "78c7e125372412e8ea0d310ff3e95bf2b900b52eea0c67a776373fc56c23ca31"
      "bca98278338f4816671e4d1ff46b3f200bbb9557ee3998e32b11ab820f5ab137"
      "bb7a9d916d986d9ace56d934e53a3ff074980830d922a00a3d0f53a984a4076f"
      "63b3f3eb14f68ef61b69aae0afadfa727ef18448fa112c908a029d342fc60a14"
      "0a0283461e0dccb7f22d36a2471faf49aaf1b62fe8583357f508f87db563821d"
      "9b45442790ad6c8daade8a05bba18aa2d9d4cb0a21e8978806313c992d543067"
      "55afbdbba2a76d8f8a070ac4a9f24b5088eab5dfbd3b6a395d13c06035623472"
      "ec08c4d193fb7d503364d551a45a490913f293a91581e8c14f2937f0ecf83d92"
      "cd488e7adf77f2b6a6bc8f5f11ae4e0922aab798084dfa5e0c280f50eb421939"
      "7d4de16c0465fc2de3e4e790f1600ef62c2a3f721141286f251b1822a6cb71db"
      "cc00e2c77af8970764b1238f5fe0834ef63c791b3c43caeae93ca4be877bd236"
      "c1ca5102741a03833f0bb1fee112adeb0db4b1dbd140a54d910e584623470c0b"
      "11c174b3b09d1608206b7c729b3cf7de1b255cd9a9b5425aa702eaeaf5fbdc5f"
      "bf84272e703a1355736d12391682eec576359c0a437a3b3f1618ec7833022247"
      "0d767a47f51a6846333cca475805d39f5f64f3e4bdee32fb8a1acca5d67b574c"
      "35861afd87818a99be39201594e8f60e8a5701bfa022740f973457f827069314"
      "4484dd7ba4ee37071e4a493782a7b8f96bec3562fac248af9a34f0dbf9c12308"
      "4169f190eb7db0531b4dd1b71d87afd6d2e74eb00b7f656d4733b649ae095811"
      "610607a0f3c6d613cbbebd9958c191ef1999d85e764033ee03188474484fc44c"
      "588e673bb285b047244ad65695d6422550cf51b9657eebe2fe237810f30d80c0"
      "d4a0b2a33842023b93a2c45a80c595200c9e0946c507f1f9a0212aefebf57b73"
      "e324b3b64ce3474c3ac2ad9ab71d4e180cf84b6aa48eca09811e3d29742f8d1e"
      "e38de54fe67c5be7cf51fcf5e156d8391c56c1691c6c3764e9332fccd1b059f4"
      "d79d9588fc73ecdd2a932f1b58d824d91d942b1977be37ceff0707f994786a93"
      "5e4e82aa5a34f9b7461b253c22e1bc5fa2486cc98bfa22d7f822d5266ae29ff2"
      "34145862a878221b65b728d5218814ffc01149423d67d25ada3c38d06b5b1e33"
      "f48af534bdf7f8da90b9e2acf65b11ae1aecc336ad967b1a430b9d71d53010ed"
      "652a164c0bebafbfd6d205be62a27717145692b5e4883f3bf22bb3b13648d282"
      "d373265a5e677b2e679f645118eab40eb96d23590bf505b1af3ebc37edc84026"
      "25eae70d0195450d8474226057db2a5ad2ead5c14dc7c8b81b33f79ef3627984"
      "8cbe8ea13a6a43494fe0a7c43fcd470d7d6e48d4a35ec13a673294d0be5dc265"
      "b7b09058e22af298e20c2eb8f56f4852f1a23ef80410efe44f153f5a62b613bc"
      "1bea2ab620e926e36a40303617d38614a59a534a5b9480cdc40c956b52d0cf23"
      "e89f3211ad3a6b6b3d162b442987bddfd0a1e749e2efd68efe24c8470b76af0b"
      "ad365d809c0ed10a2b0a981a47122d1f35bf77827350dd2fd80b3675ed6cef2b"
      "dbd6431da9aaa1052aca2cd12ced64801502e02d47ebd7361e2aca30fb9df46b"
      "32ac272aa9ce97f4ff44b2602f6cbddf9d7e897cd528f3d77e0fa25089ac01ad"
      "e13ef21c7f199ff5e638852a82e426abf23efc961c771b4506280fb4807d7606"
      "2993ac2343ab5b95096f66dfb5384d33823975aaee4aad49d7139082c3c1c944"
      "d45a5ddd4a6cb027c21fc2608b1f13e3e2230360f365dfde5b31294f17d890ca"
      "a14f6cd6660692b096638b8cb741e625dba34d74be7c4a73de2ca8e6f767ebc5"
      "47af74871484a19443ab3adae3450c9c9031b471a9f10d8dc81ac5d865a7b84d"
      "05775af5ccf18851895f7870c59cc196f077bad0fb8b6d287a36210da4e90251"
      "90d4c9278289d2838c17f17d3662379c13917123b10fec5cef2c3865e6cbea45"
      "2acbbeeee8e62d46b2a16ce81a5351a04603405174e001a2912b62f6bebec89c"
      "954f6dc46eaa648a77b39c9033af2699a7c6090658f43ff10818b671c777c509"
      "6d6c2582927f08918dc9f028c2dc349d22ef5b856cc09167d42f2ec40bc95179"
      "9b762e08ecf4aec709153376c3c80e45253074714a7ca2e07e1a31459d4c579c"
      "ea1930b1e399c4b16fd3b358079e9e997725e4aa59d748b2fc1020abc1a61a70"
      "f729196053617bf600d14b9123207fbc216311448c0b952a1a33f13de4c9ee08"
      "277c490abc05ba8f132132c4e71cb3c943b17ceff4c61d93cf2068429f89f7e7"
      "6f803788e73516aa45fcc493e0b0ea4e1e9051cb28fa05ae0c28fe10b8085660"
      "cd4347d2198e4256468062ab5096b163f5c9c96ce10bc2621a2509e8ab5720bb"
      "46d915ec66fab81a5c44dc82a01debd82d52e3876463c009dc375a5b4d072328"
      "9ad274aa48d606f44743ae865c31838c967fd9e656e688797739e664ae50647e"
      "0582505e84012bf17bbc3fccb39a311d86b92641990075d4e5154e4ee4b9011d"
      "3ce6ec37f507782a9e99f9c9fc4ec75c77ba50f5e51d39c439328153bbd8d740"
      "e562efc8713c4c2aab589074400e4b8606ffced8ee3ab1c87039495de821e2b7"
      "bca15ed70cf1696527da375d1bae1d478163f7fb7c4bb64dda39cf64da42ccb2"
      "3692a90125ebdb08990e705c3124dce46efdb492a5e9f0ff6c0e21cabc1a05fc"
      "7f9d199f4d9dfa16afa668b6dcba84f671f3cac0e8a671b984190c4a44dc0ed9"
      "6f27f33fb17a1b6ce571148608f9b54670b22101d1139376c1310e26f018cda9"
      "5794c180c81ecda15f73727d6cc890e7e128ae8d241b5ce6d621f3ce12397a3b"
      "94c7fd5a332e5540e47553a59bb20f7898e0f2d37fde7c50191227691a725364"
      "29fcb256c941b312788767ab6bcae92d52d496d8e537a798ce235f671584bf40"
      "76d8d35b87b2afb688d0966c543ae558f8c816eaa4d38116b4016361fcf02cc1"
      "e51b2c7507be5476f1e404c5f6738ee3a8b03b12a09170afe31fa1732c0a01f3"
      "cd744a9eddb20cb8a13ee54b2d078bba216bf6c34e91b6a6df06a80febe8571d"
      "45980c749e52c407d1e9e4f499d28ad1e5fd8192aefc5f1f802cacd11c04b0d7"
      "c0992421ba5bdb7bd7e214ecd1070cd14af8eaca9a7b9f2a6c0a2cde990cb05c"
      "13bf0ef4fd038a2e2234f390161cfde2466acc3ecd9a901e4006077ced09e223"
      "9e6f9852d74fbf17368d2dc96007f6493c80e91768c5d9e76c34575184836ab7"
      "2f241e96af917ca05546548ecc9ff98d3a2bb8383eca58d04026c15ed4a7bbdf"
      "7ed72a5f746f1c94273de9548faebafb47deb4b2d1738cbd0c11c54d9c034456"
      "632ff9af57cc218c6facc2e094f6bc7d33a1659cff598b5c5a054415a17abfbf"
      "be4cdec1d0279802c78ac17772638b72285b96426e52bce2023a25af98eb9114"
      "ab5281c5d7b0b3504416f0b017ea25c4d5375f33a979e2dab71827506b4f0289"
      "c7fa5021669286b656a7ab5fbfc18a9e23ad042cf90767eb360d5cc52aa92d5b"
      "74b1935b12148dd1e331c4e076d1a7bd04bcf7406bb5f9cb83024557385c26db"
      "3aa0f7b930b241f481f94025b43f201ca96f80cd2d1fe8cfe0069c5b04bc7e96"
      "5f8b9200615573da25822295372c8a527ae851ffdfa2e562ac2e099b4dd9175f"
      "4d52f025587f7e1c9c875178401ede66e74e7a331edfb0f6322f7ced9ead567b"
      "135ac9c1e70ec9d243f17eaae397c87a2b754ee757ba53f29a022c0407ecbc8c"
      "e2bd95f2889753196267f6024eee6f4e1952ff96acf2842f11175d938e6198b4"
      "dfe5889ec353cc57835dec804d96928257e991244f2f3acdcc02d0e42f156ed7"
      "4c6f109f0c167cac4e7b2ac9b3c3b8c4bc87a1e081c72f91721c79e740fbbdd5"
      "2c1fd5c650af7cf66111d4f1135dcbb49216cf9e8c87a19b4f3226bcbd77c294"
      "2f12d976bcc9b551553ffdff66b3f0079cb921bfb38686772435b2a35fb23efb"
      "9403326d164131a1f8a3533970773ff43503daca67e1b0f1b82b1d5dc17dd3cb"
      "18e3c86d614c0fd18264681f99243c743a2d09d66c1ffc041808ed82d248e6b9"
      "62cfc451ca9453a476593785c43a10b5ab1774a99c6ff803b327c17b988a80e0"
      "4203d01c9115c72a0a8fb9ab215830fc3ee9ba8b99e31841cc03607222e56976"
      "27cb27c6032653de5c9660316e693c0799db23e6ffb3a6799819b7c01e369bf2"
      "5a7ccd63af86eb9fabe10d95b4b97237a633048f08acb8346a1e7d0b12c8c6fa"
      "4d3da1a5ae5581f2006a9046fcfbfec22f1a2d6b42f6c7b8f73fb0fc7a84ca70"
      "245084df8a2cce0c58f2e1e6911a8b151b8d465814318084ea35e4311fe2ca50"
      "17f9bbd5ea4d40dd111c7492bd4b861b29aed6cc87204a3a8b18613998b899f2"
      "7f25726865105b8bc568867d45ee5c13fca6707c485977fe93096504d6fb62bc"
      "0e5782e16de3a33adce07d9763dc0e662bb67fff4dd1b8550d3469d35d8e3728"
      "c7a8cc5dfb31af28d73f212a54a8a3dec386c0eacc8d255e7629f65ba48c1f9b"
      "b7338920d5ba5f912701dc215925d79f27894894240f9d571e11809b7ba7139a"
      "6929e7d67204caa66e7c5c2f17d29c05626e25eac9816d0b3636746da48f23c8"
      "6afdbed009837a5a3a02fab3d7e99a5fb35e6725cf2f4b3e4809cc636fb109b6"
      "fa9a96ab18231818e0ef2ba09d37418290b830b092c3d8f45233183fcfb2c7d5"
      "d33c5a50921d71d0008d384c61a37983d5b60df12da45a33951372f7dfb0a727"
      "97bc1d225320762d89039fb6c84617c3f82428f7c67927ebc602b176c6c339a8"
      "a7337fe5c44cc274e65e5a7ab5f87a1b52df78c12cce30bed60dd59fce64aa7c"
      "5082764ca4f775ccb4a24ebabc56daec4f1c225d8db7746462214a723bdd4db8"
      "d524100bc6c857db8cad995201c9262199448619ed33fa367d2f8ff7c2cb6f36"
      "35db3b991277119a7a130122563f40b436189103b650f3ca1d00548eb927ad3d"
      "e4ea31c9eaf20d48eabd71e97bc56d73a7fba791f719c6dfa8129d9e7b8bc7f7"
      "034baa02088d328f9c48f149a354750eed13f63dbef8167bdd3bf35c2d470692"
      "03d0b280f813b1e6320a96138418501763876f48e1a16cd0123f9e722c35d72d"
      "b21299c08a16f26933f3d5b56b7dae3d6b9ef2e5e58e49264d0b846bead6853f"
      "5d2627cfe9d21e61ab7c17e67e2cfb7691e5fc95bfcdae4d46a9cc1400513e80"
      "fb1e0b0e6c9501ac8051b3e486647e8903ba9d59992e5845991066b4dd8e48c2"
      "962791fb1e28d07164ea136fb8af825844c1d6c85de68250783a7ab621519d76"
      "cbe88949334f6d7340f083405598a3114a03ed0e863106e0d2083c56d154fa65"
      "3eeaa1ab93d0e94d42df74679370cae17900dfd5330c6f2d5d35be9d72c80965"
      "e9182108772e578d69040a85bc1284a08ea58caa3307281b0838afaa337479ba"
      "bec63d886a34ac5fb26944ce67cafa18c5a4ee54acb2d560f7846ce969d72426"
      "803a5ccd2a9746bffbc12840654f753e1f1306a8afd056d7232faa0b65eb36b8"
      "50b3cd2a28fe6a80474b31a6c1d82fcc0cabf43b876c35fa511f66a9202ce022"
      "8fc4478858a50256979b05fe29fcb674754a39c117e6a4c98e1246fba658d78e"
      "a147be65a77672410a78696cc7ebe3f69a3b60f174025e302587db6b4588a0d5"
      "51c974771e9a73d7452216669082ce13de9bd6d6aafe79470b3885c83ccbde3c"
      "fbaa71f260c2aba0791dca7ace2cd1e164ea01e48be5d63d17814066f3869b3f"
      "a5ceb04081ff567261b82a4dc31a2e7b27c4a2522a365c14ba84a490eb2f1e54"
      "b2cd73be8978efb11fa4f32fb2c5780c8f9b3a2f1dd23aff9fb7f5c59d520cad"
      "34da7ceb8359803043ad68ad0d1b6bf437ae6581c09818bba62dd6334f9c316d"
      "87e6f309a03820fcaddcb39ebc163282bc44d379d551f987bcac7743caa1c7dc"
      "c3bbb24b3932cb45560db0b3008e5dd0f916f9a76854e281208e48294c0c74ef"
      "3fcaba341717f0402dc316783eb099f1a7460b8f0b5e045e109b59e8179f9c68"
      "eec1214c583f432fa52223f331bf9eee78e76bad91bbcd69760289202772d64a"
      "9c8c9578d96d10df080eb439221643c383eff91ac9281ca314a812d0de881298"
      "4794df0e97b4939e2c887a5b4015b27a38a570756c8a78e45d1bc562e0995ae8"
      "a9149d1a36f2fb5825ad8e572bdc40dadbd727ec66a48e36cd27408d4b328a40"
      "6451babf117b04e267d09e9a8ff7f434c22838c538a96c360300e2c96235c750"
      "3fb56c2e073ec71cf4c59644c77ddc2ec4ec99f9701fba40aabd47376af38445"
      "51abb635b0f1b4669c4d8b35ab9fb815b09f1e6843beb50830abbcef01a8d093"
      "8a350d7d1b3bd2012372066089c692bbd3fcaee234677f400a16aef855ac8268"
      "32e7259ae0bf7bd27fdbe0936dccfcb2eed6532336da5ef61ea21adc3568e036"
      "20bc10bc204b66461732dee686ecd1caa9fc0e24e11c349bab85cfda8070fc23"
      "d0936f9fc590f87a62a098b66f3c01984db2428766f8a664ff16b43a0004a1c7"
      "3bb4d75540d55c5b6e7ddb2bc63051323f33ce3fe05d592dff0007794b7f4a0b"
      "a1973d540e0934291b052bed9f75d4dbaeaea95aec9394cfa586cd546b9b0d31"
      "af3a505f47a7ea5dc123e99ab6806a7d437fd271a2fe8592481dad03bca21c64"
      "1e962e357048836427ca1e3513e2589b8784a2f79e92c21ef98036c67c1f73a1"
      "b1a54e3a9512928c34d04e9974ef3d7b29991b23219ac85fd52643b3baa022bc"
      "1f78275adcbca9eecb3b418b1a7957485dd25e4574527ac0ec3b9db5a4975d51"
      "f31f349e38aa8fa9f0437976e13e643512f26c96386a19982f3d",
      ToHex(captured_data));
}

// https://3xpl.com/zcash/transaction/b398228422beaf93113769d69005ce682cf9825baef999a87bbbb695e23a1400
TEST_F(ZCashWalletServiceUnitTest, MAYBE_ShieldAllFunds) {
  // Creating authorized orchard bundle may take a time
  base::test::ScopedRunLoopTimeout specific_timeout(FROM_HERE,
                                                    base::Minutes(1));
  base::test::ScopedFeatureList feature_list;
  feature_list.InitWithFeaturesAndParameters(
      {{features::kBraveWalletZCashFeature,
        {{"zcash_shielded_transactions_enabled", "true"},
         {"zcash_ironwood_enabled", "true"}}},
#if BUILDFLAG(IS_IOS)
       {features::kBraveWalletWebUIFeature, {}}
#endif
      },
      {}  // disabled features
  );

  keyring_service()->Reset();
  keyring_service()->RestoreWallet(kLuxuryReformMnemonic, kTestWalletPassword,
                                   false, base::DoNothing());
  auto account =
      GetAccountUtils().EnsureAccount(mojom::KeyringId::kZCashMainnet, 0);
  auto account_id = account->account_id.Clone();
  keyring_service()->UpdateNextUnusedAddressForZCashAccount(account_id, 5, 5);

  ON_CALL(zcash_rpc(), GetLightdInfo(_, _))
      .WillByDefault([](const std::string& chain_id,
                        ZCashRpc::GetLightdInfoCallback callback) {
        std::move(callback).Run(zcash::mojom::LightdInfo::New("37a5165b"));
      });

  ON_CALL(zcash_rpc(), IsKnownAddress(_, _, _, _, _))
      .WillByDefault([](const std::string& chain_id, const std::string& addr,
                        uint64_t block_start, uint64_t block_end,
                        ZCashRpc::IsKnownAddressCallback callback) {
        std::move(callback).Run(false);
      });

  ON_CALL(zcash_rpc(), GetLatestBlock(_, _))
      .WillByDefault([&](const std::string& chain_id,
                         ZCashRpc::GetLatestBlockCallback callback) {
        auto response = zcash::mojom::BlockID::New(
            3459236u,
            *PrefixedHexStringToBytes("0xc79da7c1700467f8eaa77c8fa416215ae58acf"
                                      "13b207b5f1decb330000000000"));
        std::move(callback).Run(std::move(response));
      });

  ON_CALL(zcash_rpc(), GetTreeState(_, _, _))
      .WillByDefault([&](const std::string& chain_id,
                         zcash::mojom::BlockIDPtr block_id,
                         ZCashRpc::GetTreeStateCallback callback) {
        auto tree_state = zcash::mojom::TreeState::New(
            "main" /* network */, block_id->height,
            "000000000033cbdef1b507b213cf8ae55a2116a48f7ca7eaf8670470c1a79dc7"
            /* hash */,
            1787590517 /* time */,
            "015f29879834793579b3eecd6c635144bdcef3ba58ea55e7d19808f833dfb220"
            "2e01883efcebb77313cd9b5cfb687188422b6417b07cd76e2d80e82540ba3a97"
            "f12c1f01e58dc09233a19ff7974a08c1bd658ec1aa4e0f5361bef5f925aa3acd"
            "a9349373014f46038118062d31db8a57a4b593926c111e1ba2838269f5df0d91"
            "b1ed92644c0000000000000001d5ec124db8a1d299a90e423001be38fb902ddb"
            "59bc48f7a514da8566afdcec3700000103d1187b8d91f9b17135eaf11ea773ac"
            "8f2226a7037227c9c24546d63776c65101ab65114517839c9df32720a9f91f5b"
            "e6604f91ffd2193b3a9d16c4ed2a81df73000000000190eb9e2bc82b8b980aaa"
            "63ba44db65328553ba840c38c5011a465efd8b233b2200013e2598f743726006"
            "b8de42476ed56a55a75629a7b82e430c4e7c101a69e9b02a011619f99023a69b"
            "b647eab2d2aa1a73c3673c74bb033c3c4930eacda19e6fd93b0000000160272b"
            "134ca494b602137d89e528c751c06d3ef4a87a45f33af343c15060cc1e000000"
            "0000"
            /* sapling tree */,
            "01e6b34dc50c2aa70c681e21b0fb6ba865a088a6153df790d45150232bb43bda"
            "05001f0000000115233ca4e8f48647786bfe4fe9ed5c1dcadcc7124f5422eac8"
            "af9d21268c803d01f26993b77bc967e7908ad3d71290731368d378c2a6100311"
            "3f9be34ce4c8c21900014462e6bf4aeba1c8220bccd4a2c8133789a8124acfcd"
            "b2c590fbc6b61dfed81200000001301c9a99911227ad85b4a1ca524900638cf2"
            "1845e410312f8b57173b6de7a83f00013dda000e1a152209563e47d7dedd8ece"
            "9caf5f808a9b0bd841709ad46eccc81501e0828ebbc5dcb2be8011aa76b77c3b"
            "7e4a630c587f42fb350b0b4f570344f3270001cb129832ff83e5ab567f159e7d"
            "0c58a04c11074a9d29d2f21b908fd11dddc41800000000000000013f3ddc746e"
            "57791a2cf8900143b86b9ff7b82454626f0ba633404f9305b6c32701e2bca6a8"
            "d987d668defba89dc082196a922634ed88e065c669e526bb8815ee1b00000000"
            "0000"
            /* orchard tree */,
            "01a016cf287616ba3582a85f1c93e936fe67e97824cb4506f19acda68365323d"
            "25001f0132fc3355ab6a752178d1c551c3918b1809794b48e6fb4f654f4b4849"
            "96f6c23301418e2e3a14692f3f420702037508ac717285ad3f4c405a8904b1b1"
            "08a772762c01c2cf5ff0b7510634f9f2b259a89ce2d6846ad2bfea07297583d5"
            "d9f0aaf2fc2100000000017f938e1bd2cd570bd64eb69f418fadb5cc31319263"
            "dfb14836fe5aaaefce8710000001c1bfa72c7fef883fd72bb1339f5fdf1419dd"
            "514850edd5b7737563d5d47b7e0000000001f1294ce1ba5cd0625efe133008fb"
            "b34b61f8fee5fd47e84ea1e3aa6faf72d039018e1e8c93493395a08a8d30ba34"
            "9a6c858ca13c4bb34b4f214f99afe7e420452f00000000000000000000000000"
            "0000"
            /* ironwood tree */);
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
      .WillByDefault(
          [&](const std::string& chain_id, const std::string& address,
              ZCashRpc::GetUtxoListCallback callback) {
            std::vector<zcash::mojom::ZCashUtxoPtr> utxos;
            if (address == "t1TaxLKgMGkftrEskViMbgDeVRG43DfvdYS") {
              auto utxo = zcash::mojom::ZCashUtxo::New(
                  "t1TaxLKgMGkftrEskViMbgDeVRG43DfvdYS" /* address */,
                  *PrefixedHexStringToBytes(
                      "0xd78797b93c196f10683481e1299f56d11c0cda7e3f4c249912241a"
                      "8f3dce1213") /* tx id */,
                  0u /* index */,
                  *PrefixedHexStringToBytes("0x76a9146a8b27a8c0fa0e9bbd96dead02"
                                            "4448a56c2ca00c88ac") /*script*/,
                  55000u /* amount */, 3459127u /* block */);
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

  // Ironwood bundle creation happens during signing; pin the RNG seed from
  // shield_all_funds_test.txt so the signed tx matches the captured test
  // vector.
  OrchardBundleManager::OverrideRandomSeedForTesting(
      kShieldAllFundsIronwoodRandomSeed);

  zcash_wallet_service_->ShieldAllFunds(account_id.Clone(),
                                        shield_funds_callback.Get());
  task_environment_.RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&shield_funds_callback);

  EXPECT_EQ(
      "0x0600008098b684d85b16a537a4c83400b8c8340001d78797b93c196f10683481"
      "e1299f56d11c0cda7e3f4c249912241a8f3dce1213000000006a473044022066"
      "ceb3dc308a1ca47f1d7d59457e8df5449e76b16c7a3d2c010b740ee02bd50c02"
      "205839e4c30b0276322726935abcb8a7be9a62d4805ec028eb8e87250b9ca500"
      "070121022974d8433874e318b47d6d3e159583803784686b2166229e74c92dd2"
      "66e67e44ffffffff0000000002e4a767e358d4ad6ee5468da4b51f2ce319d793"
      "f206e5d5134855e27db710899331eb3a20ed50ca95a59b35dc847238c77c1eea"
      "6effadbca31dcbdcb80a279a18cc009d7d50988e8e2d21d562ad72709f61b26b"
      "308d0960fad90c4dfd2bea03938d153cea998c2da8f337d84804c08143ded33f"
      "5d31f70e199485963f28935615d5ed8f862df904567eef4d5949f365816eab91"
      "f98b67cd29c0f4664b8034f0b5a01b3e4e291ac68ed7be6a937adab5d8e1a58a"
      "58c57a3b3c3d3f889970401556cc7185d29e14085ded4f4c87cec9241fa561ff"
      "503728e1aec3533523e04ffcd6d799e776cae0edb1e814b0a9d2f04897136ca9"
      "9ef499b3dc9e45fd6e1c4ca28641c543d91dca7ed45a7a44fc97cf21dcf6ba8b"
      "600023ef9e0445eb1933e6c5694f04a364d09ecfaaddffe393082591c55118d2"
      "2f6937a2add4a8e749cbb02ef44c945247103552088a4106ae2c09a756bee615"
      "2e13536a3509b996502197847ad7fa3a8fe8bb42e8ddf3a52d2fdd5e726923d5"
      "94f9fc4e627b62eefa1914388e3e53f85f8b054f9419f8b61896dfeee432839c"
      "3996549a28bf17ac63b3e6c89346b97df6bb258d3eeca906a790a50fb84165fd"
      "5fcc69a6c308aacc0f9b063b21354502138dae2eecbef918e7d570166245ff37"
      "14a824143cdb9a3f18a978f366514f49ad5c854ca00793b41a156895c9b17daf"
      "5eabafe33aa2a57ac910459ead7d27eadabba65ca678b6a24632050ae79ef77f"
      "3f3bee34971a0541eea1149dad9ac524c3f607fd2fecb7e677c4a9ec4cbb7bf1"
      "18781f43b19efbadba0d6d5787287b908de9289031d15d775319c0298a162a9e"
      "378b20fb051a08ad925d0233a47ddb308573a650b36db9a308f5c88c1127543b"
      "5d512d70583089c85703c3bac3581feda2aaf0fb539d6a2c3591056d243a3aa2"
      "ffb45486066e8081c328a13c5c7f85ddbd746431d0b81319615c76fe47467e83"
      "0cffbeab03fc95bdf533dc6ebe2e20d0d5ed5081d1567a4ec85d64f7088c1c12"
      "f1552b72fe6fc575ee089732fa22a71504d75019f697a348c5f223acd27cf35a"
      "093da8e75980b1df922438fc518991f163995921e2ae6b61fe6f6b68a95322ed"
      "1ca1d82d27a361c0359f5b79b67d30db277804741b76b2fd9fb2d2e9ad02bfe1"
      "0d9f367f03fcfc10be1dd873785a3ea01e79d8ffce75e8d87b4a843d02a54add"
      "95adb4984d9c79e708f24f5b18bc86b1b3df70442cdf2697af4055b9d9c4fd98"
      "2ab986e7adfe04c8ab845e0b28af9a21a13846882b28328d49ac0af7fa5d4e92"
      "2a785676864ce6e06d409f4107b9fdf78bc43acabbe50b2f81d7981ac7b074be"
      "0daa150ca88709fc0476905e5a6ea2dd10d22f4f232604e59ac9ddc96d196811"
      "24afa10cb5476a6067b6d4036201ea6430a6372250ab919c4038ebc586bcc740"
      "475f5bea9ae3e1dc680fcb1666cbc13a2c789681830c2d8e1c6d3280ba1d4725"
      "73c07b634866f61153f505dd967d565a3e2b54358cde2a352eec84ce800a07d5"
      "8afdae0ed1edfd2fbe26fbe10c4fb27b778e83d65dd5d3ae268486177269369a"
      "0aab797775d1924abe825b3f95e5f8b120f94186489b814d6a8da6325c211c0c"
      "ffc884f1c62feeb5fc4959d6cf32bb84822bea2dcf4cda234c6d106bd4993ff0"
      "57d8233695368567b1409ce7e91ce6df7dc91257d2e5ad10730480498876cf3c"
      "e687f9471493d45b62a64665c48fd8abd6d7a3730be7c16e7f9b5a3eb4119775"
      "39938cb5b2d65976cb131f088d58a02bf9a17e4e8f368c57bbef05b92e70e716"
      "6c59d0209a3cacbbb89ab48ea83309770691831e4cdbc4ec1d4eff0da077c0af"
      "cc8568a2a2a0df92e01cc3af3491bdf8c920c25638a2bc6be36ce829041e0f6b"
      "eba4029f6a96cb0108f3fb8579b97f1137c277ed00344fbb286cf7a27d8b0067"
      "9143b0b499bc87e5f2bb5a6a7076d6c6892584fba85f4e7105cbb42f2807ecf6"
      "151dfc654b1080a93edbad7ad0d84ff1584d0108a0754bb47ad7978ebed89890"
      "d7cbe4c8763e5d2687f24d00a7871a6644c7a213981763aa3406511bacbad281"
      "ee48ddf06803d07aeadb41faabaffaed2ba79fbb5e3d28d8c9893672bac9327d"
      "68b1130eaa96d64154ab0e638e64fe45d596df5a38da61c15f4da2f3a8aaafa9"
      "3292b9fe97ab3fe5d2b5ab4326b5b12b859c2e8e089a1590e03bfde8356c91d0"
      "4165ce88963e340f97369371e02cdb00dd30968195e5e536692b4ac36a8c1566"
      "4aafe6508c569901b9f58c989f32a049570ae26b52bc843de3826a65aa2c4c39"
      "75062c6de84485901a62aab315c55dd62c31bc1e2f07c063ffffffffffff5848"
      "92f895782f57de9151b70bb3552875bdeca10aca40912c6da07e2eb6b50efd60"
      "1c424dd9b4753e7b3d02ff56879c25b381fd24f963f368b312374e6d27f57d6d"
      "204ef9d518cf1567236aeb2669bcca4f2ea80a830c522f8765f2176a9edb863a"
      "881eca7bc4ef571bfdb015de120652044a1fe498f9e091e3e8ec9bb9faedb498"
      "34db9a0229aace3ddd64f3c116ac1f3bf85229487867646ab346bbf775ea8383"
      "96f6afce9a7d9b5c1dc4702a6ab285f8fb00d25b6f159b97ee749e756596ea31"
      "2793deaf31dc9ef446061e28f7103c2685b6461d560cd6328840a6e41664be00"
      "8a143afa14a561804f690de00419a9b487cfd41c6c130fd789369a1de758861c"
      "3bde75887c41b39bca2dd141eebb89eb446bfec5c3512a38167d610faacb99e4"
      "3c0d4a57b0a3bd86d74f3dc9ff5991ef1fba35b498f8f88b824d9a84d116abbd"
      "853d3505af10a5b61a08c3f646c60c5113efc4ebdaf3d6830aa1be9b276c370b"
      "9679c2f8f74022dccc29cf4331420c65715573edfb70d7ce467bc98edc80a59f"
      "aed40b7f3ff8c0d1002b4c7d0a46d03822a782dffa3632ca9cec1703ad150cc0"
      "b11321bcbe656003e1e8112609b8cb640df5f1db10899d68e38aaaaee8581e3c"
      "9ce5c5c23a59ea8f46c32d42aaf2b242e32bab52953621e73a234b83662e5895"
      "20a1a183ea713942e5ae73a009d2909513930b353e96e724cc25ee7d372fb49d"
      "123654fdddcfd62a778320c43d1fac69821d5e3226345c6f9c61e2ebfd6e34f9"
      "27716fa7ea880daac8aab9f184133779ed4e1e86f5bcdb5a880004d5bdf58e87"
      "9da85abb54f5a062e11f0396d12c14494f9e3ecc1cf51cf5f556d376f9288f87"
      "2d19f5d31ca57fac5a12094849efba3687e4554641b0b843d5199b41807c9b4a"
      "a9fa464dec857e2116e958135336c469cbbb017842be5dfa647410cb77ee9130"
      "a4041e39e6c70c045571dad27ed7dcda1b1f7aa957b46766e0fa4b70cf688c7e"
      "1720b4afdbe48038ee13900ab3df8873aa37fe696bb7dde53782065e9d6ffbce"
      "26ddbfeaf24e3b4f981dfc5ed5ef7f5ff91edb0c034ae7939d2f90797c1b4ea8"
      "25beb25f265c253e1956a8019512454a10203c33dd2c3430b428a0c9a1521bf2"
      "28d9a9a33cbb21d245f2ac423f9e28250483486d12259dd0afe8f77ef1706e12"
      "0c5fc60451fa66e179b17d740c801e1abcd1ce4b910bedf0fbcdfb37a79cf023"
      "ad01773949d9cbdc66950eebcb34e0845564475a12f93db4d6fd589eb9f416a2"
      "1642aaadc501f0b1d041c0122c5895d43a441dbd9638653747c4a2f95b0850e6"
      "0fbc3511b6b33cf35735c5857d6ac2e2623499a460e5a35608b6886fe79767d1"
      "0f65144f2854511c147513e44e72c6cea7c9cd3aacf7649661a943ce79225f10"
      "a5589b54f0593f1dc35d1645536a1de0b40d31d7161a8c70f09828e7f4528675"
      "1392e2e006751717130682a01d45a89a4174cd7c8747eb5189b5fab718e5d9c8"
      "a4cfa7a0153a4016a9e33e6b5ad01d27504c5a010707096870bf22c333c41164"
      "3025ddb35354708207f1f6e6d6e8c0c1454a2bfdea23c6ef898b4e3af1cf789a"
      "01ba090481f2d9dd650c4510e978dcc50f0bef29037216c532c635fc03f9b391"
      "8c4c753be42a39502903248e3e4de8aeeb330e5b518200228722631ce817a974"
      "3e891592bdc731c4cb823a6ca4cee8f71d123b7ddefa710099e95a0e624e7db4"
      "243572784a60c0c1c2cf3f5cc4887b97e42430b69364d49162d85459d448144e"
      "1e0372bf62caa12ce3800584a33c468b10a3a181fc646df7dbd3db431ffedf28"
      "8024b50299457b520da1005983f9e10318da2cd0e33c10af8dfa10f429c441c9"
      "12258fe06476da0c300b926a23322a7f5f5c2dab65d8b326b6484305f7d6f8d8"
      "2b675e3e3bde9ba43233780458e8824e3d32b87db2d5bf8ff79baaf148d6e196"
      "1f2bac723d03a9ea91961a7c4e78c895c73d57ba23e7b3a2a311481b90b06546"
      "8e3377ff2f9339b92aa1dabe7122cbc5e6e78b7fc594c1b67a5199cacf2ea40b"
      "1a5e9b709a8f92041ff65abd256e3e3d3c40b6b92b8678a071394deb74a3ef62"
      "af5630424c3d8c2058cbe2e29ff935a9dd3e076330d848b442b53debc1c24bae"
      "96abfb8f5b042028def554e74e0ce74032f25da960470fe25a8ff42ef103f4f6"
      "334eb64ed5ab2b1b2919185b57d928e29314170c3ab5b2ced02a0b72aa4e7cbc"
      "10c56e2ceb37373a14f4b103cc5bc0601e5957364fddf165973e7ba5ed2bb442"
      "1e3652a257eb85675985b8cb160b5e11f96c0648175d3cc66944e4453775daaa"
      "bc6f2ecbf4205f76bcefcc784ae0db7158d10f924a060e272f938925b104f1d2"
      "3baac49e288ae6140564f73f75fff0bee271c89e67b6785d9c14ab1a5c0fe09d"
      "903f38f749e1468e1ab046af81db0057d5b82d94c491499e9cdcc4600f4b5d94"
      "a1d781edeb476d3245f90fb32380ebf8a8d6ba344d217dd9738181d226fa4a9c"
      "06188045223be7889a6624c12e728a3af06a646e971e4acfe93ee1b6a8ea4972"
      "3344523d3ab90fa7c3428c63c4c4be9c1e07d6d8e09ba387e252ecd07c654c97"
      "1cea742b1be430c85b36d14e3c7c433b98ab99c632db21e5bf9c56fe75e86737"
      "311e3afb2ad25eff69c49625ff244f704802213376e38e8b4fb5a83d2eb5281b"
      "02a644ff0196c67a09835b52c754e9860bbc93d06b71a8303cc467184eb310d9"
      "29d3c6ce0f7517d2c7be1f129aad6bf1d13a6ca610867771ddce45b2990af1b3"
      "104d27697734c883d13c187c877c52ed167ceed43c38a1be7833899df348c494"
      "1ffc4041cc111c45efbe192a03cffb3c47e1353ec02c34ce6550dc57a7cfffcb"
      "30db877624aa018ed4c248c57b0613d20996b44c89dc6d3e9ec34582fca2bd15"
      "12e06743ed78e9f64ade516130a80ce7c3f67949603139d09e58ad7a7f1e7c73"
      "319e79ed5e8932c753fcf8f1a2110a37e7ee859204e73c4e09e094fc620028e5"
      "0bf7515347b5811c6f5c166197bf815d736d762a8c5820281f5caa9e1ad90448"
      "29d4599806ba66d27d2b5170691d18361816c58d3c625f4571ea0a47ab684a9b"
      "32d11d90ed07f07dd03564bce3869e24c59c070967d37e2c6008d0770d8724e6"
      "0fe858125d9b1f489d4068c0cc221e3c966a77c18d66dd6c0deee6c00d7c2d83"
      "28385227d9e88da22d8888cf9f0446f69b67d84496cd2845e2de007130cff2e3"
      "1b3e432241a1d8acb0a1a72cdffee3e5eca5b99b327165b54924bbbeb31c189d"
      "2b3d094b3e98b96e320f6efd7068d22baab08cbd890804a101678b2995a5ce49"
      "35683525508c790b840f2d93a231dd9a3979990b5de5e8331496d0ee4e581078"
      "0fed3dc519deea1a0ae3c8ec75408f5af42e23223b97b58d3f4bfe88ea95deac"
      "0df23b79c274ae2cc5c251547b08464739cfff717be1e15101753e3790c82885"
      "135226430f6053f7662f2770c7bfea75dd72c576ba6015fc7e2f95db47e5cad2"
      "2e5c2ab8a6028175fde353b2228fd5a5d21eb0209b59eca4978207b0f1db894d"
      "136be85dbbcea2a1aa860d01ad83cd88cad685d6508459fca14db7d8ded1fde1"
      "1270263ed9b3d48654f6a2256671f0a5d31dc5751ea57f994f327f2817d0fa17"
      "21a99e47a599d17dd63019d71a18a8ebc17306b8eefaace77578df99a46219e8"
      "0b44e54ffa0f712713dce794e4b042e7203d7d66cc80a2ae95e799ed2ac67966"
      "30a3fc31ecdc98d4e7aa4e435633463664bf4fd96f650d61c1944189f837a63e"
      "0d888e1f87ab9e716e2908f57c5695e92e2123539543f302cd880fbf78dbe81f"
      "1288d09441ed4ee7466bfa77c375524c4f3c220c2781f043a4c60bd405250731"
      "3a9c78a4b78e2073be92e73684325312300794f34d1673b62960d463c5fb5947"
      "37143875f9bdd3891d3e177a5df078ab9ab226a7a1bc8b492db667c6d9c388ba"
      "33b0719caa3c3d9e34c6340505362590bb6ed02b85d4a6dbf076de94dde292bf"
      "12e0c5b4df786a08710b3d9f858403800c85a5fcec3d7bb6a4c2fd5267066f4e"
      "118e8ef3439f3c8a7f1bc4b303ab2641cb47934a984e96e8817168b9902edf91"
      "1ba4ce5147b7deb432c0a0e0d76299b52f2e8e2e267541900fc2d13c74b32206"
      "2e53f8b56c68b2e72aaa0c7412c8906a41c99680d12fbc155ecf5ffcb971292f"
      "237db3041d7bf72890e60e249d3a1b8685867b91fc7b3847433ef0514318a268"
      "38eb2288d8e390b3dc056cdd8d640f563eb6fd17732429ff642dd1e4269a459b"
      "3f2654d12142e3c4f0e093caf587188ddfc07c1d6aa5cafabe740cd2028e72ce"
      "3c784d1d7a1fcad0a67f793b59d554961d3f6dd879b985990649e298210c64a1"
      "15639119fc22d502e1b1d226d8a9a25a76daecc9310b30758333f0b91e31f313"
      "238289b5f0ade06f523bca08920da85a00ce8c54d54ee03e03822540b305eb1d"
      "0d4d2fe23bee5d48262afc4c184d714574f86992f9d50c297f4b159f2e47fdab"
      "26d4aeccb9e4913d44661a24c0e7fc6910f22b60af39e538a6609134f0f0b90a"
      "33c23345cd7bcb6d9f76b89dcf2468ce77c5e86c9e7060682c00e3cdd493975d"
      "3e9eb2e385990449f9e2580ec73beba66b2165be8e345760231efbea1ac56dce"
      "1a34e627f6e5350105bd00bfbbfa8353cd0619d6b89535005f8297d9a8567353"
      "2df8d6662f0f2519ca658418b210f07419780922a4c977883c21674c3a959597"
      "0929781aa05a570d178725c6af0c5ea8fdffdae73c35fee69f8dc810215a1b6f"
      "2e25f081457cfb68c6f1df9bcde2d0eb635453ac7f8e832012caa9e3fff091b0"
      "3ada5264d73cb31a52a6fd675e7704c4d7be0d140dc9f67b0d5ed8c65cc70927"
      "339a19fc651b6b2322512f08364869fb5a168fc90e4a4d5b9f37eda180880cad"
      "24ac12994ccac12f625969cd38a05f1af9c6ce283ab5b35826c2ff7b040cf66c"
      "083a9352cbf2ec56872c4823dc3adf5ea4114443de7e4ed4e47ee00d7c88a367"
      "3f733f93e21fe9c4de14ea597d5ab077a1da16005698ed9e48065f8973d8aa14"
      "112abc8a811493d8f374038e7d50f9ee09a5ea9a294907a627291f39ba459082"
      "1b919e32a09570a7d73d39d633787777a95252333c3aaeba4bc086fbd6c20674"
      "09e9e5a1c89e02e15492b5de56f95f925f8a801ccb70ec0a78ebaa6d00d20d7e"
      "1b0774df7ae82795a126427a68284a163f0270b9d8a09746db0831c6cef3b5e3"
      "27a9e360f20e9c93f4266b932824ce59cc3d89eddc83b0365a008031538c65fa"
      "00c5a0ae09437b541fef15624e8040f1f59253fcb91cacd60716d49f7d7f4fd2"
      "18e203985b697ab4d528485adc78a85689bf7fcb961e2bdf85c7a81d041773bf"
      "2b32f697a457f55d72996fe8cb4f359b3b4a53a7f2feaec8314ca0b73c26001b"
      "1dfbf357d78e6b0899cf5cf66274499fe711d3781055c9485c602dd1546eb8e1"
      "323bf48c001330b5c4957a6fb7b0fe6894cc95cfebc774b2f6b7d1ef7ac6ac67"
      "2cf8a7a3d7b1f29bd1586e79d52bf6712448ffe5e69d1ce66a87fd5f55745126"
      "1063ce96baf8ba9ba8b5eae037ddb1052c5e7a1422af331c06de759aaad49098"
      "26c70ccc8f9a7fd0813fa5205897fe72975dbee58c34a77158cd23ceb9371202"
      "17450e3dbb7bf58474c717b782acd19cc8fb77eb310415121087043fdc7ef243"
      "15900a1a80c5dff79de09d8ac8abcc2d8dbb6c11ca05621da5ce9592547d7db7"
      "24681f43971bc4ea091226afbfa7371ce3caf1962e660e7220b37bcd0fe7ced2"
      "09d2969a7f56a0e1bec1bf3df5f074353debc9b162c1ed6add1b4f49ee0e3762"
      "3ff2c9629c579ad0269a8b314a950b7c2864a02611fb31d125caa474eef51b71"
      "17eb1c48c266c612c8aaad0ef015209dee6c63c104b0a2fe525b7d1bcf69b57c"
      "1276c93a1432cd35a008db663c2fc28f7437ec5f9ccda3f120b2cd50d9ea99d2"
      "0d8cfed6284e4a5c778fb4cf25cbffd661c88610b6d3cda9272aa5c2e2b7c929"
      "387ce785136f02d0423d09e9b4172a79963e249aa468250f158ecba091d2aa1b"
      "11abb4190a4ee3cf5eeeefc44bbb1079daadc348a64ee3487bf8283f4bca690a"
      "246b284af02d53744330e66c2803d06330b070e36c91b56d64a3d9ca7251cc7a"
      "1dc50990daab99c563e65314396921fe5551ae590c73f048665b2ce306f86e2e"
      "389c093af42093593695d6928b25d1ab765f5fe518daab6ed7c9a834ee752904"
      "14b4af6d660cca558f355454b7a9774b6b40c28b0bf661d8b1db34f4f59ceaa7"
      "1999798bcd4315335f94343df2985096e289eca260af7c25f66b791dc1c62659"
      "379119af7d667508167a91e401550e8b50e4b603b645ed7fcbbc0f5fb6ed311b"
      "23ed339dacff9822463b47ec7eee21c5d31f65fae688dd3cc3ac0568fa3b05a7"
      "2e07ad1c42ba89fe836d62e064b7221121fede9e96a859ad198be0c0b97bbced"
      "1d091b852913942b9c91c1768c28e7db821ae4423dbc3e375e67bee8d9afa39b"
      "1ac084cb7f1700d6758a9e147de986528a0d4fc32a2470f20cc92b52302349f9"
      "34b1ac3700ddb83f527d83e204616167c22d9021727e66cc595388a94e896f10"
      "23c2da7f47c0ccccf8cabc5cdffcba65f4aa25620afce6743a02756daaf74fca"
      "3a4fd73f23cb28d7f86651240ce56a9dc9789149491ddc15ecf41a184c9041da"
      "243c9920e193c891df5116437d37c4a3376d5fdb8a68d81362d43664bb551d6d"
      "354dccf8c58fb2c079a38c443c97147bf14f3e5e6fb0f570b26bbccf913b8b77"
      "35f14e62d9f850ea83c86593ce27565e5c9b936fdb68390e1e15fa36d9d605aa"
      "1037df42721f9d8be8cd2f00179304e91a1c3f42441aacb874bdeff3ce2dd7d6"
      "19f8acb65bbd994c4ad913f61bda71ba3c8f019344dd4e93c196a921136e1dff"
      "3d599f30816509abc9c24f4a762dc19d3c2c4113ecf2098e64b92c299c1f0523"
      "3e85a710130b742f2151c55681d5dccc9b374cc154ff936afb16f5fc58f136b0"
      "3fcd9719c72a3759d0245d90b62be677e1dde53982463c3e48cfa846acef37e0"
      "11fc4775fb93a01d0e42b82dc557fcf041f462c53d72e0ea564dbf7597d6484a"
      "16245bb3560807cd86af7d6f7c4770721bfdfb4b8d8c34ad765371c6cf786c71"
      "0478ca2cd209eb5fb73b6340867610fead10baf7d4c903446243efb2cde44b61"
      "22ca11b8620f294fb6f0e85de15b5e3e4ae065f401468a42ff77a2e497636c86"
      "305ef45df4698046eea35009fd50778b1e9126d15128186873ecbaaa019933b1"
      "3bf152cb11d3f0cfbb82bd28f9e4262c7e3c639ed0397f42b2ae49a2e5301b05"
      "18e1f00dee940ad901227a6d796a922da92dc9c147abbfa20a2922748114022b"
      "2c7582a5eb7853d887ae7ecbf12b487877677c3482f9fde5f8da6b58566ca0fa"
      "28fad8b3bc7ac797b587c88164d074a4eb557970a94ab998cb15853a087ef447"
      "2b342faeb89268ff522efe765045da27d7b8f60d35b5bf056bf2d5349a7ce3b8"
      "386b34e6a09bbafbfeb0a8b5267139663c62e71b9b11c52998b6fcdb7fc6d980"
      "13077160a1274702cefaf539a77095182b73835d3957137e2f97b8cbd0270f15"
      "1d0520cb1bd7fe5f814ee6ce20e82b46253472c5963e86400c7e934a069d2ea4"
      "190701108bbfd4e4e2442c9d446ace723a88021b9413cceaf0f5ec204b2c9060"
      "2f71246821b846895e85432233ed1e06589e276aca527c2d18d073eeb8d842f2"
      "0edd746a8e0d401b085c8b1a66292e328b48ec04848e33ebd3ace0c05e3e1714"
      "08925768ec508fa2927ccf32214eba4062e394bc6c056a470f216b71344be308"
      "07040bc477d01901e2fee0bc6c67b44076f00bd11c3d99ed0984420c41d4b682"
      "0644060e4ffc66e1a0ce7d774b80f3634444c697376d852457bd822f943717da"
      "2f40519b160eaa4efa3b7890081d08f83b451d7b1f6caf7e60e5a75bd24b075a"
      "0a781005b45265e3423abfa525fcc6913b13aeb2b47e6d757bf1d2181aa8f4f0"
      "2bc75109dcffc1f56576877fd2587d002a375edb961d7f17cde4f0bd18920daa"
      "3cade89aacb7f0d1820c7b39a4f534724dc81008bd692513e4a60b9e0b4bac2c"
      "1108ab388aed892edf99bcc56fef1ac595ba5005719e05d5c5daf3b819594a70"
      "3e331425e077f710b1d8c2f2eca0ca70a44285a64e8c866571f45e797a10859a"
      "10925f60a28906aa572d30cb06f4a04e0fb7c0b40f7d5ebadfbbd1d0324d023c"
      "3a0f9212debb1f2fe47facff5147273ca54746e28d6c02227d3653d211e79612"
      "01790a9d1747d65125c88c51a03465614af0a24899b40d97feed8154b9014a14"
      "3a9ceb9f96e3d19347e7c42f1a4fbb1943891044831c301c71a9a0f701a59e2b"
      "01adfccdfcbe432f889a64eb728c6eca779aff46f4228c3a8888ec1fb42f3e59"
      "2c62f25c65ccb1ac99a3705749388ac41f043bda3e00de74fea5a4336399f3f7"
      "087aaa9dae5686a7a9c6b578481a77b264207a021ed2c8e5fec04e518968de6d"
      "1c084c40ee2259dd5168ff466fd2005da7b2d2248ff911f27f4b0e374e56d56a"
      "386200c86385b311fd5fab6b8a8474f476b1bb14e1a574c2f26a433a2216966e"
      "3b97262f64d346325143633d909855fe34a7797427fb63f89a20f76390923c1b"
      "18053b56ef2c1d399c6161ecf58aa4aab42159f6bf9b29825f51a2d53eadb6fa"
      "3b460b5a58dc05d794982a045c80eba2c41bf6db8389640a40bab6c1457a99b0"
      "0bc7ff928e36a97ec02f1456ee232a8067ee53807aaf3cb50c3b021280382624"
      "3d069b407711587bc8448aa982eea3be56a1a1b5d7cee47c520d4a1fa401cdb3"
      "1452f8fa81f5d0e171c2eee089358bf0958f762a2792c4444b31e02c72d1a525"
      "0d1965467f3d748edb783c1a9b6dbfa6259658336dce7db79cf3187db188e927"
      "05e4e63c1f6d1411e92708500e25dc1156469160ca216054abff32cb1befc26d"
      "0086013904c34fbba2f1343d064c399f55a6f61d0782d94233f6b48d56d8c21e"
      "a9ff6f04e0227aab66598495863cca4278ff83604fb0171318ce9099cad11efd"
      "2b065dd8260678caca58fab18dcd7d114366fb9a292857a70b33ee3028c0d86c"
      "2442c9b57fc975e017e8a1eddf6650e96e81a59df5250dcf022d3d42a32b7f4c"
      "217a694307ee30da9c0bb5ecf76a138fdc48e34451ab88b0a7e831dc948e4005"
      "2ea33bc170225ffe75c6858a8bce31f5b30e38abc1c1c767934743c65e53d1f8"
      "15f733c2824cd5c2be01c95857036d055959235613a6f84d4d0462cf7a6adc8c"
      "2d98d3614aec5db559be5c099f0a63729789da77a92202abd376c8f73e9cb0c0"
      "18d85056ad562e521bbcc87d1aa496acea53dfd37091ba87fc288c2ae575c275"
      "3c85ef0503e544b5a01ed1ac69a13480ad71098bfe9cd8ec2522da76de66fa40"
      "b777fbdce14d67bad7d94bdf3289c5440c2a8b900753db21b10bd81b10549769"
      "8d4b47b90cc6148a81beff76fef07b196ea9717e8f345a5d1d1eba15af966c6c"
      "3415c3cf41360adbf85798f92beb4b0fa9edc17b7d1613c201dde462758c4b00"
      "16dcb691ddeee8ce74a7debcc73e67ea37b28d98ebbde6079615d0e907fdf241"
      "819d6871d1e77e76714baab1334241f593272a026ce58b2cf32bf6a10eba6ae3"
      "0ba54e12fc41f1130271763d26835ebae12b9ce76a3e4991fbdfafc88b222bb6"
      "a35818632c1093e8408157b6c28cdf85e41cf0db565ed8d065d311a326db67ef"
      "3e9e4ea9c0f1a98783cedda449539736f89f8195ee6efd3bf47f0e9f21c9fcb4"
      "90b8e6c64f1609dca98dc3267c76bef312d19ee35a50198c57e3dad589856b66"
      "0045b65413c77f9a66c07f4b83c5ad90a241d405688ab6ff9cedcf499d1212c3"
      "a8159ab502de828e08ab496d6b11ac7f122334f8c18365b113a3f6a5e968209f"
      "1bd00d2c5db8f86ef620adcbe95dd9c3f5680debb17e1acff6fca21a6a716795"
      "10dcacbac786d4b45c87ead16c540669f13ed5154e9ec764846e3f5b9c5899f3"
      "181eb095677a9bfde418c11aa6f9388b8c08b0a72be5045c0372b988788fb245"
      "838d8ac9ee0a564319936cebf5d2ae31fb9b9f60e23b883fe66c438c8fa84bf1"
      "a99686283e13265b4963be5d4bedaaf3e8d417a73308e2bdf57e2afca0f10e07"
      "b968964d67db7036c72743ce432a53ed1d880ba43f2f59b2a96874e2c11a9b82"
      "2d2b2777196b1ccca972361e840338192348375aae5f8b8cb57937b7dafb817e"
      "93da17dad4a6dd8fdfd882d730d3addc6a4178db266c6d7ae4d5a37ec18284c9"
      "ab4363e3e3937a1e98c43237af973f73079c6c6b7da8a5e683b6222ee4d7f406"
      "31196c4750f1735b34749dcc55a26b5e730937c198ac119be8154db73112536c"
      "1cf5ee081b6a9a2336b806eae3de20033097758174d1e9299f9a7cf678f81619"
      "a5589f8ab4cde855a6875344a4031b21dbefa4eb09482c7eae637675a353bb0a"
      "3feb3e7b77170e6c9529f8029e1b6036b1bdd0a5d65a2500563bae16cab9ee67"
      "33b109c0bf99ffcd2edce37e7adbbab97af0ab8c767b01fd8ae23df13b6194d1"
      "1417bb7e5f37862361bda24400b191d0ddb18d6f9ae4eb69323fa47a86d79ede"
      "22110c4b1ac1daaa80dd7c9eb1965502198064a8d9433b346a0f60f78f65827c"
      "17",
      ToHex(captured_data));
}

// https://3xpl.com/zcash/transaction/aeedf7db7e1e50b44cb73a658e57751a4142d7ec1c402fda2eb038e617de3987
TEST_F(ZCashWalletServiceUnitTest, MAYBE_SendShieldedFunds) {
  // Creating authorized orchard bundle may take a time
  base::test::ScopedRunLoopTimeout specific_timeout(FROM_HERE,
                                                    base::Minutes(1));
  base::test::ScopedFeatureList feature_list;
  feature_list.InitWithFeaturesAndParameters(
      {{features::kBraveWalletZCashFeature,
        {{"zcash_shielded_transactions_enabled", "true"},
         {"zcash_ironwood_enabled", "true"}}},
#if BUILDFLAG(IS_IOS)
       {features::kBraveWalletWebUIFeature, {}}
#endif
      },
      {}  // disabled features
  );

  keyring_service()->Reset();
  keyring_service()->RestoreWallet(kLuxuryReformMnemonic, kTestWalletPassword,
                                   false, base::DoNothing());
  auto account =
      GetAccountUtils().EnsureAccount(mojom::KeyringId::kZCashMainnet, 0);
  auto account_id = account->account_id.Clone();
  keyring_service()->UpdateNextUnusedAddressForZCashAccount(account_id, 5, 5);

  ON_CALL(zcash_rpc(), GetLightdInfo(_, _))
      .WillByDefault([](const std::string& chain_id,
                        ZCashRpc::GetLightdInfoCallback callback) {
        std::move(callback).Run(zcash::mojom::LightdInfo::New("37a5165b"));
      });

  ON_CALL(zcash_rpc(), IsKnownAddress(_, _, _, _, _))
      .WillByDefault([](const std::string& chain_id, const std::string& addr,
                        uint64_t block_start, uint64_t block_end,
                        ZCashRpc::IsKnownAddressCallback callback) {
        std::move(callback).Run(false);
      });

  ON_CALL(zcash_rpc(), GetLatestBlock(_, _))
      .WillByDefault([&](const std::string& chain_id,
                         ZCashRpc::GetLatestBlockCallback callback) {
        auto response = zcash::mojom::BlockID::New(
            3459279u,
            *PrefixedHexStringToBytes("0xfa032aa48ec4c686e744350678c8d2f372b496"
                                      "3fd95ae6a7873c0a0000000000"));
        std::move(callback).Run(std::move(response));
      });

  ON_CALL(zcash_rpc(), GetTreeState(_, _, _))
      .WillByDefault([&](const std::string& chain_id,
                         zcash::mojom::BlockIDPtr block_id,
                         ZCashRpc::GetTreeStateCallback callback) {
        EXPECT_EQ(block_id->height, 3459275u);
        auto tree_state = zcash::mojom::TreeState::New(
            "main" /* network */, block_id->height,
            "0000000000557b70290b2bd4b5065457a408288140de85c1553fee9c3d6fe4"
            "d6" /* hash */,
            1787593109 /* time */,
            "01da30492163078da60d3056c85819709bc5a90e6eef8887d25c1fb7fd7281"
            "da2b015c3a0791193b8f88f215bd50ac67ffa45143cce9030dc9e1662f2010"
            "ce4922381f01d4e26d4f8d68a8437462f8f25ece7de09a425ceffd53863053"
            "95420fc69ebb6201aa6d260bc6ba6d8dc33027becb6aa1bedbc91267ff2728"
            "91166b2266183e8a0f0001fabed41d6543301f233009f057814b81eb96ad2f"
            "8778db16567ac539d892072a000000000001d5ec124db8a1d299a90e423001"
            "be38fb902ddb59bc48f7a514da8566afdcec3700000103d1187b8d91f9b171"
            "35eaf11ea773ac8f2226a7037227c9c24546d63776c65101ab65114517839c"
            "9df32720a9f91f5be6604f91ffd2193b3a9d16c4ed2a81df73000000000190"
            "eb9e2bc82b8b980aaa63ba44db65328553ba840c38c5011a465efd8b233b22"
            "00013e2598f743726006b8de42476ed56a55a75629a7b82e430c4e7c101a69"
            "e9b02a011619f99023a69bb647eab2d2aa1a73c3673c74bb033c3c4930eacd"
            "a19e6fd93b0000000160272b134ca494b602137d89e528c751c06d3ef4a87a"
            "45f33af343c15060cc1e0000000000"
            /* sapling tree */,
            "017126820af9c53dbd69f688430e8f73d23fb0cb17dba0cf6fcf05208b3177"
            "f32b001f0001f5f3703e8516c75fcbe0b2836bce89bad8cac9c6b240c8a086"
            "836cbb3c9a2b0e0000000128592c7ab6891effd017be9b5bcb31cde0515d9f"
            "1325cea3b3ef45ede7d25d2200018c046edee47af312732e21bdd9980a5f46"
            "a2056b512f7519743ab64c503c2f1c000001301c9a99911227ad85b4a1ca52"
            "4900638cf21845e410312f8b57173b6de7a83f00013dda000e1a152209563e"
            "47d7dedd8ece9caf5f808a9b0bd841709ad46eccc81501e0828ebbc5dcb2be"
            "8011aa76b77c3b7e4a630c587f42fb350b0b4f570344f3270001cb129832ff"
            "83e5ab567f159e7d0c58a04c11074a9d29d2f21b908fd11dddc41800000000"
            "000000013f3ddc746e57791a2cf8900143b86b9ff7b82454626f0ba633404f"
            "9305b6c32701e2bca6a8d987d668defba89dc082196a922634ed88e065c669"
            "e526bb8815ee1b000000000000"
            /* orchard tree */,
            "015b619fc8231fda78208aabd8046126097737fb42c9c0060a630226778061"
            "0012001f0001ea3fc706ac1737c1059e3905aaa3149df23ce2654df7c60b3b"
            "16f4b97e4fcf3f019eee6a4dc8be3fda85e5ce15cb535a91a424c9422addb7"
            "d26c625611df085a39013b9b7534cb2d715273ac0328944dd39b1e93fa7455"
            "b63a8e4fa2de962a3f950b0000000001b5b2ab4c83cc7d7ddc47686b7496da"
            "2f8b41a175a4584123efcfb4786e29da060001c1bfa72c7fef883fd72bb133"
            "9f5fdf1419dd514850edd5b7737563d5d47b7e0000000001f1294ce1ba5cd0"
            "625efe133008fbb34b61f8fee5fd47e84ea1e3aa6faf72d039018e1e8c9349"
            "3395a08a8d30ba349a6c858ca13c4bb34b4f214f99afe7e420452f00000000"
            "0000000000000000000000"
            /* ironwood tree */);
        std::move(callback).Run(std::move(tree_state));
      });

  ON_CALL(mock_orchard_sync_state(), GetSpendableNotes(_, _, _))
      .WillByDefault([&](OrchardPool pool,
                         const mojom::AccountIdPtr& account_id,
                         const OrchardAddrRawPart& internal_addr) {
        OrchardSyncState::SpendableNotesBundle spendable_notes_bundle;
        if (pool != OrchardPool::kIronwood) {
          return spendable_notes_bundle;
        }
        OrchardNote note;
        base::span(note.addr).copy_from(*PrefixedHexStringToBytes(
            "0x037f9054dc0deb4c01dfd4b6c47aec0d2b57f9260b6d35f70f2cccbeb39b0dab"
            "9b253cfaa51f12fd428a83"));
        note.block_id = 3459239u;
        base::span(note.nullifier)
            .copy_from(
                *PrefixedHexStringToBytes("0xbe914ad4e52c822c2252cc24cdbd344298"
                                          "276a6b7c083efd9dc58944a9717925"));
        note.amount = 40000u;
        note.note_version = 0;
        note.orchard_commitment_tree_position = 100641u;
        base::span(note.rho).copy_from(
            *PrefixedHexStringToBytes("0xb57c90b512efbcc94098fe4df5c1ad5cf1b1a7"
                                      "e23daa64cc94f245235a081a12"));
        base::span(note.seed).copy_from(
            *PrefixedHexStringToBytes("0xd695030000000000d795030000000000d89503"
                                      "0000000000d995030000000000"));
        spendable_notes_bundle.spendable_notes.push_back(std::move(note));
        spendable_notes_bundle.anchor_block_id = 3459275u;
        return spendable_notes_bundle;
      });

  ON_CALL(mock_orchard_sync_state(), CalculateWitnessForCheckpoint(_, _, _, _))
      .WillByDefault([&](OrchardPool pool,
                         const mojom::AccountIdPtr& account_id,
                         const std::vector<OrchardInput>& notes,
                         uint32_t checkpoint_position) {
        EXPECT_EQ(pool, OrchardPool::kIronwood);
        EXPECT_EQ(checkpoint_position, 3459275u);
        EXPECT_EQ(notes.size(), 1u);
        std::vector<OrchardInput> notes_with_witness = notes;
        OrchardNoteWitness witness;
        AppendMerklePath(witness,
                         "0x1ea51094434bb0e6e016c826177e3a6e21560526eb555887acd"
                         "50ec189366419");
        AppendMerklePath(witness,
                         "0xade4bfcee33d94856c8f4a82b3e4d0c8a9aa7f5adc005193943"
                         "0bfd9365c3907");
        AppendMerklePath(witness,
                         "0xed16f252146432d1fd582fe2bd1c789aa24470d2b8cf90cd37e"
                         "dc93d9c8fa43e");
        AppendMerklePath(witness,
                         "0xe3cb6399027d41b37319e45e3fd9c9c1e99caa37897dfb55752"
                         "0cd7817bfdd0e");
        AppendMerklePath(witness,
                         "0x3363ea69fe7acea0e738af744b6cad243411a3504547adea620"
                         "e43c197825700");
        AppendMerklePath(witness,
                         "0xd3c16a51a066a2b4663e136b5205d451b327da86a7d71feb6c2"
                         "863f9fe4a3c06");
        AppendMerklePath(witness,
                         "0x6a255899a41d43144ae3eea9d11a1c8156388a24ec51f08815b"
                         "a2aba59b3b537");
        AppendMerklePath(witness,
                         "0x269b75aa2941216958a5579f2def35e711ba3eb0b37e2275cb0"
                         "520934adb072a");
        AppendMerklePath(witness,
                         "0x7f938e1bd2cd570bd64eb69f418fadb5cc31319263dfb14836f"
                         "e5aaaefce8710");
        AppendMerklePath(witness,
                         "0xccf5b808d602c05deeb3d0533726546356690aa5aa42129861b"
                         "88b11aee10417");
        AppendMerklePath(witness,
                         "0xa3c02568acebf5ca1ec30d6a7d7cd217a47d6a1b8311bf9462a"
                         "5f939c6b74307");
        AppendMerklePath(witness,
                         "0xc1bfa72c7fef883fd72bb1339f5fdf1419dd514850edd5b7737"
                         "563d5d47b7e00");
        AppendMerklePath(witness,
                         "0x22ae2800cb93abe63b70c172de70362d9830e53800398884a7a"
                         "64ff68ed99e0b");
        AppendMerklePath(witness,
                         "0x187110d92672c24cedb0979cdfc917a6053b310d145c031c729"
                         "2bb1d65b7661b");
        AppendMerklePath(witness,
                         "0x3f98adbe364f148b0cc2042cafc6be1166fae39090ab4b354bf"
                         "b6217b964453b");
        AppendMerklePath(witness,
                         "0xf1294ce1ba5cd0625efe133008fbb34b61f8fee5fd47e84ea1e"
                         "3aa6faf72d039");
        AppendMerklePath(witness,
                         "0x8e1e8c93493395a08a8d30ba349a6c858ca13c4bb34b4f214f9"
                         "9afe7e420452f");
        AppendMerklePath(witness,
                         "0xbd9dc0681918a3f3f9cd1f9e06aa1ad68927da63acc13b92a25"
                         "78b2738a6d331");
        AppendMerklePath(witness,
                         "0xca2ced953b7fb95e3ba986333da9e69cd355223c929731094b6"
                         "c2174c7638d2e");
        AppendMerklePath(witness,
                         "0x55354b96b56f9e45aae1e0094d71ee248dabf668117778bdc3c"
                         "19ca5331a4e1a");
        AppendMerklePath(witness,
                         "0x7097b04c2aa045a0deffcaca41c5ac92e694466578f5909e72b"
                         "b78d33310f705");
        AppendMerklePath(witness,
                         "0xe81d6821ff813bd410867a3f22e8e5cb7ac5599a610af5c354e"
                         "b392877362e01");
        AppendMerklePath(witness,
                         "0x157de8567f7c4996b8c4fdc94938fd808c3b2a5ccb79d1a6385"
                         "8adaa9a6dd824");
        AppendMerklePath(witness,
                         "0xfe1fce51cd6120c12c124695c4f98b275918fceae6eb209873e"
                         "d73fe73775d0b");
        AppendMerklePath(witness,
                         "0x1f91982912012669f74d0cfa1030ff37b152324e5b8346b3335"
                         "a0aaeb63a0a2d");
        AppendMerklePath(witness,
                         "0x5dec15f52af17da3931396183cbbbfbea7ed950714540aec06c"
                         "645c754975522");
        AppendMerklePath(witness,
                         "0xe8ae2ad91d463bab75ee941d33cc5817b613c63cda943a4c07f"
                         "600591b088a25");
        AppendMerklePath(witness,
                         "0xd53fdee371cef596766823f4a518a583b1158243afe89700f0d"
                         "a76da46d0060f");
        AppendMerklePath(witness,
                         "0x15d2444cefe7914c9a61e829c730eceb216288fee825f6b3b62"
                         "98f6f6b6bd62e");
        AppendMerklePath(witness,
                         "0x4c57a617a0aa10ea7a83aa6b6b0ed685b6a3d9e5b8fd14f56cd"
                         "c18021b12253f");
        AppendMerklePath(witness,
                         "0x3fd4915c19bd831a7920be55d969b2ac23359e2559da77de237"
                         "3f06ca014ba27");
        AppendMerklePath(witness,
                         "0x87d063cd07ee4944222b7762840eb94c688bec743fa8bdf7715"
                         "c8fe29f104c2a");
        witness.position = 100641u;
        notes_with_witness[0].witness = std::move(witness);
        return base::ok(notes_with_witness);
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

  zcash_wallet_service_->CreateIronwoodToIronwoodTransaction(
      account_id.Clone(),
      "u1ymq8s3fku2lgecm0caj9wacyhp5q0mahrseawalpjgyxl83xaq2lmxhqhf2d8l8zftwq7h"
      "zz39qr27df0pylftghnh54umhscs07pa0t",
      10000, std::nullopt, create_transaction_callback.Get());
  task_environment_.RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&create_transaction_callback);
  ASSERT_TRUE(created_transaction);

  std::vector<uint8_t> captured_data;
  EXPECT_CALL(zcash_rpc(), SendTransaction(_, _, _))
      .WillOnce([&](const std::string& chain_id, base::span<const uint8_t> data,
                    ZCashRpc::SendTransactionCallback callback) {
        captured_data.assign(data.begin(), data.end());
        auto response = zcash::mojom::SendResponse::New();
        response->error_code = 0;
        response->error_message =
            "aeedf7db7e1e50b44cb73a658e57751a4142d7ec1c402fda2eb038e617de3987";
        std::move(callback).Run(std::move(response));
      });

  base::MockCallback<ZCashWalletService::SignAndPostTransactionCallback>
      sign_callback;
  EXPECT_CALL(
      sign_callback,
      Run("aeedf7db7e1e50b44cb73a658e57751a4142d7ec1c402fda2eb038e617de3987", _,
          ""));

  OrchardBundleManager::OverrideRandomSeedForTesting(
      kSendShieldedFundsIronwoodRandomSeed);

  zcash_wallet_service_->SignAndPostTransaction(
      account_id.Clone(), std::move(*created_transaction), sign_callback.Get());
  task_environment_.RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&sign_callback);

  EXPECT_EQ(
      "0x0600008098b684d85b16a537cfc83400e3c83400000000000002655f64786168"
      "b91c4cd2d9c0861d0e5e391736fb99d1d68d92e18d1130cca7b9adb4984d9c79"
      "e708f24f5b18bc86b1b3df70442cdf2697af4055b9d9c4fd982a401ae646fba7"
      "545ecd641290f13d74d102be3025621aa90a4ae85721cab074bcaa9d9bfaf0dd"
      "d78d9afb1d9a680d8ee849758deff2ed1d3e00365e5bd70daa21166860b22f6a"
      "ae354607a3c2166e63cbb47a7ea6058292bc4ed56fc25f212b3c606874eead7d"
      "2837aa89dac44a35eb94ea70124c0592cd73160c308ad7f8e456ea5dc0be8fe4"
      "a6f75083fb613ddf3bdfce7062515d7bfb37a4bd930f3129083c8e26eca8163c"
      "1117933a892a2194ddcc76d97bb398a6ef60d4c30de5c113681aaa68ed776c13"
      "4d6d751f42b24b0192ed7204525e6bef09d5c7b0de4d45042ff4a9b73f2cb975"
      "113fe1f0dc399e66fe5f6184b7330834a3c4be2c620e1b9856d5991575bea8f2"
      "b09374c882ed5d41ad7773950354636c0c4485ae51aaa0de3bdd0d9dad20b345"
      "fadbd17eaca3d787f13adffa621a7de981cebf3b6e7d6ff0b27b98c0eec16d58"
      "0dd86d3e97cf374f834316c2c7bd5c65f5289805aa93ba9690fef360675992f9"
      "677b7b763e89e9b8b8ec6da02a14e61df18aec7e064cb7714b152743f2446099"
      "c28b575df923ecba7f2a6033cd7b8a71751bea9cf22b239eb79e18d6342655a2"
      "2a6c6b9112849d0d191fc097526da0eaf50ea92e4046b2ec6d8b6c5366a3feb4"
      "b39180d1abc4c7da1d6676e1d9f49b741e6807e9bac1b69da4a5652c6037d269"
      "4acf55dc2b5588a003035cd6f1dd2e53a6c3383870f1e83f3c83958e12d84e25"
      "5e9a3a91991beb0f5c68c6cf947a877b3b91a0f853650183a76f675d468d33c1"
      "8f774879a5adce5314b8569b12d1844d778b88b338799070cfbda34cbc2c2843"
      "f6d1189e20b2a7ba733311f77b47c50c350479b7fc4fb9bbaa81a52d4d3845e5"
      "753a90c571e4d6707e29576eefdc71eb080d384a47d2214774e89129fd14f787"
      "33f083e941fad2112a845a02a6b056909764c1e164bc558562196b77bca10d99"
      "71899b484909eadf3aaf51fc9d6d305c3e2cb22f8604727b6fa922ad249444b3"
      "0fb0c064b49690fc22f6ad080bfeee420fae0909d0aa47423af27b8ad54d6d47"
      "a76d94ca9f20b5aa4a51f1f3c24cccafb99dd312e94cfa20abbf0a1d7e72c38c"
      "3b35adba7a206b4a6bc1ee0b1f94be914ad4e52c822c2252cc24cdbd34429827"
      "6a6b7c083efd9dc58944a9717925e1bd7830daa0de5b38049788748fc19cf613"
      "301f5a7a5b1c87f2e7fbcbc64e839e2952115a6d7211d6d56b6eb03f0d3e18fb"
      "b5224b06d2253792cc953691583799ee318de039a64a74c3c1a3107394484d30"
      "074240e6648c7daff7dfc5ef5621408636c00f41fa7fd71343a2ca63c1933780"
      "8a2ea1f45ece6a7dc062bd653f0d27ab8d6a2b863cbdc60c21d543a03106b77a"
      "76308513bd084e18e0bdc508da964c80a109d6ca5a166fc54700ccbbc237a764"
      "13263f1b7ea9c33a32f97091f0221e37fab3131690199b5e0162f48a908f5f06"
      "e300c8b12d310b1717f5a6eebf28d9578a62333e255cd9e7db15a16c7276e803"
      "a1d8580b837ee78bbbc21501ce520ce4f73d7005738cc86de2dd2fcbfc61a3f7"
      "e35d65447665845bbb4b3fef73f62200a58ab29e3bcd2b1cdb348e4f6097ffb6"
      "66bbf2632f6d7bf925a74e7a16b9b49ebff4dec60f0cb4a73460313d28c4afdb"
      "e638d3cea0d608f74f59476a3707b9e4771ff06ab214eb141578dc0bcdd5049b"
      "1e6570c7fbf9a76130986592d26eadb3c7bff6158676681810976d49f6347790"
      "a7489e671787fd7e350ad390eaab43426059930df4cd75201a2f7b75196344d6"
      "c196a20f7c5111245f963fa268e39ac579793fce477f9b0c30e0388d1f250048"
      "d63d9adec6020917574176affc6c4853328208cf8a0f8f415953a42b1fa7468d"
      "506b1f026d7ab3ab699deb83fc23a8f6120ca9730a5828b1c8b4b4cf73a7fe4d"
      "d868277378753a338e95d57fa94650bd8f070c00031294dcb380116b240a9e71"
      "9fbdda80afbea3885dce6faa45c97310a4ac1ea6b883dc8aae7ea3940d9d0c51"
      "81234a437020be4bcb138766e0b16c67155b395f65cb425e4ba44a545a70a496"
      "cee549b03d40dbb1104c21be2ec50730056e038eb1b6c78be6edb96992ebd880"
      "a3d02690339142517f033fe8ad25d656bb251c0065dcdb2ad4ef896321327382"
      "5c2059ae08d7acde6928134d5d288e4752dc56552acd242cf678885b10e2e8a4"
      "e69ecaa373167244a9c37fb0f42373b4ea885b1ec3883ed8125ceba1eb108c7a"
      "4ed1071027000000000000393752bc8e3cf11438c1f1b4aa62d11e20c8e934d3"
      "68cb578958e2451bdf0d2cfd601c17e951dd28986703bfd6e81c198c796d06c2"
      "b2c48abee48564849aa4844c7ea892a6c73ec2c0900231f43295f3000ca54fde"
      "4c08035894df3cbed67aa6b89e9aff00bed0cffbc6ddb7b704ae4ad56d230bf7"
      "07d1c73b57ee28ba6063fb8efe0069ed59135cd29a524c7cda3f821b5ad42b0b"
      "f2cada4ba822d087b7d54727762f2c96d365cfc5b32634d054c023f41d1c8970"
      "9df5eb49ed319c34e45351e35a208445d3047a82150363c84d7945da369aa076"
      "e855e4435710675fde830b1ae98369dc010aa09256e4cab33ef8ae1cb72af2da"
      "1eb5595ff020c69da3d1227b63ab349a3520f9a03aaa6eb991505d1e723c9107"
      "612a1b72f8b7155866c77881020117afc0595db09eae37bc0cea4e50d3885061"
      "d643c938aef0dfbb73a504bab92ce8186ed991ae62d49c3c0eddc34780e7495f"
      "7e8f8a4091c4315c4d5f3f94e8041a812d522a2f19ce51706214bc7c264ae3f1"
      "ddd643c4f51bc32cd77da794b422b49c668ee13dac581c5f4cdcf2f32419f9e5"
      "e64bf994130b133d92942a2309882f627986893b40c4a7fd3b4e38b2de923c3c"
      "0c5e14e40b8a429d45fd4b53d0ae301e809b47bb99888dd48f2e92d674020086"
      "ca469c44998049ad3f8134b0d484da99d711353e346df92951b3677b311b1096"
      "c2458b1272f3d591b5276c03a239faea2aeea88398608c5ea6c1260d8c62bcf9"
      "37bda92f123fb0a2b379d13fa3084ba59fe27d8eda992d47abcad17a46e1aea9"
      "68ac6c264827e46921a68f5ce7ba27e398f53954df0215a3688d1911b86d24ee"
      "92b97423cc219af12836aa2f8db4bfef9359b04ed84f82b1787fdd6abdbf9fc6"
      "48fb84f27b5fee0a529ccf6c3dbfb65b03fa8a63220e6761543bd50fd03456be"
      "41be98387fa038cc6a2b2b2140a04bfcd780b7155ccea4d379152093b2495c50"
      "2057817c67829de0891b344c8b0c03b06924d3bb2c4c41d5c9ab21e724849bc2"
      "8cb66471d273f6eb4637dd45a919b60312a80544e91d4eff2c1d5f0581414c45"
      "eeb8afa3cf2baba4f3bc26882f3e356a18085cbcb42c4100bb93bcec9f6ee2e5"
      "37e626a5c52b29d0e7a26530289bf1e2e1bd45f9350ca3cfa4a896402221807c"
      "ae875651be387a69e10dac31099a14f3e631c7cba786e9877b44b94dd922042b"
      "6757396a1001dc09c0d0b9c44997a6eac556c9f85d3558799e175429c75b901a"
      "c0804ca28a6ce1733b2f58acf287b92dc26b5404d50bb1b8aaf9d79bb05aaeea"
      "a9764a5fe1625e782d1bf5583ea3bcd6971d64bf08e8c8f49fb2d7130fdf3808"
      "07749fa826b212f2a884da326610b30b2f81c6ceeb43d988b0957fdfdbc46ee2"
      "410d374db9890429c597bc1057b2c96ece4dd335a06558b1e99eff98d48dd78e"
      "8514bac38d9350bab49cd3671b3bf95c3dedd72b8b8640b21bf9ab4c4c06c8b8"
      "9ec0de0032362460bbf59967b49d0dfc7b78e992fa7352aaab66f87095c08ba3"
      "a3aca392a871b9ed5ee83d71b08f662f3e864e653e5938a7c269286782f1f787"
      "97c5c5e5342c94f1d47423d160836aca4131f13d1f20a4d28719dc5f85abb0ab"
      "69afc8e85cbbe26b460a5f18278029e4eeaf182fe7ed30ed5ff92f22ad32db95"
      "7a81c32b78726ecfd8f02a2856b8ffcbcdf120bed1a132b5116a25287b27410d"
      "9601800cbc9fdd18f6f4c85ed38bd44ce31ccebbc83d8ca1d088bd5da1f70732"
      "69a8a950af1f457165bd2575e61dcb02e3171dca54a4d212740f68e2f0206752"
      "3f39935941416deb9b73b68cb3183358094b1d1f22b774e6464ba4e9261cd4c3"
      "a065662eff9d656250c099cca7b49d90970f3591e5bf0373540b9ed172b5e2bd"
      "7b1dad39d252f99d05e33daab4281065af1cdb88c0f4133df9d788ce5169d10b"
      "fa77b6ab51770fa7e0f43d6a9482d3be91b5a1b1673c3bc45f50d4ad3a83773b"
      "87941c8378ce19aa1d0d12b0eb83618d5d9565ca919593cea1ee39d8db4c6921"
      "11546437b81d23e44e12a1c1021310eef8dae67c8361c2b2a4edbfb7391af3bc"
      "3c875018f56d4e3b0f34cf7dcf293db4577a9c73f27288d884b0a23dd6d73148"
      "c5163597221ed9b5080a661f1c31aa9c591753a493aaf1c84d315f8c0acbb504"
      "1e85ecc0ea4610199957d8285c0d117c900f72439133f6f413bb20c124df5ed7"
      "bbd594a468973b91050f41e818b5b8e9e51dea3f7a981e1f1e305039685039dd"
      "9c2d823d800467d92325795237b261a7faaa0ee427ee21ebaf90ef2426cc086a"
      "4198890cf0215deacc09faa488887b4bda439691526a1a4f1b2b7aa18be175a3"
      "21407dbd779efd89ec9594ca29162e50cdc5cebfd9599fb6bdefb62f5c69dfcb"
      "ebc4118c5f74d64e263632c1740337fb72b589e25eb5b0753809535feac4f69d"
      "007aeb507763d9ac7851e312d03af1541c267faa29275d08a5e3c970d61d33be"
      "47cd05ac00ca0fe0245a757e133625644a929f85b0361f2b81e794104fce80dc"
      "012c5e4238ae3039161eec4484094928fd9e6f529a1b6df05129c5e3cee81e44"
      "a653287631788b77098287ce72013dd6ed5a71ec6eb8df830783bcba5fa7add7"
      "032b3313dc4bad1ffad05345c71f108d51163ac9cb63b0073d21fa2e1abaaa36"
      "ac64cf1588672e59792c4e242202b27d4144113e09514e2dcf607ffe1b83bc92"
      "ac105a9548a7bdb919af86b7433e808154365804f3420b1d8de421db0d07996b"
      "729639caed1eb84039b4c42bd105613a03508cb9c2c8306d9d067962a25ae7ee"
      "5f18b6c0fb4efa488320a42cdf30d9200e75a74b5e97496040e0daae6ee6cdc7"
      "f4b168cb9400e9dc8e439f513224b5f184f250b2b3310a71d06c7858868a296e"
      "190df9ee1c07ae5d017ef7ea0a06c959ef78ecd25b06fcd770db6746b10922bc"
      "8ec08fd68391c8af7e2b7daa262b0bde751b1ed462bf4f56be2cb8f727f47232"
      "5f4458eab6fc031467018db4cf33e084c4f0b5954bdb3883999c8a45b0a41d28"
      "ac787a325b39b29191c78602f21b09e6ac861289a1d0bfb5f8315c5131d675ea"
      "b35ca9e34e47051ff3f7d4c8ac0ae63d38aacf5318bf234a67b90d69867c6d53"
      "f8e9ce6e60baa311e7bbb2f7932480bfcc2c7f08008d2fe7751af5e61879a105"
      "a3f48fd77cab94a8f0ecb3c6ca064e4d94b5f279c2ac7a553177c9c95c746807"
      "197c14d28703ad5cf04244119d3317dfc9711ec8d13de09ff0d6eaa8173e8c58"
      "6178dfd562b2e93c56f778bde001f46d6ce1a5eb57f165f2c8710a87029a874a"
      "2c0ad71a4dd1f84f4a97d31a32185e81a611b4537393e17f010f84afbea38793"
      "c9c32a2cc4184f1b8edaeb9ca9307a4e57f55e749a9f73e2e419f873a5ceaaab"
      "1d4d97c361488c9d788032fcff1b0435dae4fc6cfdb8ec91c9c558d66b4d30e1"
      "02ce8eba5ae49ae8575aa191b207f7097c2972c73b3b305a10febd69a17a34db"
      "8419154668ab3410f5586f49a63d197c06f588fa01d024ad7da0684a40a4d688"
      "55178d13ad06dc928db8759e6a1b04f5c4b36b8bc3e972bd3e8199bac6548f00"
      "2e9614167a4328a8ab8302ce462f20142f295bd01137628d515e6f2123dcca91"
      "a76de65e3e8f2b9d9f7f64448f1e570ebfebfb178914bb0c8f192578ccdb9970"
      "992b3d99742ce500d4509114f00a6747672a51bc9247ce61037f78caef13aa55"
      "d7338b5c0524110c12660b305b2b33aa2cca4159e9d395898f303e6c9694cae3"
      "025d5efdb2f8a3fd9f4ec6e9ae1b16f2f37485cd314cd69f43b4d9abc5e34be9"
      "eda38b94c7ce0c4ac0b54eaa33298896a083e7038ea2680e766892d518b85dff"
      "22a43ba11e09904d2b46283ba7238b1235db0b6139fe544bb4874bcc1e562b8c"
      "5f093eb9790eae855e7b2cf0473baea99d535030cbfae59dfb5090ad5ac546ee"
      "13c0c2e67908f7a6b1e8b6ed7f112851712d8762a29e771d2966be94b4a1a285"
      "6ef7de32a4ccefac15c3e858bb075939f32fb9e0149261dd83ac450c09ccaa6a"
      "fa4841a2d5a3d41b6759592b7a266143653f543b03a4ff8684bebf24c91d1af5"
      "79dc4c7e05f8ff9e23055b69a827a617b7fb57a6a9f5c30e969e54f807c7223f"
      "a4502745b25be0d42f70833bbb3f6df097b689faea2542f3868567ffbfbfa710"
      "97eab64b1ce81f6914466fc9193fe0b38e0510c3dae70a881ba2291047ee637f"
      "d80009b70b0151b9ff7fb9a0cd2c4a6cd6a3825c703f018156393023bfeb29c3"
      "5312274787f287c72b677cc25513cae36597e46e1d6ec62bcf1e9c42c7904889"
      "ec4eab687d5060494c4a13eee62ef91b99bec8826761812a01026e2c09401f22"
      "aa6e83bab65dbe360a83c6abb937be764ccd1d49d391bc602f5689d1409d3ad1"
      "3014f651c03eb81937fe4c4f570333d16bb3af688ddd09c8f38ad7f309c46ffe"
      "cce0a29ad7903e8a31ff2b29e630d6ae5832a9c8574587b35ce7de493b926e0d"
      "03ede292ca1bcda27d836eec0228263c0f01f8859192b75ea4b413569e2e2633"
      "c94260f40f334c0fbec8ffd1eb074a59f2b12d58c6239871e0c8ad6eb800c07c"
      "9d3422d55b59261483bbba90ed0bda72db11c5bf3b303d6135182e52e6c0e4ef"
      "df9a051495f64c0ef786f591022985ac11eabc0c43b87a699fb093b8a8d90cc3"
      "95a3cf8993623a3989ee8b465a2b895f318cddf6c4abd1eb481ec1c28c29ffe8"
      "8c2228afd51a7ccb76d12e62bd06f926c6bc96d792680cef1130673fdbf464c9"
      "564b47b0c78af9ce4e8fea879417b6188e6305c88690cb1be1112801b8995365"
      "94271fb5b0d25df2d70bb443410a80c80348f1b6a06138a0327af3beec5ee42a"
      "1550b317881c025ccece92eede33576e21244a258a5e6b77c6f3202e80464b4a"
      "2a681dfcc892dd926fd5d55b30281a4464367021c0503bea8951a3c369b96b6a"
      "167d5acd0c79e3337ac9b39379299259d17492edbd9b6a8db6ab7bfdbaf2999c"
      "719c41742f5fb18391007e449b0f5e3d3805b7e6d611906e6e64b3a0edf3aff7"
      "7671f6ccd227ebce684dbd0fec089dafa4514b754119d0b1222b90976568b2fd"
      "3bc583d9cdb9e4fc3c3e4038453696e313eed5518a9fe9fce70a410738e804c5"
      "cc31286f52c2e9dbef117212ea0df95dad3b33ef5341ad5c079befc3127f1f00"
      "29af2e1964c26c80a10b7ac6ba0d5ea97e004ea8e0d8ea27a3eedf8fc2baefed"
      "43091805b19367f243a4e68abc2c23b7c0b3d1f2062e66c5ca115a8ab7e1d454"
      "29bd16e6c16f13c16f4c8effdf0703ff9743122d70ac4ab522ddbec024e381aa"
      "db211f8c226d7d5a32da35b04301ecac8e3dfc92c67a938786f61aa860adf74f"
      "d68514c54b801b1b030e515ed81b0e62d2f2a5503be34ce44d0ff6c95c432bba"
      "ff2bc92518675024b83c38cf1f345277cdcce4d65dff7419420290c2d9b9c129"
      "3d9dbf7d0d566a6ecedf3b186a04223f4c44c56bc46f1daf52b7d780cd7e278e"
      "1ecd7d23da10fe2eadbaa24a443b97c507f5980013545b29f00e4d1bce6701d6"
      "875974b4e5d5f24e59705084d829ffec03a7150ca813448c078c44139694cb01"
      "a5d4dd53c9cfa7f8ee6b04feef2ce24a6c4f32272f03ec4eff2cf3d425f455fa"
      "30101a7cf6a8a7e715815a9b6d22100f200be4a28592300d2abd8930cc512af4"
      "9dafaecf0d537bd24b0e2ef5763b6643743c3a0ce66b3d41e03b2214e079405d"
      "ab2aece350bf72fc393bb9c034186cc7a7315dd376f3ac99339d91f704d019dd"
      "0a5096bbacd47282674eae223908c1f4d9ff7360b34510b3c4c6c34cceafcb74"
      "84bd45d04fa743f9795c66ab61041f2230fb546b74bb691318cd3024a3a0d5ca"
      "2e5dab90fffa91138d7712237112d46ecdbd8df804d83615e781cb37da6cceb2"
      "9266484020125888c64897c64432009a4c92a2dfd610a424724de884b450acd7"
      "1024fafbaf290d834831a0defa39a943897e3b0ab467b83b23ffe4bcdfd7014d"
      "c7e03bd7eff8f3c967852c72dd21f5d71b4a50a340927ebde74b0e922d2131eb"
      "8265d8d9386a03e95f1c1fa68c2f532e709485948f0fb67be9c0e0c5faf320a5"
      "d7a83e139d1138c5d822e3ce6d0a21e1a69cf2090a8f499af7faba3ccbd0fa7d"
      "58094e81c1f070752763f92a222e488f5c7e7787075850bf0e528b300d3fc765"
      "4643bd57da8b41efac94e1e4440d62ee5b017aff9076e2121b2ae4dc66167a99"
      "758b73318a0efc07b1c70b23642706278d042a766e4083f2832513e809130cc7"
      "42e49d940874d05c4017a92b9c3626e02ea0e40c7f573ede79800ac98c63f3b9"
      "2f3b8b927049cb48a44c7c46f730ef2f8334eb853a27fbc9da48586b9ee913e9"
      "30fdbb445daa7fb1aeac272ae60612375079860a04f4ae4c1e57f404b3213ec7"
      "47468354a6ff907e426718dab32f8c6708bac0130fc43e4634bf95e0b4e9d32a"
      "8b4ac5ee152e789224160524760e551c789477cb48455d6fcd9b46d92588d719"
      "d083cc7bfd68e24962095638fc21efac25b51bfeb0ede16f9220c8bee2099fc0"
      "c2c49d1a1ce7f5fcb509fbf3071cd1c0daa099fbbfc36ad6388ca441c426e77d"
      "610c3971e86a9609c1c23847b71a24d740e23e6b68cfd57d70a7ed276716d047"
      "9fd430d889319bb902dd44b4ba16bc0f0c0566467d712b6cce7a4a9e437748d8"
      "1f98f2f34aa4c9fb22c533f48f05d471f5255c90b9ac52ba4d4e181fbba0c808"
      "dedf73199961f618ab31e02d1720f20576ffd88228d0f55cbdca473ab83a7950"
      "952855177e333e080e0ac8f4342f938022d5dae4c48aeebed68532ce34b0f900"
      "72ca6150c4a10d68d06d87377f160ad63edee837b01ecbd0e15c574124327a70"
      "b99cf3d0ff6fb6645041b4a97e2a2a1b92230b4333e4e5dce15eb844d3292ef6"
      "d2f2af0b5b873a321ac9459b593b64d53bf5630548811cf98e9ec295f3e03c35"
      "c772cb096db4acba33df443af60d88ebe81cf1602731759fe0efd18e9bc0ae76"
      "4619eea7e82267a97e45006a0b2e01adb649167431501cf18f31d18153bbb789"
      "09ff23d633f8f611dff8d871f60f74d546a98fc62cca416e92160e48096f551f"
      "2b6d7d735458d87a0f8de432fe07bd52ba30c8062a1f581e8521874983672eb2"
      "3b2800e02509baa9fca19672ea3551256cbf2ab3fa6a9f3628e20948e315c2c3"
      "7404d98c4663287e94b54573b62633304c50b3c6fb6902587968a69fdf1bdd36"
      "7a9c25349c986b2f443f17960432543ce9698b7d3d9b5d6e5bc0214548c694c7"
      "aabf24001b61a626329fc6491d00f23317df3721c73b34f8d9d1fa73f36b832b"
      "dec1dc590dce80bd545ef4569523a659c6a86847d5e682cca5049cc827324790"
      "1016635a73d34773855231beff21d2f3a9310ced0d299d3f59495690374e58ce"
      "3da6c679c1d6e599248acd30390c6fb49a94b52998cfb67b1403ce65fc489a56"
      "97f5de171ce21c1abe7c3fec6b35f1058d4779a16500941778b6b16f8e9a8776"
      "63be3a984625872aabf9ca156812763e271e40de5b619635dce6763021d5b524"
      "630bad74cc6c11c1375490814d308654bac0ea08197667bccc8ea6c52e54e56a"
      "8517657a9667c328c1ece144b201f4afcc904411dcda11626a2e70244cb11a4c"
      "aaaea1adf458036345fccb75d33b4d7e10b182f18a65086b2223078dc6eb6789"
      "a1735bcb0701b56ebc08582c5f0f5f6c71c6624a3bed305b7040261f29b8633f"
      "f7581071bc724f26d6562499d42bd304dedb98697d2a6d2b1a311786d9935249"
      "e12c8605fedbb398928971ed780fd271fc8133631ab14f0fbd36dde5d62d5e77"
      "0f808a144ee4fd49dac6c69d393d87c492a937a3916a486d1f96c2ed195dc347"
      "b55b5d83527230d367f49d4705270c3f6b3bf2a1ef7c238402bc45f563405527"
      "f12d0e14a741591708ab7945d706ce5f4732e9496bfa4bf80fc618399aef1709"
      "3fce1049d9d52b5ba6e971ac122a843281ad5441bd3ff0e82cda90f302be7d10"
      "959ed746113a80e0a82299b7f40a87f5898a9bf358d4b6f4329ec36370abbd42"
      "f47f28acbc5965d558081421343da1b19d2423f1d65997588f179d0e4d3adde3"
      "3546bb9885ecc5ecf90d8e620914d23238ceefbbd2dab979e901594ba16fe60f"
      "051335c970af23fbd36cea3a1239cb98f7f7cca7d28f979b9a3cd3000d14b9cc"
      "ae3d5fd925ee18fb7d077ac27c092c3bc1974362678cbad8eb2b3b8664a47af9"
      "75a4f281e4d5b2924327386b3521717f17e1e1ec3d98ced4c84846f04d9176d6"
      "629b2262b3e9c2f598aab687242b4a3cf9b6fb95a811ec084c3915f5b26b3cd4"
      "889b6b3ecd7a28a0ce212e61e00cff325ab3c6e8c9b2772e4df99ac4a2bef2dc"
      "461546f8ab066b53c62eaf50b834c19d3c9e911410c275f9f9215f8e3dfa685a"
      "dcd11fa6607f4c78923586bd4b3709ec88bc3f994c5447e7904ddb8e2e061ef0"
      "d1272163407ea4f97cfbe5766b02d7dd90f4ea44cd08b3a468421bca49a79cef"
      "df25f383e17338255c1abd05650fc89db75588f8994e94029e02844871b1a25a"
      "c55ef24d0240652b5f1726c94309c6a6487304f9beafbf82c6ef1c7d248b880d"
      "cd15dc286c9fb0b0d5882770b20ccf962a5792874d598d7405dc212fd40fe7c3"
      "4660bd3701d0d14450cc06ea5f3f1de33b95dcef0d620d78c95b0d9ea0948a63"
      "db713bbec09307fb7740c7bd141655043e6d4e3d29b4b692974005e6e9f52fe9"
      "a4cbf4aa25aeba14ac997f93f120c0eceb028becac28a3d055f8366b07ccddf2"
      "94aaa76e0edfebe4bd147628a006f1cc60b67b76a9afc492dcf53ff3e9edcccf"
      "3bf5d6900d905a9d61f333678a15b271fac8cd36680ee64a4809facaab6eb7b6"
      "b6134450b8ddba635dd3fe8bc83a7eca7537e3a5e173828881574d5541b878b6"
      "ebbf591c7e9cff592d8d91e6042f5186f231587ed72af38f583540d0a2e29192"
      "455720a069b2aadd8de138b6e40ca9a66bba48338eb247d62ee073e378deab6c"
      "5188aa989047cf018fa3705860b5cb3146e6814e1642955d4490b213aab01858"
      "a7158e33e6d23d8f58cc3d80fe1ae6294984f717000e812eb29f0d121acedad2"
      "ef79408750334bc85e10df39a52036ca591cb1a27506513976bdbde1aa0015cd"
      "d808b9231edb56683a258402ad0dff0655029be9104dabd780dba73a3504d99a"
      "24724e886e7ef6a2426d1981ca0945543d167f8d50c58455f4224f0899f1afbd"
      "58084eba1515a0699308956f3807dbb5e4af7cf397b8be17986d0a11976ff946"
      "6e698708484e6bc57649a83f4fa9e2d5c5c16547a2d343f4cf635514c4b36af9"
      "9745f8ed2ef03a2870fe3a999aa1df4c166162d927ec3d57c661ca1372c27c93"
      "5db8d3db0b86f259be8953e928230f2b52a6398aa7678e9c26411982e571c068"
      "78c3d455c1fe0ca7b973ed702693b3b0281480e073ac17e8142194e0d0fd07b4"
      "d9bddeefd18cf4c8e91035c0e69e61394ac81272c74661153a3f072e7b3dad20"
      "f6a635b10f47db6d97a2e5c9382276a6bdd36edebafd498c8bbd17059cbe3197"
      "e2527ce754cf530565b3bf80629918f8c790aef69c99bb4f9ea59811f21dcaf2"
      "9eece6740894728f719d699bdc8a49227f318988fb3034bb9ca3b6e3c84fc813"
      "ac7553c02400d7b4108423b4dd31377889d0d8bd27208a7121e7b9d7f1b35f25"
      "62f000f25a55af957b2792e556972e590b569d60c19080764eccaa42b3c679f0"
      "e23b7e241d58a52c50df281d808633e94b9889aa755094ddfe3c318b05957b4a"
      "ca24e602ab28f507d8fc57d960083903aae7c38827a18649f27c1ed1e09ef112"
      "cbf876f077988633c30a57a74336ff63649f8a13cf60bef0c28ab99cf81cfc4f"
      "d97b37974207c00dc24e4ee9c893805db50f63cb3da1d84da2eabb4376a9af80"
      "c148be35d61961cfdc17c069b71e3a9622c67a2940321101884475b21d9edfed"
      "764b4391d3addf4fc96bcfdda190c016293823a710a9cfc81454e27b13356335"
      "5fcd31503ceb86885b23b0190c1c2e191dbb69ab6c7c9eab0a5823a2de00b5e7"
      "a3bbb878b77d42369d0dbb09d51fe119975aa68308f921ea8ea11d7f26b9cd6f"
      "b36083cdacbac13194e8b8a2b69ee1cf0526f0d189beb59c8211cd6ba89572cf"
      "5de9677da53df437310cf59a2b849e83501a9370dc12bf766195d69add8165ca"
      "d0979279a1a435c9a8afb5aac22f44df480121d44692b8ddee8f8983d07dc131"
      "6ae14d808de3f74a94029238be2f7b7dc70030301a3c4f109eba245927010852"
      "19725f67425e498a95743ec9c6bd78113af1cc27b1a04fecaf37c7556f31dcd0"
      "0eaf39b77402d7fb7d04f108002e7b19010e918020d2b548560d11688c1c3b87"
      "d27c71ed9b7915b28bb7de992a00f946f20bd261a2db8d391af7d50d070d0f44"
      "8da80d5e73454a5298f7fb0c9f31c8197fbe5153b86f610675cb3d5c288f1f9f"
      "9b603c5fc668992b603d502a1d1bc046710ff9bd83fde8aa58848ebc826530fd"
      "ede956c94299de5fb36276feb90df29b0cae2accf2c6e4dbf0215e5c42876b7c"
      "2ff2db14deecb1ebabef4736ad17d8d95cc24b3eb28aa4ae40abffe71102a0e5"
      "7e9e30729c7958f0dfd88586709e1ec7dbe7955e2fa900ce62e3c61c4bfd8627"
      "92edf0af1f2e326024404d236f1e",
      ToHex(captured_data));
}

// https://3xpl.com/zcash/transaction/51c140f2a42f5d163c9fae8ed68cd54055260b63334af9b384f6872f8acd4712
TEST_F(ZCashWalletServiceUnitTest, MAYBE_OrchardToIronwood) {
  // Creating authorized Orchard bundles may take time.
  base::test::ScopedRunLoopTimeout specific_timeout(FROM_HERE,
                                                    base::Minutes(1));
  base::test::ScopedFeatureList feature_list;
  feature_list.InitWithFeaturesAndParameters(
      {{features::kBraveWalletZCashFeature,
        {{"zcash_shielded_transactions_enabled", "true"},
         {"zcash_ironwood_enabled", "true"}}},
#if BUILDFLAG(IS_IOS)
       {features::kBraveWalletWebUIFeature, {}}
#endif
      },
      {}  // disabled features
  );

  keyring_service()->Reset();
  keyring_service()->RestoreWallet(kLuxuryReformMnemonic, kTestWalletPassword,
                                   false, base::DoNothing());
  auto account =
      GetAccountUtils().EnsureAccount(mojom::KeyringId::kZCashMainnet, 0);
  auto account_id = account->account_id.Clone();
  keyring_service()->UpdateNextUnusedAddressForZCashAccount(account_id, 5, 5);

  ON_CALL(zcash_rpc(), GetLightdInfo(_, _))
      .WillByDefault([](const std::string& chain_id,
                        ZCashRpc::GetLightdInfoCallback callback) {
        std::move(callback).Run(zcash::mojom::LightdInfo::New("37a5165b"));
      });

  ON_CALL(zcash_rpc(), GetLatestBlock(_, _))
      .WillByDefault([](const std::string& chain_id,
                        ZCashRpc::GetLatestBlockCallback callback) {
        std::move(callback).Run(zcash::mojom::BlockID::New(
            3460502u,
            *PrefixedHexStringToBytes("0x72c481fd571c6b88c6275b7b819cf5b623fd7b"
                                      "763b8c7d4660014c0000000000")));
      });

  ON_CALL(zcash_rpc(), GetTreeState(_, _, _))
      .WillByDefault([](const std::string& chain_id,
                        zcash::mojom::BlockIDPtr block_id,
                        ZCashRpc::GetTreeStateCallback callback) {
        if (block_id->height == 3460496u) {
          EXPECT_EQ(block_id->height, 3460496u);
          auto tree_state = zcash::mojom::TreeState::New(
              "main" /* network */, block_id->height,
              "00000000002028f08675e3bfa521f5da266a7c48e373f5d417ed7eab7461"
              "9405" /* hash */,
              1787685639 /* time */,
              "0133a0b2cee1c11e73d1bdf9cd0fe0bf81cbacba331b7d622a0b6432c425"
              "278502001f01993d4ca90b979b5fbddcc9325a78fbe01b9bc779aaae311e"
              "0d37132bf0875617000157b0393349c3dff6c9f0f337eab38534c6d391b8"
              "adf63ca2aca9774380a1ff330001878d96cc87c7bdb052be64d6ca1d4a03"
              "5a9cca41d6a45d162d45c6af3e40aa4a01429cb689a47f27d2bc7454e2a7"
              "5e2e0f6c7ef04d72cffc72427e78caed63cf6b0157d2d07564559b0c88e3"
              "763bfd3b2bc2eca8a8d61eeeb839239e7bb01b69104a01e51346b2597bef"
              "3c69f5c29eb947726536a5a9ad684985b6edc2f18a80eae4720001d5ec12"
              "4db8a1d299a90e423001be38fb902ddb59bc48f7a514da8566afdcec3700"
              "000103d1187b8d91f9b17135eaf11ea773ac8f2226a7037227c9c24546d6"
              "3776c65101ab65114517839c9df32720a9f91f5be6604f91ffd2193b3a9d"
              "16c4ed2a81df73000000000190eb9e2bc82b8b980aaa63ba44db65328553"
              "ba840c38c5011a465efd8b233b2200013e2598f743726006b8de42476ed5"
              "6a55a75629a7b82e430c4e7c101a69e9b02a011619f99023a69bb647eab2"
              "d2aa1a73c3673c74bb033c3c4930eacda19e6fd93b0000000160272b134c"
              "a494b602137d89e528c751c06d3ef4a87a45f33af343c15060cc1e000000"
              "0000"
              /* sapling tree */,
              "01530ea3f8f03f1a859967f87adca3b3f382dbcffa3eb42e14a4e038a92f"
              "442a01001f016a8781ccfa8e1794bcef3fa26fde837189c3b7894a56dc70"
              "1c76e84aec81d20800000001d872a37242bdf5861eacbbd93dfe924a67ec"
              "1d504d2ca153532097c1070f3a0700000000000183902b4f1843ed7b6d0b"
              "47eaef5efa82237f9533889650855fe60cdd10cd650d018b0cdc0969c2c2"
              "070ca79277185757e8094801a44fc3fa33c0eca6373bdeed24013dda000e"
              "1a152209563e47d7dedd8ece9caf5f808a9b0bd841709ad46eccc81501e0"
              "828ebbc5dcb2be8011aa76b77c3b7e4a630c587f42fb350b0b4f570344f3"
              "270001cb129832ff83e5ab567f159e7d0c58a04c11074a9d29d2f21b908f"
              "d11dddc41800000000000000013f3ddc746e57791a2cf8900143b86b9ff7"
              "b82454626f0ba633404f9305b6c32701e2bca6a8d987d668defba89dc082"
              "196a922634ed88e065c669e526bb8815ee1b000000000000"
              /* orchard tree */,
              "01dc433e1a776755491150ed21e05699543b66b65d41ccb6dc5aee6bf895"
              "9f852a01b18d4674c535f798238ba68ac2cb43fa3b9ca1cf980431419334"
              "b2df73b119081f000194732a42aac9a9ee4ffdb20892422126f1c36c187f"
              "6a228bb810c9604a0b0500000000015880c0784722f97ec2359a1dbcfa5b"
              "d3abae8ded577c4facc2a70798783a011301fe66c60617008d6ce530558b"
              "384c936b9c17f09cdfe81426cfcbe6ba98629c150001b1b53cc49c78037e"
              "0a80b96e740e2861593be984e06fdbed77fa06b3defca40c01993aaacef4"
              "6c4c0b5aeab79b7567728f4267752b184a95987340e024fd98500e01b84f"
              "db5b5c21852b8bf828622344a836da0e7b8d98d63c297bbe713394da3430"
              "01aa72d28a1e3fde14a1639af23bd8cd97700d826c5ab8c4ab9cbe202424"
              "5ab015000001f1294ce1ba5cd0625efe133008fbb34b61f8fee5fd47e84e"
              "a1e3aa6faf72d039018e1e8c93493395a08a8d30ba349a6c858ca13c4bb3"
              "4b4f214f99afe7e420452f000000000000000000000000000000"
              /* ironwood tree */);
          std::move(callback).Run(std::move(tree_state));
          return;
        }
        EXPECT_EQ(block_id->height, 3460502u);
        auto tree_state = zcash::mojom::TreeState::New(
            "main" /* network */, block_id->height,
            "00000000004c0160467d8c3b767bfd23b6f59c817b5b27c6886b1c57fd81c4"
            "72" /* hash */,
            1787686117 /* time */,
            "01bb676d335d49acdfc34e30301e0d7e35c87a88539b7983d56de9382019a2"
            "0452001f01cf37baf809350592b045b010eac53b7a6cf5f3d1b42422c5f61c"
            "e52fd277e924010defc193173e5e11e4164796dfc678a8bfbb4234790429db"
            "6d2536cd4f0252210157b0393349c3dff6c9f0f337eab38534c6d391b8adf6"
            "3ca2aca9774380a1ff330001878d96cc87c7bdb052be64d6ca1d4a035a9cca"
            "41d6a45d162d45c6af3e40aa4a01429cb689a47f27d2bc7454e2a75e2e0f6c"
            "7ef04d72cffc72427e78caed63cf6b0157d2d07564559b0c88e3763bfd3b2b"
            "c2eca8a8d61eeeb839239e7bb01b69104a01e51346b2597bef3c69f5c29eb9"
            "47726536a5a9ad684985b6edc2f18a80eae4720001d5ec124db8a1d299a90e"
            "423001be38fb902ddb59bc48f7a514da8566afdcec3700000103d1187b8d91"
            "f9b17135eaf11ea773ac8f2226a7037227c9c24546d63776c65101ab651145"
            "17839c9df32720a9f91f5be6604f91ffd2193b3a9d16c4ed2a81df73000000"
            "000190eb9e2bc82b8b980aaa63ba44db65328553ba840c38c5011a465efd8b"
            "233b2200013e2598f743726006b8de42476ed56a55a75629a7b82e430c4e7c"
            "101a69e9b02a011619f99023a69bb647eab2d2aa1a73c3673c74bb033c3c49"
            "30eacda19e6fd93b0000000160272b134ca494b602137d89e528c751c06d3e"
            "f4a87a45f33af343c15060cc1e0000000000"
            /* sapling tree */,
            "01ed851c205f65ade80eabf87e25364a28441fac6f0ed2c313df1d8287d796"
            "5429001f012d9e98343148a2f226a1910b18524660a96b92ddc5ef7b6b7dad"
            "04d17e16e22d01533e4c900d7155730de7b719874bd3977ed4333e2e9c255b"
            "d1da174ccb60f815000001d872a37242bdf5861eacbbd93dfe924a67ec1d50"
            "4d2ca153532097c1070f3a0700000000000183902b4f1843ed7b6d0b47eaef"
            "5efa82237f9533889650855fe60cdd10cd650d018b0cdc0969c2c2070ca792"
            "77185757e8094801a44fc3fa33c0eca6373bdeed24013dda000e1a15220956"
            "3e47d7dedd8ece9caf5f808a9b0bd841709ad46eccc81501e0828ebbc5dcb2"
            "be8011aa76b77c3b7e4a630c587f42fb350b0b4f570344f3270001cb129832"
            "ff83e5ab567f159e7d0c58a04c11074a9d29d2f21b908fd11dddc418000000"
            "00000000013f3ddc746e57791a2cf8900143b86b9ff7b82454626f0ba63340"
            "4f9305b6c32701e2bca6a8d987d668defba89dc082196a922634ed88e065c6"
            "69e526bb8815ee1b000000000000"
            /* orchard tree */,
            "0154e343e94285ec6556282add6b077541e8dbf9d1c211f6b8c5d027e33817"
            "a733013ef9381f27911692574aac0e95029a03be58d3f85b7981c095d33303"
            "51157e3a1f00019f886cebfdd2b8969069afdfbe775dcb5003f12456278f46"
            "1f51e7ade71a263a000001b47639595f71acc0ec899e0881c8a0187b4aadbd"
            "d2798e72b2247dd785be6200015880c0784722f97ec2359a1dbcfa5bd3abae"
            "8ded577c4facc2a70798783a011301fe66c60617008d6ce530558b384c936b"
            "9c17f09cdfe81426cfcbe6ba98629c150001b1b53cc49c78037e0a80b96e74"
            "0e2861593be984e06fdbed77fa06b3defca40c01993aaacef46c4c0b5aeab7"
            "9b7567728f4267752b184a95987340e024fd98500e01b84fdb5b5c21852b8b"
            "f828622344a836da0e7b8d98d63c297bbe713394da343001aa72d28a1e3fde"
            "14a1639af23bd8cd97700d826c5ab8c4ab9cbe2024245ab015000001f1294c"
            "e1ba5cd0625efe133008fbb34b61f8fee5fd47e84ea1e3aa6faf72d039018e"
            "1e8c93493395a08a8d30ba349a6c858ca13c4bb34b4f214f99afe7e420452f"
            "000000000000000000000000000000"
            /* ironwood tree */);
        std::move(callback).Run(std::move(tree_state));
      });

  ON_CALL(mock_orchard_sync_state(), GetSpendableNotes(_, _, _))
      .WillByDefault([](OrchardPool pool, const mojom::AccountIdPtr& account_id,
                        const OrchardAddrRawPart& internal_addr) {
        OrchardSyncState::SpendableNotesBundle spendable_notes_bundle;
        if (pool != OrchardPool::kOrchard) {
          return spendable_notes_bundle;
        }
        OrchardNote note;
        base::span(note.addr).copy_from(*PrefixedHexStringToBytes(
            "0x037f9054dc0deb4c01dfd4b6c47aec0d2b57f9260b6d35f70f2cccbeb39b0dab"
            "9b253cfaa51f12fd428a83"));
        note.block_id = 3455353u;
        base::span(note.nullifier)
            .copy_from(
                *PrefixedHexStringToBytes("0xecfdf10af714feab31084926e5ed0e04a2"
                                          "bf901e0335d37b27c67f016ee74237"));
        note.amount = 155000u;
        note.note_version = 0;
        note.orchard_commitment_tree_position = 50401880u;
        base::span(note.rho).copy_from(
            *PrefixedHexStringToBytes("0xef8751c34903a0e3ff0ddac1bce8a3dc3a02e6"
                                      "7ec8749fbe93326aa22e311f38"));
        base::span(note.seed).copy_from(
            *PrefixedHexStringToBytes("0xd438146954a1393dc488b6ce44145be1d8e8a2"
                                      "c73caee2c9621d1551119775f4"));
        spendable_notes_bundle.spendable_notes.push_back(std::move(note));
        spendable_notes_bundle.anchor_block_id = 3460496u;
        return spendable_notes_bundle;
      });

  ON_CALL(mock_orchard_sync_state(), CalculateWitnessForCheckpoint(_, _, _, _))
      .WillByDefault([](OrchardPool pool, const mojom::AccountIdPtr& account_id,
                        const std::vector<OrchardInput>& notes,
                        uint32_t checkpoint_position) {
        EXPECT_EQ(pool, OrchardPool::kOrchard);
        EXPECT_EQ(checkpoint_position, 3460496u);
        EXPECT_EQ(notes.size(), 1u);
        std::vector<OrchardInput> notes_with_witness = notes;
        OrchardNoteWitness witness;
        AppendMerklePath(witness,
                         "0x3afe0ea889600524e5b9ee41fe163209d1cbce54a199de615a0"
                         "9e3460a1fa11d");
        AppendMerklePath(witness,
                         "0x1db428335de79e952fdf1e7eae6d0c22d751c65e211d4bf22eb"
                         "03ceaa30ed70a");
        AppendMerklePath(witness,
                         "0x20f389af330a7a28d7c547978243a1cbbaa17f3cee62a9d5a75"
                         "8ca8c8a5d5e04");
        AppendMerklePath(witness,
                         "0x4a817e76e861ca39c6f1e915c89abf2b6db65dc655718f64482"
                         "929bd3c760813");
        AppendMerklePath(witness,
                         "0x6857b7e8d966a86e055ad80ed8c81d041100ef50688eed35ceb"
                         "aefcb64d4e305");
        AppendMerklePath(witness,
                         "0x799995660e4799a0c8664bc1ed6d5d2b42ac8893cc7a659b9a7"
                         "09a1d1735032a");
        AppendMerklePath(witness,
                         "0xebb55817d10e729b61bfdea7831af5112767e8e65d188016b48"
                         "283afc4205603");
        AppendMerklePath(witness,
                         "0xc5d43c6f1f31e8eb6da4c7dd423028e2593feff9a162cfc7bd3"
                         "89ebde667852d");
        AppendMerklePath(witness,
                         "0xe55930d7336e314c26cebdc45813dac5ee15e07250fc4b06801"
                         "6f3719924063d");
        AppendMerklePath(witness,
                         "0x778817a6b1ff35920b58b5c8b7ed5d92e80453cef9591b98754"
                         "d9182bd6cfa2f");
        AppendMerklePath(witness,
                         "0x4d4499803aed438f90326fa17990a46f32061512aa5704112f7"
                         "f26b52e482133");
        AppendMerklePath(witness,
                         "0xa58b9412b50437866ea0052d11ef08645779fa70b139d63d392"
                         "2435bfb700f3e");
        AppendMerklePath(witness,
                         "0x6bbf7c03a6649e73b9a0165fe3d61106d66b790161af59e19a0"
                         "2532b5b7da216");
        AppendMerklePath(witness,
                         "0x37dd26bfce3e3d691daf5467c11d8b5358f48221616be1778a1"
                         "8b56c7122cc0a");
        AppendMerklePath(witness,
                         "0xe00a1b780969c97ec779969d534b484dcfc350aefec2240aaca"
                         "d29f8b38a7c0e");
        AppendMerklePath(witness,
                         "0x63f8dbd10df936f1734973e0b3bd25f4ed440566c923085903f"
                         "696bc6347ec0f");
        AppendMerklePath(witness,
                         "0xcb129832ff83e5ab567f159e7d0c58a04c11074a9d29d2f21b9"
                         "08fd11dddc418");
        AppendMerklePath(witness,
                         "0xbd9dc0681918a3f3f9cd1f9e06aa1ad68927da63acc13b92a25"
                         "78b2738a6d331");
        AppendMerklePath(witness,
                         "0xca2ced953b7fb95e3ba986333da9e69cd355223c929731094b6"
                         "c2174c7638d2e");
        AppendMerklePath(witness,
                         "0x55354b96b56f9e45aae1e0094d71ee248dabf668117778bdc3c"
                         "19ca5331a4e1a");
        AppendMerklePath(witness,
                         "0x7097b04c2aa045a0deffcaca41c5ac92e694466578f5909e72b"
                         "b78d33310f705");
        AppendMerklePath(witness,
                         "0xe81d6821ff813bd410867a3f22e8e5cb7ac5599a610af5c354e"
                         "b392877362e01");
        AppendMerklePath(witness,
                         "0x157de8567f7c4996b8c4fdc94938fd808c3b2a5ccb79d1a6385"
                         "8adaa9a6dd824");
        AppendMerklePath(witness,
                         "0xfe1fce51cd6120c12c124695c4f98b275918fceae6eb209873e"
                         "d73fe73775d0b");
        AppendMerklePath(witness,
                         "0x3f3ddc746e57791a2cf8900143b86b9ff7b82454626f0ba6334"
                         "04f9305b6c327");
        AppendMerklePath(witness,
                         "0xe2bca6a8d987d668defba89dc082196a922634ed88e065c669e"
                         "526bb8815ee1b");
        AppendMerklePath(witness,
                         "0xe8ae2ad91d463bab75ee941d33cc5817b613c63cda943a4c07f"
                         "600591b088a25");
        AppendMerklePath(witness,
                         "0xd53fdee371cef596766823f4a518a583b1158243afe89700f0d"
                         "a76da46d0060f");
        AppendMerklePath(witness,
                         "0x15d2444cefe7914c9a61e829c730eceb216288fee825f6b3b62"
                         "98f6f6b6bd62e");
        AppendMerklePath(witness,
                         "0x4c57a617a0aa10ea7a83aa6b6b0ed685b6a3d9e5b8fd14f56cd"
                         "c18021b12253f");
        AppendMerklePath(witness,
                         "0x3fd4915c19bd831a7920be55d969b2ac23359e2559da77de237"
                         "3f06ca014ba27");
        AppendMerklePath(witness,
                         "0x87d063cd07ee4944222b7762840eb94c688bec743fa8bdf7715"
                         "c8fe29f104c2a");
        witness.position = 50401880u;
        notes_with_witness[0].witness = std::move(witness);
        return base::ok(notes_with_witness);
      });

  OrchardAddrRawPart receiver;
  base::span(receiver).copy_from(*PrefixedHexStringToBytes(
      "0x3cc201134a5242b938ab8d98ff2179c96a733321d58563320a1777a57fdd91693dc55c"
      "c06798259ccf63a8"));
  auto receiver_address = GetOrchardUnifiedAddress(receiver, false);
  ASSERT_TRUE(receiver_address);

  base::test::TestFuture<base::expected<ZCashTransaction, std::string>>
      create_transaction_future;
  zcash_wallet_service_->CreateOrchardToIronwoodTransaction(
      account_id.Clone(), *receiver_address, 10000u, std::nullopt,
      create_transaction_future.GetCallback());
  auto created_tx_result = create_transaction_future.Take();
  ASSERT_TRUE(created_tx_result.has_value()) << created_tx_result.error();
  EXPECT_EQ(created_tx_result->memo(), std::nullopt);
  ZCashTransaction created_transaction = std::move(*created_tx_result);

  std::vector<uint8_t> captured_data;
  EXPECT_CALL(zcash_rpc(), SendTransaction(_, _, _))
      .WillOnce([&](const std::string& chain_id, base::span<const uint8_t> data,
                    ZCashRpc::SendTransactionCallback callback) {
        captured_data.assign(data.begin(), data.end());
        auto response = zcash::mojom::SendResponse::New();
        response->error_code = 0;
        response->error_message =
            "51c140f2a42f5d163c9fae8ed68cd54055260b63334af9b384f6872f8acd4712";
        std::move(callback).Run(std::move(response));
      });

  base::test::TestFuture<std::string, ZCashTransaction, std::string>
      sign_future;
  OrchardBundleManager::OverrideRandomSeedForTesting(
      kOrchardToIronwoodRandomSeed);
  zcash_wallet_service_->SignAndPostTransaction(account_id.Clone(),
                                                std::move(created_transaction),
                                                sign_future.GetCallback());
  EXPECT_EQ(sign_future.Get<0>(),
            "51c140f2a42f5d163c9fae8ed68cd54055260b63334af9b384f6872f8acd4712");
  EXPECT_EQ(sign_future.Get<2>(), "");

  EXPECT_EQ(
      "0x0600008098b684d85b16a53796cd3400aacd34000000000002414099eb28a398497db1"
      "ec9dffe58774b5266eb0fa02bcec60c752b2146bce0b4fe75edeb8f964bf0ac1afa66dc9"
      "cf850d86841b4fd3a0b16c4c713893daac3246dae6dd6330ad51c9c51b59fc9a6c4f3928"
      "e7dc1b35fe3811ca83e56b8587b12e536e40ba3df7405ab3b4f7b8aa13e1b5ae672d67b6"
      "b6e453c378b13799ce3172f7d0df4b8895f0f469d7ce38093029955c1a99775d976c8563"
      "6bbe09204838b3143923fb17529f3f158b901fd7b7dea9130ad1cdd4aba07e14de55a512"
      "ce5977d8a38228240a7ce41ea62c1e78501d8b233518777949eef28f913a0068618e222d"
      "6a568d4de58bd6f1a4729cab037a6db67052c5e71b1e8710bd0bddddf556e79116d1b935"
      "4a24439368f4116d7970db6c269751ca08c6479070c0141df3f323c0b0e058a8897049fc"
      "1206c0d5142e30f8c2176e5b7cbac849e249a6894a810b5e6194fcf4c4cc86ed8d748151"
      "0bad1387569633e6423becb67f8d453beac413f200ad793ffb2409e02ae8c57e0ef18362"
      "a9e96383f743d57d1b505faa1e1d4cc1c0043c5168b7e0568990e7933fe6bb699223bb75"
      "8e8bc8c8f7ccdfd810d942f7900437ab902e9f7b5079036e971484fafadeaf27034a4a48"
      "57bb9bc226c8692574c2a09027171b6ccc6639a52a4eaf059ccdaf6b98d7967a82d28f30"
      "70d9ae887f51399f3b094f9d60e681d5509d03ef774392529d72af044ac34f0fe288ad21"
      "5882ccb0c1654407e2d11207390d31f772bfb4c499a06d53d564e2039fecc149a42daa54"
      "8cf055c42635ff82abb8a77441794582e3f8533ab7db5b06a8f0ebd0167fd1504f1cae5a"
      "6eab0844e73190336bf437ba809494b8f944663de9a5dca46a729f997f98022b7ff0f0f6"
      "356afce3f52560db9ed7a4e65fe4f3d2b03cd7303e328b688e184d63ee55a4638588fab2"
      "8d9bc6a40b72db0c8dc257c6ac6da83d15997dd8139beec9a23b380392804b9bd65ccc69"
      "e89dffd31c7135ccc6e5d8bf936c7aab569e1afcf40a749d6b725f59b045f2204fe144ae"
      "00116b36cc7fe63096877edcd4cca1f463aea66e6df64bb497c8826be2e7d1d4b8cb5e9a"
      "72e99f402471cb18f523c4b3106283b3369ab4325a883ad2b392c3fdff0ea82e8e0876b9"
      "e3231c95be75cf9c63e50eaa57f0c23dc1fcc6fa8f378d4df8886a4f3b32116ea7bdde25"
      "127ff5f69e8c349868b5603c1e8fecfdf10af714feab31084926e5ed0e04a2bf901e0335"
      "d37b27c67f016ee74237f21ff3d020615c45cbea0f5229fcb46002497533a4d7ab20f357"
      "3ac097d586054d50ccb2a9e0b2d53d2739014108c3de234205a3ebb6a3112e888f503591"
      "9a0a72ec1920017e94369023e1795c566f445025310c2c696182b577bc38f107ac3cfb51"
      "78310d64702ea7429d810316e493feca222ed8d002aefa09bf2dc9ea7a4659206ea9519d"
      "382c2700d61b2334a9418f4f2df03509307ddb556c1162ff081beedba2864814d35dd999"
      "47b683ca0f47dcaee33a5c2c8783e3cbb71691333e68a24037e21332132ae80f6d268c48"
      "4f9792c42ca84190d3771b902a08fa1f3c9dc9ff9fd8b634ed0ef054c875e241f8e2d005"
      "eefa659e311a7d02839353806d6514646fdfed58f660399e60fd6d701e9de8286bc04b0d"
      "7dfd2ad592a4e6b24e912f046aaa7e479ef5559c77e702b2f02659d00235e31a1991c770"
      "e56ce9c409610a3fea0c2694ba7b08263158f05dce1f3574ec20551d4688196bfe96a2f0"
      "6c8fbd3424da8c2f86f3a0ffb086aea44ffaf535507e31cb63f26c710c744fadf950b2df"
      "3669fe28752352be8a5834807a998326180731e4abe45c339210dd0d3aa1ea6330f7d8b8"
      "71f55d85c880d5e0870abce1dd9f2e2c1fdbcad47bbf0d08c42b38dba01d98edc61b0649"
      "5d6adb90cbdc4b3b65c740e8bb74dd566ac2f428e516dabece0e3373925ca8d161cc3c76"
      "c6624127112efe1b6aa403f97da360e4c465797b39457e59d70e76dfdb5e623e74546181"
      "451557e306384e9ea9a95d8f47fcd11292acb603ff899115a1f4e1a896313395591a4113"
      "4df2917d5774fa616f28704b33c52ec5d4bf81b2b30a4c4f5b99c2a13d6ab78b5d8c5044"
      "708e4d90ee67ad96598b69ec714cdf1411a463d6fac4b9ffcd6d773c5bb4c4dd7f1148fb"
      "46bf0eaf2cf387debb3eb0780f8480979c05d10f54eda51ce45a7e0d326f2c347cea2eb9"
      "c31090a376ef6c6fe4ab1e2d12884e81dd20ac48f0531272ff3f1e0c1418b5cb82448e20"
      "18d09837d63dfba5e28126f2095312a42913fbbdd8038092959900c99220da9fabca82de"
      "5a59837ba16b0b772e040330750000000000000ba249c6bf0c8b2a21725f7e2a5dfc35d6"
      "1be610c8e34afea0502b511927f61dfd601cca36ee4a0c2fdb780ac937c4479172421d2b"
      "abcd085cbab8a6556ce2bb1fe42db323996eb3f252e283d681571e952c552285e4297c90"
      "3c23d3574a75054b8c05a07b13b1d650d5298ec57cd0850fb0392bc159f86d564e19820d"
      "a7fea975309fd23fd70ec1f169fd6ca85a767c6a7d58b67e6462c087de3bda5671e8479e"
      "5dad981ef4d69ddf64972b3b681be1fe273fa77d60c825ac012d78dfed1d89adcd0d6bc9"
      "0ba67e0860e0d2c647bcc2bc4455164b06fd6fd5e286d8cd5d5165984fa5e13b359896de"
      "9576e367a8125fca468a70b7801fe0cabfdc69af7413601e7b3b0238e6b9fe941a1e8c0b"
      "0a6880893d5fba81bd57e11033faf40f152f37491bb007369ec0a8e9b4c60c4269e7533a"
      "a90eb407e70c042d7d350768e7a0250cfd987abc039d5ea1d5b1a25424fc3d2762df6969"
      "63c3c98300b5a5fb33df4994fe21dfa2b412aeae7681631f6f43b5c9cfa2ca4385c4a7c8"
      "a03f489e7774b6aced93c2d09ffef519473a1eadd81ea99b09ae747b3f70e27220fd89ce"
      "2a105af16b053306af3a544bbc4805e151bff2affac6a67007d1b20e9cf823a79e14434a"
      "bb08680c3c74c51dd32745c49f533a75978595b9438472054d898caf65a9c6e6e1924b7d"
      "2945a9c7555ebdabcbe82bd9b62fd12a58cb39ed358d1522a51212c70dbc8b14f506b3a2"
      "859f8d7a67652c05f3e69f6f470a6f53ad3d86b458d99f6d67b08747146c7ceaf4f76ea4"
      "70bedd4c5002df59c32292be84f01b96cc8f443d599bac8bf2fcc8867e7b53aa0855db8e"
      "c51ec55653a3ed3041ec9058061c28298f07259bfb7cd841aa77204f1a436ae5cd60ab79"
      "9cf6db73bbffe8685af04fafc31d3ac6e5189fd46cc42b0f4b371da64ab66c42a9ac1b1e"
      "d3f4751b607e64da4103fb6773d962d78f6dec781b818dfd36dcadfc8f3b0ad35186d276"
      "2ae320409015244ff13659a4f0e7df6536aec01277ccf55a01d86e4cd3b162dedda8bbf9"
      "519bff38cdbfc1f58492b99c38d94b45e53ba44a4652e85e89cbd2700221ddf04ab53f2d"
      "96a72c944023640c024301b67d64f303155ae1497ecca87d7405f86499392ad34a774129"
      "5ee3521573e8ee8b55137516b0705f591dd050fd4127073e55bb8a88f254fa227d96ec46"
      "fa89c7556a3b4c2591d3394fb43ac0b554deeae7cc1f5d3416defd66247f310c1a76566f"
      "6c42cdad9de105d06bd8d396fc1c5e1db71ebf7ba071ba0c800ed7393cb308344b641866"
      "ea6518fdce08f5f4b9251fdc97a931d6b59c88c31713e55380efaed68ca1d40f620089e7"
      "8340a001324efef978b7e4a2f0b57d206c7c8257221a48d7df4bcc109752a0cf80ffee69"
      "91ffcc3e88bad53865d7e65c17be1646acbc06aaed7fe266f43191f638bb0d41d9aec2cf"
      "3cb138dab38d50840648620becf03ebbbee5f39561ae8a3c2864120cc42b87711115a2a0"
      "f5fb607d08a1a19ff3a7f50c39d8470020be5775bb4697a9b417d424f7a53763f17b6f40"
      "b78412729628df138e96b4ecf71a48fe726d9f6795f073fcf833f503d762c2d1527e4aa1"
      "c7233d7f558c68d3282c57055ad64d96e5b30cebe18b3a1ef49d47acb71d35e29eb474a3"
      "2b4e1c67d58b195f755f1270cb6bcda22498d880a27319dbc388974e20f4cc3a2b855ade"
      "2f9209a446d696f5aedbc7ec468ebf8d0529606736c71bfa415140d318b7a3f318a6aa16"
      "6a4bbd895e1963839f1ba121663d2840e118b4158a0c8a4f077e100d970f16a180d3faec"
      "2682ca1093a633f71013758154d56dedbd0f125a39ed735e89f7f1139d95e80b18b821aa"
      "8803631dd584be20e0a0402846cdd7d429ae0b85a33d2d0bb23cf0dcd640a289bd12db54"
      "4176ecb723621027bc2689e4b8c81a057eca6a3fbed42fe1998c78c5748364d01c29cd77"
      "65865f489db4af97a0cca8fbdc7f3834d2e09d1366fbfe3fc7314ccdf9e425367c4578b1"
      "f3edde9510b3f2dbe0293f1b5e35649d535e42aab192482803402fafe25b0c4ff65cda78"
      "a5ce83927d4588bc51e40073e738c11f80b158dadcb01e20d8da81d1f8b0db8597b8f79c"
      "1230fd02ae59954f69f27f1833bc1a85a699c1bbb211efa372bf0ccefdfb88204584fc72"
      "3bc0c5bba94864af5ea12ca1cbc61434494321511babac3b7a0197bbe2ce285f6990bf06"
      "808d3e20740c959e28a215c7761c83a7086f95e9487eb89d32d4452e822403829deafcca"
      "93a9e2e96d756ecdf47d7a40907c69d535c26acf1dba79acc83f7763aa595e4851829856"
      "2d2f3cb2e14334945f5c5e8f6551f9bc4968dfc33108c7332ed566c5dab0b2f6c7a376b2"
      "bc14e9508d4c1e36f39b7017bc55ac58946726b740db8f1582849221781319745553edad"
      "4f0ae63c3d8cb7f955f452a81d4d4d7e2517a9a0948abc39af98eef9a5e6e3b37f20a2d8"
      "fb79d1c5060499ec8ccbaef551bd3188be1a4dfa8a2021985b9223cfaac64409184db0ff"
      "8f5c6acba267dceea3db9f6c8112c09ad02a575e0904cc3cefc49cae1caf7e72a0c10561"
      "9f6f0cb50a30bc3b131ecb524f5915d303359c8d1ca0efa34d386d871681f6588b3037c8"
      "01a1a0992901e8e143ec944d489645674b05f7ed84781eed6ce4301b5d8d7b7e6f1bd20f"
      "9829f1a4c06ea9f8efa99575a6932f2832da9a58f6b5109556bf232fa5570e4c0331c076"
      "d08f752a28982763cc6bef1669b28590dea1139e17752b8337d985eee12350d663fc3e5a"
      "7b86fc5dc1f73451a01e14f3274374cb98e9868b1d567436852d1a5091ad827fa5f055f0"
      "c2e53b90353b25a6013cef664126c3db62dc661c8e09230cd2d26f038318b65e7b9e9523"
      "112b0616e314848e1585bbf9b92dcb52372d8831aa08ef8d2118a3da44aa295f2583f12b"
      "82bec41c130ec5ed7d47eb78d83ad9465b6fa8817e07c9d93437a1779195da8ccd388b1e"
      "b49f189d865070a03d0f34492d20df656ca4f94d2403d50dedcee6f2858d32ae318dbbf6"
      "f9dabf43de3a7cd2a2765f0f377699e091688bc3bec350364847169083d136ae5862276b"
      "262e010c61b23b00782d6a906308ebe72c299b9d44aa80f25c052e27fa8e9e28000583d7"
      "faa4071eb2a1bf32e7c3ec1128fbd95749b3fcf090902dec8019ef42fa2866771a7f60da"
      "edff82799b83a3ac2b66e694ed6472fd3c590e7a0676ef184724889a1ff6f58fd1b14ac1"
      "ff19dc8db51ef24a1ef9a1e66d6a8b52ef221e080427979747b0b86443b722ab5b8949a3"
      "f458ab7321503326aa7454b80f4adc6d3b35a586fe4822a76e8199af62808b5ea50139f3"
      "ec861b6f687f6f16933e7eff9c363e80c4350735fcc6cd0ec93f0874fc7cc93696d2e111"
      "c0177eec1947d13e822b8d65efabead3d8105841e56d84fc26214f59c257234ac63208c3"
      "a1bff25b45083edeb4a002968c0424693208c6a93f4b9ea5f530adda74c170cf6eee4a37"
      "1a2699d78d8f80b03a63240d2bd42cf251f6e65807f61bf138dd61a283b916c2be2ea439"
      "3466a43285c60b350e7a8a8cf421bbbe349dfd79c6c076c59c6832462414d58ec76e1b0c"
      "87e3fe575f94a31dcae18b30025e10a07b33f2168da2196eda1777f8fce45c5d70093ee8"
      "b8fafb85f5591e6da3bfb0f904f2a94659b98f69c013b26734d0ccb254623dc0b2bca74f"
      "be3f8f028943b080d1481bbda779f57dd02cf14299b905dad8e97e54e7a0a467c2924763"
      "8b7cdbcc0c01a5cba6f16fb1fd19af3798b7e95687c654281ffa9ff904d1c6688fb9970a"
      "c6fd148f7b49bd67b5337978962fbd710fbf9581dc70edcc29921524fd93f8409b5c26b7"
      "8b9e94f71301f409e0469b2d4374e7842daf9d83ef6186950cd3704408d581a4b3d2fbe2"
      "5520669a6e6682a7426e508ff65519afbdd21873b9efe35b33d4505150ae1e3b021d0d84"
      "49df00a1e4f8257dc8c60ee0f535317434ff569e305f2981fe01177cc71651c06e5c9fae"
      "85dda070a24e38f64d80f273bfa3d0d2e5cf90773b1ed1ee3f0e77bca3a6c947f49f7cfe"
      "93bc4c75370559e6b8b925fb72a2748ee70f8cef2a222ef96a0a14c0de5414934df9a29f"
      "c598f56fd13030d7c1774d9e707df513083fe72f05924c80dc25e0ca8f66000558d61be4"
      "efdf1b79aee79eb9cf43063c6123756f1f34cb249d2f7e6cd29ebe08d3ec6faaaaa13365"
      "d7f22f9b3cf12651580e5f9e7038207304ffd98dee075cc30c552c30b8e96c62decc9704"
      "ca765db6570e0232c5d6b6575c38aca08e693dc97e976807cd87b272ce5ffd2c0805488a"
      "3730457ed88aff4162a373e991327fa8d5209b52ddf78f3f28743d94e2514b3ff631241d"
      "0247a7edba64a94fa4e95b20e296c82f4cf9981b397f70615f50001a11279a2d264e111e"
      "8ee405016550f47b3c798d68a6bf81fe1240607a80eb1756820541a2056fc5d1b3545047"
      "704138c4c911a90d0f5d955b7476fa0dd89f41f4b600e68ef9936d6eac6bb0d2ee2383ad"
      "0c56516d56284d8bf1febae14c40ddf35d104e0719497ff80579f671f35c662a08d62e89"
      "c55b5bb0fea0b08b528d4b6e85342b0e0b37c7eeb6609cd826921b2be46322209901810a"
      "6d53b4eb73d72936312bd4ea6d9ab5323cd8382079d85557b738ec66e5967758c0dbfe02"
      "cea48088842b56181d6ff0cc713903a461070ac559d858fc0adb619467bfcfd7cce74caa"
      "8013200855e007896b6eeeeb943999f5457dbeb106278811e161e372e28e80605d07ca81"
      "7d29293e40dcc6165a929ae142f669d0118ee3903b11f75b7d6afb35f61e05594a599b51"
      "deb5fc678085526572365aeb9ab9b7134dd0c212ec345eaaaa159060de98aaa3032a285b"
      "c4c04fc2abe128967b432a72c04599a4b80b04ae7e24c436bb9e56f702e10035b5069efe"
      "6f90df3f9286aa3b258860e7f8fea457763cdcc8c060267e90f77c13eb7b210118ff3e10"
      "99295243e5dd1ec250dfa7be812a09eba39f414d2562d37edeb5e53b31a73069eb9c2fcb"
      "b197cad6d38d79ee4f356811734097b3504f71faf843d1d3eeaa2616f11ab96b931a9151"
      "c457a393fd0248d7ea60fba924137ea2161e1bf9903495cf69e9f1d53e2b559170751657"
      "3c302b6e45cf0ed448f3574c9c3ba3c02dabf8e83d924e5f211921621beb24f4b10beffd"
      "e79907d993b3ae71d5c7a01705f9b4fe60c02e6cd604d70a6ce0077d0b231a852b71ee34"
      "058933f6a4b0fc2bdd7c0e941ec1b1eabdc1647d3c3f7989362e7dac6f8c536ea57e339d"
      "e9a02f67f71b8201b0d35f4b5ef29001a8531473271fdfda509d4689e69047177381fe2b"
      "df648bcc5bcf85deab6dae8d0a096cc6a73f3614bb80052a80dbb9127dd6454d659af44f"
      "3c72c4c747dcf55999062b28e80411db68c7812fa1c56d7100f62d79cb726e4db47cf0df"
      "7bb8c1f27a3f007f012f70f1b4b1a82607a1a48451c313d0439c09c96590aa81419c1460"
      "6f1a90b7e81b79a70f4835389a36902b9e1d227054f93b3254a4e1d86bf54e725a7e679c"
      "2f22a9eea2d017893bd7ea79e0edd0a71a7ab8e8c1d5e4d64f428cd64ff6e1b281270af5"
      "3718e4fbb6b4d6f26943d2ab39a7b52eb4f3b975812881874ed240b62e061d8414826e3e"
      "8ea10bb420c06c843a73e497907b119dc1f07d6b66c4d109e21bf9ebd9b1279170f8a63b"
      "6dae0ca5e751da6bd7c046c9159b2aacb0f294ea953060aa085ca4727d569cc49bbd3c48"
      "64e77466fb363635d92d6d5a7b2aaa0fc808db265035ed50b1dd17b95b25af635e572bf3"
      "26dc57dd33508418d1747f82f3141b070f68a5720f2303fcaf99efff42ebf7fc0b1d9e91"
      "b98dad247b70b179352e5af581ce5c43a8c68853720c6f45df947e349b2b38cba798b6c6"
      "7c989176fd35527fa497134bec1dc86122f17c9d9de13a284584af6a64b428525ec9bb1b"
      "273afa9c5a7f6873a98c0dbb18091d3d602b8f1bf3228d2dd15cd6cad1abde6b913aa8dd"
      "f134b805b38a20ad6a1a2eddb361f2d13146023c655574ccf0c80f14b938a2583c71c792"
      "e6624f50163e3f6bcf56601dd02a606fece89d3e9bc0a648571481608dcb9b4ad8f9c344"
      "f35beb3c14d17a91ab29c3849bc1ef5ce7f58f358403aabb61c71c89c8f47513cc181ac7"
      "ffa7b36f8edca291a79b176ca0b6b4508204925e1f580b3822fb14059b8ce4779f4f9606"
      "cd4683be50f6fb7c705192f19227e2f4b20b400cd790238a13067b3d63c0c846b9c89d49"
      "35e2c3788cc0104b181e8a6c5db25e2d6f36b56d5a3183d94ea12ff40f040cbf91f42361"
      "9fdf00fd311509d24f87a290fdeed8852fcf5ed6a89f72a5801e564cfe883ca55845ded1"
      "b1233536143217f8cee0af6b148e3630e6256ebe9aaeed1a26dc6774cec62d44ab10e3f2"
      "c3b33197139bb61b3d707740a29142868fa08ea5b3515a79f764e4e99e34ef560e216dc6"
      "7fd31ff330e70d3df3358b55d26fc815164c4eb8687e3efc170b4235c2c5c2ce4303ceb6"
      "161eb4aa8ea3d814b4b4ce66a1127d7aa438e99a8115390f53f46339ef1a8a74545c5239"
      "9ca64db8cbc3cbd079bb76fd6bea2229580eb99249f672c0ece2ac6751af11453e90b514"
      "70a5404b9a572631b24321d737196e9c0db2120608df7bc385cff491cbecd400a8da07b8"
      "679af3f6fe6596e00807bd781ba43ca86d1b58a6914da3ddbb8b594556c65e422a5b355b"
      "08491963392fcb64df4345161a3a3bc9c7cdf9f29ad9f776fba91933022769769518b91d"
      "843b34011b0e14ff4b1a0a51b76b2f5b19ab75b0e50e64a1d2e4b7db316d0b486a165d1c"
      "4eb3e812fcdbc9f82d759cf248d17506b547554efaa64ec97caca78194107109271c7a77"
      "07a8f4b874f56fcb2e2923080b586081c8265f7248dbc0562c0e99b73f0ed28c50f95258"
      "7fa2d3d593dac9959e0e05c1884b91da6133217c731887484104a502d797eb74984ca38e"
      "a932d49a42af09444351900e5825402419397057fddeb54694cf2d486c62bbeba6006668"
      "b84574cefa742be7bcec7c03ce14ed6f731eaae25c3341a45ddf35a05c1a86572cd03127"
      "8ca712a9859cce09611a857f4d359d0f77cb4db35ffd77742678b4d2e71f16e8335d1dcc"
      "205de0e0800444d442f62f5bbc2159716784dc6829c5d324d90669d799163f77f7847b90"
      "6b3330ee41a4b4f64294f7165ee2b93e6752099d75af6abecb6d3e371f597966760b8ed2"
      "41e5cb0b929cdc29b5361f092bc2b7e5a1f3bc70c96502309f4d35689c3a62cd1d29df2c"
      "a9852fb84696011c1bc63a90deb473caff5867019311569c173135ed88c539f53f279622"
      "42b23ffa5210d9de317aa4f80d2262e358f98b9f4600486a28b8a06ba3c0112b5bfe732e"
      "3ae93a04a300904572984ad9db68eb10b816098b89cf43902581eaf3204162a36d48e369"
      "c6b1ace8c90ac7d8a9b110c10f329b5a1d778c6bb7e57ee5da9624c3c92288b27015d62f"
      "fc8a654d0806a6f8fd2a355712bef04dce1ab3eaa7a49b68b004569dafce295e6165391d"
      "4e97b9148c217a860a7a53b06e382b7a7682ec3bf6e94f2a473814be2243d150034ac7b5"
      "ae066fa8d12a9b80ed1571bc3d6e03870208faed210d719a3ed2313f4c2fa1242723fbdd"
      "88a542acccf52a0bc16ba1ea8dea263e690acb627b3e5a72d5ef697a721c7c5ad74dbce2"
      "3ea2d236cf1ad500c868af294966b9f90e0bbb358d7d9b62801009ece94f58abd6e07d7f"
      "e4949c1862bcac0174c095eb56efe47fd61928adca183ded8935ef297409702ead52aba1"
      "482d9ab0ce53472728e13203fb821bb219219e1c1f50bda74bcf1be20d570d3a253cff98"
      "a3c82972110c1ed6377cc92a2801152a419a8f38b869790cc303e5a7631db8a80d3ba998"
      "c8cc6b1240e5bef10c1704993111257e03699dcb10f00154824b364bb6e4aa7b8ab2b66f"
      "3341bedde92bc4ecb15a2ecd38e6531f71f80c29634df46a7b55ffb96653d860d40f6df0"
      "6b07bcc5308933fe2d55de16e1a1a72ff078463a3d05ff968da613ab7fb4d1bb8217c1c0"
      "d29d4ab4ff428839c9cd430db1bca12c2db732c33db4842cbcca9d4dfd388d231d2546bd"
      "175d46cc77ada1f2bf85c35ecf4e705f899e796404912fa33b28248e385ca3c83bbc7bcd"
      "e319fa06ce8caa77ed1a7fbba53f1463e8a130049f14f5a2628cb084554d778ba9b83091"
      "452eca58ba3fb51e42acf82117ebc089bc284bbed3292ab8616db260a729350cd24c6e6b"
      "992955a6728a7f4e152f50850e35c7020f2a233d8a0a81c66fe143a4e379aedfc96a9a38"
      "96f0f846b14ce8609a3abe0aa79593527d749dc9c5ea0f68a85f533e70d23b8b5c7a434d"
      "f32e3e10363e2381d782dc839bac53eb1965730dc7f77e0b2176c90958b9bbc427a4334f"
      "2d3057c433a156e3f64e5d16a94761cff7cea0b4b5f49c53ddffb59b13ed15168b279e9c"
      "0f1ad7af8a303de996ea1192a4443aaf5d29247bb9ab606d45760fc6103549b93da73962"
      "fa90252fde7614e1cfecb6964ced85b96d4a4f30198f830d24389ae025c9a77f270a7630"
      "c6ee89a6e61b191d654119eeeb97bf131575e90e170347c4be9842840067da014583204b"
      "755f214390ceaaf277aede3db09cd701980617aae74ae38c54fcb0d4ca6f6e660c4ae550"
      "3a37557039a632ae31c6af78c52e6d49155670a00e6e4571cd8b5825aab2eeebfe5fc78b"
      "f7d7e6abe8f08ddda73ad0e209a9eb72eee42467be37f1e979bfdde06f795ad36485f21b"
      "b24b311ff5323eb849d2a96bed99a55b7ec36891f59a4a137762dd44e2a11f3ce6b677f0"
      "9e0e84740529ac6540a195704e0e066b43b784d3251329774c3d58e867cdf070a92f8bdc"
      "18fbb3a7e1af131e36f7435021b2916ce730a939325729f956896be2e033aa563ea517f2"
      "d7e21ff47758c6ad6610e28cd79b18add1ff376819a1968ce4195b8dfe35150dfb84c711"
      "5613d572616e948f857b3f8df3707a084ca325f555b82179faac283c66f0076e14a50cca"
      "cc37cb33aec1b545d53ed1dd8c00ffdb3d2c94d426265b369996beea619cfd16a217fc53"
      "8d3829e36961ec11cb6159795e20921fd82162f0cf2313f58507a0f51502eef2a3a39b4a"
      "b0f34181ceea8cfaed2ee1dbad5e3e8368c74747519e113522828c7af8c470e3af21e58e"
      "0cc2c78e2a33193d5416f0639ed891f6671e059ebdbd2f97effbc84cc3a3a096fc4d2b71"
      "1004ee6a35bbe3db30fb2b10e6ece56ecd1563144e56ebd354a7ddb100526e84e11be379"
      "9f3485b6b52248de51279d23921f366c8008dfe44a6a922a98839e3835b6f831648ce8dc"
      "a618365dafe0b1f754a15417d29c3132c6bb65ba1eabf7f63719fca95bab30a73942afa8"
      "d145253d10a4eaec5764cf47ced98e018e4d453620ae2414f547c0144b587072e192d833"
      "aa04b22a5f32e10adff7f876c06d5c23252e733c6c47bb64cc1f10272d72419c4d4eb61a"
      "37fa636c05a1bde718cf459041b5679bd47eb0d56e058be48f8c8456f5c1748d86184a17"
      "390feb1ef8adec70e52175cdac426264f91842fc43ba14b997cd5246b51c20cfcf049767"
      "6125b7773b2b5623d7b6914b89d4655918580768e4cd86583a1907215eb3ccada31ebfef"
      "8c0f8106a8a0e5ea49ced6cf2fa484c10b9a6b06a01ccc72e3ce579173dbfd5ce3302e4f"
      "29e6dd36911326acc0926463724c4d9bb687aeb112ea6b1aa4ed4f20db95a7af7b54ff25"
      "c7feb1f297efe9e6ba7ae8f958a133483f99c53be5f9427005a55b2765771929149b6ab1"
      "3d3c053c5c8a973a8681e87d26d8401e0d0753a96da48373ca84ad4b2438d15d8a1d449d"
      "ff3d6f3fa5a59b93e86b2b2a339370cb763b3a2f01154e1d7a8610e1cb7d465bb9ef9746"
      "81eafbbf62c68fc7e85acade1d3f3aeecb2e8684296562f7f54a5bfbcb2fc50b93fbe8c4"
      "fbd8d20823d41aebc596573eb3721d69a23fe15fab20becfe67025197be348b16d33481d"
      "0456e5bc290df4f541823a4cede5ae811caf259d562048713eb56c0d06701b6d4a25e8a4"
      "f42b730f5d7f1b36b9a2ba658a9626e5644bc8acc04f1aea66d6d0925b4b20586b0f3781"
      "cb9023c3b38445fb8d7af0b9895e3c09ee3ae2670fb3626bfb72121486a9ad4df286482c"
      "0b9ff4d55de88d66399c2d15ceb13551f6b1b518d53d6e0ec29fb9674d9e2486f1b26719"
      "068c1ee28623c5478a76460ca881726ea2cf42494eadbe6cfd0ff9d253c61025bf3c4f0b"
      "b02e550f3be37750cf775a9bbe5fae9db339e3ea04d48139c44183b3303dfd7c5fbab7da"
      "096f01c6a6cfc576bedd1b6db620d5b6099c72f81cf50c42e6470934514a6c543eed571f"
      "c6f426424e0519fa1b3906c564d4969b8126604df4997ad871fcda2e1f1198387c50a1c3"
      "982d46a3ea3b133b301480df63537437d9b856d62885e35d8e4830c2c5d7dcb761e3ca85"
      "6a0880488e3beca80d9a604c69f9caf99a289b8f8b149d2616b0628c92ebca7f71346d86"
      "108f68e2b6f742100d4cc179ed2a4ed8ae62b7b586ac6e7532b3174c961469dd80fd055c"
      "6d63612e90693ea7139e3344d97181243284a9b8fe6489ec690336fdb26ce6d9360721a5"
      "a1f31c66ce2d01d47f08167d2a6683125b57d1550f2c025dd1ebfe3b7f5ea68198851e66"
      "8badf8297bdceda05ef66c819f060cd2f9342247b5ca8638d4c117b8477805b23e92edbf"
      "7a99517c620c27cc7f28512de6fa164a9608df89b71b47123f6cff8f768f880385a13915"
      "e67c847f3005ebb9e6b234865b34e421199e1d6ae5de46f5fcf875b4bd728f7f17eaf052"
      "34a0bbcac6eb340076d6aeb5cf6e2c8cdfee68f9150b6c51b2b54d89049032ea59ca9e94"
      "fea7b9ab9f2600df3fb6ac8893efa96264cdf278737fae49040ebe3f9eb80ae8463a7641"
      "59e3b240d0b337c3877376eaab1e5824abb899d96944430dafe19c5fc949bc09473df68e"
      "d2b7d5113bd95a295da9443922e8b86e29edb0da79fb6526a8e7a50d194823a0d2ff2349"
      "9c3c81e7ae77cbd1a5dac4a04b93a26b9fb8a816a4d98b15dad7ba59fdb82bef1fbe8fc4"
      "e1ddfa0ab2b52423857baef18db48af6ca623d63b0d2e167bc784688ec7cbca9136ff301"
      "1c1b0929ff1ddce8007c48a0245a479c9bb5300fe89c589a1b235c2367f0bc1fa54961b1"
      "40b9873db64dfa6ca12318f688cac8483d792457e6e3284a2ed099a5abbc59f0cb0362d1"
      "987f4e2153c218a0c80f5e14686815f5fcf40c5b29dbdbc14dc1e0d2ae4a73b3105daf8f"
      "579c5d4b9e654e1c82e52b9a1f8a9722345a64fcaae298e23c17ba9cbb2f8156556d20fd"
      "d5880664d13d5dc2d3a1dbe2b96618e0c63dc7773882ffeb636913a188acaa13f96bd8bb"
      "901295dc56bb3ba40f1933fcaaaf02c9d4c78660ad94e44876c50cacd4cd4600f1f349c4"
      "4c9ab00d9c2f90d37bb65059c1cc5ac808dac705f5cd8e2f961be8f209ff3833be084c30"
      "5300cb9f4478edeea73e2d57ae88daeaf2e64f61c8bd6212ed1d488016fbaa8240544050"
      "eec49f998446d57c5f8adcbd290c18e5135a942313c206744b714526dadf7a51b2545290"
      "9bed52c7abd4c8f401faa86f9e6a9227b4e9a58157d95dc50dddd281973a0f2cf4c483ac"
      "93067f98b8be1771f7fc9298cbe02377255f416db23066ff173325954ebc40522efdb570"
      "a0268cadfed3dba6f1121c1395d36fbe6da08deddbb2a9bf58a97de41a2645b9ee9ce9e0"
      "3b09494e9fd799666aa9cfec102aa74360b05e68f5c8b12f4f34a8fcd69ef2795224bc7e"
      "852f56c4db6d4ec58684f059b0d198cce69d2efe4efb1939869f1d0b571326c448dcfa05"
      "fad882021b9f63fd9d55088cc93a8d091f4ad7548439c94a7513caa9e8034e0428c856fb"
      "df1f19d1da523dafdeb6e666449593f1b4fa4104ee6df058f519f6b3ce0ae7988c64c700"
      "a33d0c2e21339ff32e2c3f1d0eefe13f9308985b81ce02e2990506ef6b2b3406350010e3"
      "c2a2a2c69effb4982e194df99187f333f849c3aecf1bb27fd9eaf7dd52dea837ad992804"
      "32384e79dd023135d4bf0cf98a0f9dc5e3c7a656f32a6047aa73374f7092b21637669676"
      "837ee5486b96c2f90a6d46c2d6cd58223078cce70bcb5ce2db96e672941c06b460e333d6"
      "9fc336061644d5f57aefa28c0a842047bf7e20714facc8822df6e30b1dd99f6255bd0bc7"
      "b1828477f7101e7c2af9803275fc08ec22cba5b1b7a5126e7ed9fc6f43ccbc6cbb1cc9c8"
      "1360d19624bdf802085b9f90a9323fc884f46f6698e29ea6895f6c03cd3826c74ea8953d"
      "a759208c97b48c9e73b3f478a01dd18f6bcb0374a35eef77941ee6c33ddab5d7b2e13292"
      "b4e8ab3de8d31e7916f414ee64df6071b2e7cd335805bb6efef4881f60b0382a7c51f813"
      "7c3e0f2e4f8a84aed99ed2bf854729990dc608f8361be16ee17c086deb8e30670fd7c66c"
      "ded882e4ac32357e1e794afd020beb7ef78812f161084bd4ab243e06bc932d30052d7715"
      "58f7e335134f5696c6bcc05fd8be23e7de4809db0f7490da30e0cffab40eb4d67b4ef260"
      "516bd9a27bd052052f4d174bf878c4a5809acfd1bfff87277b2300d9d9da0881b1290cf7"
      "88cda17a8afe0f8375cdc2cdf53d76322c31d00a1de7dac2259ca3babe926481a64668ec"
      "f5ba571e79f2402f45dd65afc9ad03f2547a6a0371dca5f9cb1088810194e986f2491bd9"
      "b3f288adf603fcbf60b3fd1c210519178b39cb61250f8b4627b494d107c1ee111ad1680e"
      "4f0167b31083db15dad426411012b201611305b16531040041f51717aaa4c0a4d35d2f28"
      "b95097e550d8e938aafeb733234a8b158c519d4a5c35fd6b359ebac44554e3238b53bf64"
      "c5c1d25a341f59338661532bca8acf59c016487f532dbe002eea9fb952865590c8a5fd78"
      "da3ba3d4004aee0866d3a093b827fc162c2870e88478784d7a5f1df3833f3a2fc43e9758"
      "d9be0fa0e01b3307f0d8ffffffffffff7219faf117caee2f2478d00768dae6bcacafbdeb"
      "c6d9778dac54bdfe9bf3d106fd601cc3c265cc8d2be865dc96ba92c4ad96917d88b87b29"
      "99443f98a546ad85f0c8af957f0703984c76e52f638f18cf71195c78f8cb21bfebc2ead9"
      "0eff009f4e8b17eed014167e49e4789503c99ce360ca4502d24ad9cdac16f8cf6b615b82"
      "d1ce2142d6d2fe78513317bec6ad4121fee4429a0c8626fcc862b5b34925ef05c139ae48"
      "0bcdb5293b3b16ce1168f2b3f6b2486a323720365fc29dce3db750f52efba1c5e745617e"
      "0abd685cbc874a5ad29530426747dff2df741554818d43cf984920a521fd9d8c2e844c64"
      "aacd023a50f747ce5022a4ef3ea85c0e5a4b81a44cb925dcccc107fbc60e836012c5288e"
      "331d5fee16e6579d03d1830d6cf63e70374624aec029d872257f9b897fba9f9153b551fe"
      "59789af4678e94ce71078a2fe6130752a25c8b59abbe3828c6f3824515a634eed36eba9e"
      "ef2696cef0d8c596c954b88142f5b27020481f4dbd14caf998b260f3b421a9f8df5187b7"
      "0484bf8655888ce8c366b6bbc0ebd321c3d153ffc8df9da2aa8ebcea42db688ac9b0e785"
      "6f5908162f2c23143e8b292c889c5d3f03758d649d2409987f49df298f16e44c376013ae"
      "ddf7dc9da6e04ad9bd6c6110eb4b39131c48f5b632c85ccfc5b54229def907f4109d1fc9"
      "e0185800b2c6d9186d3c86e9798f9067aaa86e526638a7c024ec321ef1950b075c0d57ac"
      "2f918c408b6e688bf7377b149e46cbcb93e5c2d6f8f399987b3ae13c501388ce30f9841c"
      "a62f1020221cf799e7fad719c1449985c8f6aa7ab3669737778cdb1525ad763de352ade4"
      "009eb8b4bc58a3e73330174a8256186ae48988d1841ab39550a93e6d0b1357b012985dee"
      "e730e635649636a0b6458463b97c13b6eb458bd75f3562e3ee951a5d7c4769fdcc5a4609"
      "dc99535fd7ce995910cbf065a257919668cac05ea3283f06bb87eb29190539fdd9425df3"
      "73c40219ca6ad7c2b4062f66929a9d9559c0f08ac52b39e34895fc8ba84bfb3d0f180276"
      "ac641d44252ec3236ccb6bb206db5ad475baff1e44b771ed3dff0812d62a040c43b351d7"
      "44dd7fd9b52b06c4b1023520d603799709b4294edcc21d785c4b9a7d591171a7644241ac"
      "78bc1fa38c9152194ee34c9b4bf0168bfc4abfefa848a21b8ac1b35d8b062a26c9681ccf"
      "dd11e02c4b5dc820a8aa6ae10ee9176deeb512deade9691982052c611ff5db879197d339"
      "38700b29e086a2c0422a4b07d6c59d55d5e6bafa044db7dddcb5cad46289e409922f64b4"
      "d1eb8133b83c695ee404a264c9f186ab367501e9404e5a23f21474656bebd96ae3ba0a70"
      "179ebcd174363da323c51bd97625401ea758110262abc9642da7bbd5993a784faab276e3"
      "0c1aaac9480d4e492a556e19e0c3d08d3b1f70fcdbe448a2a3cf4f159a655a150d8b8986"
      "87be46bf5d632b9c13e36cd0b3ebf9f7915fead4718f1299285c9a4df362b7028347bf22"
      "2b3641f366be48648001b1c48b1785f3a93306caa46c3669ea0d822e1a0a4b8805ab162d"
      "fbd372e6c428f43463752bd4f7d6c68aacc1d264b35737725aa13e232661d961eceb4e04"
      "ccac77ede13324bf4bb20e19cad231ff61cd1ec45a1d494be8b74601cc90ba066cf0bd6c"
      "0e07399d44a721757d4e92806fcb22802b2c08c327249016f4d432d10a11e74140311779"
      "827443e98fb996721fc63932b1c8a4232475d4debf2fcfec145519ef7a0881efe846fa92"
      "eff8034809fa846fc1784a8bbb9c6c94619538fc150ac5efa1fb630d23af061bc2e87b75"
      "fea51c748c9ce19003a42a00cbfb9ec746913a8cc5db1c065104d04e30d93d62963308ee"
      "cb1cc27fd98e95ceeaf05b8d39747863ead1924beadb4404ba9911af00889d81361e2183"
      "bf4d3f1872aa229ab1afbbd56d4e6858694d1e5f7aebc6a73e4cbeddc3ee638e7ae69e30"
      "a7b4ac67b424b775612a7986b2e5493688f07fbcb25d86b2e74192d714b03fa898524348"
      "ee21e313adea3b33f94a876d6bf41af3312fbabe3d5682041757c82219f501f55e8f8a9a"
      "99606b3f52b226e3dcb85a02bcb1898739ef05807988084cb7aab91d5ed8ea000438c84b"
      "79556674adf77794186bb255028cca2df1ba8be34d48c79cb2c5b5e800a54155fbc7f29b"
      "abad51cdc55ebee2a96e0918edcf382a04f5ed2be8d4d334d9b755fd2a6aa4cc2520dd3b"
      "ead1087379e937bcecef6f90c3c7b975bcf90dc1a169e52748bbcfb0675b51c13f6d803e"
      "f5bc8b229d9fd135c292c12b8958d3854708661b33f6272897c1167c9c571c6e1a79c843"
      "16ee7fa0da7a6b0caf789f0ddd71a65977a288f53ad7ad7a56dabcb012a84bf5365cd615"
      "f409743ba0f02d307e5dae63967ca956d54d4d67939200dfe79769b7e960aa7548c1aae0"
      "2c041e8109171fa1eb9a418eb0e8dd4a2278b13b902019457b5740bd4bacecba131f6844"
      "fe239bf0eff5bf412d300d8f14c902d644b2ca908bdfd5766a9845c1a79c8ebaf9378cd4"
      "784b805dd57aa7da23671432d0004ee87e47e7f5cd3a7a25f0c548e80146b03e9d7498d5"
      "6557f8b0dec208e3b7680cdac1ab569f751de460c2cf698995508397c5369ba90f30fc8e"
      "e7430db15260f5a1a7cc7e7b1ed004a4decdf5aead619638ad9f83c890f600541e332422"
      "531709f2585bdac8edce66348772e2d694ef409041931f62f44ed48609b10dcefc89b881"
      "7ed886067ed111b5915d4da37c2298223ec7629c6b24e49004bf3858b3947867ecf1b3c6"
      "15125dbd5d3ebd8e3b26eccfb9f3b3b9b650387241942302ee561ad7d70e12b93baa249f"
      "1e21e6f758ec3096df7aaad04ae2c167e4c23d73b7a135567b5e82d098e7ebf815c1f83c"
      "a26525af4658565c3b615e12e4c408616b534f5a6a4ae9f19b4c1b66f51c4184c57a5cd4"
      "4837400fc598e26d837303b9eb567e193c39968b9effc0cd0f152494b4631c9940ffe116"
      "b7aa6363a3a61dff01594edabb96dace270578ee4e50f4122fa52eeccd7eaeb61461c72f"
      "90d1230a54f424b090f4334bc4fed186c0e51ba61170663451e1646c24a6c4bbc4842b09"
      "5b22e5a07b2a772986d4a2cd70c5459a4d36dfa45463cd9685c083651d7f142223c2edde"
      "6fd8bd19fe7acdaf35d2a0dfcaaa89f732bea38f8f26d375263e1c51f52843e30ea61f2b"
      "a466846b62867621d8696b8acb6d5e5d7af0c6b6af9b347d459df83af0c4a5ffc9910403"
      "c7313047488c1bae233c3df0155c47d96d5111e06d48978c8dd0e6746ccc1017598ef5b8"
      "834a8f25d2269d65bc6e2fe43ef238aca22db0155c5eb2217b317a671fe633d88239a08b"
      "7e8f1ec059774a94886a34f3ad96c81e6ae716b138d702b6334d071aecb68f1be4209a78"
      "180dd5b284e011bcdf861b06ddcbe3739d1ce2beb935859edd55f2cd31f4c7b2fa72b831"
      "54a51c20f1cca42f606591ce98d41ae9a6ee60b4af55ab62daceed5b38e0b76b5b061486"
      "4aecfd65981014e9a3cc9e2b2d20f51e52b059dc831fe138094c6c487b481afb593f7e39"
      "c18046c1905bea6ac30b0e106e9e8e3583cd3a43f7f9dc1babda14e5c769393d9f1b6f25"
      "a6f078f7fa990f8d89692beb153d23a43f8eec9bb48831a4991c8640112e9bea834dfb5f"
      "6c314130702549a7b22135c67cb7c268cfae291be70419f0650508b94f79bd9a4a234b01"
      "81ede018db317b04be719889d99512d645ae22910108a0df5d6ce508a8af9777d5c3c14d"
      "518e53b79d037849733504e041a297855e2d743a7070d42387b5308e244fd993c948a3f2"
      "82f93461e21429833c0032042d3829cac055de3f17426590177c79c0813cfa4fe60ca720"
      "d54627532b07becfb963ee1a552a1d906e7b7d61a1a712d3fb3e6fc02509961a2f32337b"
      "a986005718902ab079978a639a52ea8daf5ee2be23349b3aee71693e142d34f183590c49"
      "3cb75aa2434068d2f3812234bdb120bf1ccb6850fa1155fa216110235e9ec6f1350442b7"
      "29fd1facf365a8df5cea4b44054408c4ab405a35dc3c3ac6a3a2b42d7fa3df14c0164a22"
      "c8f2af1accdd613e86ef00db21d6acbe1c85061e5f45530e4c84c49f4c14527ee1f1dab5"
      "e6672c749ddf2e1749acb9c51e941358e91cb842d6a01198d90739afd4eb9b9e3c13f492"
      "adb70da52035fa94025b2d4287ec6a9907ba3ccaa60d23044ddbe72bd392511d0ffe77cd"
      "7ec510114cef3c1958d2c3dd7a735d829e4bf3ed140fb27ab9a685fbf23fbce0fdff3337"
      "6429245ea43c8682fe65ad8559891a47d6f2bb4a6054a1df87e118ad4e040062c17c16c6"
      "0a80ad6c85a08c5e329958277ddaa01d112f2cd0bcc47a7e7e1e7d7ff6d52a59416aa4f7"
      "9c714f33953347f95e54f57a4dd67e62f69ab4677f0f0349cbc122a28cf960044fd00a88"
      "8c646a422cc1b8beeed0c9961e2504b0608c212a37fd0dfafc44cfac78aab81e0f8cf761"
      "e7b605b9253d7d01526ec5e4959bae753b1d1a57b462ca4c808fa919ce77c61e0c4753aa"
      "85a4fc2575f5df013d50e8eb6ef50cb31b0ba364517f55add2947849dbbd8864e9be4333"
      "62a84c9da433868625a022cf74d703fc55b6726d8523d202cc75acbc5a23942b856dc881"
      "377e4b32603b3666bc017689f98afe312329b11ce750455fe2d8b5fa373b9878d77c894a"
      "33b417b64d79429e57c8a7ef9d4cd24ecf1c99447f7b8a53e328b01f640b143581ba247c"
      "0729c5a5ddf145edd2240e97e6da10d2cfaae1615d927dcb21b613139f341e6a3bb56240"
      "0743c846e75a654012416964fcd0b5ed6603373066785d51122d1d301ac3974c825a5a61"
      "fdfdc1c9b183e778355f54e96333a845a602a1ea673c10db6454dfb28ced40f3d53bbe9d"
      "088ad04202f15572bd007de3bf409f9277331773f726bee69d701ac4a7d25e3f148917ab"
      "a7287e90c0b4b554367deb34c6703e4e8f7d68014567df3da8578fcf282e798ec59fb6cf"
      "c80866cd233112a6ebb6125b3b1d36627e0fb6da057708805d4c05f87ee2499074484648"
      "67041379ff940dd4d3088d90e3a258a4453516fe3781d6d966cafab063db5c4fad9f4d22"
      "1a683e8aeec275fa2a31a7f7b94d1c5ac411e52fc3e44123fc7faeba32f852ea2f311d07"
      "99404e091109bc07592ce6c40f0eac30d8df26b8b4977dfefac3424f954a08bf2c2d60b0"
      "c914a947dd2dd14b2349cb4407a075b6c1491e5034bad843c9061cb9499c80f44d6e5bb7"
      "6a7da03bf2324f3e0cab6145291f751a643c902ec05c2a68af7b8ddccc5438a70d137fe6"
      "a88e0472e72872161f630a5172529eafb968250ed634fde0e2711ecf3e0a59e67553b552"
      "ee827e592f3c4a05eb6275b9d61f00110aa669ebd37ce19269a8934bbebdd2d7f09388c8"
      "87607bffa8803ea2dd590d24dc802b37a10f36bef3c2dd2242f34ab46cfd2d61a512bcfd"
      "eb77ff53118c03f93347c3e0bb7c48d64c75f97478e69ea32773aa70faae87b5c934ba4d"
      "fded3decd8aeb17a5456b0747670868166743da9a8612a7748c3d305f2f2cbe2c38431f5"
      "79fda1784c1c85492a6e02565cbce177f3edce95ea61c9254b7c7c11703d1ab41b66098e"
      "b0d26e221a485fd35648323cb0d682bc6ef484c332ba72b1a28435e8c944678b8b50f449"
      "6a84fe1c4d87909963d57302f8c836e756654f4133943875996dd783057618b1589c191b"
      "2de55f9fb01bdfe8937b7caa3211da7df1cf3a1a25fbc0cc3b391b29fd4f1b5184803760"
      "d6eb3eda10b271a6b9edfe90a2f5092ed859b90895871aedf7d5953fd9c1cc7e06e239b5"
      "24aaf4965b295cef8f012fcb5a72090b7f0d7682a7a68891a3d45b99e8d00cc5fd3d3621"
      "33e445c2c4d806d572cc5aa127d52d6d86c56649c2c0c10dedf41977a08fd71a554dbeed"
      "56fd2191570ee3dda16aa54c1abe0620bebb62e8ff3d3ee0d457d5e0f7a237b720991c95"
      "818dacd98a0af441b6dae447f49d7b304ea9b21c3d58a7a8257edd62ad4f02bab9b89dbe"
      "54c1f2aa61afd6aa742e85bd003ea1a9872dfda478165de12e9f2ed5f2b1ab3e5269cc9c"
      "4f3ae1eba926e3cadf54a7a5c10df64685c32253bc7620a9a83c4d121147e79d93cdd7a7"
      "c1465a471543912677cf7f145c7efe1c05473e308011d44f5d077c7221b12cf24508705d"
      "d7548be1fb279f38f00293133e283673eb4464e69325db0c61a517705c565a8e8095bd3f"
      "8d986ec33760bde83a13073ab9228d9eab2dd39b835c9b8ae1735489e69381a9f17348b0"
      "86facc9f4595311df69fab253203bb3a62403cf1537485ad21e70939c0a030f2c9aa5edf"
      "572c0f8f7543df2d8fcdc2be65754da85fad1b7ea055b4e387d30769a85ffdae78d21ce6"
      "8ffc3f4f3cb40eb7b9bed9b81a197cf16b0d4f46e4f08f26c1c3136e05302f3c24a984d4"
      "2994380aa534db243453c1afc6b42a2cb590b3787258d7b845e03523aa6a77da75755178"
      "98faa61a8850ce22401110d495587478a8acfcf650210d51e880b6c5b05f242fa354fbb9"
      "fe114f3f811112eb26f71d14b3d3841a53973d064279d538a2724aa0feeb5c3e121ff550"
      "23ad0e0d586c94e83af3e2afe78b282c2f7252725c96d36d7d6c18d1ee8b99448b2c2c38"
      "98cf4afa26d02f8ccfe30b5fb8363c0f40ea3da72d0820c3818ae9824e96808b6a4136bf"
      "f00fbe0d532527995d71d5751928394eca144c98f5cde70a44a525cd03df873906fba4ea"
      "08063b23176b55c86d1d7a4e5e1196f855e5fac868cee74918297562ab98d5eef11902ad"
      "fe7c2e9ec4e3b8dc911d76ac199700e362bc2c26dbc1b53febdfc5b804012f65b3e60acb"
      "c071c545980e403dba893f1ba45d991292e8860ada448fbd885f331ac54943aab1e387c6"
      "e13453a2c372b4addc613710661b64a5890064cb55230d89fbf5d1d7336f84cf859b6d46"
      "33c193a1e439bad6ee4db85d6eac1f672335082ac81702af43101afcbbb0b07f8a0e7de4"
      "f1953e00ce53e2a67343c263393f20e6909eaf8c0776b86f7f3a36cef15e4a2cd2f58d3a"
      "e3de1eb6f92fb5ba4193095278cca673f4838926902fb1f9f4134e2b695db8a68dc09083"
      "1e6d5eff5db41f760f71b08e738f7bddaef24c5a268622ec095c07e86bba75208314b9f9"
      "b08911248a5b2543b4427e47c7a406bdc002be6fc9333286d6eb1b64b29c87fa48e61314"
      "ff2a102f1a0c9f93969f2f5b11e58b26b46e37fb125e044e653398f09e56341b8df20ef5"
      "9e2dcb828dc5bbe7827b82ea48ed9932da4008c71f3c9a03b38e2fe11af20999c9f679c0"
      "c725fa016b1f2eecdd4a4e93d9156187bf278f6512cd0773cbc286e90ee808d748045c7c"
      "ecc681647116b91699f71b5040758fb2125301425a743b0baebd3b4d05c7da366b71c536"
      "b5bf7f167fb66e7f9077e053694a213e7e6ca722e963da499658d120227dc4fffa885f5d"
      "6d23a6a482d6d7a5710b010fbf8d9d8ddb2effa52e4ee2e6dece00e186ac678dcccf7445"
      "bf8050ac59dc33679a9deca7ed89e825e7cc0ad9020fbe9e7d15312a08d478a05f5f8d25"
      "76c82f0792114295cf9bf132053f9ddf98e6699fddc8b6e9067a2659c9e29261796c2e71"
      "d326fa7a70f4ab7d0baeec8243d01592e0a11f901e0b04ef916b954505582c7c38b8fd26"
      "a46989b42b74a2048bd97819a680dbfab4de25a5a8816f42f6c828245e85dfc7d26ddbf9"
      "285d2c37217d2bd449a657148d2f79a42e865891f4512f15708b596be0e08b7ab9afbcd9"
      "22a4270d894e47b6a40f22c55a49399fe8110dba828f085391933bd8d9cdf26a0d8c86df"
      "f6f1692307863b60a917a408e2eb34029b826e11697d4a65a06433d47a1c0dae3483d866"
      "c98bca2ef16b71f69a2e0ef6b1fd55f4d053e0be01e0487e29f66f690c62dc76d4546e7c"
      "d4d39aa9a5b118a6e2514f9788851bd238c78276d641d5f4541920a92ed53e79ee3e605c"
      "7f2f22d084d9be77c9d3907859a8c2697d8020bc2a1b7d1861f19e7fd42694dca2a9327c"
      "886c79293005d2d62700beb76729e44bcaa2022eae50d276cc0c9ef50dbb3d2c282ca648"
      "dd19474090c1ebb6d38166a85a6b0f566c1e718a42f005388e30018069080a2d36e968ba"
      "6a6b6e9522158e39a43b73a38058e0cff066884da2902498c73132ff118550a059281997"
      "735c5432f64ab9fdf9c6e45defc2c8ee70752a32f8a22f264a33113cf2a18aecc403587f"
      "deb3c92c6af82488073d79025c7617f2221ca5670db1b70b91c932dad95e75a092f01084"
      "a3170ea696c51c7578db30ff090c0010849ab2ea5441ccb4e23709b0e19f36c4984c0fee"
      "ef219c77b2982aca2e3cd8737fb96334695b5b1749b4b78f52aa3efe27473beda030bc25"
      "ba93149ddfe45b843a893615c46508e126a50707a7f0a5cd0115be476a83a6e864820fa1"
      "644569c1dcbc15e825215257b0c95815dfdc5dbc106fd41fff5db076f2ee282e87dcc440"
      "631e09ad0f1cbd9b27d90b0ace1bc7a93627561b29fe96f1bc690be28cbaed49beca1bd6"
      "f4d1eb48cc8131d45631828b5711eaa69e2385afe44006c476a70f9e722daf2270dca3a8"
      "bc241b37851608940bd4bd4c73b51236d6490c65de4ce856267af897fbfedb5b7e65b60a"
      "8096463c3d61d40eaaa54262e536230f9b212e5d37f619e401c830f529be2215fc66f7f9"
      "e80fab692271bcdeb00e3886db7e5092252e467bf8080c98a7642760bad31550a74f0efc"
      "e93f27caf57433970a204c65c2ff9eba9748ac9eb2b67f6156d92d88d132a98e817470c3"
      "5b2f344e7dadd9a8413c5d36366cb8026f7f534de9fecb5f0f5d8006b318b6c888ca3665"
      "086454a6c693c0fdd5d05059dca462e5ef466352d0a0c7d09660239450270624fb815bd5"
      "07eb18f252829d7517caebc23004d5edc44747eacb4226c419c10768cf571d90c1bf1396"
      "7e71c30c06d04ccfcdcb3c1315c4ded250ebb3bc5e391f6350938c5a8e314141e417dd4a"
      "afddd5ddd6258721c84eaeaed6ed81c8731a1e8cb57a7fcf3d0511e22818fe4b879e9171"
      "e8465db90a983ee8550ae30333b73938586faea4b614b3dfc990e889d6cb07cc316b9aad"
      "9e5bfc4ddf12cff7cb40083fca1ae377fd988471593d34bc24af51faea96ed3f2cc496df"
      "795281e062093482a15cc87e386f9f91fe50ce8c94fee284cca78a0e274b8f7325b8a8b1"
      "d1bc06ab9670e47d8358acafe6df3dcb1b7a84077fc6760e9aa62de4a09b280ffc943d4a"
      "c0bd4af79d40300025e7c913fc89926760ee4ffb9933e28f4d0e5c05e8189d047f43eddd"
      "1d55c750432b2d633454156ce824729ee64d37bfd7d4e52cc7082f571b4c9caeaf2f8c02"
      "38ec296facd3ca77114844160700fa8d5595aef37f5d260bcbd2d850f580c45a4acda2ad"
      "865733a187c7e3b1ac3c8cb493cae4db0b09864f1ec50dc8bc76de146f7932d3b0a1bb4d"
      "11402c58885881b1b02fe0dcd7110934ed215ba86aa473a26d4eb9ef56b98c5b9bee57ca"
      "9929652e743aa088061803a307b33d04af89e7deeed3b4dafc3ffe35491fa7dba31ac5cd"
      "c5514f5a6cb6a5ef89a5d2407ef2730673d8db0b0d7414db75a08339ecbb1e70d6901e3f"
      "b24f31d44d55abd4d3fa388e69f6007ede703cd21316637b9cf2868d9ebd3bfad8cf0710"
      "dc0c0ea8286be332002786d7c0b98111ec59839fb341648a8f1d1d96762a94fd428bfd0e"
      "b5803032a4e25b959b0347b731ed649e951ba67e4f4e1050a4432c0ec51d04635f6d4dbc"
      "aef3780f75a9a0e05c9b03bfd7d223f7733d8bd0eeb1b5e6a36b5d6994c893f29544da8c"
      "a8039d0d3d03014fb613ca62983f6668f4891d118bf73fb1f341450dc36ea4c8d32583c9"
      "220e19212f9b38fab8916da17f0c9fded4d7c1273089d3b9cdd6657a931caf67daaaf224"
      "cb20205bfed5448a507c2163bce187dc2ac7fe63b25c1943f93d356e4e40e67e8db7a5fc"
      "b71931212fb1863c76fcf402ab4e731ca1f30f09c1f719ec8f810b291654641f17e0048a"
      "b201208f2604f5c4ed4e3760707482a52f3940d54cd1dcb2eb16d62a01163e207a2087c5"
      "be2aeacf30d12fa5c195a3759e87db02aa3d267eabc0df176a50160b960a0abac78983aa"
      "b5a696f5b4ac2d911aaf95e9f27c067a9936a7d58448fdb064d1b9c4cb843cf91048a84d"
      "d7e106180c8ffda5ef4306d9431d32ae1f712b0847c5946ba24871ea381939956c3f1f06"
      "ef78543d8cbe98e04a9fd57503c773d4de0b918c28d221572603d677faa2ab509f176c3a"
      "0445b6707cb9407a2123350f2996bc10a4b0e1455683d3147e9a6e980543bbcc16181a09"
      "9d84233b408c839e621201cff3640572b30b33663044f6c26b18222351910463b235be1a"
      "1e1226b3a9eb21cfc1ba36387ad32c31957667abee364ce377f21b7ce633f81fe188d99f"
      "b5c005a7d279ea0eaf1b951538d553f2167a983a175a89a63d464aac881ea8c3e9510feb"
      "dceb1b88a613f29da14b9763a7b922cb97782e1007b81a196846ab968292909bc3b2dfae"
      "37a69a91ca2b82ad45727a3013df636fe6ff9212ba5a311edd6c24ce1c226f1972e01d1e"
      "3e2c11f3128173a7e574597babf92034b4265a7954ba0435e549acf9c172f8918b575893"
      "67c0f26eb1db168c6696ef6c12d165ce696736",
      ToHex(captured_data));
}

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
  OrchardBundleManager::OverrideRandomSeedForTesting(kUnshieldFundsRandomSeed);
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
            "00000000006f3ecd55d028513d2e8438520afba943b9b08c7a2f1a8d60a09b"
            "5f" /* hash */,
            0u /* time */, "" /* sapling tree */,
            "01402e998bd87610213bf252de2486f2b1fee9bc0abadcb874c5bcb1a288f2"
            "b20d001f016013236dab911d31fca5e1b8e3f91978a9a961d2a3bceb9e6df3"
            "c2012acc521b0149f1a3b1f33ee8f31b0af63b05e9c9d2872ee18f6420e7b6"
            "d95e1c98ebc3b82d0001910d11f215f2bbed7d8ab93745c1fb4936d98c3a27"
            "b707b4623856517e22df3e01af345e1420ea4164e1482c45131e27d9971934"
            "5e9a979b65a9365e9c6d07fa2001bb016db508b4d735447b6d3e8b657d8529"
            "e2304cf15091346bb569672f8bce1101b57679e11e026ff0c52d93d7bb2a12"
            "911c323d6acc1b357f1b4facd14fd8bc1401559356c3cd59408f21a491aba9"
            "6293feffbd04a392e894fe4d4a0abf7812080d0146928a2747437d53dadfc3"
            "60f9a1aa7efee8999ddb7f4f433925a23b2fb5eb3c0001e66fa600f841db69"
            "98165796315a885f31668979ba7335170af2e0481ffda4180164935f5a70dc"
            "cda0ed0963b96e1e2eae4c401545df04a3784e572e47f95b50030000018f37"
            "179e8204e48fed138154f5ac888a307467595feea345556d3e3ee7cd6e2d00"
            "00000000000000013f3ddc746e57791a2cf8900143b86b9ff7b82454626f0b"
            "a633404f9305b6c32701e2bca6a8d987d668defba89dc082196a922634ed88"
            "e065c669e526bb8815ee1b000000000000" /* orchard tree */,
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
      "0x0600008098b684d85b16a5376f97340083973400000110270000000000001976"
      "a914d7d70569db3474c588f952a198562a7c24affeaf88ac000003a105e92761"
      "1d4c4b87b53ee54adf4ee0a406805505871e989821d839d07b420d2077e07a39"
      "f7c7d46bde478f7ace002be596b3ab2be9178dcc07636df0461d0cef940238d8"
      "7e53f25a71a490e30720a855bb9865545684a3ebf8ccc82da866be712e7d7335"
      "24e23a388f545a998da6adee4aae86bd4fe4915298de70c928c13a3bb29cfb33"
      "25d266a9eaaae0a49199164c5c6088aae2ffb38f444f2a07cdeb21a8da000000"
      "000000a9da000000000000aada000000000000abda000000000000acda000000"
      "000000adda000000000000aeda000000000000afda000000000000b0da000000"
      "000000b1da000000000000b2da000000000000b3da000000000000b4da000000"
      "000000b5da000000000000b6da000000000000b7da000000000000b8da000000"
      "000000b9da000000000000bada000000000000bbda000000000000bcda000000"
      "000000bdda000000000000beda000000000000bfda000000000000c0da000000"
      "000000c1da000000000000c2da000000000000c3da000000000000c4da000000"
      "000000c5da000000000000c6da000000000000c7da000000000000c8da000000"
      "000000c9da000000000000cada000000000000cbda000000000000ccda000000"
      "000000cdda000000000000ceda000000000000cfda000000000000d0da000000"
      "000000d1da000000000000d2da000000000000d3da000000000000d4da000000"
      "000000d5da000000000000d6da000000000000d7da000000000000d8da000000"
      "000000d9da000000000000dada000000000000dbda000000000000dcda000000"
      "000000ddda000000000000deda000000000000dfda000000000000e0da000000"
      "000000e1da000000000000e2da000000000000e3da000000000000e4da000000"
      "000000e5da000000000000e6da000000000000e7da000000000000e8da000000"
      "000000e9da000000000000eada000000000000ebda000000000000ecda000000"
      "000000edda000000000000eeda000000000000efda000000000000f0da000085"
      "c15ae920590a9bd5a5e0b48e1f6bf9d978edfbc333d8d229f2270490436655a1"
      "abc353056782e3d389a7ca0a351e8a9cf535b8b26da8045609d7a7ee5493a05e"
      "d8bf33769ae9b3a7e78305c615da062504978b964d654003f3b3a7f27f899574"
      "01c18cc4fa559ffd9a5462419da30c77d24b59daed34f3ecfda1f5360ecb8d19"
      "27f135bb20809ad050127d30410c087406f092c02e61cfe38e67567a3339fb74"
      "a524d945252deeaa190a0a8276fd2b83a1c4bb258b6e25932414dd9b09fcaa54"
      "bf873c587493bfb3e15bedd1f55602130d407e1c5f7318f8c0dc4106a8d69d28"
      "30b67e8ba0adffeecaf64596849c1fc1991451a21283e2e9e6c027b9098edb98"
      "0ab4de124ffa5659c1fe24fa9ecfda0df50d86678409e270887b10ad81555ae2"
      "63a1a6ff31bff66eb1057d4ce98b6ab4d6505f75f77efbb8362a74d38c3a1c8d"
      "f00a36fa63732e484e6caa1d859dda52af4938c8d708a971f2d0f23b5654b76a"
      "fa91f833cea0dadda122b3d10b2b6f0c5b892b27845a3239e3aa85387c447eb1"
      "e852e030dc2d328356dcd468828fe007295960b2e6145d527829c7354f7469ab"
      "001567848a145309cae051ecc7ae5389ac22546ab34734328a2048675f86bf1a"
      "781096dac0b84b5415ca0dc72bc25cfef1b25601bd1380b4d3d51a6a8f3593ae"
      "f435dcb38de125a07e89f2ff3d42664d002f952134d1acf5f237f181bbcd0b3e"
      "4da7ada2e8d907f34973d365bf232f611a5376da2f4280f08b49e5ecf24dd758"
      "43f5d1b352fd8db5660656de980d47cbf525e8ec3881c212fe0cc2c0c2ccb585"
      "7576de827ff2f665d5f9c3a3c95f3f45306fa2bbdf82bafd8144fc966e286252"
      "efe43e29f8892da06ac6592a0d3e707579b35c8c82f579b2295463ac09b2815d"
      "1f159933a6abdc3ad54c7b5cd05fa18db8f2e0c760d6d313f13737655d382a41"
      "c238c67b6bcdc82692c91b25d6f19607b0964ade41885c79ee1c12d81faba510"
      "fed8cfde09352e0e3b7a016671bdc82332e06f9f9c26af2df621fd3a1a774326"
      "0d7e3f6e4e7812c69256d52b1f5ee2339b564a57bd242ceae8865ecd8f8fd2d3"
      "db7ee631c1dc29bbb911acf473ef3bb35291b47e315a5cab3f7beb853350d648"
      "6f6edea80d892aee8b389adf2878856353cb165c69cd8d531819a2ecb92e57c9"
      "3a4fa4fd70d73dbd5e615f083b4d2ff614a19e4afa5d5c3d3e9f159e1b7091d0"
      "1be2b8af3e07e67445dd3798a63ea93465c04f1b4aeeeace87a54b36abe864ae"
      "3cd0e12f347a7126bf95dd9aa6e097b11d543d453c981a6a972df4baa152d085"
      "8fd90f91a6a8efd7d2521c718e26f23c73a1ea601d704be18d49295bc4decb51"
      "e4093dcf0c7dbcc1bb8f45fe85c8b3fa1e112c9b28bb39a7a4250bcd6af5fb6b"
      "e60cbc8aca9eebd385b153fdf99339bae76ca83cf9e74ee42a7f0bbd96b6853c"
      "c36806bb79d3379ac47c697f225bb0eaa46b67b56de8ffccf077ba4627163f8c"
      "33ce1efd4fbbb1039df975fba21de46fa907c2b825dcfcde0857339af4dc4f2b"
      "8150b60b51d9743ca5110569a4f35360b5610f831d880f08bf8f0c293dc19cb9"
      "d1f7319edff169dc76d29746f62046f7ddc57a1c0816f94dc267788199e4c991"
      "8be952e2b2306e987b2c27b21a4cdcd6358428b9891d65fa3211461b0b9c2a55"
      "21f15eae4192211dddcbdab566ccfef6ca2cf6d11c13de3736a19fc76b656770"
      "8fb58edb08376efec07de66d08b4debccbad0108f7d70ff27b036bee22a2c142"
      "92b39ffb58bc7f31a96987be42e308ec4d5764f62e56c180926fccc7effd1d67"
      "6e9babce099709bc709f8da1be3ba64e9c0bc713d17d03fa8a2937c3ded9311a"
      "9679f9f8f96eea7cee9ed19d31e2ad2b2c1c80f02c7932814137635d54a7e682"
      "8e3494c9bb7f2fae76316ed069173aea7f99be2a90026716595916c66a07ce0a"
      "750e2e2fa86958ecde25285e6b30b4e6f6a9251ecc4a6af7a9a814ac7da53240"
      "d775b56325afc627157d175e4bf5d73132f827673c7cd8273ed30ff4e4066e8f"
      "62d1cedde25a7eccc7c11882bdd41615afb85555337e1b8dc96bb057611f66fb"
      "fb46d41a392ae35550d08a30696c9c406cf7725e0a13d28aea3cc2a3a65fef13"
      "4565781b21a98ba452ce604c850d30b18778c4bb9853a7273b5c1fe4d9269973"
      "723f2f7cfa665a34d1f50b2925fdfe07e5286efb0e072a8470ee6a5829dedeea"
      "8e30102d2030b0caf1a54a4644a0eb3ad91279c4b07078efc1590f4cfeb00b3f"
      "a3dfe64e8b42dc3b40ad16985473308e6ab40c53575033e29d472cb0ee1b8488"
      "e9739857f56c9c4ff41f32a3b5f8cf02f927a3d20ffba2dd183b09cc828cdf0e"
      "a8db765ce32aaea9d9ce0f48bcfafc46b49849aa6d5308006da83a46215eec2e"
      "73cb39276c0fe3d5dcad56a4f4b7f2393dd171878b9749033075000000000000"
      "722a7a4378d3240dc294d2ec9a3347472820e070eaa62bcf21e397a016c83b2d"
      "fd4025b6c7c0754fdefa1a21d14401e9cbb4dc82a5198781e5cd8bb531173e3f"
      "dac8bae38548b806ee2fc22826b37bc58f102ace062316eb53e0a64a57846f58"
      "37979629df601a5c1931a4b030134b19439a5e7454602e87c36d7147dfdfb655"
      "d782276afd458fd0d80038b33c67a49bbf0b0db6535f829e4520a6cf377b4dba"
      "338e07fe210b12bfd0a2e5ec0c510453e8310cb3a0666ebe36f9b81f5f7d2ab1"
      "9903351282ef0d88fb4180af27cf52c77d358bc337374d8279bcea64899fd643"
      "7c3399b8c0a7394097f476f3c0478452c269ad29c4f5308d4373152e92a5c6cb"
      "143ba38e93bba1f9ce0653d029136e9f04f34e312d09806c6205006fe3bb9232"
      "7b2d812c3d2dbb0b7a6a891c6ea0b69fe30cc984c47ce9c9e50a9aaec301799d"
      "eb8d1d4f9ea452a031931347d7e491151be2cb7190e33b62bb868d0d8d30d792"
      "5667129b91d23681f5e1e3bda8c2ff79a3da03b360b6134b3e8aefd788bbc122"
      "142931b858aa641828fb5792e4953ff376497b6195198324f035043f10584388"
      "f1569fb12e2018cfe8be9a06fedaf0361ea33b4167658cefdf542e431e9d1bb7"
      "9a86174a5d5490fffdba37be5252bcc9647b2291d33b1b0714a70b2dac72d234"
      "d533185057d58bb996c688b458f06226f59462266770aa9e85015b1506c693ac"
      "ae66844eea6ae71ab0b9c0bbf3f162a20ca9a31da2c60dc87620152953ff57ce"
      "6fdc374f533f07c59a487b1a7df43407e885d4fb5e1afe50a5d29c59217eced2"
      "6e8e0e44ff5bd658e45e5f1e049e7e0b469dec46b6f380c66e0fc3fc20334c9a"
      "1590a4a241c44ea63faf4fa9487928188939f928e8968b2bacb3cd2b6f7f57de"
      "2bd6355242a10f07bc13b56087a4970a5b256efa7c6ff8f521cef839e492a8d5"
      "6f7703f37520184489807705ebc20bbba6b30f86e0f812577e50ded33189a8e2"
      "df332f3262445bebf94da95bd4c99095306c2a6eef9580f82a51631b9c6c473f"
      "09a92649431f991ef012539ba19294a976906f67bfc88e79899955491ed91c85"
      "da2d0ed560f1ee51e3cbe8756c7ea7fd20fb3cb765ad04c9f5f223751483dc4a"
      "b20026a9deb5c033636fad87d94a8a1d64236262bcb690196c9564bc4d307c6f"
      "310faa2f17f54af00addfec2579d1f2d9e65b16af43d17f106516905aa113bb2"
      "f1d62dcfd1ec5d243a087775bd1f9bb9816f78f32383d9fc3e3c743a5f2ba457"
      "6cf79ea52999f59c13e9c2584ea120c64593967fecce832492bb6a95c9286f87"
      "60d216b9b9cadb8d9b9568ec91adcfc018f153ba20cfb59ba338f960057ab5ac"
      "16a482c524a1a934284b985a864a74d83f5b8efa10feba8367c32400f8eb3cde"
      "d7730470f79e9d8dc5290fb4c0970ed10036b474e3e74fb84d89e1492267340f"
      "38cd97deab43f747db2a1a6e9d67ffeda81d87b788b28887d7ece2309712b397"
      "cc1e3c156e352eee98bd3a5e7444ec1c432fbab5f9824d1b8c644a2ae7f4e544"
      "940617546d004efbe71884d9293e1c6207fbb8ddb8faa75e744920dfa2d1172e"
      "912e93f7077667dc32d90aef1b4e2ef5032d11a57c073a02e787a610466d49d7"
      "a5d5047a058f21855ba9e2c685c4e5057319c4fec3d572bca88e40a6e420d192"
      "100125f87beef000bfd503c1639b1c9c6788d58dc6efcb0f05edbfeba794af12"
      "f1dd0ee1b4aaa8c43b9a4e4161bdb71847e8a374ff47879f7cebe7f1d59011bf"
      "138725a67c47be322647fccb31833aef7cdd899f2ec0321a837f2ebed48ab653"
      "6cf4b8fb29b2106990fd1a4a90a4e775b3e7fc947ed1481c11a48ce7d5b3824c"
      "9541b64ac0a661bad22b6ef7a6f10caaab99d2daef9e2bc4ccb4a781a754a686"
      "83e5316eeba77ffa73be1e883bea3dbe1b0047d553c3eb4d7ff066807acf9a2d"
      "9dd2b6ff5be3127daecd633374e33f92946531fd62c6421be4d97ebb7705b8e4"
      "0bcfb037d3c82134f83537be923e8094c05923bf43d7c30355207acfa9af992d"
      "25b203f14bfd2d6dd80b713a4882b8fe59e5f7b26da690a065e67471db59112e"
      "634e033c2d8bbb83f03311a1dcc17695f5b703c893b10e84f6db3acf4c4005fc"
      "ea8e89c9fb4fcecd4adfc9065ffb6cdf4201decb2951c90a48d5b272921a9f19"
      "4ea011a867cf80ba7c3f3688af287c9bde45c48e56f17ff6a28f6455023cac1f"
      "3c242591553b4a959d23f8efc4588d6be4dba16c15d6730459990c6b2ad55b6d"
      "fb749f784022a1a84d29dd09a8673fa965d2985aff21a279405bf7ec747440ac"
      "c7ff09f94681495535a93196810c6e8aabba75659a3775b037fd7544758014a5"
      "970a0a503d05bfc08ba8323bf34974730098bcc377c84243b276a985a2368d18"
      "242808772b52909e7b686ee5f6428172b2844dc95192a7f63498e96bcbfd673e"
      "521d9b14cd414621960618269645f713c5bb0cf887f3cbe7919b179fd82c0ae4"
      "7f410a41cf6a0aa59a6920b1103b20c5efeb979f315c073d2298bbda62afcde3"
      "dd6493e91494a2898f690899c707c8a01a8f9fe2578f09c64a5b81a7b0c1eb4c"
      "e0488c365a2007e783d32636c45068645d22868b8dc82f414aaa63704db66bad"
      "51388b2d3cfcba96ec841343eadabf18e5b8bb5639dd98fecbbf28635dcfa5cb"
      "f54b262ffb845dc516f4aba9e4191460836dcf073c4c90960f46c863b7b8e500"
      "7162bf44cc445bf8048ff6477f39071dabf494bf7600e3b1956d6ff685fa82d2"
      "fe6929430687839f606581095f507f9932694d26ce965afadc90ef7395f07dee"
      "ee612359b2f5bffca179ba09822504738f410d71c612be31c73f1243b6c18339"
      "0f8e275a00fde27f3757f892e2364e9413244b6963bdecb93cb81c29137d4e51"
      "b7ea91c3fe4f690ab354f5bc3881df58636a7f09b996c39a71aa8cb327654fdf"
      "1541255af71b75f8ce835acb450fe308865ca36ca6cb0b9b3d3cd1f9abf10691"
      "d6eea57b10ae3ad62793015377f91437aaa811c8684d8f4b759cdfb5a9d7d7d7"
      "0d78316b76dead228667370d9904fac1380dbca778b47f509d3e5ec494f332ad"
      "5b451184cd44c21252ab83d2960a616741136ab64cf6f47085c32aaf507b21bf"
      "50d5210f5c2c389ee2ba72626ae07e7466827490f367b760c458cd12eed082e7"
      "ff8b881dd4926202ae54822f4dd910985003cebc44a73c325ccfbe302e3ff8d1"
      "145e06af186d52c47388b4586ac93ba528ec3f254a7f95b4e226b1fb2d0f3c02"
      "6835b261124b1fa07dab70ca214750218916af1b4037b3fb9b9b162dbad1172a"
      "e713996c21f62749d7715d18fccc606afda784b0e1e64a21626121320d62832d"
      "0e439099a30b2927e40aabae26c5dd98dcca1d8c3a19a2163c85bc974aae62c2"
      "d95c066b14d6335cad2b2b76cd92cac8ef8bc92ef5a3fce6dd491a19d47dae53"
      "19f00847540078952c7f30246f27a60de186619507be8de2602934f794b43532"
      "e58e1f5da9d0ea63d272c2a3004989ec18782766490696b40616b25611a10bf0"
      "8e133a0d508af145fc54b894eb0eec205d33a7ab1c27f37a55fa6785d6485397"
      "0d432d9595e9357bec00972c56f0f04d24c351a7c5eda638540001d886113b07"
      "aaef1ee95269a6359317c7a7de0b921fd8bf3b9c009113b2d3f76deef6a2914e"
      "cef70624f2c5d36768d17f55132b5b06410413841250feb5a4c9175a090a446e"
      "8d8e065e51491f46cbea4b16a395e6ff82dd4179b61f6a4cf17c4250614752f8"
      "15e40d8bc32b800f6f864b3cb51f5fafdcb19574091372816a17177b46335303"
      "740c374ce1beccfb3b1b4a78ad4430f473d1264d7f7f81bea1c0332cd8793eef"
      "791735a89aaf2820f475ad03f0c7e9c6560befa76593527da8e830b3f469194f"
      "9fff2f2bd109cfce3212b4755455c27d1ba78a11f12c04bd047fcabb7efc9d67"
      "9812393414582f7aa29c2212d4cff89b33481d422e2069c507564b2dea1e94f6"
      "31c5385774d687d5bbf21a64f643491425bd1e8cb6058c73a4e9d050bf907da1"
      "490f1e3db72f5929bd5f90d5d503d86f8faf18c8b09a69fb2d5640ab38b2e010"
      "0216017100f698264452752d7319c0836b354dcc6e7e9fe3f0b2e28c70b96492"
      "5aa21c478511afe1dca7281ec38b73d6595776057457b089cbec59eed5ab3a62"
      "27ac178d00d9c4918af1c19fc9cbc5076c062731fe6ce5c3d12d1bb4d56c0669"
      "d9da169cfbc256aef60434ab70551b3528469263bff8f81f6b05e281fd7157f6"
      "bbdf16f5f7e70ec530be043d63552b04cfa3ca8cccdd4087d3af4a98aaf5005c"
      "d5713ddb393eaac3b6ec5813f8a6d74e3e2b2a46a23f614f7ff1df495ebcea0b"
      "3f4f2c228a1571984b28564ab4d8d439fa278471aa6a721d88e6bd1414db2d9f"
      "454b1b27e5aed3c7c8e848622a5d6f61ef5e509f5b2dc06755abcc2bc4b55c08"
      "5adb1bf6e61d8f3717876a0047753889cd8a03bb04ea630a3a1b1478a5104052"
      "572602cfe7d118811d0be8eb25f3368520b142acfb95be0698fdbe389aa8015f"
      "388b27bceb4837d59414481ed3c218e9461a472e055712e2d67f4300e231b738"
      "56113cf8566cf2f90dc1ffde29424f242527d31d7fbb669cfb5a3b7e38f84001"
      "a8a41835f776c9efd274f3979ae5b7ae34ea4ec95db0a6e30e07a953c48c23ed"
      "3ecb2204e6015339e0eaa5ee53da9ec6b7d916ab691b12f01c2e518ce8cf1747"
      "7fef15c6a5bff6eb1bc87043848b9d042c2662dd61a587078bd2d6bc6cc61497"
      "0b5b14d461120e5ccec765ce6890760eff48f7e62b6097649f80e96ccc581045"
      "82502302f8deeeb0538fe8e86f2cc4c7620d683eaf79cdf7dc13986d0aa28cd4"
      "0bf42db3a60dced3b2bf9cad29ef5949177db54051771717101ae5a3c97afd9c"
      "459b20c1707b62867e3ae125ba99e526dd688e9c6d341101a36c585be559da3c"
      "76901cd4fa5e05f3f4a02cd2f8c6871f920c72d6657df27a25df7d2695d45d36"
      "b2d83943f465028a98b344caf3beb3148b23c8aea019302a294152f68664e485"
      "d92824740e026da931af865a93f22ccd69cd1a511eb96ebea8792a85f1ba6608"
      "78c036286816c2559c90cf8e08424f2c6c6b1acda0f246abcd76637555470bce"
      "2f2d27a5959d9b63ecd4f0a0b2ba2e6fc1c063545c6b29d50fdf45e5e35c6f0a"
      "e367217f435e71f447d7fcfec4408b65d319cc7587e8f847ff71440f37df8d1b"
      "3a5b103ac4ffa5d8cc47ca6c23dbd04c5bbbdcbf075ba6653d2e0f454068b9fe"
      "f9a230f9364919b4f0feb1aa9b1741be7aa448eabc95d6ef1df1831c67276d5f"
      "a85f2b58ecc691263ef6235769bc13580b45bc4c043776d0563f9a192ce1b87b"
      "784f3b8d36e66c5e6f92c7723025be88c88372ea2f5c313ec3229affcfe71ca9"
      "662f3aa9d62eae0c2189043d25947edb063d4851abf8c683ead60eae0e102a5e"
      "1f3d2fb4d10ea3c67bacab875c298d3f32362f69b0266a7777d5efdf5c7d844a"
      "e0732b89560763293501880a427563ec80ea3b5b56cba949bb4510e28bff798a"
      "d46232dca6eb7fe1eefec831a8c2bb6ccea2497980e39ac951ecad379157bb36"
      "083232e0889328f51143250baa8f9675f2927bfc67901e183eaf74f49ad3590c"
      "8a5a3e802396ca22d9eac5659491c0eeac157754db944db6fee48a316a59ceda"
      "c34b32a3d4b14232ba80e9293a8b23eef3231929d1f53816c5bf572ad56c6264"
      "5c1e26b5efeb93c74cf11adcc669de0ec3a976d0736ff9268ed7e8df0f1f5f84"
      "6c6f241331e7014943305f129fa012d584644e2d641078e37d6cc1cf77a8f63d"
      "98a5386964aa72b2c1f0a1120bd4240db4c2744e3eaeec1298485a7bddba7ff9"
      "40551241b9e106fc3413ee217786cc6b1049526532b8126956350df16ebe3337"
      "bf5b2baad20844a71d3bfedd7bc618981b6e545fca8edc00e716b97a2ec61ad5"
      "05ac1ad724b07752e7e29d1347550c6a03619fb7bfc9754f2410c057dfa5bca2"
      "18df37f3cb47503ca81a7936836d304e295c46e4e7749fb2882d86faabeb0c7c"
      "ffb1021f71c199d3898539d094fe1503559bded25c70d01cc8b0230afd69e16d"
      "7dd709495ead88961afa25c7c07e37801b85784e68d29fec64f230cbc09f2e57"
      "b34f0bf1485e59902fcd1ac7cf926ce23f0bdd2b637fdbb7cd611cf5996788a5"
      "bbf9186b43572fa7dd2cc8eb5e5845b2e0754bc27cdae93d04d0701789c5eb32"
      "adf33a229b061658c737eb299c82086ea35c16d757b80f329da26b494e6f16dc"
      "7d692f6d24ed23419a36ad0b2eeefd84587e0365beb7c6101f5a90d5eec43858"
      "038102d5ad6aac827a9dd40c690cd6a6c5be4e460d307047dda203647ea5c2a8"
      "ac8d054d4e1b83ef920040a0fd86e63eb884ea75be43f9af2325d4f217ae8461"
      "4fba397a55613c659e2d278173f0d4c21268c3fdfcfa241bd521041edafecd88"
      "1edd0fc6ab12234312730fd655d804831e656c33e036381e4ec8e803e3f67d8d"
      "4021179946dc5cbccec97c3a36358e52f4e7d020138e063cbe1e7a1ce4c77aa2"
      "47bb37fec142b018a28a20863b5f92dadf635498e9bb1c569735826221e31151"
      "e9331dbc7d573da78712e16a371fd91252634c3e2d6b72f3843a519192828acc"
      "c83f023f2b567b9b31794b6102824adaa10721e27ec92fa7fe05958b2c4b74e7"
      "a71411cc27f2c73a44f341872e30e0a7565f84a3af75b37af62252bb34e75546"
      "d76a35ecc629ccb2505017d95832bd0e55201b4f4ee4430d3ca9626ddc55abf9"
      "e60d3e60f631dea9440f7c10592aab1f82873313de46d20d0dca1bc651e580a8"
      "d3a23f46e76a27b17859fd71f4c7a7047df8b94c99a713e9a8e447d0724805cb"
      "c71413ae6b06e0d2006b06c972bf9d1dc323a4b9c474df0793e4db3a89c01566"
      "10381f1fc004147e725609f7f37c7de94aa2651dbf339c13d6d10d5154f3c64a"
      "e07904af16e2a151f5c8a0a097fb666a6a83a059fda803ee8dce6119dd5936ae"
      "b18c395f5c153f195e5c921ffffc6b8a6aa36bb91c0b4f63bd42454ca677ad53"
      "0bb23228dceb30c96fff12d4b89192419c718413521604e506a6bb49dda858ec"
      "f5660bb4e81879c1eee6c89c35b90fc54ac23c1d70bd4a6dc7e86e016ee7113d"
      "72ce2abd8f6607bd174da2b3b0e1a932bc01f2275273c94da9c0466b43b79a88"
      "87df00a56c2b52e7b5a936029f5a50f7a821616b8a06db87be2eadd9412c4d70"
      "ab110ef21cccccc02f4a138df59b93eb088af9e68474e45186b4515bf62dce37"
      "f7083c43879c2a6fefb0c004cdcf25c2a870f41663e53ee8a893441a4aae8092"
      "1a1016d18ed2432b5dedafbada6910ce0205d068c285ac61bcbf0037270072da"
      "3e6b3329a5f9e04c8c323baa25a87be46a5ec80af46150a23c01b980fc2ffa1f"
      "16142dbbf7eb4ca9d6138d7c747a281e6f5bd491fbd64e358c826b1479198dc3"
      "87bd2f573013b6ed83a6a7d8358f7a77c9e7bdd78cf0451f069bfd77ff209c0e"
      "10960078f7a8bc12b2c1474930f511ab756705249e0661cd16ab3bc4717cf6d3"
      "3c730d2859614f6b911e4bf20bd8fd4927d59dd1a5244288162e9907d29e3a65"
      "57d829252c5602d081748cb3d358ccd77feb694fb4bc51ac5ca777dba0d5a593"
      "577f0609ca129f863d6cbc4d0057fb62763beb3bcefb8e7c74315528f1888655"
      "e2d3299691510dd32c2b6bca236f62b79cc67a5559b191f68454c821e3fc5734"
      "ad450449abd27665ab8fe5c591304a11c99b2a8fa6f1931516bc97aaf0d4f30a"
      "968920fa5220b1a872b4c1f3ae105a79d873dba08a1fefb3cca8dd9d4355d7a5"
      "ff9c3f6689e3174f7a54bac8a204b7114009706ad6a7b896485acf96a2040a43"
      "763c0cd6916ce331934ef9964570b6bbab6d080375a5df8960fa8b3ecba1a36c"
      "abe900e6290014ce948fae8f1e884ffa081da87731e05b94b1b32c5adbfb688f"
      "281804985aeb4d7f113c275ce10f9c6021cc5b964c32502a234779f4660513c2"
      "366009a7f403accb10546ab319723f0d0ff96f8de3d3b853d80c4d86ba97abf1"
      "84592dc3bed5ba227aa72eece9e67932d7ec1f4ee9386967e1ee28bb4573d17d"
      "563303a980d9a2a7ae5cee77b1e1e0a241983be639fb1c640cb82418af568532"
      "66e00e233f704ec84a52e677e217baad38ca12ccaa7d8f1cf5bcf8bb99b5d227"
      "edaf3fc370109b8268585d046d8d6ddc6b47de5401ef6c5af7558231aeb2ce92"
      "0e1a2dff7257e0ab76312f52ba352792563198d074da9dc17835b1488d4bd6f1"
      "088e16b0983d4097b74a55104843775a7d2ea0cf0a3ada5b707234f3328c337d"
      "4f553fda1eca3bd0c50a4dc2fb5ff6b227bef388593e10c443cf9817006b7325"
      "0294102bcbbeb61fb741f017fe2175596497198522da7584c406de6be6e9ef17"
      "3c5410b14a74a961de1f3109ffc546464f9269f3fed0b47d042d36cc9d02eb66"
      "681b04b08e385b2b7c5e90f33ae3e8d000aa1ee74b1c8f0d93975a83108df5e4"
      "385f282f6706968f4ae5ce7cc9144ced47a6a8d0155fbd4d1a58a207bd39537f"
      "e6d93b1fe506a56857903cca217bfc27fd0b0aa27c5cecac3326eceac2704cc3"
      "f6d913edbfe61f4cbd9cec58b0d2f71d8f2ad42f12949146483b59b5c1113a21"
      "3a7937646344cfce972a0691df5fc0c7f928963209f78729638a743c52e5980f"
      "29311cdff4852a6fdcd588524daf0d5b41d8de544e963da05def5ac271aa14fd"
      "e5841991927c01d69f7d11ee3bd37b745a2bac8d1accaa266dd9a081b1a404c1"
      "124d2c5ea6cccacf41c69da28158142b12d832face96f37427eb79debd43f1de"
      "5f00379e8f7de5a0505d5eba831eab2850c6b46b39fa3838f348f0509737d5b4"
      "5b020b6c3dc2fc130ac044a4a7841aae98adfb12d48a0e1c5b33d1d9598f0b02"
      "cda93f88847b405562f9eee55c3b357b2ba224a5ea483380f57cf38149099299"
      "caa407153693f05c1ad926ac1c0f130287d8aa413a5468ec071c94850b0eb902"
      "8bd916d5e59db0d90c8f42283030ae2e4547a01dc415dc4013c1dc7d1b9b3dcf"
      "b7c304c5b1e328a225edae9c27c5ecda3bec135e97a1f70aeb991670c89fd41d"
      "fea5028f9609c6618c05a43e281230f3e2da7914c59f062ae73fed3b1c5d2f31"
      "346e0aa72b583e62608090d692c7dd56b3950a12312677b0d880e31a37a8a87b"
      "3b02157ab6ace9e1c389f8a8074f7e743c9a4ae3a72be7e40f9124155f2a5013"
      "b83617b0f16e40d4e3c5df1636cc0d5f5afdf6e80095e3dd870e977fe8859e2a"
      "46490f9e3e60e78189460e3002952eb25028d434b9082211d7f4f84879236abe"
      "becb36a0b3ee63683a7996ba2db078e806f72aedaf81234246cf050f409062a0"
      "411f34fe9ef5fc671ac77a2c4931f06653871306df37c768bcdbd31669cac0af"
      "f322195c953b9b1abc483c785bd17d59da91aa18ca4e3830673a52b61e17f379"
      "c8241d550be7bbc4904fdc7f4fe31d87f0873335b0a58653e6c6fa59abe38729"
      "708b38c9693d881a42c518174ea969cd1654a899098d9908382d77fe36c4110c"
      "56a20558f91c89722309b1f13b1754421ada7d55e80e546537bbda1000011f76"
      "2c1c0db55c42047eefb7d13ee961e9e0201e8d9d5a767d4039bb4987eeebaafe"
      "6d7e1dd93d131ce95d65c115fb1031d1551906bdaee111e24eea1a1cf87ec155"
      "1166339983e4d359c050d6ff92f9f95d5476b5621482a21115d7418c196efad2"
      "f4c021229b60010c4e341bdcec9a13a245875dbce0b00b2a3c8483abf453fd03"
      "60893276e3025fe89859172d26b0f91b2e80b3312e062cfcaf3848382f5c9d4f"
      "22c92975d18f7bbfb19716086a87451bc477a7006e031c420947c03a1e53ebd1"
      "aad0206eb0e883c87c4e2ebd5f9be569d910f74db8ee406efdec47066b59dd02"
      "e6900fe42041d0b5cd1ea5c76c56b4da33b1cbb91faeaedc4f20fecac08c1100"
      "18fd2d64dce11a0f1487438668f26ac52af8bdc992284958b28ef5922020aa8e"
      "79273ff23ada52c022c2b42597d324cf864bbcbf54dfd02a7552b799fa64f296"
      "1312389253da434b46281645f8427bd3a1f53fd8788221e1e3de7d09f34b9205"
      "a8620f4597cb05c35074227097b72de19e408572e22950ebbb2c334080dd3958"
      "ccef0bf89d67dfeaca71ef5ee3502c9f8c0819a79379a1257a8a31833d7f0cc7"
      "37020e87d7669f5926b09a31600ddde2882b91baa97ea7156b11cab9883c6529"
      "b1383abcd87c1b08fba52e05d05ecfef02d4f7704725ad7bbb2f57ff5e898a5a"
      "df2a0086ab8d3b759d105575bcbba72a00f56bee876b67ff9cdcfa7996ca9a4e"
      "042c33674c9afee4b42d395a1357c63aaaa4d56ab63cca8319d7492402a2b86b"
      "e9552886cd10441066d87b540a751cf7d637062e54f9cd53dbfdb9a0deaf1ef5"
      "f8f60c340e734ea28d76a8d2e5d96c552cac3a156643f8a2d1230aa24a27612e"
      "bd510f6995b3e1fed4cd70e4f2de46cfaffdeffff3bbcd21d0b983446aae4156"
      "753f342784d673086748a156bb3e79bbdeab879ab230309349cce779565bfa5d"
      "1c2231dee8f60dd22e774fb0ae5f5c84600ac72337e5a8b0552551615463617a"
      "25490fe668625b4a656f290916e02a99967c807230b1ed14012d940ce4022d2a"
      "7ab20eef9fe138edbdfaba532fdfb988962d369a1100496267a9b6e074cc1027"
      "229416cc6a439860a93bf27849a0ed0fdc446b8d25162083bb2ba37c75d8eb53"
      "80b412446a3aaa9b86b0a6a8cbe47fed87aecf81995bb808f91cc7af01fe7bf9"
      "5e7c1a1d4fe2b7c9bd99bc7bde0ba6ec4664b2e76500fb97a3e79356b22a6ac3"
      "1cd42505d43d22adc5b28616b29307678d4080f5a7a7000b91fa46d66da67282"
      "f35d1b0300311212210e725190fd03ee41e39b71c7b5bbab11ff3910c0822437"
      "ceb318c238430a1c58789d264370b1c104a663e9b67056177b821e1d31d20740"
      "54bb085eb35c6c0d23416542188c4a8ef6fffcebf1e567dc9002416adc999662"
      "937b161a01b4f9cbe35938da06ffb0a411e71d5f4e9b465eaa36c45185892516"
      "616f076896345ffcc53ec71bbe18916c90905f563669a570d2188c3085544950"
      "0baa250d54c5e21724830319ee0f18d80fa3b17059016fa63db29993f1b2e60f"
      "075f32dbfa57f68de8880c73959fe61186475e2e74a4e25c7f1097f662651fd3"
      "67d23602d3c6ccbc71eb34af296245ca138d0aaedc8e4c359224adba43c4b2ad"
      "96aa2a1d3e13b39648ec3de311fa32ba717e6a46296628c32d75c4148af7ef6e"
      "cb911811081095af15cc704c01d160a746310674e1adf5a1bd7076f20abf0876"
      "682611fb4034e9140b9cdaf32038ea763d9df26a4f403ef41852e0683c80f78f"
      "bfce291e323d093e82da820c3e823646447dd35ac06034b262a965dc315820af"
      "15af0911d43122c6072be641f876f5bb251b6d83ed7494bb7836cf59e5be66f7"
      "22920e57e99af48672dd954042b93d71f863e5c9a8ce2eb7df6e88aff0770165"
      "d8723befd573e0c62a49297387e184654fad24cbdfd19b39a025e2246a678d29"
      "563f39dcb1873857d36c81aa0551950ea08925dc3a1a1425854e1666bcf4b45c"
      "3a4304ab645a6630b8a2766da7821d584b09f5aebae6d96ddd110b82d998a2ad"
      "4b3d322da4ae336ce7a97429f9eca8fb7dca9b07feca4887c0ce875d6da3ff3f"
      "a6631592ac92997f73d28f89c59dba6bc6bedc76d1a6fe8e2c22783afb4685e3"
      "92e51389bf726c14e2f42302bde58b589c83520915fc63bf67fef4694bc48bdb"
      "f468161abe9e36288b31f1264055e3d218e231deccb609392230187d6db04b69"
      "e90309528732689553e0e0a409485fa04cef9f5fa216098a346fd8657141f5bd"
      "fc8602c324da79764ed12785086486f3c13c648d34a74f9b75aed3c01db68e00"
      "07372898d141259ec21d9a4150eb52573256cca2bbd34e6af1a65bb7b78803fc"
      "2d19311a55407d47246b36d21e009062e3808d689b479c8049b596327f54b6d0"
      "d83223b911762d72c5a602281c31eef61badec9bf2101309781b515a3cf41363"
      "70ac17ec3c3f720e5a1c2a8dee8db1c0e7b959684332acfdc296fe587b3e734d"
      "ad260e5833064da14980958ddd062228d5f7d28ccdb4d396b302b606d4edbb25"
      "44123cedf5ce1ad462c5787cf31a209fe4191b49a1f49285aeeb4d683001b60d"
      "599e09582ee964e3ca8ee38c0554d62e3cd68ef62cfa86dc3a56a4944f9f4172"
      "351328c1e93dc200447d689fcb355ed7fcab3ad360386902f74031bab9135220"
      "0b9e344b35088d48d98a512a618227955ff1ec604900e21262d829450102aec1"
      "cea80ddc34f0828f1e0b385615df92852a97fb7cfb0bb45f53b1ab00f83e1fba"
      "59a41edf2599f69f4535e6e26b8c702ef6f5ea2e964a1c6304d6cb59fcc3471b"
      "a5db1b4c03f17e85cf2b697a6dbf9a2db33ca0c0373297928fa572b919044660"
      "ff0204d6043c1bb41b6e9f4b52e75944608b5b7c1f2e7006b4654e2f89d7804d"
      "5911a181e073d570dfb923510ca477b6d38744e81962b3206ac22d05183f5f30"
      "5da63dd75120546f88af56224564c8666e4e3044f22d540d6ca684ed9a2ede96"
      "37cd341bb5a845f3540e0d251f96f7f0214bda42d73873b5e2b4c30b3ffa4f6e"
      "5b2b283fe388d491e162033174f777ba31d54efd84772fd98e1c779f9d23dba1"
      "bf3911de26b6fc29d5075c7cdee4dac9bbab0e0f34a43eaabc764a05b77d24c2"
      "b87c36b921b32e770679ba99289033a1599ee31f3386378b409c81faaeb718de"
      "e4c93b7d9573fa21f3c3fcf9c4f1320c7ae31b22eb4f06924c74bc864d9fee34"
      "9c63b01788b4f3761803cdb11a8464ec78bc0b9e0c3244f36bc02028de0505f1"
      "5a440d913de3ff96f79220add424125c3d8ca3e9b8a338a88caa25084bc8d205"
      "404616f8196d6ae4cd2b8cdccdcae0e736f834c35b78aa22b528fb132747fba1"
      "671ebf6e955372af0311d797b4a4fa2f215d7c6b082dcea5fd80c7bb86713567"
      "e1748ad1626de6f2e8ff621179fc97fed2bd2cb558f10c559c09a41ecf0c499e"
      "7f74b033f189e77402bc60c0e07c3c97948047bf47665723cdc6ae1da6b5b6e6"
      "0dcfa0c9567694ad6844567dbd66f8fa7a6c92c6bf5d6e995b25fce0d6f86a98"
      "bcb1279003c55c0de3b36648f244cd400e6a3bc809566290e8edb4efa94c808a"
      "c57082254f69074fda62fdd2a14ee56d91dfdcd1c78e28942521309e38155adc"
      "4b98b8ab54242d7bf6a8621cb5f7a6ff0679b063bc94e95c3a922e09e2b4085e"
      "7eac3a0547e0154fc82ed2102e6a43fb8a61a27aee71d96cd687636caf236a0e"
      "9dcf163b9375f6a46e7626a425aa4e02d48bfdf0c4911c3b43726cc492b732ea"
      "2e9535ce8fa2d7365cd3634a58044a21992019ea380e38a11166eac23d385fea"
      "2a97b5dd15f468554c6622a7a937a753b8271a614adf3580eb0c4df2c0d19084"
      "436ea8c1ce66a9fd5195b46fb65f5f1905a26e8d4dc953d5272ca81ab4e30db0"
      "90e29fd92abd58b736666f2e2d6cc91301f3ad4cb9bdac1f54b3dc07a7f742d5"
      "6fb52b51a167da0b79148847fe4b438642bb8ee2e72bc4dd07096fb07b82dc40"
      "e88521bdc3e4412b2ac1b23b8f229df35ae9e045017a58b37e9e8c0c57bbeacc"
      "beb68d086b02143696c877029ed6c64734c709cbda28455cf66635cf767744cb"
      "e61409992b4a807cd349e8b6f7223362d25f3fd074533b6a1a64b7d6a61c0965"
      "1e071674f72b8422ea095d73cd483d82ef85805d32b4f55faf07564decbcda35"
      "f2cf24c0d76263442891faf5d3dfe5b5534bf7aea4c2d314ad05ee2bce967d54"
      "25f8053ce0bb8b8c79e657d5b455cac0d3bc0ae4392d29298fbe3a93aa1fdab4"
      "8e0b08dd3813e2c60dde78f4ac9807ede6ea1500dfc136fdb44a1f4982079b5d"
      "5ac32fc2265648254ef6e0a8b0b8c51862a4eec29929be5ee71ce85bb13f9885"
      "2df90400",
      ToHex(captured_data));
}
// https://3xpl.com/zcash/transaction/ef274e8345c128c7cdaf58353c72ca2edde51facbb26880d43f187787789d2ec
TEST_F(ZCashWalletServiceUnitTest, MAYBE_IronwoodUnshieldFunds) {
  // Creating authorized orchard bundle may take a time
  base::test::ScopedRunLoopTimeout specific_timeout(FROM_HERE,
                                                    base::Minutes(1));
  base::test::ScopedFeatureList feature_list;
  feature_list.InitWithFeaturesAndParameters(
      {{features::kBraveWalletZCashFeature,
        {{"zcash_shielded_transactions_enabled", "true"},
         {"zcash_ironwood_enabled", "true"}}},
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
        if (pool != OrchardPool::kIronwood) {
          return spendable_notes_bundle;
        }
        {
          OrchardNote note;
          base::span(note.addr).copy_from(*PrefixedHexStringToBytes(
              "0xddde44ca4124798b61a4cb19f74d516a3560eac8fdcbfbdae4b8f34e8f90ab"
              "7c2a414cd0f7ce7ed81d751a"));
          note.block_id = 3444263u;
          base::span(note.nullifier)
              .copy_from(*PrefixedHexStringToBytes(
                  "0x7c490677d8d38db902f773e01b913a40cb37d22447e5e0e18dbc4c6f30"
                  "151105"));
          note.amount = 10000u;
          note.note_version = 0;
          note.orchard_commitment_tree_position = 46912u;
          base::span(note.rho).copy_from(
              *PrefixedHexStringToBytes("0x17570f17db3fc509c8d59d4ad7d9e5054307"
                                        "a51929c18a40bf62078da242ac1b"));
          base::span(note.seed).copy_from(
              *PrefixedHexStringToBytes("0x86fb0238d4ad5959e0d82d67475181fd9e96"
                                        "f600b9c3711a5fa1d2b9c4b18553"));
          spendable_notes_bundle.spendable_notes.push_back(std::move(note));
        }
        {
          OrchardNote note;
          base::span(note.addr).copy_from(*PrefixedHexStringToBytes(
              "0xddde44ca4124798b61a4cb19f74d516a3560eac8fdcbfbdae4b8f34e8f90ab"
              "7c2a414cd0f7ce7ed81d751a"));
          note.block_id = 3445014u;
          base::span(note.nullifier)
              .copy_from(*PrefixedHexStringToBytes(
                  "0xfd3c324c58dba1c668bfd299af81d8eb8c82489437089b3239b3a7cde0"
                  "fbc002"));
          note.amount = 10000u;
          note.note_version = 0;
          note.orchard_commitment_tree_position = 48943u;
          base::span(note.rho).copy_from(
              *PrefixedHexStringToBytes("0x17ca7d23fb14a42cb4dd997bbaef4a90c18c"
                                        "ba9069d32d1b1694f55cdf34c23a"));
          base::span(note.seed).copy_from(
              *PrefixedHexStringToBytes("0xbf7e270026e4fc7b8c2b3018431cbf264206"
                                        "72f474d32630812e16a6f0f15847"));
          spendable_notes_bundle.spendable_notes.push_back(std::move(note));
        }
        {
          OrchardNote note;
          base::span(note.addr).copy_from(*PrefixedHexStringToBytes(
              "0xddde44ca4124798b61a4cb19f74d516a3560eac8fdcbfbdae4b8f34e8f90ab"
              "7c2a414cd0f7ce7ed81d751a"));
          note.block_id = 3445082u;
          base::span(note.nullifier)
              .copy_from(*PrefixedHexStringToBytes(
                  "0x4ba1a9f99635e10ce12edea69ae002c72119a0fef6db745ba1ac4770f9"
                  "4bfc3d"));
          note.amount = 10000u;
          note.note_version = 0;
          note.orchard_commitment_tree_position = 49227u;
          base::span(note.rho).copy_from(
              *PrefixedHexStringToBytes("0x04461f5af41d6ab88567f08d91a277cfe04a"
                                        "d1f6e0ee9b62101103935c30a937"));
          base::span(note.seed).copy_from(
              *PrefixedHexStringToBytes("0x1fc7e79a82273acf020a43757abed4a326ce"
                                        "4e3f3bdd68ad327d701e2b261a60"));
          spendable_notes_bundle.spendable_notes.push_back(std::move(note));
        }
        spendable_notes_bundle.anchor_block_id = 3460449u;
        return spendable_notes_bundle;
      });
  ON_CALL(mock_orchard_sync_state(), CalculateWitnessForCheckpoint(_, _, _, _))
      .WillByDefault([&](OrchardPool pool,
                         const mojom::AccountIdPtr& account_id,
                         const std::vector<OrchardInput>& notes,
                         uint32_t checkpoint_position) {
        EXPECT_EQ(pool, OrchardPool::kIronwood);
        EXPECT_EQ(checkpoint_position, 3460449u);
        EXPECT_EQ(notes.size(), 3u);
        std::vector<OrchardInput> notes_with_witness = notes;
        {
          OrchardNoteWitness witness;
          AppendMerklePath(witness,
                           "0x62ddc0ce228672ddb8e26e41edde34b274d8de24080dd9228"
                           "f7e59d6969ed23c");
          AppendMerklePath(witness,
                           "0xb496227997972e0ac1c7a6b372684781434d1c3aaf628a911"
                           "1eb78f10003af05");
          AppendMerklePath(witness,
                           "0x5bc6466a65c6a1f4e93b974ac8ba645744ac0b56ba8e35221"
                           "0acf3d17ad2ae27");
          AppendMerklePath(witness,
                           "0x28520652bb70892bb2d8b9a88200ae124de9bbb5b1a11b886"
                           "ee6b879d8b30c22");
          AppendMerklePath(witness,
                           "0x615238957d44ea08174163765ee497f96fd1f70dd84cd147d"
                           "3e0d28882838935");
          AppendMerklePath(witness,
                           "0x7a4f42527c3f3c449463c04c379c03306cd4219ce348b2b2a"
                           "9216891a1e9d526");
          AppendMerklePath(witness,
                           "0x69aef82fba16d6b1b9c35468eb5da761b1bb939b24af27162"
                           "973d532fa871d2e");
          AppendMerklePath(witness,
                           "0x574d55bf40086c08c5cae1716d63e56cb9adab944c24f9488"
                           "614cdcb5218fa08");
          AppendMerklePath(witness,
                           "0x46a4005e8b61caaeca9fca5a69a40cbd42f26e4737a22b3cd"
                           "ba997243526612b");
          AppendMerklePath(witness,
                           "0xbe39e7fef640dc7db98099f286a2bb8c2ea797f78304ca516"
                           "53f3c84311c9f17");
          AppendMerklePath(witness,
                           "0xcefe2e12d427f2febf4b6d9633a7f37d2976201486aa54c6a"
                           "60545b42624d70e");
          AppendMerklePath(witness,
                           "0x278db37eb6f451f7789807b13f3ec850c7ee3fe5acc53b9ac"
                           "74af8fda4482e1a");
          AppendMerklePath(witness,
                           "0x7d4226db55a77cca1b9c74390963e173f4a91956b59885de4"
                           "0b257aa6afdc519");
          AppendMerklePath(witness,
                           "0x1a9818bdc2d64509089714bbd2ef7b103d857a6fff04b7732"
                           "6ac815fa89c6032");
          AppendMerklePath(witness,
                           "0x0f6d79de62a9f24a7ca9d5b6f5c3b4b0afda2ddc57b233165"
                           "311de2b64382f23");
          AppendMerklePath(witness,
                           "0xcb19b53836b2b5c0040cc1199d04d92a0630da204e8017ae5"
                           "79c4dbf2f90be1f");
          AppendMerklePath(witness,
                           "0xa935012def03eac1c4310193c463a2fc691650893d0d4c614"
                           "74c57912ecbc139");
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
                           "0x7097b04c2aa045a0deffcaca41c5ac92e694466578f5909e7"
                           "2bb78d33310f705");
          AppendMerklePath(witness,
                           "0xe81d6821ff813bd410867a3f22e8e5cb7ac5599a610af5c35"
                           "4eb392877362e01");
          AppendMerklePath(witness,
                           "0x157de8567f7c4996b8c4fdc94938fd808c3b2a5ccb79d1a63"
                           "858adaa9a6dd824");
          AppendMerklePath(witness,
                           "0xfe1fce51cd6120c12c124695c4f98b275918fceae6eb20987"
                           "3ed73fe73775d0b");
          AppendMerklePath(witness,
                           "0x1f91982912012669f74d0cfa1030ff37b152324e5b8346b33"
                           "35a0aaeb63a0a2d");
          AppendMerklePath(witness,
                           "0x5dec15f52af17da3931396183cbbbfbea7ed950714540aec0"
                           "6c645c754975522");
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
          witness.position = 46912u;
          notes_with_witness[0].witness = std::move(witness);
        }
        {
          OrchardNoteWitness witness;
          AppendMerklePath(witness,
                           "0xa3ed270e58055274c59481f6c1f516959ce36303e87f5aeb1"
                           "dfc1b4a403fb61f");
          AppendMerklePath(witness,
                           "0x9dc77fdd2a8ba3352b1b94df0d12586a6c57921ebb2e4dfd9"
                           "b80fac46d28ad09");
          AppendMerklePath(witness,
                           "0xa07f0bab55bb984b46e1414fed774d5a8f47310d1cd5fd200"
                           "9ceeff07a162d12");
          AppendMerklePath(witness,
                           "0x59c400dd0e8ab5491d51e0071137966aadbdf3cf9df93bc16"
                           "bf6caca0fec7822");
          AppendMerklePath(witness,
                           "0xc92d33b84ed83dce95136ea335b6f6976770b2476955e1ed0"
                           "7434ecaec609b00");
          AppendMerklePath(witness,
                           "0xd0d9316535424b5c963e4688adf4916e0b82ab21fdfe09446"
                           "ba60050cc2eb92c");
          AppendMerklePath(witness,
                           "0x713fe64812e53d033387e806e2ded3a4b7e508aee0e0b0d93"
                           "55759c39a5ea511");
          AppendMerklePath(witness,
                           "0x8b2f38755b3c63bd58e40eedd2fc3af8d7717073a3ae74e45"
                           "9330579b2b4c635");
          AppendMerklePath(witness,
                           "0xcf9322996705018ec0e6acf90e92c25e1f67b8df4628cec30"
                           "5e2b507062f392a");
          AppendMerklePath(witness,
                           "0x14f5bdfccf532f03125fce64805b814db52209a775b1802de"
                           "95ea91a5338781a");
          AppendMerklePath(witness,
                           "0x944298431f1f2564b0a254b4252b4b11dc80cb1a49f2ef507"
                           "9f9475647877435");
          AppendMerklePath(witness,
                           "0x839a6982c6908049c0b0d328cf523318da9abd65436ed59f2"
                           "b7ef50c93e13704");
          AppendMerklePath(witness,
                           "0x7d4226db55a77cca1b9c74390963e173f4a91956b59885de4"
                           "0b257aa6afdc519");
          AppendMerklePath(witness,
                           "0x1a9818bdc2d64509089714bbd2ef7b103d857a6fff04b7732"
                           "6ac815fa89c6032");
          AppendMerklePath(witness,
                           "0x0f6d79de62a9f24a7ca9d5b6f5c3b4b0afda2ddc57b233165"
                           "311de2b64382f23");
          AppendMerklePath(witness,
                           "0xcb19b53836b2b5c0040cc1199d04d92a0630da204e8017ae5"
                           "79c4dbf2f90be1f");
          AppendMerklePath(witness,
                           "0xa935012def03eac1c4310193c463a2fc691650893d0d4c614"
                           "74c57912ecbc139");
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
                           "0x7097b04c2aa045a0deffcaca41c5ac92e694466578f5909e7"
                           "2bb78d33310f705");
          AppendMerklePath(witness,
                           "0xe81d6821ff813bd410867a3f22e8e5cb7ac5599a610af5c35"
                           "4eb392877362e01");
          AppendMerklePath(witness,
                           "0x157de8567f7c4996b8c4fdc94938fd808c3b2a5ccb79d1a63"
                           "858adaa9a6dd824");
          AppendMerklePath(witness,
                           "0xfe1fce51cd6120c12c124695c4f98b275918fceae6eb20987"
                           "3ed73fe73775d0b");
          AppendMerklePath(witness,
                           "0x1f91982912012669f74d0cfa1030ff37b152324e5b8346b33"
                           "35a0aaeb63a0a2d");
          AppendMerklePath(witness,
                           "0x5dec15f52af17da3931396183cbbbfbea7ed950714540aec0"
                           "6c645c754975522");
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
          witness.position = 48943u;
          notes_with_witness[1].witness = std::move(witness);
        }
        {
          OrchardNoteWitness witness;
          AppendMerklePath(witness,
                           "0x4e056ccfd32fd695d1cd5e31fd193f8c9050aa3f293aac584"
                           "97af0da34222734");
          AppendMerklePath(witness,
                           "0xd99e84bb47a1ed8232e67fccffeba5922de10a4ccc537231f"
                           "d707d1fa0ab4910");
          AppendMerklePath(witness,
                           "0xd2055a97b4cd1f1a563f2422853852973223f39ce5b3d210c"
                           "8386c3c7c424c30");
          AppendMerklePath(witness,
                           "0xb63920628bf03a406c900e9e89332c37d5e25d4b062f248db"
                           "eaa481f7f83b238");
          AppendMerklePath(witness,
                           "0xd5649c09101661283d5917e459ca81b21f3ba8972204a8369"
                           "9bbb04b45615413");
          AppendMerklePath(witness,
                           "0xb2fe1da933ee43130fc5a1463dceaa5fa1443947690ce8eae"
                           "981042c4b4be902");
          AppendMerklePath(witness,
                           "0x9d0614db7c30a4c5b3ddb162b154c9612c7a987a3892d1e85"
                           "ab9452ca8e92b36");
          AppendMerklePath(witness,
                           "0xc1c93ae8d1071662aafbef64dc70d7dd6cf385173da3c5cdd"
                           "253daeb4cfa6f00");
          AppendMerklePath(witness,
                           "0xf0d021378db1528895096a9132be122962329c3cd9a93b79e"
                           "56e75e8ea635706");
          AppendMerklePath(witness,
                           "0x0b0320c4d42bd421516794c77ff1cbb979073a783d042c870"
                           "e5fad4269070d17");
          AppendMerklePath(witness,
                           "0x95388016f4761b159ececa581126604b134b2a10127101c05"
                           "2f56c7caecdb136");
          AppendMerklePath(witness,
                           "0xfd900e2cadce8cdb147e455745f8cca26f4d3967a934417b9"
                           "07628985395b310");
          AppendMerklePath(witness,
                           "0xf25062a245fb486517a33d657adf44e4e4af5919fc2f92281"
                           "ce5ad10a9a48e0e");
          AppendMerklePath(witness,
                           "0x467579eeb24d2a6c85a6aed03bca2d999944468b5d059cff3"
                           "8eeb99f2322b018");
          AppendMerklePath(witness,
                           "0x82acb44126c3b7af886af143aa01cb339bacff8de4e10dc13"
                           "e5b94399c45842c");
          AppendMerklePath(witness,
                           "0xcb19b53836b2b5c0040cc1199d04d92a0630da204e8017ae5"
                           "79c4dbf2f90be1f");
          AppendMerklePath(witness,
                           "0xa935012def03eac1c4310193c463a2fc691650893d0d4c614"
                           "74c57912ecbc139");
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
                           "0x7097b04c2aa045a0deffcaca41c5ac92e694466578f5909e7"
                           "2bb78d33310f705");
          AppendMerklePath(witness,
                           "0xe81d6821ff813bd410867a3f22e8e5cb7ac5599a610af5c35"
                           "4eb392877362e01");
          AppendMerklePath(witness,
                           "0x157de8567f7c4996b8c4fdc94938fd808c3b2a5ccb79d1a63"
                           "858adaa9a6dd824");
          AppendMerklePath(witness,
                           "0xfe1fce51cd6120c12c124695c4f98b275918fceae6eb20987"
                           "3ed73fe73775d0b");
          AppendMerklePath(witness,
                           "0x1f91982912012669f74d0cfa1030ff37b152324e5b8346b33"
                           "35a0aaeb63a0a2d");
          AppendMerklePath(witness,
                           "0x5dec15f52af17da3931396183cbbbfbea7ed950714540aec0"
                           "6c645c754975522");
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
          witness.position = 49227u;
          notes_with_witness[2].witness = std::move(witness);
        }
        return base::ok(notes_with_witness);
      });
  keyring_service()->Reset();
  keyring_service()->RestoreWallet(kLuxuryReformMnemonic, kTestWalletPassword,
                                   false, base::DoNothing());
  // Ironwood bundle creation happens during signing; pin the RNG seed from
  // ironwood_to_transparent_4.txt so the signed tx matches the captured test
  // vector.
  OrchardBundleManager::OverrideRandomSeedForTesting(
      kIronwoodToTransparentRandomSeed);
  auto account =
      GetAccountUtils().EnsureAccount(mojom::KeyringId::kZCashMainnet, 0);
  auto account_id = account->account_id.Clone();
  ON_CALL(zcash_rpc(), GetLatestBlock(_, _))
      .WillByDefault([&](const std::string& chain_id,
                         ZCashRpc::GetLatestBlockCallback callback) {
        auto response = zcash::mojom::BlockID::New(
            3460453u,
            *PrefixedHexStringToBytes(
                "0xa8939162de5c56cb17bfdefce1069f8283cf338c0f2dc7388f135b000000"
                "0000"));
        std::move(callback).Run(std::move(response));
      });
  ON_CALL(zcash_rpc(), GetTreeState(_, _, _))
      .WillByDefault([&](const std::string& chain_id,
                         zcash::mojom::BlockIDPtr block_id,
                         ZCashRpc::GetTreeStateCallback callback) {
        EXPECT_EQ(block_id->height, 3460449u);
        auto tree_state = zcash::mojom::TreeState::New(
            "main" /* network */, 3460449u /* height */,
            "00000000005710f79da8760b38b81abb7c94d41de5fa71738c976002d70c5204"
            /* hash */,
            1787682851u /* time */,
            "01ec2ff50501d52ec827a63f8d56ff4d47742641307a8139adef59a1486000aa13"
            "001f01e23b810e77be2bf8d9bda1243130dd99b538f45713626165351fa9ccc88e"
            "4b4701a9f641d2c7f5527c93efeda2afcf5f78a0d0744bc9323260fe8ca7400a91"
            "cd14019d465372d4c5ed79991c476663a4ad5a6d5a100de356f7e2d9706c602e38"
            "aa1d01458695819928e22034aa59e13a246da3c72085edec9a0a3e9a5036bd2813"
            "48300001429cb689a47f27d2bc7454e2a75e2e0f6c7ef04d72cffc72427e78caed"
            "63cf6b0157d2d07564559b0c88e3763bfd3b2bc2eca8a8d61eeeb839239e7bb01b"
            "69104a01e51346b2597bef3c69f5c29eb947726536a5a9ad684985b6edc2f18a80"
            "eae4720001d5ec124db8a1d299a90e423001be38fb902ddb59bc48f7a514da8566"
            "afdcec3700000103d1187b8d91f9b17135eaf11ea773ac8f2226a7037227c9c245"
            "46d63776c65101ab65114517839c9df32720a9f91f5be6604f91ffd2193b3a9d16"
            "c4ed2a81df73000000000190eb9e2bc82b8b980aaa63ba44db65328553ba840c38"
            "c5011a465efd8b233b2200013e2598f743726006b8de42476ed56a55a75629a7b8"
            "2e430c4e7c101a69e9b02a011619f99023a69bb647eab2d2aa1a73c3673c74bb03"
            "3c3c4930eacda19e6fd93b0000000160272b134ca494b602137d89e528c751c06d"
            "3ef4a87a45f33af343c15060cc1e0000000000" /* sapling tree */,
            "01fa6a2e17cb9c2f93bccf0aeb31325d49cc8ac42a5e87e926c99616ed8ab1e72a"
            "001f000110622464857a93c4c75cd7795793963a7690ef09205d18cdffb17d7fe3"
            "839d2c0001a3080e45876dd02d2c4b0ebc2c53da030b854aac30e3197c61e66916"
            "ab96773f000001d26bedd1fd38a4a554d9896797c80afcef642a72d7074ec9f02b"
            "ef1922251307011d0a4b9a7e04928a41da79c21b12e5bfa161ade2da508ddf8383"
            "4b7679d5ab3501e0fd716b416fbb05c114dc046ec81d65c4577a2ecb55082fe0ed"
            "8d7dd74ce52a01542ea7b4b846faaef2f0eb855b974ab9c904fef26bb18bf5a2e5"
            "d8858d924e3600018b0cdc0969c2c2070ca79277185757e8094801a44fc3fa33c0"
            "eca6373bdeed24013dda000e1a152209563e47d7dedd8ece9caf5f808a9b0bd841"
            "709ad46eccc81501e0828ebbc5dcb2be8011aa76b77c3b7e4a630c587f42fb350b"
            "0b4f570344f3270001cb129832ff83e5ab567f159e7d0c58a04c11074a9d29d2f2"
            "1b908fd11dddc41800000000000000013f3ddc746e57791a2cf8900143b86b9ff7"
            "b82454626f0ba633404f9305b6c32701e2bca6a8d987d668defba89dc082196a92"
            "2634ed88e065c669e526bb8815ee1b000000000000" /* orchard tree */,
            "0199a2e70e5d7c0b9101f9201ee5649f0b0f5005a73a4551fd84f420035dfde007"
            "010dfa1b8fff7b537278a32ba9aecab365748611af076f6103b65d447bc577b825"
            "1f000000010bbd5afc1147241e6f533f60796450c82f2a9985e56d41c00416ecac"
            "51059b16017ec4d8bc91061f3184dedd2e6ac652de46c61743b5e9635202292ca7"
            "3a334b0300000001b1b53cc49c78037e0a80b96e740e2861593be984e06fdbed77"
            "fa06b3defca40c01993aaacef46c4c0b5aeab79b7567728f4267752b184a959873"
            "40e024fd98500e01b84fdb5b5c21852b8bf828622344a836da0e7b8d98d63c297b"
            "be713394da343001aa72d28a1e3fde14a1639af23bd8cd97700d826c5ab8c4ab9c"
            "be2024245ab015000001f1294ce1ba5cd0625efe133008fbb34b61f8fee5fd47e8"
            "4ea1e3aa6faf72d039018e1e8c93493395a08a8d30ba349a6c858ca13c4bb34b4f"
            "214f99afe7e420452f00000000000000000000000000"
            "0000" /* ironwood tree */);
        std::move(callback).Run(std::move(tree_state));
      });
  base::test::TestFuture<base::expected<ZCashTransaction, std::string>>
      create_transaction_future;
  zcash_wallet_service_->CreateIronwoodToTransparentTransaction(
      account_id.Clone(), "t1WkX2Vgd1hQ7DoB7xG7rhyBkhCqwratj1W", 10000u,
      create_transaction_future.GetCallback());
  auto created_tx_result = create_transaction_future.Take();
  ASSERT_TRUE(created_tx_result.has_value()) << created_tx_result.error();
  ZCashTransaction created_transaction = std::move(*created_tx_result);
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
  base::test::TestFuture<std::string, ZCashTransaction, std::string>
      sign_future;
  zcash_wallet_service_->SignAndPostTransaction(account_id.Clone(),
                                                std::move(created_transaction),
                                                sign_future.GetCallback());
  ASSERT_TRUE(sign_future.Wait());
  EXPECT_EQ(
      "0x0600008098b684d85b16a53765cd340079cd3400000110270000000000001976a9148d"
      "4290fce2327f5ea7b430c5dc5c23a7ba9b2df288ac00000003427a47ab9f005c57a42b86"
      "3be5448158143a03d5ecf0501d29d4b2418f333895fd3c324c58dba1c668bfd299af81d8"
      "eb8c82489437089b3239b3a7cde0fbc002ff587bcd5d0db89985d2d688df24c0b0f1eae1"
      "28b22aa68ce50d8c9c4712972e0634a46032d98305767115d608e615760b3538342a6b6d"
      "9a2656ea7529bf46061ac9af88919651ef8b474d798dad89f81d655175325e68af5a1c86"
      "80c13861aa4c05b2a92f5e34cc3b8216c241209c585c386a192cdcc1dba1b9084e67545e"
      "aaff4533fd627906bfea1418a8835734f32ded8abd8456ecce6ca52603f785174c91a22c"
      "392c2da00e46bab8fcd5580646af3de7e4f58d67fa07eeae097bbda9bda64865fa0c2bb5"
      "8beb43a30f5dff4b8c21fd19408e6de91176fbebbc7b5c6b1692b264aec2c38779098dac"
      "642ae9032e99b3a3eca8dda119435ca402c92a928c741a20f6503e2ef9f6494310b32961"
      "2edb0055493c202164c4ec45b97f7c773b4df776336df73409c013154ec067f3f1139c8f"
      "f4700364cb03d932d74f51dc8ab3a10082dc3edd1eb61bf54e403ad8662f220160988511"
      "b89e70291a5fbd6136d416952b85988b177eff773a1caa5d8836c700e25c44e021d82d62"
      "f44f5de59a27126f0c83af80ce901714027b772a9acdb891215c02504f68e4d5129bfd5b"
      "c50196e9de6f103f5def048a134e14ded91db93da0fe3e1eb525edb1d9d8b471f31014cf"
      "059102f0676188ad94a0783d8633acdafd328b1cfabb81c3a942a8bcf7f27d062c60551b"
      "4e30a67a8fbd8ab54f8b28234a6bd24c03678ddbafe210b71e7acc875a9f6417d3d5591a"
      "c7126e73b3378fbb93a73353a121b51e7f2e10004e5afabfab88306b18cb4a1a79c5736d"
      "843d21459d01c1ef16c489fa1a27fdb0b4882bcfc1c371c5d8b0ba1c5ad75b59f63f31c5"
      "1f8cd36c933da834a1ce6cc6247726a5080d1771ed5431070ceeabf0c15e1f8e2e1c56c5"
      "e085b4031d653c77dbd2bce77a427e4271b9c443444301e75f4920d6653f4e8be91b8af0"
      "2dc4b71685d91e7478defc7a0c9465a64b70c7687c15ce32ba8773870be1b25fc8c0c8ef"
      "84b25735d6ae5d92397b77f9a34f46914840ebab941c92bf8875c05d572beb2162ec3d9f"
      "02b3a27bf19db7fae410025f9aa5221e0ef1e61e0df2d7a77f98d41e2812fe8811cf7c9a"
      "4110b7e6e659243c557837ea014ba1a9f99635e10ce12edea69ae002c72119a0fef6db74"
      "5ba1ac4770f94bfc3d70b35c7fa91825cac0a67fa99ed576918cd26037cfcf43284da51c"
      "4262d741a29795cf4e764df521d267c5dcb71ca7f0c7ed9dfbefe61f0abdaebf58e59cf3"
      "3ea4df5995e78b77506eead4bce24d80e9891f77feebb974e88c3fd09442e17a34d86b40"
      "5748e9d004646fb7940fa1e4d2bd0fa70e446cc8351bfdd1c17455de9b9543da5e85fe18"
      "7680ffda1916936d26de89a0b3cb080128c7ba76b2c3d440f362ba075ac89f0e7b16e9d9"
      "36fcff9106dda1173c917293cbbd2abba33c42ff7f55e20fac2f98b0059260dffc147c2c"
      "92ea71b5e99f23cacda83c6e9ac8f5a7273c369ea483667cb3238544407938f166f33391"
      "33a5dac894ae52f6272e0330939f61b9a84e1e9710d35463881da5f899856b5c0929377d"
      "58995e44873d7630c38b8f81066006b54ad1c70ccf1286883cfd7a605a67337d9a86cf88"
      "60b06ea979bd16de64ca46fbb8180d03ad81c21306d407edfc4a2a282bf34a40b8bb6d81"
      "58b1c93a2c7e75990c930461757f78e21b0cf515ea3a22b6c7b8bd1852dfbf6e7138ca89"
      "e7f1330c7f98ca0a6bc33324472b3834765646cda2681a338c4ab14d1f87a511ef61ff19"
      "239ec804f0b5bc858243de63090b436dc187fa268bf1b267e5d4362da71d36463cf27bcd"
      "f0d257297e533deeb1491eda72bae373aca3c5b4ff42d50fc280a816485d649fc2ad44d5"
      "15f4e7ae9ad1d430d057aeaccfc973d9f2c8f72ea6048e42f41c2e8a3e5cb9644e97265b"
      "3575fea9811c34ba74d6cdccb9292eb0d4073fbc616463f191cff678ec4cd53c8291bb24"
      "9c10d86c637d6f2e1247142edc84e8226ddbd0908420ee7f0ab66a2eb3bb501fd4e7a7f1"
      "6e66fa69a31bc779a960fe43fb3536c0a882f36ce78a3030cacdc9253a379e33df531ef6"
      "7b2e5b06da0e3708374db8419537e1fcb39ded8ccd52f52ff655f0bff04165016d2cd78f"
      "67a5bf47b3377dc96d117ed83f481805452ed2937ab9dd1ee9d294d05807666449c32bc8"
      "34973045d6434b3ee3da94665ae0098a06f8e66e9006e14b86051fe55abbb01564e25449"
      "6269e051960a1c70b11dd24df75719250a1e50c23a412f59c03a2915257dec7ede45b679"
      "2410adb7177c490677d8d38db902f773e01b913a40cb37d22447e5e0e18dbc4c6f301511"
      "055ea1f829598e736d5cfda6f9ed0acf6701b163849ae09cd01964c044ba5f0d0c5a29a8"
      "cf427029695f465a0b02f1a1bd0ff1ded97c9afa4e99c4fe7cb45f961982e895fe0fdecc"
      "1e36bcc42c6730a8d7f74cf36802e99455c2df67404e1bada73a7c2ea7c678149581c141"
      "5b9103d3d6171d5a4490065dec910d1baaf7770e2c658e45e7df6e773c28188ebee31987"
      "d00340978d13692c59565196b9bd315168970caab550761cee6b34c27cb1d44d0a995417"
      "72b9c2ebde8af888cfe23625ab057feba5ae2d8b774286f8d6166eba56a0dba59e3a559c"
      "e8c870727a97d4d546be0a38b498ba1cc6d78f9fd6f76876e01f1fc02755f830d12eaa0d"
      "e5993490301abe6e8a637f515b2d43d779b1ff73f7c2db25736780f78ac1ed6ee0c5306a"
      "2b391720692f28c931ed716db418c121cc5e97e12ffc149e843f6df3cafa3b41d2ccbbd5"
      "da78fae60522ebd3dc3e77d9eff42860438fd2f4562e2c522fc7f69249e5fe6a0ab3439a"
      "c44a86edbb5580eca0305d3cfb388d1ff07012b1bddd3175cefa007bc509c9f96b014a67"
      "62db8cd852cdac7e6a17a35a14c914cbaed60eb0e3f08117528005db82336313f807ee80"
      "3662516b85abbec0fb3723df922636092a4b454cce06429efa9dbacf2bb1b2aa0cdfdec3"
      "f10d180b72cd8c222fc126bdb7e04a89fb921e37fb5fab8700a55cbb7ab36642dd40f6b2"
      "1570d8efa4d921161a6d6e73a791bcd5e43f2326fe9f20029297b71b7ac0e2edd1f93c2f"
      "e82b9dfc6b66eec7a463a846d083bbf9a05b2905a17f00b0d8fcb2ce9c054b26bb771789"
      "c40f9ef17531f4fb86ed5f4dcad449b9d0ee9d2d63b37f6043458c495be033d23166423f"
      "0d93918ffd9c5e9fc567812de80fd67ba36b3aaad366760cac32eac52967917f0580ddd9"
      "b9be40e898078e166d4468a43ab7c18ac931bc82e5452dbe3fbbaf14027745698ed285f0"
      "746ea68adeca95994ae87d864d0bcb4dece2ccbf75f9296bf648056cb5ccbe4a522b44a7"
      "e27399414a452d3d92fd65971b48874575617b8751313ad4cadbbff792a4868244ed9f4f"
      "4c0730750000000000000367741c06d83b773b21007398d04c9bae572de5a770b3298857"
      "65360aebe02afd4025aae56680d2b808ca2f495ab05d35e18977d4798d7dc6a0c0053ad6"
      "7c9f77960486a1e0e2f08ee5d1e625345dd0db691a8da7ba1fdc2dbb4da8d1a6d6bbdf71"
      "08398c5958264c91af0927c170775494811764e428e77c8ffec393d16176d26bb8aaf6e2"
      "b0969e47b715f484aa090d0b3d16bbcf9932570ff675db36af65d0e119410e8c97fde984"
      "cf2e6012792ae789bf3244e1a86e2378861122df945a3612be94ec636b164d066c0514ec"
      "95d92a4b62efb036b5c63925e9eb70bc66d848a5a7e18c1b14c3bda16dcd99dc212f0e71"
      "cff32ea36db385c626f7f9250384556008b0b9b13cce83478a1788d24054a578222a34f8"
      "aab5428ca0643c6371a95ffb90939aceb29238da22daafc09c89a72a20aaa97fa50c0e4b"
      "ff058eab4307fff3bb20db0ca43d440981d724519bae6c23f53596deb178652d88ba96b6"
      "445d9d6b15d3f867e5269278922e0f6c76622f18a46e6c0e175c8514bd384831833100cb"
      "2dbfacf6c1a5796bb2ad082ab2dfb6797f6a3549e1c242d4897191a7c10f2fce8759c3ee"
      "d8267d28704b53dfeed9bfb9adc9b31caea26dd82967ad2dd7f0de40b0fb049639f88014"
      "747b86cb85fdf0d6c444845ba70e006e709b1637cb9b111516728ce8fd308d01442b5685"
      "1592cd5a739876ab40515cc7c4a7548a023bb5920b703ce51599e39ce1a79257fcc6aa0d"
      "f7fd900dd6456c966bb815b3bc7d6031196dae4430aa68327e5dbbd6529ed66b0383de3e"
      "5fc61f907c5314fd85b349b09d4b085dc67d01f33d1983d126fd911114c19b656795d1f6"
      "5615f19149170d3438ced6f6ab0f22f55989b0d35fb80b8ef9ba7dbeeb86793a8945e69d"
      "7e0626cd8e8b4180ffbb5e884fe26e47547561e1439fb8ecc63dbe8d739b5b170ecfcea4"
      "80caffdda841a9e6d9f39f6a6b039002eac210cd3d14096da71aa36b6791d5ca37c78695"
      "e0b64d72bfd9864f009107107598dbfc7b9243e2dc06dc2157fb1c2b3c7fe168e940a1c1"
      "1cc375e94edba32fa93d925d3ae6dd0ad195ff7a06db796288c3a50094a6c593a174afb3"
      "81b3a72b2ee9f61a4e5ec93d25339d997b51cdc4304c32194fb3fc02405300ac22836900"
      "e8fa64f886ffa326473adf1d4bb91e5f04aa61757ef238231918eca29458cab553eb384f"
      "fa94856ea285f129400a93c417953af6a126038821895c8d401a011d7448670ac781fdd1"
      "f4f1ca81d81a31d61e78fad620fb34d969aacdae05532e41c9ede740f25591706940a9ac"
      "92b5f0b286fa04d62db97f34212b6690dd9c41057abded99ebcfd45f8164da01a07a2f28"
      "a117001f26de7d57d768a34618408a6a498d60c4ace8b29f5e7d0d7e3a50c3cebd6c3b29"
      "adc2eb8ac19ae8dd7e7adea1d031ec281fb728c45e35b3d2121a5f000586a55b08269337"
      "2e706f4d3106e04cdf42360a4ee38042acbd0a2535e79aa8bce7f47050defbb7a35be882"
      "578d55416322994c0044836d1f1118a3129abe0f265cdb6f02e7672d9392f77925f455a5"
      "ca4782c6a1744b4412c1f64af8c0507d2dd7290a18e85237949055a9c2e06f1a4ed3658a"
      "d3b6d0d02ce96573b6af8a6486bc44037ba6a379bc470c6b038d65f03bd96421bf04822e"
      "f5a4ac5dd38e17848efbf9b0a43dad30d335892bb9f7f81c4bfc4ed34b7e513bad6035a0"
      "940506a0816c52f7283e88140ec165dc36870bfae916db10a55af1fa3670725690ec6b2e"
      "a2c44663a46f40e5d20bac540caa710627d2128e32c31b47a2ba0bbee98ab8b0032e503c"
      "a170970ccf8dbcedab3fe7f45fd3c5189c414032325f9af6b7fc9339ad8eaef03f12900b"
      "902973433aa178911a1707d30f2fa5ae2bcff6e39c53406603d3612c80005cf9f5acf73f"
      "7e15d9bb6a82e797612449bd425ab370327909cc3158bb09aa6a799808be52e84b6f9fc8"
      "b3185d287e9578080a410380a19f8222299836bd988122cd28908a1dd795ccfad5f703e9"
      "2dfc6dd2a992fc16a2589b6a120d48efd329b85f0a72538278e2dc9d10ef9e2aafddf145"
      "7d4f40710f25d02fa41a61ef56debe6d4492418f9a006727b963680468c37acbc6732682"
      "97ff8c4ea6bca3dcde6597b6321a3a0fd82caf388e88037dea8574d90dccce626eede461"
      "12ff626c07efcc091c34ed66e344b97d954e508dac7d8b2dd0ceae92caae85989a973ec8"
      "d9a7b545fd16cef1cba39d8091114be211dcc8cb247990b471ab76751b44c62f39435a4b"
      "cb1fdcb592b704ac45c062dd8a0427be0ccc9410a8d122d89080c3345bb99a180219f373"
      "bae9b212fe56a74ee2b4a9da172794317e242ca2ae3fe18eb92ff504f5c13aadf8ad7dcc"
      "e12997023eb1c704fb8a6067744f6de41266a97f5424edbb99d4c2ab30354a8a08bf6245"
      "d2be68abd1ddcbb90c830435abd36cdcdae9652a9377aef45ca43cde610f9d1b373c363d"
      "94d95efcea014aad112b3400542939a2f642d743298693f0b39ad6fe4e6940734fa4c5b8"
      "9b03b2cb807bffb8c468902a7789cb985c6109cfcfdb652e7840ad90404ee74ba973b536"
      "89bb0a728bf3d68a549d7f6fba3bd93a5ff134918d62814cb57c7e856517ccd7a656e7e4"
      "7f8f8ad38a7d804884577bf1033894ef8eec4aac71440bce527c61290f9b107250b5e7db"
      "62be9ff1d55151ea9469e341466f4da0f7f6f4ac22fab6ce2fd3c367edc93e96a0a3b479"
      "c94f870b4cbdcac60ee916d5ae58f02b64350974861bcd44b2bd225d9b28bbbe70a27c25"
      "4bc9861b1a8f39baec1d3377d1de76e92e911d32b6f8ece7275bd1be4d9d2d43a2a7fe89"
      "95b98b038cf66cd53481838535aad6234b42087138f866ed59ae8eb0376ab99e9f89c2d5"
      "f959b2587d91088524ddbf1ebf7c5666b2f0781976fea5e4a2bac33c1cbd564a0e2dcd3c"
      "12fd2d4790f4c3e2d778e5d85f05b1277d4a985e9e9c2b2f34eba1867b8f97c2fc440617"
      "80b21d8fdb95266e92b52265ed23f52d4dcede56d25662af983cad5d19505515b61f3a87"
      "26979360f96bef86b55775c78afa596fe167cb8e44eaa2fac5a75bcfb43b986ee5e22515"
      "12c66e14b92a0a8e0a0b73d14acd9ae8d79e61e4e829771d022dcbe245ae269cbaa95b16"
      "f48d6524ad98b2dbbbc210bbcbc722cdffde268228de9de3c7ba4f95404b587d066c3645"
      "1c198519ab8befa3a7a0adac60bec92c8122d62606ad64ea616e5912bd89532594ac9316"
      "50c0152e05c3e3e1cd70eaac8b72eaa9460bd6066073b70452434be58bc4be6bfdcab66f"
      "0fdec925756d6a84392eb558a32b76dabba766b2b3915365e39f73a9610c936939ecc2e2"
      "001808212c1760c3576ce459d12e951366ba85bdc232fdace91ddded09f62743e27d2b1d"
      "170dae9f53dee854078b80aec23751b9dadc766067b0e2f58b4ee935f562d8cb32ae6c75"
      "74c4de6fa029f15fcdfe21555e48aa8681ead7dc0820ce9c765c08b33d3faa669e255efb"
      "4c46c5302d80407bb375bedf55861a0939623843a7d824450081248a31f687948bdf52f7"
      "39a795c59c6d7116c954fd85b29bec696cdd743e0dddc8f5902e3066cdec48c2fbd484f9"
      "2be301f368cec7f614eec209a67ec9b80c76c061dd8d15a62ec0b01ec79486761f2dcdea"
      "b099735b3564f08490825bef2c5c97d62c52b422fc35413802645fbe2051c2a097140fea"
      "92a95c3ac01d945d0c26498f985953ee4a1a1c8cc93514d3b7bb2d44214d46be5548d6b3"
      "7fd875d716fe88409b9050b4d79ba8eba059b23801a2245046779a4bb519835e1ec9520d"
      "0b8b2e1648f1ba5026384e4089bf43673c9ee9c56a99ef4730e1ac4505d2345934ae010f"
      "7d79c850f59570267326a31fc4632c85259095ed0abd81548c70957e0644b9c2bae78764"
      "1c159409681a7eed1ad3209dc800bb788d9fc782bc23951b0b80f39f69ad40932a76f045"
      "1f0f5d8c95af4f30e16ddc733480dd646ef69591112a682aad77581b8d3fed036783b535"
      "aec15caef710fd897112dd0c00b350251b48d2872640e8425eb175c73d0551ec19cc3d0c"
      "c28a920d5ab6de9982943cc4330e2d1229e43d74498b1386d5fbf5cd3bbf9030d7273a0b"
      "aed0a50b1ce067762865fed9aead2fb93ccb3e2fbaef0cf851bf58b58d885355cbc128c8"
      "185905b033f05336f37e553b104f2e3b524daf195cb719a79033efad36a0c398ceb1565d"
      "0d8be7032bfa3614266dc19df6ea759d9ccbc579fe43d58cfb8d062fd14300622fc1b41c"
      "d0c5805a9c20c6b9b6e04942a77d6ce766bbfe5a07e5fdea66aae4153753dd0e44f3c021"
      "6cc33a83f3ddbb08e7d10e07f9e17134fc3701932ef54c76376d0a77f7c4d29062775d1e"
      "416448118025dcf9c426a20c1c1372c009aa55e222b758e9c100fd60a4f0c6b80f16d1ad"
      "4ba29707c15823eb81e9ec7a2907a347052eb7925139931c6f7904222d40455b8a4a9c3e"
      "4b711d02c71e3a2fae6e9a4011246c96af00cf2fd485622baf131b4f2174c1d391d14f11"
      "7a15e83c298433cc34637681234f2444af5ad2b8f1ca6e82fcd61e0b97ddce8c5f221615"
      "7c0b27010c4d1ced4d3a35c00f67738874629b4f1a2bb571870e287583fb71ac356daff5"
      "1febdad33beb35e58400198563f02ed54058f9768b9ff4176572293e20a61b42149b5e24"
      "569ae08956faacac0c145e1ce7b18bdbcaad37e0734e2d928bce18380ff38a105fc3af22"
      "aacbf9e6d4b85920bb25ec973652bc525a3f96583906f7962fcf62a39c9d4ac4d730728f"
      "4c17fa1fb9b09c7d97927dac9511a9cada700786203a34ccb87e42af6fcd19db1e557a60"
      "d2a0996ce8477f63f1b7f7124f5ac646285c628a84ed33d24eb422f3cb8cec88abb13176"
      "b43d1068ef43c269ca31d6642e9e398eb9da6bf2eefd5dd8afce3074efa70937766ba85a"
      "abbaf5c55c728f6635db429eda2abfc9fdd30c7fbf50d1529fc92a04f1b568b0ecbb1b97"
      "296fdb3c3e87279b1e729a58494b8ddd1c2de186cb342c8a0c47ebeb472a72bcc9980546"
      "3e12a45f7abe1a9e00708481d4e831db69c7a8a75d21f49096a9f09f2519cca23c31d924"
      "46a781c216431ad40a6ba0894d8209a8814eb7067cac18eb189443b83b7c31111f9ab87e"
      "2e1fe2732a20d66001247dca3e841b1e2af743b436854787043c819c717c603af76b5bb0"
      "17563b1b06569a34c32eeb37c3dcdc79d8da27eb02f33ff2e4bf662e48030a628bfd7f92"
      "08f402256139f5cec1586581f5d6e76e343836334e48ed4c38009ce7e8c1c6ba99a138a4"
      "488a73c58ec6be50b7aea9b73c456e6fe34d919956c6e10dc17d1f56ed0ee2283c3bfdad"
      "5ec71372c6f4bee52651e567f4ccba4041d9585d0ab6fd6b37f70841eb832c415cb07755"
      "9391a990178f16f5d3e65db85168b0a06697f3565306e9519e98512af83a369421c9071f"
      "29b3bd7b3035b6abb7b6ec16091a84c5f7d1dfad2d86bc9bfd6aec43296e8c640f02102f"
      "0bae9f8aa588a0870b565975ccdbd4e36d60e7495b7c7327ffd2e2781738d2cd9e84091a"
      "5ef61ce08e88f9f7857217a2734abd9be566a9cccf4d8e132ae41f4eff88efbcef33b899"
      "49c89dbb6d209a32f71e43a8e396e58198e19bc901dfde50ce14f176e6a4659dd2fd3af9"
      "96f02455f7b8019394474527dc45a8df1d12989e29c2a61e1160df2218681558f76c15f2"
      "fa59f06282d669137df064d832a149034c8d80ddf04d86a2952ad1c1e9930fedaa413de1"
      "9e9f7b8bd63deea02e5f04d099cd5e8526bed165453618414d45cc5dd3ef61a37606eaea"
      "bedbb4053061095288d048a907d9f834aed1058cc4aa6034dbe6b21f9c542fa4e0f7b5ff"
      "1703fd1e5fd3750fa5e4127dcaac7aaaf68a82f4d1ee0d3dc7df388731f9f05006e227e4"
      "035f35051a466e15bbb2112059d4193dcabd2a600abeecf61bb5a8290d67a9db2cdcc993"
      "a8d40842aad60e98d43006415488298ef06f58e7932b07da362ce5b53df62e1659bdfd38"
      "bf16c438facd3bba2ac24970400aa6adf3e2b2fc2abcd826f01b9efa1530c345d1fc7d7d"
      "175b984a60d758b90ed642399928b22e0651c14843a5eedc40ea3059f1068fbe69f0ff7d"
      "fc5605ec2b06d04d89c73e5308f480fb615d246831ab5796ebc8f71412ed574046156f1e"
      "1106d38400261d852124baf2b88df8ce478900bb2110725c4a8fc0d5254bf7c8cfa6a247"
      "2c906a0d36d30f76992ceb5d1243717560d2d95939c7c797a846ec9dfd7cf9bec5b0fff8"
      "01dad2d880db7b8bc258b03dd880be8fad82fd81f5dc733c4849b62535f332df23ac8374"
      "c1bc919043ccbb87829e5810f41db978226585f425342f23eb56204c295378138fa2a036"
      "373a7dc7d98e9084dde588597794a20284d46a7f8855a827005c5061ce712a832776e631"
      "93da6ad2842ad6d3efab41b2de71b6c1e076da0238a7a51006345d541ac92ee9c55a9c1d"
      "4fff93cc303627a4e3c4b0e25c4fc88b08f15eab3ff444a5db38ae5c5f0c4fa464bf4fc3"
      "8f235296894d05955ca05fda309f604e36686f32c29fa8228080f335d532e8ff46ea27e6"
      "889fc15471c433df34f9e5c4f5f14cfdae8e474dfcf70376ee234459b135f5b61f9ed74a"
      "e404e6d91fb9de9841b5318fbb389f1be36af6d8be5a65a60f6db513b97be7916b840c49"
      "2fc06276f16c0fe9dbe67385f61c0d04d002ebc29501ff8c65760eaa2ab0b14215690860"
      "ee2726c9af84f63ee1d105b3215a616eaaf7f9bd43c562d115180e76101b14e43bde7b3d"
      "a27f7e6a01715f73eb25a74a8bebea8135ae88a4e4c67f6b333395fd848db07685b942c5"
      "235d12c6e80d954726c0b4723ecfd76233df17b3304c0fe57418e9d327eaecc419f532a6"
      "a845e1259f955628f66d9addeacbc952266e17f436ca6245eb0a3a773acd6081e852a873"
      "abfc64044dd0049d75bffd723f167747a720dc2a586ba3637c53e501c61da71dcdac0c7f"
      "31c370e44db9668f20354e9c59b6ae181718017746b758b963874a736cc461b093558c02"
      "f39aa67a2d1992137ad105702edfe9a0b66dcc8da602a138619d9d03726bde31f2f8dd42"
      "2df8bff2f160e1fb1027d7e8fdbde0a89fa5adc4d51c7905f0fb73a118f9d2c205426f2e"
      "ea94d8643d78bb037ed149314e7892cbb39859e65d3adfa5e0b4b87424fa76d158545ffe"
      "22bfb416d94513e5c965281e02315ddeda93132cfd14f72712d29e4dfadf93f536f09aba"
      "ff1f2aa259421ca24d3c8d763db46b5cad9e88dd0a0a0f8c44505c6e24bb68a8a936e908"
      "783925701725fc480ded76382bfa6d9010e2baa3f0ff26ff9ad50ef3c8dcf088bfc943e0"
      "5d11fd9ea39a1a01bc3dd7b4378cbc699e7b0511a6b1a39b8ad0d480ca35b0474839873b"
      "1b99aa09a8a542b91abba49a4e5ee735009e8277fbbd776b26a0aea78b4de00e5602b2d3"
      "77057f480d0310299ce643987861c980aeb4af51e140c387ac383c4a6b0f1585f2f545c2"
      "3bb72fb4d42e0a867b0010e802c0ae91ec022c54c6190935b7fea7d9cd593b581977b1c6"
      "a577c5f00109c0494bd1e0fe20fa30ee80c40c19f9958dc094b441ce386ce05a43a2c5e2"
      "41b4f9f533a26fe9867a92ec7a6c10767e0476b4796983852b601f81327ec6beac37caa1"
      "c3ade25b6e53449043b3930882d2f2c5b121555816dbb2c9158b8c337851c96b664558b1"
      "2e3e1c3c58babc7ce8ef97201223449f02aad6257289941512a384599ef84a445f782beb"
      "84410a8cbd36e07a49193b2314f97476c26226f27f09f5877b6a1c826e971fac55561ed4"
      "6fb23a3fbefc939405d94b6eca37c42ce9f37a74e01f829f3414dbe84581c7a5276ded75"
      "d053e80c148cadb73052d558770d8763a2c2502eb73e9f88009881c2f6c00e7cc86d0263"
      "3c570ceeb276520a29ed7fc3ac537ce9dbd42f345d4973a313a971b05e10b02e3244eaed"
      "2058b03174578765917578aa4f313fccca0cddee061cd32ff1f6f5cc1633054fee00077f"
      "330ff1634810e5b70c8ef9d1c54a100f06d25b0858e24be103ca51b68f9b9e02dd1a56a6"
      "c21f09606fefc8907e063ea28728b8959bb8251a144ec4aab4f8adc75d560426d5a600b5"
      "409b2211fc717f0ad288833c013efc3a366b2cbb6f19f76d5d801314754942d0bbdd9581"
      "040b9da57ba2b63afa8c0f88135547dd6c816ab9d6af0317524c8f8de065ae085967162b"
      "969a1c4dcf9d801e3a74414c2013d39ad9409d50cb5c926b5229823db42b61a4fc2488be"
      "598457fe2dd79eec71796ebeb51ad6ccbfaaee8a47fde3506cbdfa10691f9f8737facbc9"
      "27504cff691306c6e340392bb9579b832e6a6a1e21c4c1d1a095b131d21ccc6819357b22"
      "2ce7e0b8b994b33e3eba6a8bf26328544ac0558983be30699c88ccb411032ffbb16e8b60"
      "a9f4eba77aa854b536deebb451dd627409b231d3813d939d01f15bdb7bd3280ec4636226"
      "80a69456771cfa62b00a1e6a68ceb2c779fff05403b7449d77bd1b6cf670c6e3af705120"
      "1ef360337a91ddbb4bb21d33efd7574f31bd4b29ec8933237a91b2ae4e0300761e84a6da"
      "8cb3e33846b30ef1d0c925b72b96e750f76e0be04880d4a1208fad71ea8f3cc2176195d3"
      "add5c809eac5e95b27695cd95ede9899a6427fb4f1685f84c3495124c6d643f0a21a6e20"
      "e4b5c5cc08f7408cfab59d9ee3f2118285ce591f376b690f35ce445dd6329acf3c8b10e0"
      "2d7b36748547c0cd62acbf0aed6fa299d5eb5ae2fef8f71eb3b9ac14c3a0fe7d020a15a8"
      "a311b2af183376a8e12758524638b7b991b0c3f341b717ca8b40673720e09fff14726548"
      "d5a801b072ecbc4a660d38b20ed8d7c56b6a5e4b49f0e9a83b616ef7b7bbe581824c9744"
      "4fe6a51b8bd9604ee31768387e719ec3a285d6380696c122676f81ecbdac2ef811625958"
      "5150a5ae0a6300dafa5ddc72b1bd4e761564671fadcab3c78030112d904b3ba440a6f004"
      "f2a5374f3b06671669eeb61a11b237975eb7810276f5e910d453d1ae26dbde4716ea3572"
      "00c410807537762e0dcd21b3e529b3a8b8730b5f88e1611f55947213ce2a9d98b19c2cfd"
      "7638ce211252eeba11024723424c188aace76c86a4b474513799aff07134b3161f3eb284"
      "29cc6f242ecd631dbbb582e03695940ef8c048d487bb9639dac14e93f815ab722a45b032"
      "dc6b84cdf4de22c3ff88f4885775ec772b59361ae69821ac8db4a4db16e173e22fd5463c"
      "603599b48e1ae148e52ce9f5575e4f275c32f136393791992ba6cd76c371a26d80b10837"
      "7c10afbcc194591d40123265c9b3339dc02a407136a1c6db62a9d5799bfde07b0258b75a"
      "46ae9354d113f942e5405f5c5df0472d397f51c3aa52da20e85f9f4231c530d9cf614cea"
      "b2b93d5b9bce82feaabe70733d16def53d9a2d8ee442b7c546bec045e679b97047656970"
      "67fd0b6b8c2be2ac15a3f9102c43da3ec794dc1fce1903ebdeb98f18d2760905351e35ac"
      "ea5c096d072e5fbad3442580f22d752f57f506a9e2d00b597d2b05ccea5e2bcc9e26a778"
      "3369c1d5cf74fe91b28767eca1022211fea26365604c0db79c9fec9d8a9b877f2a7a9528"
      "130e1f5c68893c2cedf81604a079069b8d54c96be317235a5de16b051bcc882d29bddd85"
      "18c4ec3aee253a5df41f018d1bc6fc55b66620d213acebee02d2e01511a01ecd839b68be"
      "9e8636e0321eec652f4ccd04a4a948bcd3c4833b29f38e3b2adc5205ea2f8e33b2bbc2ff"
      "ff530b0993f524b8d3cf8eebb73e0e7f12df58ba0c83c3a494bc4b2c36389d2a9058e8b0"
      "72bac2bde658bf90771991b23fd80ed8f15e0ef517113fd01f16e95456a5443923a3b29f"
      "a34743f71890b34b31790e8cf1a49004078c618d3d63f93d3e51ecd80d6cc4571560a581"
      "ea41e5d30722dbdeef6cc73dd49a9d3771a08e8efcaa97a051bde9be60c40dc76e331353"
      "2f0270598590f18c62d315a8ee5426cadb2514f136529308e6dd83059e1273482bf3f14c"
      "c0570a9d821550cd2596918ccc90d62851ddb08bba4582377ee508db2e03fb4fbf966720"
      "6c39f0003bb5b5651f8e44778d84a1c66212321c4a99bf491968ccb3a952fc16851b7671"
      "3caeed6c5246ca2773310c150cd19ad8c4beab8a27321f5e8564ea2447444a5dca5ebe07"
      "0f1747757aa5da9ccc9f105cd2ac42bf1fb8706dc44530ba78036f84b15550dbb2dcaf35"
      "92bbaad8fe1d55ed8ee43684135d0430923a17b5faae71dfc53cd2ae937da743e3143403"
      "e1d746af282397821e44aae60a8029c96b744bf43de73cfee1b60253fdb478f6e3df4127"
      "fc2cfe7f3553e8c30c1a8d14118322d789d8c27d8972b1e1c5be27263a97512b9cb98178"
      "2b783999f6abaafade0ace7772756f0d226b3fd324ba66312980fb72c216426827620c72"
      "17e9d5674dcd6b28b0f5d0f30d5eb63c5599079a9a54c53dcb45ca8c2130e8daea5e0561"
      "05e255a1368a8516ec3e8f3d2f836c9f1dd891229573d243176b85dcf6fb4ebaea1a81a0"
      "190d43da059e61d7df7bb2f1b8ff67ee2937367217b42aa3c1233e32f8c9c3d3ef01985f"
      "50b73480a16ab31f51bf7e9d7ee732761a68ad7b7016afcfdc1f8c36769112b2383610a3"
      "089e11c38db1948dcc7ca13e3c49ed75431b91813adcb773d09c16c35f24089e2711f4c2"
      "8017f5cf448a3ad51afa64bcf369863f78bc35397cdef13f2d710a37ee6f7a448f1010ff"
      "e2834e4d0a5f2ccfdbafc58d70e09812e9aea649326a10a0601acb62711123f2a5b4da22"
      "35535e54a8611d343b05cc7495e0c12c986c130b9bd66cc6319f89fef170542203e3ceed"
      "7dd6eba37355d36e5445989fa6077bb3f4fb08106d927f22818b56cf3c8ef88ef0735b0e"
      "10a55307b13c1e5f652c8b9f28cf0318cbf779f72ab9024705592a3a506bd3a94c5fdb4f"
      "4b340463bf34b5189a1d9867c2b4a2456cbbd79216fea72b6de33d092f2e1eea40a5d7f2"
      "492cb605cf87959163582e6e341da8d000e8031ca39f50efe454e655b47b1790fab76c38"
      "14d4fee8eb6ecfd50856525a2ef990cdb71edaa77e2498fc3289e3ae95641b5f954434b4"
      "e8bbfad5563323ff2c8ff9f1cf27c47a9a9a37c2b502c65131ad0af348e5a3d221852741"
      "bb771a2c0cead03dd59fef05f3145de7f53e1fcbc5c429150fce44ca970d6c558a96ac89"
      "2c77ea55e1da8d4000f85f9196091a8d0e550a6fe4883e16d7790fe0a18f204a2398878e"
      "5ea9b67c4eb14ee46558734182351c308a631075d11f2460a1da38490f13d7aaa7652a67"
      "fa34d7a20a3a150655291900bd35db19b39dd5b9d986105f39ffeb474342a59c61b9cffb"
      "31280d745569b0e2d8e8060a9902d7725ce808ca0f579a8cb73a8a1e884a99663a82f5a7"
      "09a8454f8eb0e8eca4365e24277dc51e3f072daf43fe6f539571f5b62967fcf5aa4a7832"
      "c1ae620789753c2adb9012bf38eebe184a253571b0df0b3f9a761754bc7cf44e2b5ba88d"
      "efa8d0242f8714350a9db97ac358c82000a2423cec8c2ec5fcacc4fb42c3f62f89bbd9c4"
      "9decfe680eb0638da43ce0b030c240342f14071b17880ecbfba5bc3de78d389a55501229"
      "3d6445ecee42b4c1cad6c19716002ffbe658c24f0e93a81b5fd5a9d8f9e6b8593d07e67e"
      "d0be900dd8ec7341f934ed2451916a1b7caedb66976ab9b4af7457c00d9050107f92b223"
      "d1c3bea0a00e01fdfce1085755cb0cf6f683c3c70fa9268715e89e1f5d30085f36fc9c67"
      "abe1b77d9f5153506ca403f96fb425570907acb115e6541c55231c0fd04ff7f167bb1d71"
      "ffd236663232b41a46bf5c9650a47adf118856f589b92a6a103109c664c4d355ecaf8b91"
      "54e2128a3a4f05f19511fc6821c84c0901867f9c513a76bafb456a5902e330820e13a48d"
      "2fe7c72cb798a5b714e44328f759763a0a36b7af40fc63c641d448f03bdd11f2d55c3151"
      "7422c9fc0454aaa38ff70cc26b8f42560430f66175401bb164f1336fcd623fe83a55cdb0"
      "2687ae4e9b00533b8b72d69ed2369ca8bcc890b0ce48bc7c126b4926d79202af34a34127"
      "799da66634a73c496935288efc25f5db49e39df57c3627dbc27e082029e8c78efe844b4b"
      "10f41dc9726eb416d8e296f2b1ff20bee748783e4030d8b531a0557b3cddf9a731f1f03b"
      "30f402dc30175793abb52219a224b30d15efd51e0fa1db7fde288538504c6f8f67f18a49"
      "1662d4fc9a1d579517d0b1d8b3acc9e705fe55d789c882337dc8142b84be8909fc177be6"
      "27113fde2f090ac55e3cd1f31bebef0215bfac14789ce3c0c3540ffc7cfb172adf2f5aac"
      "5c340a63d8a11c8d1a52166d6b9445880ad63397574b9cc525a5200c5f9abc2aa1a1f409"
      "86f874063dce30f3f0139a891cf122e2e71692f9e8f48f6c7df4713d57eb1f135bb0c7d4"
      "2ad36caf09bcc3756fd2235ceccc4954eb6ac4c839a7206689fbae14e87cc78a16458e66"
      "e115d50d08308b7b850bdfe6a731e500cbb91dc195ebbd6dfde5e23a82a5b87d91197b3d"
      "261e574f536929afe4200918d6d90403281ccd5ba12c7d4184ae2b1658de48dbbc479b09"
      "60581a2b4e35117eb2132ec4a99e33cdefa4a142188bf9407061c9f5000bf7e42dd138d5"
      "5bd74b11a1433c4718e2752e28810eb81053903d86259e845137219ae4cbefe044ebbcfb"
      "a140bff0ce81069175ab3c821f3572dac32168a6027d27b363850d6d9d8f652091e0b23b"
      "eff1ce6eecd451b3965efb9f70d8924cba67b40ab9f2da0a4d41e5b88ad56bd8bf9b0f77"
      "5087406f1447ee193583a355c83360e4edb636d0c13393c03aa27f18fd036baadf479d0b"
      "0b3ca20358bcf3a67e9fb17bab0a4631703bcd517247c07fe17552f48416b5e21ec1f114"
      "bd64be34b72c71e53ebf0a012a23f5229f68ae7718c86f4e43376883b950c928efe9e0de"
      "414228f3b0e09209ddf761c45fc6bb7b144a94c114619c1a0e215d21df1b6a542d345330"
      "49894da1240692b4146b1a354c9e39c37060413fa90b124f92d2605058ddf622f9d3e108"
      "9e1af636eb6beff26094789fafc77618859db79a5a617a8c948334c3224eac112cd79951"
      "8374cf0e2a357df57fbc859498ed2e276d867e6615ac42387e35e904a03797ecbae6ea3d"
      "f1efb428c6af188e2796688894ea925d8df6ec11a7c2f2729f1246a6844df2db03f635ab"
      "2521faae3478e8d0f7fe3ed96e9cb7356082580a8725b44ada6584eaed4893bd534e9f64"
      "b406c318590f1be241557344948a4cb036f346a2aa072c32fee5fbaeacc64deeb98b7d4b"
      "d48ee318028396e803ed9aab8a6426375e255b9b874f71f5bba29513ae93230e96f9e152"
      "fdc5eaa657a53a6cfaee9dce87a4f73dc74de2b3277c3bf338b64bad8f2f75a37b1412ce"
      "d5ba9561f87797005dec739c1ea16f76852f196ba03dc4ccb770a3f8ad56cdb0145fe9c0"
      "7173d184b68f66c4dac1257d90c3dd100615ab3f9f2826bc951e107baeda6bc09daffef8"
      "a25533f85a5f3831c4aa84f386f3dadb383ee2066a25545198af6691f1bb00da6da7ce2b"
      "bf1d5d8d8cde36a335bd3653152cd6d2240dd673cbd9922f3bfcc2c0198a0b7e285b7afa"
      "a2cf9a530981ed76cd0749bdb714aaf7ac55c4043a22b4aa33e0d3eed42faab2969aa54a"
      "812a4cb423f916ecbbc204ba24f6f0b29ba41dae5f26466f751b15d82b7aae801011f8f6"
      "b896c9bc6e272d31d53f7081c1945720e0e6e9718ebbee9594ca9db3852ead7683d4f859"
      "01d53682722bbd7ab48fc22a0666aed6c435c8488f2e41533ea043fa92b1002554705eb7"
      "8274792afa708ff3fc599f29633f45d3de20ecd22bdd922431eb950a6fa9d68492ff1afc"
      "0244b86f2de37318837df0fce804b77f062838fd647bbad993f90dffc8426154a7ad2cb2"
      "7a109e3ae629850fe6544cd4b72cb44a9a4155232e5d73b86bb12011e30db29b14f1e2e5"
      "f9b553da455d64c623",
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
