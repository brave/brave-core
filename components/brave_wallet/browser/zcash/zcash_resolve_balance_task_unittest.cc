/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/zcash/zcash_resolve_balance_task.h"

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/files/scoped_temp_dir.h"
#include "base/functional/callback_helpers.h"
#include "base/test/mock_callback.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "brave/components/brave_wallet/browser/internal/orchard_sync_state.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/browser/test_utils.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_rpc.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_test_utils.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_wallet_service.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/features.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "brave/components/brave_wallet/common/zcash_utils.h"
#include "brave/components/services/brave_wallet/public/mojom/zcash_decoder.mojom.h"
#include "build/build_config.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::_;

namespace brave_wallet {

namespace {

// The account's next unused transparent receive address(derived from
// `kMnemonicDivideCruise`, account index 1). Only this address is treated as
// "known" on chain and resolves to a transparent utxo, mirroring
// `ZCashWalletServiceUnitTest.GetBalanceWithShielded`.
constexpr char kReceiverAddress[] = "t1ShtibD2UJkYTeGPxeLrMf3jvE11S4Lpwj";
constexpr char kUtxoAddress[] = "t1aFpD4qebqwbSAZLF4E8ZGmrTk36b1cocZ";
constexpr char kUtxoTxIdHex[] =
    "0x1b7a7109cec77ae38e57f4f0ec53a4046b08361abb92c62d9567ace684f633ab";
constexpr char kUtxoScriptHex[] =
    "0x76a914b3b55981e7bf53e10fe51aa4f45fdef06dec783d88ac";

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

  MOCK_METHOD5(IsKnownAddress,
               void(const std::string& chain_id,
                    const std::string& addr,
                    uint64_t block_start,
                    uint64_t block_end,
                    IsKnownAddressCallback callback));
};

class MockOrchardSyncState : public OrchardSyncState {
 public:
  explicit MockOrchardSyncState(const base::FilePath& path_to_database)
      : OrchardSyncState(path_to_database) {}
  ~MockOrchardSyncState() override {}

  MOCK_METHOD3(
      GetSpendableNotes,
      base::expected<std::optional<OrchardSyncState::SpendableNotesBundle>,
                     OrchardStorage::Error>(
          OrchardPool pool,
          const mojom::AccountIdPtr& account_id,
          const OrchardAddrRawPart& internal_addr));
};

// Builds a `SpendableNotesBundle` where `all_note_amounts` become
// `all_notes` and `spendable_note_amounts` become `spendable_notes`, matching
// the shape `ZCashResolveBalanceTask::CreateBalance()`'s `accumulate` lambda
// expects(pending = sum(all_notes), spendable = sum(spendable_notes)).
OrchardSyncState::SpendableNotesBundle MakeSpendableNotesBundle(
    const std::vector<uint64_t>& all_note_amounts,
    const std::vector<uint64_t>& spendable_note_amounts) {
  OrchardSyncState::SpendableNotesBundle bundle;
  for (uint64_t amount : all_note_amounts) {
    OrchardNote note;
    note.amount = amount;
    bundle.all_notes.push_back(note);
  }
  for (uint64_t amount : spendable_note_amounts) {
    OrchardNote note;
    note.amount = amount;
    bundle.spendable_notes.push_back(note);
  }
  return bundle;
}

}  // namespace

// Exercises `ZCashResolveBalanceTask` end-to-end via
// `ZCashWalletService::GetBalance()`(the task has no public test seam of its
// own - it's only ever created internally with a
// `base::PassKey<ZCashWalletService>`), focusing on how the Ironwood pool's
// spendable notes contribute to the resulting `mojom::ZCashBalance`.
class ZCashResolveBalanceTaskTest : public testing::Test {
 public:
  void SetUp() override {
    feature_list_.InitWithFeaturesAndParameters(
        {{features::kBraveWalletZCashFeature,
          {{"zcash_shielded_transactions_enabled", "true"},
           {"zcash_ironwood_transaction_enabled", "true"}}},
#if BUILDFLAG(IS_IOS)
         {features::kBraveWalletWebUIFeature, {}}
#endif
        },
        {}  // disabled features
    );

    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());

    brave_wallet::RegisterProfilePrefs(prefs_.registry());
    brave_wallet::RegisterLocalStatePrefs(local_state_.registry());

    keyring_service_ =
        std::make_unique<KeyringService>(nullptr, &prefs_, &local_state_);
    keyring_service_->Reset();
    keyring_service_->RestoreWallet(kMnemonicDivideCruise, kTestWalletPassword,
                                    false, base::DoNothing());

    zcash_wallet_service_ = std::make_unique<TestingZCashWalletService>(
        *keyring_service_, std::make_unique<testing::NiceMock<MockZCashRPC>>());
    zcash_wallet_service_->SetupSyncState(
        OrchardSyncState::CreateSyncStateSequence(),
        std::make_unique<MockOrchardSyncState>(temp_dir_.GetPath()));

    account_ = AccountUtils(keyring_service_.get())
                   .EnsureAccount(mojom::KeyringId::kZCashMainnet, 1);
    ASSERT_TRUE(account_);
    keyring_service_->UpdateNextUnusedAddressForZCashAccount(
        account_->account_id, 1, 0);

    SetUpDiscoveryRpcMocks();
  }

  // Mocks the RPC calls used by `ZCashWalletService::RunDiscovery()`(via
  // `ZCashDiscoverNextUnusedZCashAddressTask`): only `kReceiverAddress` is
  // reported as a "known" on-chain address.
  void SetUpDiscoveryRpcMocks() {
    ON_CALL(zcash_rpc(), GetLatestBlock(_, _))
        .WillByDefault(  //
            [](const std::string& chain_id,
               ZCashRpc::GetLatestBlockCallback callback) {
              auto response = zcash::mojom::BlockID::New(
                  2625446u,
                  *PrefixedHexStringToBytes(
                      "0x0000000001a01b5fd794e4b071443974c835b3e0ff8f96bf3600"
                      "e07afdbf89c5"));
              std::move(callback).Run(std::move(response));
            });

    ON_CALL(zcash_rpc(), IsKnownAddress(_, _, _, _, _))
        .WillByDefault([](const std::string& chain_id, const std::string& addr,
                          uint64_t block_start, uint64_t block_end,
                          ZCashRpc::IsKnownAddressCallback callback) {
          std::move(callback).Run(addr == kReceiverAddress);
        });
  }

  // Mocks `GetUtxoList` so that only `kReceiverAddress` resolves to a single
  // transparent utxo of `value_zat`; every other address discovered for the
  // account resolves to an empty utxo list.
  void SetUpUtxoMock(uint64_t value_zat) {
    ON_CALL(zcash_rpc(), GetUtxoList(_, _, _))
        .WillByDefault(  //
            [value_zat](const std::string& chain_id, const std::string& address,
                        ZCashRpc::GetUtxoListCallback callback) {
              std::vector<zcash::mojom::ZCashUtxoPtr> utxos;
              if (address == kReceiverAddress) {
                utxos.push_back(zcash::mojom::ZCashUtxo::New(
                    kUtxoAddress, *PrefixedHexStringToBytes(kUtxoTxIdHex), 0,
                    *PrefixedHexStringToBytes(kUtxoScriptHex), value_zat,
                    2468320u));
              }
              auto response =
                  zcash::mojom::GetAddressUtxosResponse::New(std::move(utxos));
              std::move(callback).Run(std::move(response));
            });
  }

  testing::NiceMock<MockZCashRPC>& zcash_rpc() {
    return static_cast<testing::NiceMock<MockZCashRPC>&>(
        zcash_wallet_service_->zcash_rpc());
  }

  MockOrchardSyncState& mock_orchard_sync_state() {
    return static_cast<MockOrchardSyncState&>(
        *zcash_wallet_service_->sync_state_ptr);
  }

  ZCashWalletService& zcash_wallet_service() { return *zcash_wallet_service_; }

  mojom::AccountIdPtr account_id() { return account_->account_id.Clone(); }

  base::test::TaskEnvironment& task_environment() { return task_environment_; }

 private:
  base::test::TaskEnvironment task_environment_;

  base::test::ScopedFeatureList feature_list_;

  base::ScopedTempDir temp_dir_;

  sync_preferences::TestingPrefServiceSyncable prefs_;
  sync_preferences::TestingPrefServiceSyncable local_state_;

  mojom::AccountInfoPtr account_;

  std::unique_ptr<KeyringService> keyring_service_;
  std::unique_ptr<TestingZCashWalletService> zcash_wallet_service_;
};

// Both pools resolve distinct `SpendableNotesBundle`s. Verifies that
// `ironwood_balance`/`ironwood_pending_balance` accumulate independently from
// `orchard_balance`/`orchard_pending_balance`, per the `accumulate` lambda in
// `ZCashResolveBalanceTask::CreateBalance()`(balance = sum(spendable_notes),
// pending = sum(all_notes) - sum(spendable_notes)).
TEST_F(ZCashResolveBalanceTaskTest,
       IronwoodBalanceAccumulatesIndependentlyFromOrchard) {
  SetUpUtxoMock(100u);

  ON_CALL(mock_orchard_sync_state(), GetSpendableNotes(_, _, _))
      .WillByDefault([](OrchardPool pool, const mojom::AccountIdPtr& account_id,
                        const OrchardAddrRawPart& internal_addr) {
        if (pool == OrchardPool::kIronwood) {
          return MakeSpendableNotesBundle({5u, 15u}, {5u});
        }
        return MakeSpendableNotesBundle({10u, 20u}, {10u});
      });

  base::MockCallback<ZCashWalletService::GetBalanceCallback> balance_callback;
  EXPECT_CALL(balance_callback, Run(_, _))
      .WillOnce([&](mojom::ZCashBalancePtr balance,
                    std::optional<std::string> error) {
        ASSERT_TRUE(balance);
        EXPECT_FALSE(error.has_value());
        EXPECT_EQ(balance->orchard_balance, 10u);
        EXPECT_EQ(balance->orchard_pending_balance, 20u);
        EXPECT_EQ(balance->ironwood_balance, 5u);
        EXPECT_EQ(balance->ironwood_pending_balance, 15u);
      });

  zcash_wallet_service().GetBalance(account_id(), balance_callback.Get());
  task_environment().RunUntilIdle();
}

// With the ironwood feature flag disabled, `ironwood_balance` and
// `ironwood_pending_balance` must stay at 0 and
// `OrchardSyncState::GetSpendableNotes()` must never be called for
// `OrchardPool::kIronwood`, while orchard balance resolution(including its
// pending-balance math) is unaffected. Note: the shielded-transactions flag
// is kept enabled here because `ZCashWalletService::CreateActionContext()`
// only resolves `account_internal_addr`(required by both pools) when
// `IsZCashShieldedTransactionsEnabled()` is true - independent of the
// ironwood flag.
TEST_F(ZCashResolveBalanceTaskTest,
       IronwoodDisabledSkipsRpcCallAndKeepsBalanceZero) {
  base::test::ScopedFeatureList ironwood_disabled_feature_list;
  ironwood_disabled_feature_list.InitWithFeaturesAndParameters(
      {{features::kBraveWalletZCashFeature,
        {{"zcash_shielded_transactions_enabled", "true"},
         {"zcash_ironwood_transaction_enabled", "false"}}},
#if BUILDFLAG(IS_IOS)
       {features::kBraveWalletWebUIFeature, {}}
#endif
      },
      {}  // disabled features
  );

  SetUpUtxoMock(100u);

  // General(least-specific) expectation declared first so it is checked
  // last: gmock resolves matching expectations in reverse declaration order,
  // so the `kIronwood`-specific `Times(0)` expectation below takes priority
  // for `kIronwood` calls, while `kOrchard` calls fall through to this one.
  EXPECT_CALL(mock_orchard_sync_state(), GetSpendableNotes(_, _, _))
      .WillRepeatedly([](OrchardPool pool,
                         const mojom::AccountIdPtr& account_id,
                         const OrchardAddrRawPart& internal_addr) {
        return MakeSpendableNotesBundle({10u, 20u}, {10u});
      });
  EXPECT_CALL(mock_orchard_sync_state(),
              GetSpendableNotes(OrchardPool::kIronwood, _, _))
      .Times(0);

  base::MockCallback<ZCashWalletService::GetBalanceCallback> balance_callback;
  EXPECT_CALL(balance_callback, Run(_, _))
      .WillOnce([&](mojom::ZCashBalancePtr balance,
                    std::optional<std::string> error) {
        ASSERT_TRUE(balance);
        EXPECT_FALSE(error.has_value());
        EXPECT_EQ(balance->orchard_balance, 10u);
        EXPECT_EQ(balance->orchard_pending_balance, 20u);
        EXPECT_EQ(balance->ironwood_balance, 0u);
        EXPECT_EQ(balance->ironwood_pending_balance, 0u);
      });

  zcash_wallet_service().GetBalance(account_id(), balance_callback.Get());
  task_environment().RunUntilIdle();
}

// If the Ironwood pool's spendable notes sum exceeds its all-notes sum(an
// invariant violation), `CreateBalance()`'s `accumulate` lambda fails and the
// whole task resolves to a "Pending balance error", regardless of the
// transparent/orchard legs having already succeeded.
TEST_F(ZCashResolveBalanceTaskTest,
       IronwoodPendingBalanceErrorPropagatesAsTaskError) {
  SetUpUtxoMock(100u);

  ON_CALL(mock_orchard_sync_state(), GetSpendableNotes(_, _, _))
      .WillByDefault([](OrchardPool pool, const mojom::AccountIdPtr& account_id,
                        const OrchardAddrRawPart& internal_addr) {
        if (pool == OrchardPool::kIronwood) {
          // Invariant violation: spendable notes sum(50) exceeds all notes
          // sum(10).
          return MakeSpendableNotesBundle({10u}, {50u});
        }
        return OrchardSyncState::SpendableNotesBundle();
      });

  base::MockCallback<ZCashWalletService::GetBalanceCallback> balance_callback;
  EXPECT_CALL(balance_callback, Run(_, _))
      .WillOnce([&](mojom::ZCashBalancePtr balance,
                    std::optional<std::string> error) {
        EXPECT_FALSE(balance);
        ASSERT_TRUE(error.has_value());
        EXPECT_EQ(error.value(), "Pending balance error");
      });

  zcash_wallet_service().GetBalance(account_id(), balance_callback.Get());
  task_environment().RunUntilIdle();
}

// End-to-end sanity check: transparent utxos, orchard notes and ironwood
// notes all contribute to `total_balance`, which is the sum of the three
// pools' spendable balances(pending amounts are excluded from
// `total_balance`, see `ZCashResolveBalanceTask::CreateBalance()`).
TEST_F(ZCashResolveBalanceTaskTest,
       TotalBalanceSumsTransparentOrchardAndIronwood) {
  SetUpUtxoMock(100u);

  ON_CALL(mock_orchard_sync_state(), GetSpendableNotes(_, _, _))
      .WillByDefault([](OrchardPool pool, const mojom::AccountIdPtr& account_id,
                        const OrchardAddrRawPart& internal_addr) {
        if (pool == OrchardPool::kIronwood) {
          return MakeSpendableNotesBundle({30u}, {30u});
        }
        return MakeSpendableNotesBundle({50u}, {50u});
      });

  base::MockCallback<ZCashWalletService::GetBalanceCallback> balance_callback;
  EXPECT_CALL(balance_callback, Run(_, _))
      .WillOnce([&](mojom::ZCashBalancePtr balance,
                    std::optional<std::string> error) {
        ASSERT_TRUE(balance);
        EXPECT_FALSE(error.has_value());
        EXPECT_EQ(balance->transparent_balance, 100u);
        EXPECT_EQ(balance->orchard_balance, 50u);
        EXPECT_EQ(balance->orchard_pending_balance, 0u);
        EXPECT_EQ(balance->ironwood_balance, 30u);
        EXPECT_EQ(balance->ironwood_pending_balance, 0u);
        EXPECT_EQ(balance->total_balance, 180u);
      });

  zcash_wallet_service().GetBalance(account_id(), balance_callback.Get());
  task_environment().RunUntilIdle();
}

}  // namespace brave_wallet
