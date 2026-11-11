/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/zcash/zcash_create_ironwood_to_transparent_transaction_task.h"

#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

#include "base/files/scoped_temp_dir.h"
#include "base/functional/callback_helpers.h"
#include "base/test/gmock_callback_support.h"
#include "base/test/mock_callback.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/browser/test_utils.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_test_utils.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_wallet_service.h"
#include "brave/components/brave_wallet/common/features.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"

using testing::_;
using testing::SaveArg;

namespace brave_wallet {

namespace {

constexpr char kTransparentAddress[] = "t1WTZNzKCvU2GeM1ZWRyF7EvhMHhr7magiT";

class MockZCashWalletService : public TestingZCashWalletService {
 public:
  using TestingZCashWalletService::TestingZCashWalletService;

  MOCK_METHOD3(DiscoverNextUnusedAddress,
               void(const mojom::AccountIdPtr& account_id,
                    bool change,
                    DiscoverNextUnusedAddressCallback callback));
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

}  // namespace

class ZCashCreateIronwoodToTransparentTransactionTaskTest
    : public testing::Test {
 public:
  void SetUp() override {
    feature_list_.InitWithFeaturesAndParameters(
        {{features::kBraveWalletZCashFeature,
          {{"zcash_shielded_transactions_enabled", "true"}}},
        },
        {}  // disabled features
    );
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());

    brave_wallet::RegisterProfilePrefs(prefs_.registry());
    brave_wallet::RegisterLocalStatePrefs(local_state_.registry());

    keyring_service_ =
        std::make_unique<KeyringService>(nullptr, &prefs_, &local_state_);
    keyring_service_->Reset();
    keyring_service_->RestoreWallet(kMnemonicGalleryEqual, kTestWalletPassword,
                                    false, base::DoNothing());

    zcash_wallet_service_ = std::make_unique<MockZCashWalletService>(
        *keyring_service_,
        std::make_unique<testing::NiceMock<ZCashRpc>>(nullptr, nullptr));
    zcash_wallet_service_->SetupSyncState(
        OrchardSyncState::CreateSyncStateSequence(),
        std::make_unique<MockOrchardSyncState>(temp_dir_.GetPath()));

    account_id_ = AccountUtils(keyring_service_.get())
                      .EnsureAccount(mojom::KeyringId::kZCashMainnet, 0)
                      ->account_id.Clone();
  }

  MockZCashWalletService& zcash_wallet_service() {
    return *zcash_wallet_service_;
  }

  KeyringService& keyring_service() { return *keyring_service_; }

  mojom::AccountIdPtr& account_id() { return account_id_; }

  MockOrchardSyncState& mock_orchard_sync_state() {
    return static_cast<MockOrchardSyncState&>(
        *zcash_wallet_service_->sync_state_ptr);
  }

  base::PassKey<class ZCashCreateIronwoodToTransparentTransactionTaskTest>
  pass_key() {
    return base::PassKey<
        class ZCashCreateIronwoodToTransparentTransactionTaskTest>();
  }

  ZCashActionContext action_context() {
    return zcash_wallet_service_->CreateActionContext(account_id());
  }

  base::test::TaskEnvironment& task_environment() { return task_environment_; }

 private:
  base::test::ScopedFeatureList feature_list_;
  base::test::TaskEnvironment task_environment_;
  base::ScopedTempDir temp_dir_;

  sync_preferences::TestingPrefServiceSyncable prefs_;
  sync_preferences::TestingPrefServiceSyncable local_state_;
  mojom::AccountIdPtr account_id_;

  std::unique_ptr<KeyringService> keyring_service_;
  std::unique_ptr<MockZCashWalletService> zcash_wallet_service_;
};

TEST_F(ZCashCreateIronwoodToTransparentTransactionTaskTest,
       TransactionCreated) {
  ON_CALL(mock_orchard_sync_state(), GetSpendableNotes(_, _, _))
      .WillByDefault([&](OrchardPool pool,
                         const mojom::AccountIdPtr& account_id,
                         const OrchardAddrRawPart& addr) {
        EXPECT_EQ(pool, OrchardPool::kIronwood);
        OrchardSyncState::SpendableNotesBundle spendable_notes_bundle;
        {
          OrchardNote note;
          note.block_id = 1u;
          note.amount = 70000u;
          note.note_version = 2;
          spendable_notes_bundle.spendable_notes.push_back(std::move(note));
        }

        {
          OrchardNote note;
          note.block_id = 2u;
          note.amount = 80000u;
          note.note_version = 2;
          spendable_notes_bundle.spendable_notes.push_back(std::move(note));
        }
        spendable_notes_bundle.anchor_block_id = 10u;

        return spendable_notes_bundle;
      });

  base::MockCallback<ZCashWalletService::CreateTransactionCallback> callback;

  std::unique_ptr<ZCashCreateIronwoodToTransparentTransactionTask> task =
      std::make_unique<ZCashCreateIronwoodToTransparentTransactionTask>(
          pass_key(), zcash_wallet_service(), action_context(),
          kTransparentAddress, 100000u);

  base::expected<ZCashTransaction, std::string> tx_result;
  EXPECT_CALL(callback, Run(_))
      .WillOnce(::testing::DoAll(
          SaveArg<0>(&tx_result),
          base::test::RunOnceClosure(task_environment().QuitClosure())));

  task->Start(callback.Get());

  task_environment().RunUntilQuit();

  EXPECT_TRUE(tx_result.has_value());
  EXPECT_TRUE(tx_result.value().is_v6());

  EXPECT_EQ(tx_result.value().v6_part().ironwood.inputs.size(), 2u);
  EXPECT_EQ(tx_result.value().v6_part().ironwood.outputs.size(), 1u);
  EXPECT_EQ(tx_result.value().transparent_part().outputs.size(), 1u);
  EXPECT_EQ(tx_result.value().v6_part().ironwood.anchor_block_height.value(),
            10u);

  EXPECT_EQ(tx_result.value().v6_part().ironwood.inputs[0].note.amount, 70000u);
  EXPECT_EQ(tx_result.value().v6_part().ironwood.inputs[1].note.amount, 80000u);

  EXPECT_EQ(tx_result.value().transparent_part().outputs[0].amount, 100000u);
  EXPECT_EQ(tx_result.value().transparent_part().outputs[0].address,
            kTransparentAddress);

  // Should have change output
  EXPECT_EQ(tx_result.value().v6_part().ironwood.outputs.size(), 1u);
  EXPECT_EQ(tx_result.value().v6_part().ironwood.outputs[0].value,
            50000u - 3 * 5000u);
}

TEST_F(ZCashCreateIronwoodToTransparentTransactionTaskTest,
       TransactionCreated_NoAnchorBlockId) {
  ON_CALL(mock_orchard_sync_state(), GetSpendableNotes(_, _, _))
      .WillByDefault([&](OrchardPool /*pool*/,
                         const mojom::AccountIdPtr& account_id,
                         const OrchardAddrRawPart& addr) {
        OrchardSyncState::SpendableNotesBundle spendable_notes_bundle;
        {
          OrchardNote note;
          note.block_id = 1u;
          note.amount = 70000u;
          note.note_version = 2;
          spendable_notes_bundle.spendable_notes.push_back(std::move(note));
        }

        {
          OrchardNote note;
          note.block_id = 2u;
          note.amount = 80000u;
          note.note_version = 2;
          spendable_notes_bundle.spendable_notes.push_back(std::move(note));
        }

        return spendable_notes_bundle;
      });

  base::MockCallback<ZCashWalletService::CreateTransactionCallback> callback;

  std::unique_ptr<ZCashCreateIronwoodToTransparentTransactionTask> task =
      std::make_unique<ZCashCreateIronwoodToTransparentTransactionTask>(
          pass_key(), zcash_wallet_service(), action_context(),
          kTransparentAddress, 100000u);

  base::expected<ZCashTransaction, std::string> tx_result;
  EXPECT_CALL(callback, Run(_))
      .WillOnce(::testing::DoAll(
          SaveArg<0>(&tx_result),
          base::test::RunOnceClosure(task_environment().QuitClosure())));

  task->Start(callback.Get());

  task_environment().RunUntilQuit();

  EXPECT_FALSE(tx_result.has_value());
}

TEST_F(ZCashCreateIronwoodToTransparentTransactionTaskTest,
       TransactionCreated_MaxAmount) {
  ON_CALL(mock_orchard_sync_state(), GetSpendableNotes(_, _, _))
      .WillByDefault([&](OrchardPool /*pool*/,
                         const mojom::AccountIdPtr& account_id,
                         const OrchardAddrRawPart& addr) {
        OrchardSyncState::SpendableNotesBundle spendable_notes_bundle;
        {
          OrchardNote note;
          note.block_id = 1u;
          note.amount = 70000u;
          note.note_version = 2;
          spendable_notes_bundle.spendable_notes.push_back(std::move(note));
        }

        {
          OrchardNote note;
          note.block_id = 2u;
          note.amount = 80000u;
          note.note_version = 2;
          spendable_notes_bundle.spendable_notes.push_back(std::move(note));
        }
        spendable_notes_bundle.anchor_block_id = 10u;

        return spendable_notes_bundle;
      });

  base::MockCallback<ZCashWalletService::CreateTransactionCallback> callback;

  std::unique_ptr<ZCashCreateIronwoodToTransparentTransactionTask> task =
      std::make_unique<ZCashCreateIronwoodToTransparentTransactionTask>(
          pass_key(), zcash_wallet_service(), action_context(),
          kTransparentAddress, kZCashFullAmount);

  base::expected<ZCashTransaction, std::string> tx_result;
  EXPECT_CALL(callback, Run(_))
      .WillOnce(::testing::DoAll(
          SaveArg<0>(&tx_result),
          base::test::RunOnceClosure(task_environment().QuitClosure())));

  task->Start(callback.Get());

  task_environment().RunUntilQuit();

  EXPECT_TRUE(tx_result.has_value());
  EXPECT_TRUE(tx_result.value().is_v6());

  EXPECT_EQ(tx_result.value().v6_part().ironwood.inputs.size(), 2u);
  EXPECT_EQ(tx_result.value().v6_part().ironwood.outputs.size(), 0u);
  EXPECT_EQ(tx_result.value().transparent_part().outputs.size(), 1u);
  EXPECT_EQ(tx_result.value().v6_part().ironwood.anchor_block_height.value(),
            10u);

  EXPECT_EQ(tx_result.value().v6_part().ironwood.inputs[0].note.amount, 70000u);
  EXPECT_EQ(tx_result.value().v6_part().ironwood.inputs[1].note.amount, 80000u);

  EXPECT_EQ(tx_result.value().transparent_part().outputs[0].amount,
            70000u + 80000u - 3 * 5000u);
  EXPECT_EQ(tx_result.value().transparent_part().outputs[0].address,
            kTransparentAddress);
}

// Fee correctness for Ironwood to transparent unshielding. The transparent
// target output forms its own bundle - max(0, 1) = 1 action - while the
// Ironwood spends and their change output form another, so the fee is
// 5000 * max(2, 1 + max(spends, change, 2)). Billing the spends anywhere but
// the Ironwood bundle would raise a spurious extra bundle, see
// https://github.com/brave/brave-browser/issues/58957.
TEST_F(ZCashCreateIronwoodToTransparentTransactionTaskTest, FeeCorrectness) {
  // Notes are picked ascending until they cover amount + fee, so the note set
  // and amount together control the spend count.
  struct {
    const char* label;
    std::vector<uint64_t> note_amounts;
    uint64_t amount;
    size_t expected_inputs;
    uint64_t expected_fee;
  } const kCases[] = {
      // 1 spend + change, 1 transparent target -> 1 + max(1, 1, 2) = 3.
      {"single note", {30000u}, 5000u, 1u, 15000u},
      // 2 spends still sit at the Ironwood minimum -> 1 + max(2, 1, 2) = 3.
      {"two notes", {20000u, 60000u}, 40000u, 2u, 15000u},
      // 3 spends exceed it -> 1 + max(3, 1, 2) = 4.
      {"three notes", {20000u, 30000u, 40000u}, 60000u, 3u, 20000u},
      // 5 spends -> 1 + max(5, 1, 2) = 6.
      {"five notes",
       {20000u, 30000u, 40000u, 50000u, 60000u},
       165000u,
       5u,
       30000u},
  };

  for (const auto& test_case : kCases) {
    SCOPED_TRACE(test_case.label);

    ON_CALL(mock_orchard_sync_state(), GetSpendableNotes(_, _, _))
        .WillByDefault([&](OrchardPool pool,
                           const mojom::AccountIdPtr& account_id,
                           const OrchardAddrRawPart& addr) {
          EXPECT_EQ(pool, OrchardPool::kIronwood);
          OrchardSyncState::SpendableNotesBundle spendable_notes_bundle;
          uint32_t block_id = 1u;
          for (uint64_t note_amount : test_case.note_amounts) {
            OrchardNote note;
            note.block_id = block_id++;
            note.amount = note_amount;
            note.note_version = 2;
            spendable_notes_bundle.spendable_notes.push_back(std::move(note));
          }
          spendable_notes_bundle.anchor_block_id = 10u;
          return spendable_notes_bundle;
        });

    auto task =
        std::make_unique<ZCashCreateIronwoodToTransparentTransactionTask>(
            pass_key(), zcash_wallet_service(), action_context(),
            kTransparentAddress, test_case.amount);

    base::MockCallback<ZCashWalletService::CreateTransactionCallback> callback;
    base::expected<ZCashTransaction, std::string> tx_result;
    EXPECT_CALL(callback, Run(_))
        .WillOnce(::testing::DoAll(
            SaveArg<0>(&tx_result),
            base::test::RunOnceClosure(task_environment().QuitClosure())));

    task->Start(callback.Get());
    task_environment().RunUntilQuit();

    ASSERT_TRUE(tx_result.has_value());
    EXPECT_EQ(tx_result.value().v6_part().ironwood.inputs.size(),
              test_case.expected_inputs);
    EXPECT_EQ(tx_result.value().fee(), test_case.expected_fee);

    // The transparent side carries only the target output - change goes back to
    // the Ironwood pool the notes were spent from.
    EXPECT_EQ(tx_result.value().transparent_part().inputs.size(), 0u);
    EXPECT_EQ(tx_result.value().transparent_part().outputs.size(), 1u);
    EXPECT_EQ(tx_result.value().transparent_part().outputs[0].amount,
              test_case.amount);
    EXPECT_EQ(tx_result.value().transparent_part().outputs[0].address,
              kTransparentAddress);

    // Nothing may be billed to the legacy Orchard bundle.
    EXPECT_EQ(tx_result.value().v6_part().legacy_orchard.inputs.size(), 0u);
    EXPECT_EQ(tx_result.value().v6_part().legacy_orchard.outputs.size(), 0u);

    // Value is conserved: spent notes = target + Ironwood change + fee.
    uint64_t total_outputs = test_case.amount;
    for (const auto& output : tx_result.value().v6_part().ironwood.outputs) {
      total_outputs += output.value;
    }
    EXPECT_EQ(tx_result.value().TotalInputsAmount().ValueOrDie(),
              total_outputs + tx_result.value().fee());
  }
}

// Full amount sends have no change output, so the Ironwood bundle covers the
// spends alone: 1 + max(spends, 0, 2).
TEST_F(ZCashCreateIronwoodToTransparentTransactionTaskTest,
       FeeCorrectness_MaxAmount) {
  ON_CALL(mock_orchard_sync_state(), GetSpendableNotes(_, _, _))
      .WillByDefault([&](OrchardPool pool,
                         const mojom::AccountIdPtr& account_id,
                         const OrchardAddrRawPart& addr) {
        OrchardSyncState::SpendableNotesBundle spendable_notes_bundle;
        for (uint32_t i = 0; i < 3u; ++i) {
          OrchardNote note;
          note.block_id = i + 1u;
          note.amount = 70000u;
          note.note_version = 2;
          spendable_notes_bundle.spendable_notes.push_back(std::move(note));
        }
        spendable_notes_bundle.anchor_block_id = 10u;
        return spendable_notes_bundle;
      });

  auto task = std::make_unique<ZCashCreateIronwoodToTransparentTransactionTask>(
      pass_key(), zcash_wallet_service(), action_context(), kTransparentAddress,
      kZCashFullAmount);

  base::MockCallback<ZCashWalletService::CreateTransactionCallback> callback;
  base::expected<ZCashTransaction, std::string> tx_result;
  EXPECT_CALL(callback, Run(_))
      .WillOnce(::testing::DoAll(
          SaveArg<0>(&tx_result),
          base::test::RunOnceClosure(task_environment().QuitClosure())));

  task->Start(callback.Get());
  task_environment().RunUntilQuit();

  ASSERT_TRUE(tx_result.has_value());

  // 3 spends, no change, 1 transparent target -> 1 + max(3, 0, 2) = 4 actions.
  EXPECT_EQ(tx_result.value().v6_part().ironwood.inputs.size(), 3u);
  EXPECT_EQ(tx_result.value().fee(), 20000u);
  EXPECT_EQ(tx_result.value().v6_part().ironwood.outputs.size(), 0u);
  EXPECT_EQ(tx_result.value().transparent_part().outputs.size(), 1u);
  EXPECT_EQ(tx_result.value().transparent_part().outputs[0].amount,
            3u * 70000u - 20000u);
}

TEST_F(ZCashCreateIronwoodToTransparentTransactionTaskTest, NotEnoughFunds) {
  ON_CALL(mock_orchard_sync_state(), GetSpendableNotes(_, _, _))
      .WillByDefault([&](OrchardPool /*pool*/,
                         const mojom::AccountIdPtr& account_id,
                         const OrchardAddrRawPart& internal_address) {
        OrchardSyncState::SpendableNotesBundle spendable_notes_bundle;
        {
          OrchardNote note;
          note.block_id = 1u;
          note.amount = 70000u;
          note.note_version = 2;
          spendable_notes_bundle.spendable_notes.push_back(std::move(note));
        }

        {
          OrchardNote note;
          note.block_id = 2u;
          note.amount = 80000u;
          note.note_version = 2;
          spendable_notes_bundle.spendable_notes.push_back(std::move(note));
        }

        return spendable_notes_bundle;
      });

  base::MockCallback<ZCashWalletService::CreateTransactionCallback> callback;

  std::unique_ptr<ZCashCreateIronwoodToTransparentTransactionTask> task =
      std::make_unique<ZCashCreateIronwoodToTransparentTransactionTask>(
          pass_key(), zcash_wallet_service(), action_context(),
          kTransparentAddress, 1000000u);

  base::expected<ZCashTransaction, std::string> tx_result;
  EXPECT_CALL(callback, Run(_))
      .WillOnce(::testing::DoAll(
          SaveArg<0>(&tx_result),
          base::test::RunOnceClosure(task_environment().QuitClosure())));

  task->Start(callback.Get());

  task_environment().RunUntilQuit();

  EXPECT_FALSE(tx_result.has_value());
}

TEST_F(ZCashCreateIronwoodToTransparentTransactionTaskTest, Error) {
  ON_CALL(mock_orchard_sync_state(), GetSpendableNotes(_, _, _))
      .WillByDefault([&](OrchardPool /*pool*/,
                         const mojom::AccountIdPtr& account_id,
                         const OrchardAddrRawPart& internal_addr) {
        return base::unexpected(OrchardStorage::Error{
            OrchardStorage::ErrorCode::kConsistencyError, ""});
      });

  base::MockCallback<ZCashWalletService::CreateTransactionCallback> callback;

  std::unique_ptr<ZCashCreateIronwoodToTransparentTransactionTask> task =
      std::make_unique<ZCashCreateIronwoodToTransparentTransactionTask>(
          pass_key(), zcash_wallet_service(), action_context(),
          kTransparentAddress, 100000u);

  base::expected<ZCashTransaction, std::string> tx_result;
  EXPECT_CALL(callback, Run(_))
      .WillOnce(::testing::DoAll(
          SaveArg<0>(&tx_result),
          base::test::RunOnceClosure(task_environment().QuitClosure())));

  task->Start(callback.Get());

  task_environment().RunUntilQuit();

  EXPECT_FALSE(tx_result.has_value());
}

}  // namespace brave_wallet
