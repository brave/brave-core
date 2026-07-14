/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/zcash/zcash_create_orchard_to_ironwood_transaction_task.h"

#include <memory>
#include <utility>

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

class MockZCashRPC : public ZCashRpc {
 public:
  MockZCashRPC() : ZCashRpc(nullptr, nullptr) {}
  ~MockZCashRPC() override = default;

  MOCK_METHOD2(GetLatestBlock,
               void(const std::string& chain_id,
                    GetLatestBlockCallback callback));
};

}  // namespace

class ZCashCreateOrchardToIronwoodTransactionTaskTest : public testing::Test {
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

    zcash_wallet_service_ = std::make_unique<TestingZCashWalletService>(
        *keyring_service_, std::make_unique<MockZCashRPC>());
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

  MockZCashRPC& mock_zcash_rpc() {
    return static_cast<MockZCashRPC&>(zcash_wallet_service_->zcash_rpc());
  }

  ZCashWalletService& zcash_wallet_service() { return *zcash_wallet_service_; }

  KeyringService& keyring_service() { return *keyring_service_; }

  mojom::AccountIdPtr& account_id() { return account_id_; }

  base::PassKey<class ZCashCreateOrchardToIronwoodTransactionTaskTest>
  pass_key() {
    return base::PassKey<
        class ZCashCreateOrchardToIronwoodTransactionTaskTest>();
  }

  ZCashActionContext action_context() {
    return zcash_wallet_service_->CreateActionContext(account_id());
  }

  ZCashActionContext action_context(const mojom::AccountIdPtr& account_id) {
    return zcash_wallet_service_->CreateActionContext(account_id);
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

TEST_F(ZCashCreateOrchardToIronwoodTransactionTaskTest, TransactionCreated) {
  ON_CALL(mock_orchard_sync_state(), GetSpendableNotes(_, _, _))
      .WillByDefault([&](OrchardPool pool,
                         const mojom::AccountIdPtr& account_id,
                         const OrchardAddrRawPart& addr) {
        EXPECT_EQ(pool, OrchardPool::kOrchard);
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

  ON_CALL(mock_zcash_rpc(), GetLatestBlock(_, _))
      .WillByDefault([](const std::string& chain_id,
                        ZCashRpc::GetLatestBlockCallback callback) {
        EXPECT_EQ(chain_id, mojom::kZCashMainnet);
        std::move(callback).Run(
            zcash::mojom::BlockID::New(1000u, std::vector<uint8_t>({})));
      });

  base::MockCallback<ZCashWalletService::CreateTransactionCallback> callback;

  auto orchard_part = GetOrchardRawBytes(
      "u19hwdcqxhkapje2p0744gq96parewuffyeg0kg3q3taq040zwqh2wxjwyxzs6l9dulzua"
      "p43ya7mq7q3mu2hjafzlwylvystjlc6n294emxww9xm8qn6tcldqkq4k9ccsqzmjeqk9yp"
      "kss572ut324nmxke666jm8lhkpt85gzq58d50rfnd7wufke8jjhc3lhswxrdr57ah42xck"
      "h2j",
      false);

  std::unique_ptr<ZCashCreateOrchardToIronwoodTransactionTask> task =
      std::make_unique<ZCashCreateOrchardToIronwoodTransactionTask>(
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

  // Spends come from the legacy Orchard pool.
  EXPECT_EQ(tx_result.value().v6_part().legacy_orchard.inputs.size(), 2u);
  EXPECT_EQ(tx_result.value().v6_part().legacy_orchard.outputs.size(), 0u);
  EXPECT_EQ(tx_result.value().v6_part().legacy_orchard.inputs[0].note.amount,
            70000u);
  EXPECT_EQ(tx_result.value().v6_part().legacy_orchard.inputs[1].note.amount,
            80000u);
  // Legacy Orchard anchor comes from the note checkpoint.
  EXPECT_EQ(
      tx_result.value().v6_part().legacy_orchard.anchor_block_height.value(),
      10u);

  // Outputs (change + recipient) go to the Ironwood pool.
  EXPECT_EQ(tx_result.value().v6_part().ironwood.inputs.size(), 0u);
  EXPECT_EQ(tx_result.value().v6_part().ironwood.outputs.size(), 2u);
  EXPECT_EQ(tx_result.value().v6_part().ironwood.outputs[0].value, 40000u);
  EXPECT_EQ(tx_result.value().v6_part().ironwood.outputs[1].value, 100000u);
  // Ironwood anchor comes from the chain tip, not the note checkpoint.
  EXPECT_EQ(tx_result.value().v6_part().ironwood.anchor_block_height.value(),
            1000u);

  auto change_addr = keyring_service().GetOrchardRawBytes(
      account_id(), mojom::ZCashKeyId::New(0, 1, 0));

  EXPECT_EQ(tx_result.value().v6_part().ironwood.outputs[0].addr,
            change_addr.value());
  EXPECT_EQ(tx_result.value().v6_part().ironwood.outputs[1].addr,
            orchard_part.value());
}

TEST_F(ZCashCreateOrchardToIronwoodTransactionTaskTest,
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

  ON_CALL(mock_zcash_rpc(), GetLatestBlock(_, _))
      .WillByDefault([](const std::string& chain_id,
                        ZCashRpc::GetLatestBlockCallback callback) {
        std::move(callback).Run(
            zcash::mojom::BlockID::New(1000u, std::vector<uint8_t>({})));
      });

  base::MockCallback<ZCashWalletService::CreateTransactionCallback> callback;

  auto orchard_part = GetOrchardRawBytes(
      "u19hwdcqxhkapje2p0744gq96parewuffyeg0kg3q3taq040zwqh2wxjwyxzs6l9dulzua"
      "p43ya7mq7q3mu2hjafzlwylvystjlc6n294emxww9xm8qn6tcldqkq4k9ccsqzmjeqk9yp"
      "kss572ut324nmxke666jm8lhkpt85gzq58d50rfnd7wufke8jjhc3lhswxrdr57ah42xck"
      "h2j",
      false);

  std::unique_ptr<ZCashCreateOrchardToIronwoodTransactionTask> task =
      std::make_unique<ZCashCreateOrchardToIronwoodTransactionTask>(
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

  EXPECT_EQ(tx_result.value().v6_part().legacy_orchard.inputs.size(), 2u);
  EXPECT_EQ(
      tx_result.value().v6_part().legacy_orchard.anchor_block_height.value(),
      10u);

  EXPECT_EQ(tx_result.value().v6_part().ironwood.outputs.size(), 1u);
  EXPECT_EQ(tx_result.value().v6_part().ironwood.outputs[0].value, 140000u);
  EXPECT_EQ(tx_result.value().v6_part().ironwood.anchor_block_height.value(),
            1000u);
  EXPECT_EQ(tx_result.value().v6_part().ironwood.outputs[0].addr,
            orchard_part.value());
}

TEST_F(ZCashCreateOrchardToIronwoodTransactionTaskTest, NotEnoughFunds) {
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

  ON_CALL(mock_zcash_rpc(), GetLatestBlock(_, _))
      .WillByDefault([](const std::string& chain_id,
                        ZCashRpc::GetLatestBlockCallback callback) {
        std::move(callback).Run(
            zcash::mojom::BlockID::New(1000u, std::vector<uint8_t>({})));
      });

  base::MockCallback<ZCashWalletService::CreateTransactionCallback> callback;

  auto orchard_part = GetOrchardRawBytes(
      "u19hwdcqxhkapje2p0744gq96parewuffyeg0kg3q3taq040zwqh2wxjwyxzs6l9dulzua"
      "p43ya7mq7q3mu2hjafzlwylvystjlc6n294emxww9xm8qn6tcldqkq4k9ccsqzmjeqk9yp"
      "kss572ut324nmxke666jm8lhkpt85gzq58d50rfnd7wufke8jjhc3lhswxrdr57ah42xck"
      "h2j",
      false);

  std::unique_ptr<ZCashCreateOrchardToIronwoodTransactionTask> task =
      std::make_unique<ZCashCreateOrchardToIronwoodTransactionTask>(
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

TEST_F(ZCashCreateOrchardToIronwoodTransactionTaskTest,
       GetSpendableNotesError) {
  ON_CALL(mock_orchard_sync_state(), GetSpendableNotes(_, _, _))
      .WillByDefault([&](OrchardPool pool,
                         const mojom::AccountIdPtr& account_id,
                         const OrchardAddrRawPart& internal_addr) {
        return base::unexpected(OrchardStorage::Error{
            OrchardStorage::ErrorCode::kConsistencyError, ""});
      });

  ON_CALL(mock_zcash_rpc(), GetLatestBlock(_, _))
      .WillByDefault([](const std::string& chain_id,
                        ZCashRpc::GetLatestBlockCallback callback) {
        std::move(callback).Run(
            zcash::mojom::BlockID::New(1000u, std::vector<uint8_t>({})));
      });

  base::MockCallback<ZCashWalletService::CreateTransactionCallback> callback;

  auto orchard_part = GetOrchardRawBytes(
      "u19hwdcqxhkapje2p0744gq96parewuffyeg0kg3q3taq040zwqh2wxjwyxzs6l9dulzua"
      "p43ya7mq7q3mu2hjafzlwylvystjlc6n294emxww9xm8qn6tcldqkq4k9ccsqzmjeqk9yp"
      "kss572ut324nmxke666jm8lhkpt85gzq58d50rfnd7wufke8jjhc3lhswxrdr57ah42xck"
      "h2j",
      false);

  std::unique_ptr<ZCashCreateOrchardToIronwoodTransactionTask> task =
      std::make_unique<ZCashCreateOrchardToIronwoodTransactionTask>(
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

TEST_F(ZCashCreateOrchardToIronwoodTransactionTaskTest, MissingAnchorBlockId) {
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
        // anchor_block_id intentionally left unset.

        return spendable_notes_bundle;
      });

  ON_CALL(mock_zcash_rpc(), GetLatestBlock(_, _))
      .WillByDefault([](const std::string& chain_id,
                        ZCashRpc::GetLatestBlockCallback callback) {
        std::move(callback).Run(
            zcash::mojom::BlockID::New(1000u, std::vector<uint8_t>({})));
      });

  base::MockCallback<ZCashWalletService::CreateTransactionCallback> callback;

  auto orchard_part = GetOrchardRawBytes(
      "u19hwdcqxhkapje2p0744gq96parewuffyeg0kg3q3taq040zwqh2wxjwyxzs6l9dulzua"
      "p43ya7mq7q3mu2hjafzlwylvystjlc6n294emxww9xm8qn6tcldqkq4k9ccsqzmjeqk9yp"
      "kss572ut324nmxke666jm8lhkpt85gzq58d50rfnd7wufke8jjhc3lhswxrdr57ah42xck"
      "h2j",
      false);

  std::unique_ptr<ZCashCreateOrchardToIronwoodTransactionTask> task =
      std::make_unique<ZCashCreateOrchardToIronwoodTransactionTask>(
          pass_key(), zcash_wallet_service(), action_context(), *orchard_part,
          std::nullopt, 100000u);

  base::expected<ZCashTransaction, std::string> tx_result;
  EXPECT_CALL(callback, Run(_))
      .WillOnce(::testing::DoAll(
          SaveArg<0>(&tx_result),
          base::test::RunOnceClosure(task_environment().QuitClosure())));

  task->Start(callback.Get());

  task_environment().RunUntilQuit();

  EXPECT_FALSE(tx_result.has_value());
}

TEST_F(ZCashCreateOrchardToIronwoodTransactionTaskTest, LatestBlockError) {
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
        spendable_notes_bundle.anchor_block_id = 10u;

        return spendable_notes_bundle;
      });

  ON_CALL(mock_zcash_rpc(), GetLatestBlock(_, _))
      .WillByDefault([](const std::string& chain_id,
                        ZCashRpc::GetLatestBlockCallback callback) {
        std::move(callback).Run(base::unexpected("network error"));
      });

  base::MockCallback<ZCashWalletService::CreateTransactionCallback> callback;

  auto orchard_part = GetOrchardRawBytes(
      "u19hwdcqxhkapje2p0744gq96parewuffyeg0kg3q3taq040zwqh2wxjwyxzs6l9dulzua"
      "p43ya7mq7q3mu2hjafzlwylvystjlc6n294emxww9xm8qn6tcldqkq4k9ccsqzmjeqk9yp"
      "kss572ut324nmxke666jm8lhkpt85gzq58d50rfnd7wufke8jjhc3lhswxrdr57ah42xck"
      "h2j",
      false);

  std::unique_ptr<ZCashCreateOrchardToIronwoodTransactionTask> task =
      std::make_unique<ZCashCreateOrchardToIronwoodTransactionTask>(
          pass_key(), zcash_wallet_service(), action_context(), *orchard_part,
          std::nullopt, 100000u);

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
