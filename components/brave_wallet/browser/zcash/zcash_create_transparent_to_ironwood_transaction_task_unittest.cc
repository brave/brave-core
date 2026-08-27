// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_wallet/browser/zcash/zcash_create_transparent_to_ironwood_transaction_task.h"

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
#include "brave/components/brave_wallet/browser/zcash/zcash_action_context.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_rpc.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_test_utils.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_transaction.h"
#include "brave/components/brave_wallet/common/features.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"

using testing::_;
using testing::SaveArg;

namespace brave_wallet {

namespace {

class MockZCashRPC : public ZCashRpc {
 public:
  MockZCashRPC() : ZCashRpc(nullptr, nullptr) {}
  ~MockZCashRPC() override = default;

  MOCK_METHOD2(GetLatestBlock,
               void(const std::string& chain_id,
                    GetLatestBlockCallback callback));
};

class MockZCashWalletService : public TestingZCashWalletService {
 public:
  using TestingZCashWalletService::TestingZCashWalletService;

  MOCK_METHOD2(GetUtxos,
               void(const mojom::AccountIdPtr& account_id, GetUtxosCallback));

  MOCK_METHOD3(DiscoverNextUnusedAddress,
               void(const mojom::AccountIdPtr& account_id,
                    bool change,
                    DiscoverNextUnusedAddressCallback callback));
};

}  // namespace

class ZCashCreateTransparentToIronwoodTransactionTaskTest
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
        *keyring_service_, std::make_unique<MockZCashRPC>());
    zcash_wallet_service_->SetupSyncState(
        OrchardSyncState::CreateSyncStateSequence(),
        OrchardSyncState::CreateSyncState(temp_dir_.GetPath()));

    account_id_ = AccountUtils(keyring_service_.get())
                      .EnsureAccount(mojom::KeyringId::kZCashMainnet, 0)
                      ->account_id.Clone();
  }

  MockZCashWalletService& zcash_wallet_service() {
    return *zcash_wallet_service_;
  }

  ZCashActionContext action_context() {
    return zcash_wallet_service_->CreateActionContext(account_id());
  }

  base::PassKey<class ZCashCreateTransparentToIronwoodTransactionTaskTest>
  pass_key() {
    return base::PassKey<
        class ZCashCreateTransparentToIronwoodTransactionTaskTest>();
  }

  MockZCashRPC& mock_zcash_rpc() {
    return static_cast<MockZCashRPC&>(zcash_wallet_service().zcash_rpc());
  }

  base::test::TaskEnvironment& task_environment() { return task_environment_; }

  KeyringService& keyring_service() { return *keyring_service_; }

  mojom::AccountIdPtr& account_id() { return account_id_; }

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

TEST_F(ZCashCreateTransparentToIronwoodTransactionTaskTest,
       TransactionCreated) {
  ON_CALL(zcash_wallet_service(), GetUtxos(_, _))
      .WillByDefault([&](const mojom::AccountIdPtr& account_id,
                         ZCashWalletService::GetUtxosCallback callback) {
        ZCashWalletService::UtxoMap utxo_map;
        utxo_map["60000"] = GetZCashUtxo(60000);
        utxo_map["70000"] = GetZCashUtxo(70000);
        utxo_map["80000"] = GetZCashUtxo(80000);
        std::move(callback).Run(std::move(utxo_map));
      });

  ON_CALL(zcash_wallet_service(), DiscoverNextUnusedAddress(_, _, _))
      .WillByDefault(
          [&](const mojom::AccountIdPtr& account_id, bool change,
              ZCashWalletService::DiscoverNextUnusedAddressCallback callback) {
            auto id = mojom::ZCashKeyId::New(account_id->account_index, 1, 0);
            auto addr = keyring_service().GetZCashAddress(account_id, *id);
            std::move(callback).Run(std::move(addr));
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

  auto task = std::make_unique<ZCashCreateTransparentToIronwoodTransactionTask>(
      pass_key(), zcash_wallet_service(), action_context(), *orchard_part,
      std::nullopt, 10000u);

  base::expected<ZCashTransaction, std::string> tx_result;
  EXPECT_CALL(callback, Run(_))
      .WillOnce(::testing::DoAll(
          SaveArg<0>(&tx_result),
          base::test::RunOnceClosure(task_environment().QuitClosure())));

  task->Start(callback.Get());

  task_environment().RunUntilQuit();

  ASSERT_TRUE(tx_result.has_value());
  EXPECT_TRUE(tx_result.value().is_v6());

  // Only the smallest UTXO (60000) is needed to cover amount + fee.
  EXPECT_EQ(tx_result.value().transparent_part().inputs.size(), 1u);
  EXPECT_EQ(tx_result.value().transparent_part().inputs[0].utxo_value, 60000u);

  // fee = CalculateZCashTxFee(1 transparent input, 0 orchard input,
  // kOrchard) = max(2, max(1,1) + max(2, max(0,1,2))) * 5000 = 15000.
  EXPECT_EQ(tx_result.value().fee(), 15000u);

  // change = 60000 - 10000 - 15000 = 35000.
  EXPECT_EQ(tx_result.value().transparent_part().outputs.size(), 1u);
  EXPECT_EQ(tx_result.value().transparent_part().outputs[0].amount, 35000u);

  EXPECT_EQ(tx_result.value().v6_part().ironwood.outputs.size(), 1u);
  EXPECT_EQ(tx_result.value().v6_part().ironwood.outputs[0].value, 10000u);
  EXPECT_EQ(tx_result.value().v6_part().ironwood.outputs[0].addr,
            orchard_part.value());
  EXPECT_EQ(tx_result.value().v6_part().ironwood.anchor_block_height.value(),
            1000u);
}

TEST_F(ZCashCreateTransparentToIronwoodTransactionTaskTest,
       TransactionCreated_MaxAmount) {
  ON_CALL(zcash_wallet_service(), GetUtxos(_, _))
      .WillByDefault([&](const mojom::AccountIdPtr& account_id,
                         ZCashWalletService::GetUtxosCallback callback) {
        ZCashWalletService::UtxoMap utxo_map;
        utxo_map["60000"] = GetZCashUtxo(60000);
        utxo_map["70000"] = GetZCashUtxo(70000);
        utxo_map["80000"] = GetZCashUtxo(80000);
        std::move(callback).Run(std::move(utxo_map));
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

  auto task = std::make_unique<ZCashCreateTransparentToIronwoodTransactionTask>(
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

  EXPECT_EQ(tx_result.value().transparent_part().inputs.size(), 3u);
  EXPECT_EQ(tx_result.value().transparent_part().outputs.size(), 0u);

  // fee = CalculateZCashTxFee(3 transparent inputs, 0 orchard input,
  // kOrchard) = max(2, max(3,1) + max(2, max(0,1,2))) * 5000 = 25000.
  EXPECT_EQ(tx_result.value().fee(), 25000u);

  EXPECT_EQ(tx_result.value().v6_part().ironwood.outputs.size(), 1u);
  EXPECT_EQ(tx_result.value().v6_part().ironwood.outputs[0].value,
            60000u + 70000u + 80000u - 25000u);
  EXPECT_EQ(tx_result.value().v6_part().ironwood.outputs[0].addr,
            orchard_part.value());
}

TEST_F(ZCashCreateTransparentToIronwoodTransactionTaskTest, NotEnoughFunds) {
  ON_CALL(zcash_wallet_service(), GetUtxos(_, _))
      .WillByDefault([&](const mojom::AccountIdPtr& account_id,
                         ZCashWalletService::GetUtxosCallback callback) {
        ZCashWalletService::UtxoMap utxo_map;
        utxo_map["60000"] = GetZCashUtxo(60000);
        std::move(callback).Run(std::move(utxo_map));
      });

  ON_CALL(mock_zcash_rpc(), GetLatestBlock(_, _))
      .WillByDefault([](const std::string& chain_id,
                        ZCashRpc::GetLatestBlockCallback callback) {
        std::move(callback).Run(
            zcash::mojom::BlockID::New(1000u, std::vector<uint8_t>({})));
      });

  ON_CALL(zcash_wallet_service(), DiscoverNextUnusedAddress(_, _, _))
      .WillByDefault(
          [&](const mojom::AccountIdPtr& account_id, bool change,
              ZCashWalletService::DiscoverNextUnusedAddressCallback callback) {
            auto id = mojom::ZCashKeyId::New(account_id->account_index, 1, 0);
            auto addr = keyring_service().GetZCashAddress(account_id, *id);
            std::move(callback).Run(std::move(addr));
          });

  base::MockCallback<ZCashWalletService::CreateTransactionCallback> callback;

  auto orchard_part = GetOrchardRawBytes(
      "u19hwdcqxhkapje2p0744gq96parewuffyeg0kg3q3taq040zwqh2wxjwyxzs6l9dulzua"
      "p43ya7mq7q3mu2hjafzlwylvystjlc6n294emxww9xm8qn6tcldqkq4k9ccsqzmjeqk9yp"
      "kss572ut324nmxke666jm8lhkpt85gzq58d50rfnd7wufke8jjhc3lhswxrdr57ah42xck"
      "h2j",
      false);

  auto task = std::make_unique<ZCashCreateTransparentToIronwoodTransactionTask>(
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

TEST_F(ZCashCreateTransparentToIronwoodTransactionTaskTest,
       TransactionNotCreated_LastBlockError) {
  ON_CALL(zcash_wallet_service(), GetUtxos(_, _))
      .WillByDefault([&](const mojom::AccountIdPtr& account_id,
                         ZCashWalletService::GetUtxosCallback callback) {
        ZCashWalletService::UtxoMap utxo_map;
        utxo_map["60000"] = GetZCashUtxo(60000);
        std::move(callback).Run(std::move(utxo_map));
      });

  ON_CALL(mock_zcash_rpc(), GetLatestBlock(_, _))
      .WillByDefault([](const std::string& chain_id,
                        ZCashRpc::GetLatestBlockCallback callback) {
        std::move(callback).Run(base::unexpected("error"));
      });

  base::MockCallback<ZCashWalletService::CreateTransactionCallback> callback;

  auto orchard_part = GetOrchardRawBytes(
      "u19hwdcqxhkapje2p0744gq96parewuffyeg0kg3q3taq040zwqh2wxjwyxzs6l9dulzua"
      "p43ya7mq7q3mu2hjafzlwylvystjlc6n294emxww9xm8qn6tcldqkq4k9ccsqzmjeqk9yp"
      "kss572ut324nmxke666jm8lhkpt85gzq58d50rfnd7wufke8jjhc3lhswxrdr57ah42xck"
      "h2j",
      false);

  auto task = std::make_unique<ZCashCreateTransparentToIronwoodTransactionTask>(
      pass_key(), zcash_wallet_service(), action_context(), *orchard_part,
      std::nullopt, 10000u);

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
