/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/zcash/zcash_create_ironwood_to_ironwood_transaction_task.h"

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
#include "brave/components/brave_wallet/browser/internal/orchard_sync_state.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/browser/test_utils.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_test_utils.h"
#include "brave/components/brave_wallet/common/features.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"

using testing::_;
using testing::SaveArg;

namespace brave_wallet {

namespace {

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

class ZCashCreateIronwoodToIronwoodTransactionTaskTest : public testing::Test {
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

    zcash_wallet_service_ = std::make_unique<TestingZCashWalletService>(
        *keyring_service_, std::make_unique<ZCashRpc>(nullptr, nullptr));
    zcash_wallet_service_->SetupSyncState(
        OrchardSyncState::CreateSyncStateSequence(),
        std::make_unique<MockOrchardSyncState>(temp_dir_.GetPath()));

    account_id_ = AccountUtils(keyring_service_.get())
                      .EnsureAccount(mojom::KeyringId::kZCashMainnet, 0)
                      ->account_id.Clone();
  }

  MockOrchardSyncState& mock_orchard_sync_state() {
    return static_cast<MockOrchardSyncState&>(
        *zcash_wallet_service_->sync_state_ptr);
  }

  ZCashWalletService& zcash_wallet_service() { return *zcash_wallet_service_; }

  KeyringService& keyring_service() { return *keyring_service_; }

  mojom::AccountIdPtr& account_id() { return account_id_; }

  base::PassKey<class ZCashCreateIronwoodToIronwoodTransactionTaskTest>
  pass_key() {
    return base::PassKey<
        class ZCashCreateIronwoodToIronwoodTransactionTaskTest>();
  }

  ZCashActionContext action_context() {
    return zcash_wallet_service_->CreateActionContext(account_id());
  }

  base::test::TaskEnvironment& task_environment() { return task_environment_; }

 private:
  base::test::TaskEnvironment task_environment_;

  base::test::ScopedFeatureList feature_list_;

  base::ScopedTempDir temp_dir_;

  sync_preferences::TestingPrefServiceSyncable prefs_;
  sync_preferences::TestingPrefServiceSyncable local_state_;
  mojom::AccountIdPtr account_id_;

  std::unique_ptr<KeyringService> keyring_service_;
  std::unique_ptr<TestingZCashWalletService> zcash_wallet_service_;
};

TEST_F(ZCashCreateIronwoodToIronwoodTransactionTaskTest, TransactionCreated) {
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
          spendable_notes_bundle.spendable_notes.push_back(std::move(note));
        }

        {
          OrchardNote note;
          note.block_id = 2u;
          note.amount = 80000u;
          spendable_notes_bundle.spendable_notes.push_back(std::move(note));
        }
        spendable_notes_bundle.anchor_block_id = 10u;

        return spendable_notes_bundle;
      });

  base::MockCallback<ZCashWalletService::CreateTransactionCallback> callback;

  auto orchard_part = GetOrchardRawBytes(
      "u19hwdcqxhkapje2p0744gq96parewuffyeg0kg3q3taq040zwqh2wxjwyxzs6l9dulzua"
      "p43ya7mq7q3mu2hjafzlwylvystjlc6n294emxww9xm8qn6tcldqkq4k9ccsqzmjeqk9yp"
      "kss572ut324nmxke666jm8lhkpt85gzq58d50rfnd7wufke8jjhc3lhswxrdr57ah42xck"
      "h2j",
      false);

  std::unique_ptr<ZCashCreateIronwoodToIronwoodTransactionTask> task =
      std::make_unique<ZCashCreateIronwoodToIronwoodTransactionTask>(
          pass_key(), zcash_wallet_service(), action_context(), *orchard_part,
          std::nullopt, 100000u);

  base::expected<ZCashTransaction, std::string> tx_result;
  EXPECT_CALL(callback, Run(_))
      .WillOnce(::testing::DoAll(
          SaveArg<0>(&tx_result),
          base::test::RunOnceClosure(task_environment().QuitClosure())));

  task->Start(callback.Get());

  task_environment().RunUntilQuit();

  ASSERT_TRUE(tx_result.has_value());
  EXPECT_TRUE(tx_result.value().is_v6());

  EXPECT_EQ(tx_result.value().v6_part().ironwood.inputs.size(), 2u);
  EXPECT_EQ(tx_result.value().v6_part().ironwood.inputs[0].note.amount, 70000u);
  EXPECT_EQ(tx_result.value().v6_part().ironwood.inputs[1].note.amount, 80000u);
  EXPECT_EQ(tx_result.value().v6_part().ironwood.anchor_block_height.value(),
            10u);

  EXPECT_EQ(tx_result.value().v6_part().legacy_orchard.inputs.size(), 0u);
  EXPECT_EQ(tx_result.value().v6_part().legacy_orchard.outputs.size(), 0u);

  EXPECT_EQ(tx_result.value().v6_part().ironwood.outputs.size(), 2u);
  EXPECT_EQ(tx_result.value().v6_part().ironwood.outputs[0].value, 40000u);
  EXPECT_EQ(tx_result.value().v6_part().ironwood.outputs[1].value, 100000u);

  auto change_addr = keyring_service().GetOrchardRawBytes(
      account_id(), mojom::ZCashKeyId::New(0, 1, 0));

  EXPECT_EQ(tx_result.value().v6_part().ironwood.outputs[0].addr,
            change_addr.value());
  EXPECT_EQ(tx_result.value().v6_part().ironwood.outputs[1].addr,
            orchard_part.value());
}

TEST_F(ZCashCreateIronwoodToIronwoodTransactionTaskTest,
       TransactionCreated_MaxAmount) {
  ON_CALL(mock_orchard_sync_state(), GetSpendableNotes(_, _, _))
      .WillByDefault([&](OrchardPool pool,
                         const mojom::AccountIdPtr& account_id,
                         const OrchardAddrRawPart& addr) {
        OrchardSyncState::SpendableNotesBundle spendable_notes_bundle;
        {
          OrchardNote note;
          note.block_id = 1u;
          note.amount = 70000u;
          spendable_notes_bundle.spendable_notes.push_back(std::move(note));
        }

        {
          OrchardNote note;
          note.block_id = 2u;
          note.amount = 80000u;
          spendable_notes_bundle.spendable_notes.push_back(std::move(note));
        }
        spendable_notes_bundle.anchor_block_id = 10u;

        return spendable_notes_bundle;
      });

  base::MockCallback<ZCashWalletService::CreateTransactionCallback> callback;

  auto orchard_part = GetOrchardRawBytes(
      "u19hwdcqxhkapje2p0744gq96parewuffyeg0kg3q3taq040zwqh2wxjwyxzs6l9dulzua"
      "p43ya7mq7q3mu2hjafzlwylvystjlc6n294emxww9xm8qn6tcldqkq4k9ccsqzmjeqk9yp"
      "kss572ut324nmxke666jm8lhkpt85gzq58d50rfnd7wufke8jjhc3lhswxrdr57ah42xck"
      "h2j",
      false);

  std::unique_ptr<ZCashCreateIronwoodToIronwoodTransactionTask> task =
      std::make_unique<ZCashCreateIronwoodToIronwoodTransactionTask>(
          pass_key(), zcash_wallet_service(), action_context(), *orchard_part,
          std::nullopt, kZCashFullAmount);

  base::expected<ZCashTransaction, std::string> tx_result;
  EXPECT_CALL(callback, Run(_))
      .WillOnce(::testing::DoAll(
          SaveArg<0>(&tx_result),
          base::test::RunOnceClosure(task_environment().QuitClosure())));

  task->Start(callback.Get());

  task_environment().RunUntilQuit();

  ASSERT_TRUE(tx_result.has_value());

  EXPECT_EQ(tx_result.value().v6_part().ironwood.inputs.size(), 2u);
  EXPECT_EQ(tx_result.value().v6_part().ironwood.anchor_block_height.value(),
            10u);

  EXPECT_EQ(tx_result.value().v6_part().ironwood.outputs.size(), 1u);
  EXPECT_EQ(tx_result.value().v6_part().ironwood.outputs[0].value, 140000u);
  EXPECT_EQ(tx_result.value().v6_part().ironwood.outputs[0].addr,
            orchard_part.value());
}

// Fee correctness for Ironwood to Ironwood transfers. Spends, change and the
// target output all live in the one Ironwood bundle, so the fee is
// 5000 * max(2, max(spends, change + target)) - see
// https://github.com/brave/brave-browser/issues/58957. Billing the spends to
// the legacy Orchard bundle instead raises a second, spendless Ironwood bundle
// and adds a flat 10000 to every case below.
TEST_F(ZCashCreateIronwoodToIronwoodTransactionTaskTest, FeeCorrectness) {
  // Notes are picked ascending until they cover amount + fee, so the note set
  // and amount together control the spend count.
  struct {
    const char* label;
    std::vector<uint64_t> note_amounts;
    uint64_t amount;
    size_t expected_inputs;
    uint64_t expected_fee;
  } const kCases[] = {
      // 1 spend, 1 change, 1 target -> max(2, max(1, 2)) = 2 actions.
      {"single note", {20000u}, 5000u, 1u, 10000u},
      // 2 spends, 1 change, 1 target -> max(2, max(2, 2)) = 2 actions.
      {"two notes", {20000u, 30000u}, 35000u, 2u, 10000u},
      // 3 spends dominate the 2 outputs -> max(2, max(3, 2)) = 3 actions.
      {"three notes", {20000u, 30000u, 40000u}, 70000u, 3u, 15000u},
      // 5 spends -> max(2, max(5, 2)) = 5 actions.
      {"five notes",
       {20000u, 30000u, 40000u, 50000u, 60000u},
       170000u,
       5u,
       25000u},
  };

  auto receiver = GetOrchardRawBytes(
      "u19hwdcqxhkapje2p0744gq96parewuffyeg0kg3q3taq040zwqh2wxjwyxzs6l9dulzua"
      "p43ya7mq7q3mu2hjafzlwylvystjlc6n294emxww9xm8qn6tcldqkq4k9ccsqzmjeqk9yp"
      "kss572ut324nmxke666jm8lhkpt85gzq58d50rfnd7wufke8jjhc3lhswxrdr57ah42xck"
      "h2j",
      false);
  ASSERT_TRUE(receiver);

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
            spendable_notes_bundle.spendable_notes.push_back(std::move(note));
          }
          spendable_notes_bundle.anchor_block_id = 10u;
          return spendable_notes_bundle;
        });

    auto task = std::make_unique<ZCashCreateIronwoodToIronwoodTransactionTask>(
        pass_key(), zcash_wallet_service(), action_context(), *receiver,
        std::nullopt, test_case.amount);

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

    // Nothing may be billed to the legacy Orchard bundle.
    EXPECT_EQ(tx_result.value().v6_part().legacy_orchard.inputs.size(), 0u);
    EXPECT_EQ(tx_result.value().v6_part().legacy_orchard.outputs.size(), 0u);

    // Value is conserved: spent notes = outputs + fee.
    uint64_t total_outputs = 0u;
    for (const auto& output : tx_result.value().v6_part().ironwood.outputs) {
      total_outputs += output.value;
    }
    EXPECT_EQ(tx_result.value().TotalInputsAmount().ValueOrDie(),
              total_outputs + tx_result.value().fee());
  }
}

// Full amount sends have no change output, so the fee covers the spends plus
// the single target output only.
TEST_F(ZCashCreateIronwoodToIronwoodTransactionTaskTest,
       FeeCorrectness_MaxAmount) {
  ON_CALL(mock_orchard_sync_state(), GetSpendableNotes(_, _, _))
      .WillByDefault([&](OrchardPool pool,
                         const mojom::AccountIdPtr& account_id,
                         const OrchardAddrRawPart& addr) {
        OrchardSyncState::SpendableNotesBundle spendable_notes_bundle;
        {
          OrchardNote note;
          note.block_id = 1u;
          note.amount = 70000u;
          spendable_notes_bundle.spendable_notes.push_back(std::move(note));
        }
        {
          OrchardNote note;
          note.block_id = 2u;
          note.amount = 80000u;
          spendable_notes_bundle.spendable_notes.push_back(std::move(note));
        }
        spendable_notes_bundle.anchor_block_id = 10u;
        return spendable_notes_bundle;
      });

  auto receiver = GetOrchardRawBytes(
      "u19hwdcqxhkapje2p0744gq96parewuffyeg0kg3q3taq040zwqh2wxjwyxzs6l9dulzua"
      "p43ya7mq7q3mu2hjafzlwylvystjlc6n294emxww9xm8qn6tcldqkq4k9ccsqzmjeqk9yp"
      "kss572ut324nmxke666jm8lhkpt85gzq58d50rfnd7wufke8jjhc3lhswxrdr57ah42xck"
      "h2j",
      false);
  ASSERT_TRUE(receiver);

  auto task = std::make_unique<ZCashCreateIronwoodToIronwoodTransactionTask>(
      pass_key(), zcash_wallet_service(), action_context(), *receiver,
      std::nullopt, kZCashFullAmount);

  base::MockCallback<ZCashWalletService::CreateTransactionCallback> callback;
  base::expected<ZCashTransaction, std::string> tx_result;
  EXPECT_CALL(callback, Run(_))
      .WillOnce(::testing::DoAll(
          SaveArg<0>(&tx_result),
          base::test::RunOnceClosure(task_environment().QuitClosure())));

  task->Start(callback.Get());
  task_environment().RunUntilQuit();

  ASSERT_TRUE(tx_result.has_value());

  // 2 spends, no change, 1 target -> max(2, max(2, 1)) = 2 actions.
  EXPECT_EQ(tx_result.value().fee(), 10000u);
  EXPECT_EQ(tx_result.value().v6_part().ironwood.outputs.size(), 1u);
  EXPECT_EQ(tx_result.value().v6_part().ironwood.outputs[0].value,
            70000u + 80000u - 10000u);
  EXPECT_EQ(tx_result.value().v6_part().legacy_orchard.outputs.size(), 0u);
}

TEST_F(ZCashCreateIronwoodToIronwoodTransactionTaskTest, NotEnoughFunds) {
  ON_CALL(mock_orchard_sync_state(), GetSpendableNotes(_, _, _))
      .WillByDefault([&](OrchardPool pool,
                         const mojom::AccountIdPtr& account_id,
                         const OrchardAddrRawPart& internal_address) {
        OrchardSyncState::SpendableNotesBundle spendable_notes_bundle;
        {
          OrchardNote note;
          note.block_id = 1u;
          note.amount = 70000u;
          spendable_notes_bundle.spendable_notes.push_back(std::move(note));
        }

        {
          OrchardNote note;
          note.block_id = 2u;
          note.amount = 80000u;
          spendable_notes_bundle.spendable_notes.push_back(std::move(note));
        }
        spendable_notes_bundle.anchor_block_id = 10u;

        return spendable_notes_bundle;
      });

  base::MockCallback<ZCashWalletService::CreateTransactionCallback> callback;

  auto orchard_part = GetOrchardRawBytes(
      "u19hwdcqxhkapje2p0744gq96parewuffyeg0kg3q3taq040zwqh2wxjwyxzs6l9dulzua"
      "p43ya7mq7q3mu2hjafzlwylvystjlc6n294emxww9xm8qn6tcldqkq4k9ccsqzmjeqk9yp"
      "kss572ut324nmxke666jm8lhkpt85gzq58d50rfnd7wufke8jjhc3lhswxrdr57ah42xck"
      "h2j",
      false);

  std::unique_ptr<ZCashCreateIronwoodToIronwoodTransactionTask> task =
      std::make_unique<ZCashCreateIronwoodToIronwoodTransactionTask>(
          pass_key(), zcash_wallet_service(), action_context(), *orchard_part,
          std::nullopt, 1000000u);

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
