/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/zcash/zcash_create_orchard_to_transparent_transaction_task.h"

#include <memory>
#include <utility>

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

  MOCK_METHOD2(
      GetSpendableNotes,
      base::expected<std::optional<OrchardSyncState::SpendableNotesBundle>,
                     OrchardStorage::Error>(
          const mojom::AccountIdPtr& account_id,
          const OrchardAddrRawPart& internal_addr));
};

}  // namespace

class ZCashCreateOrchardToTransparentTransactionTaskTest
    : public testing::Test {
 public:
  void SetUp() override {
    feature_list_.InitWithFeaturesAndParameters(
        {{features::kBraveWalletZCashFeature,
          {{"zcash_shielded_transactions_enabled", "true"}}},
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

  base::PassKey<class ZCashCreateOrchardToTransparentTransactionTaskTest>
  pass_key() {
    return base::PassKey<
        class ZCashCreateOrchardToTransparentTransactionTaskTest>();
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

TEST_F(ZCashCreateOrchardToTransparentTransactionTaskTest, TransactionCreated) {
  ON_CALL(mock_orchard_sync_state(), GetSpendableNotes(_, _))
      .WillByDefault([&](const mojom::AccountIdPtr& account_id,
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

  std::unique_ptr<ZCashCreateOrchardToTransparentTransactionTask> task =
      std::make_unique<ZCashCreateOrchardToTransparentTransactionTask>(
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

  EXPECT_EQ(tx_result.value().orchard_part().inputs.size(), 2u);
  EXPECT_EQ(tx_result.value().orchard_part().outputs.size(), 1u);
  EXPECT_EQ(tx_result.value().transparent_part().outputs.size(), 1u);
  EXPECT_EQ(tx_result.value().orchard_part().anchor_block_height.value(), 10u);

  EXPECT_EQ(tx_result.value().orchard_part().inputs[0].note.amount, 70000u);
  EXPECT_EQ(tx_result.value().orchard_part().inputs[1].note.amount, 80000u);

  EXPECT_EQ(tx_result.value().transparent_part().outputs[0].amount, 100000u);
  EXPECT_EQ(tx_result.value().transparent_part().outputs[0].address,
            kTransparentAddress);

  // Should have change output
  EXPECT_EQ(tx_result.value().orchard_part().outputs.size(), 1u);
  EXPECT_EQ(tx_result.value().orchard_part().outputs[0].value,
            50000u - 3 * 5000u);
}

TEST_F(ZCashCreateOrchardToTransparentTransactionTaskTest,
       TransactionCreated_NoAnchorBlockId) {
  ON_CALL(mock_orchard_sync_state(), GetSpendableNotes(_, _))
      .WillByDefault([&](const mojom::AccountIdPtr& account_id,
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

        return spendable_notes_bundle;
      });

  base::MockCallback<ZCashWalletService::CreateTransactionCallback> callback;

  std::unique_ptr<ZCashCreateOrchardToTransparentTransactionTask> task =
      std::make_unique<ZCashCreateOrchardToTransparentTransactionTask>(
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

TEST_F(ZCashCreateOrchardToTransparentTransactionTaskTest,
       TransactionCreated_MaxAmount) {
  ON_CALL(mock_orchard_sync_state(), GetSpendableNotes(_, _))
      .WillByDefault([&](const mojom::AccountIdPtr& account_id,
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

  std::unique_ptr<ZCashCreateOrchardToTransparentTransactionTask> task =
      std::make_unique<ZCashCreateOrchardToTransparentTransactionTask>(
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

  EXPECT_EQ(tx_result.value().orchard_part().inputs.size(), 2u);
  EXPECT_EQ(tx_result.value().orchard_part().outputs.size(), 0u);
  EXPECT_EQ(tx_result.value().transparent_part().outputs.size(), 1u);
  EXPECT_EQ(tx_result.value().orchard_part().anchor_block_height.value(), 10u);

  EXPECT_EQ(tx_result.value().orchard_part().inputs[0].note.amount, 70000u);
  EXPECT_EQ(tx_result.value().orchard_part().inputs[1].note.amount, 80000u);

  EXPECT_EQ(tx_result.value().transparent_part().outputs[0].amount,
            70000u + 80000u - 3 * 5000u);
  EXPECT_EQ(tx_result.value().transparent_part().outputs[0].address,
            kTransparentAddress);
}

TEST_F(ZCashCreateOrchardToTransparentTransactionTaskTest,
       TransactionCreated_AllAmount) {
  ON_CALL(mock_orchard_sync_state(), GetSpendableNotes(_, _))
      .WillByDefault([&](const mojom::AccountIdPtr& account_id,
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

  std::unique_ptr<ZCashCreateOrchardToTransparentTransactionTask> task =
      std::make_unique<ZCashCreateOrchardToTransparentTransactionTask>(
          pass_key(), zcash_wallet_service(), action_context(),
          kTransparentAddress, 70000u + 80000u - 3 * 5000u);

  base::expected<ZCashTransaction, std::string> tx_result;
  EXPECT_CALL(callback, Run(_))
      .WillOnce(::testing::DoAll(
          SaveArg<0>(&tx_result),
          base::test::RunOnceClosure(task_environment().QuitClosure())));

  task->Start(callback.Get());

  task_environment().RunUntilQuit();

  EXPECT_TRUE(tx_result.has_value());

  EXPECT_EQ(tx_result.value().orchard_part().inputs.size(), 2u);
  EXPECT_EQ(tx_result.value().orchard_part().outputs.size(), 0u);
  EXPECT_EQ(tx_result.value().transparent_part().outputs.size(), 1u);
  EXPECT_EQ(tx_result.value().orchard_part().anchor_block_height.value(), 10u);

  EXPECT_EQ(tx_result.value().orchard_part().inputs[0].note.amount, 70000u);
  EXPECT_EQ(tx_result.value().orchard_part().inputs[1].note.amount, 80000u);

  EXPECT_EQ(tx_result.value().transparent_part().outputs[0].amount,
            70000u + 80000u - 3 * 5000u);
  EXPECT_EQ(tx_result.value().transparent_part().outputs[0].address,
            kTransparentAddress);
}

TEST_F(ZCashCreateOrchardToTransparentTransactionTaskTest,
       TransactionCreated_MaxAmount_OverflowCheck) {
  ON_CALL(mock_orchard_sync_state(), GetSpendableNotes(_, _))
      .WillByDefault([&](const mojom::AccountIdPtr& account_id,
                         const OrchardAddrRawPart& addr) {
        OrchardSyncState::SpendableNotesBundle spendable_notes_bundle;
        {
          OrchardNote note;
          note.block_id = 1u;
          note.amount = 70000000000u;
          spendable_notes_bundle.spendable_notes.push_back(std::move(note));
        }

        {
          OrchardNote note;
          note.block_id = 2u;
          note.amount = 80000000000u;
          spendable_notes_bundle.spendable_notes.push_back(std::move(note));
        }
        spendable_notes_bundle.anchor_block_id = 10u;

        return spendable_notes_bundle;
      });

  base::MockCallback<ZCashWalletService::CreateTransactionCallback> callback;

  std::unique_ptr<ZCashCreateOrchardToTransparentTransactionTask> task =
      std::make_unique<ZCashCreateOrchardToTransparentTransactionTask>(
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

  EXPECT_EQ(tx_result.value().orchard_part().inputs.size(), 2u);
  EXPECT_EQ(tx_result.value().orchard_part().outputs.size(), 0u);
  EXPECT_EQ(tx_result.value().transparent_part().outputs.size(), 1u);
  EXPECT_EQ(tx_result.value().orchard_part().anchor_block_height.value(), 10u);

  EXPECT_EQ(tx_result.value().orchard_part().inputs[0].note.amount,
            70000000000u);
  EXPECT_EQ(tx_result.value().orchard_part().inputs[1].note.amount,
            80000000000u);

  EXPECT_EQ(tx_result.value().transparent_part().outputs[0].amount,
            70000000000u + 80000000000u - 3 * 5000u);
  EXPECT_EQ(tx_result.value().transparent_part().outputs[0].address,
            kTransparentAddress);
}

TEST_F(ZCashCreateOrchardToTransparentTransactionTaskTest,
       TransactionCreated_OverflowCheck_FullAmount) {
  ON_CALL(mock_orchard_sync_state(), GetSpendableNotes(_, _))
      .WillByDefault([&](const mojom::AccountIdPtr& account_id,
                         const OrchardAddrRawPart& addr) {
        OrchardSyncState::SpendableNotesBundle spendable_notes_bundle;
        {
          OrchardNote note;
          note.block_id = 1u;
          note.amount = 0xFFFFFFFFFFFFFFFFu;
          spendable_notes_bundle.spendable_notes.push_back(std::move(note));
        }

        {
          OrchardNote note;
          note.block_id = 2u;
          note.amount = 0x2222222222222222u;
          spendable_notes_bundle.spendable_notes.push_back(std::move(note));
        }
        spendable_notes_bundle.anchor_block_id = 10u;

        return spendable_notes_bundle;
      });

  base::MockCallback<ZCashWalletService::CreateTransactionCallback> callback;

  std::unique_ptr<ZCashCreateOrchardToTransparentTransactionTask> task =
      std::make_unique<ZCashCreateOrchardToTransparentTransactionTask>(
          pass_key(), zcash_wallet_service(), action_context(),
          kTransparentAddress, kZCashFullAmount);

  base::expected<ZCashTransaction, std::string> tx_result;
  EXPECT_CALL(callback, Run(_))
      .WillOnce(::testing::DoAll(
          SaveArg<0>(&tx_result),
          base::test::RunOnceClosure(task_environment().QuitClosure())));

  task->Start(callback.Get());

  task_environment().RunUntilQuit();

  EXPECT_FALSE(tx_result.has_value());
}

TEST_F(ZCashCreateOrchardToTransparentTransactionTaskTest,
       TransactionCreated_OverflowCheck_CustomAmount) {
  ON_CALL(mock_orchard_sync_state(), GetSpendableNotes(_, _))
      .WillByDefault([&](const mojom::AccountIdPtr& account_id,
                         const OrchardAddrRawPart& addr) {
        OrchardSyncState::SpendableNotesBundle spendable_notes_bundle;
        {
          OrchardNote note;
          note.block_id = 1u;
          note.amount = 0xFFFFFFFFFFFFFFFFu;
          spendable_notes_bundle.spendable_notes.push_back(std::move(note));
        }

        {
          OrchardNote note;
          note.block_id = 2u;
          note.amount = 0x2222222222222222u;
          spendable_notes_bundle.spendable_notes.push_back(std::move(note));
        }
        spendable_notes_bundle.anchor_block_id = 10u;

        return spendable_notes_bundle;
      });

  base::MockCallback<ZCashWalletService::CreateTransactionCallback> callback;

  std::unique_ptr<ZCashCreateOrchardToTransparentTransactionTask> task =
      std::make_unique<ZCashCreateOrchardToTransparentTransactionTask>(
          pass_key(), zcash_wallet_service(), action_context(),
          kTransparentAddress, 0x2222222222222222u);

  base::expected<ZCashTransaction, std::string> tx_result;
  EXPECT_CALL(callback, Run(_))
      .WillOnce(::testing::DoAll(
          SaveArg<0>(&tx_result),
          base::test::RunOnceClosure(task_environment().QuitClosure())));

  task->Start(callback.Get());

  task_environment().RunUntilQuit();

  EXPECT_FALSE(tx_result.has_value());
}

TEST_F(ZCashCreateOrchardToTransparentTransactionTaskTest, NotEnoughFunds) {
  ON_CALL(mock_orchard_sync_state(), GetSpendableNotes(_, _))
      .WillByDefault([&](const mojom::AccountIdPtr& account_id,
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

        return spendable_notes_bundle;
      });

  base::MockCallback<ZCashWalletService::CreateTransactionCallback> callback;

  std::unique_ptr<ZCashCreateOrchardToTransparentTransactionTask> task =
      std::make_unique<ZCashCreateOrchardToTransparentTransactionTask>(
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

TEST_F(ZCashCreateOrchardToTransparentTransactionTaskTest, Error) {
  ON_CALL(mock_orchard_sync_state(), GetSpendableNotes(_, _))
      .WillByDefault([&](const mojom::AccountIdPtr& account_id,
                         const OrchardAddrRawPart& internal_addr) {
        return base::unexpected(OrchardStorage::Error{
            OrchardStorage::ErrorCode::kConsistencyError, ""});
      });

  base::MockCallback<ZCashWalletService::CreateTransactionCallback> callback;

  std::unique_ptr<ZCashCreateOrchardToTransparentTransactionTask> task =
      std::make_unique<ZCashCreateOrchardToTransparentTransactionTask>(
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
