/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/cardano/cardano_tx_state_manager.h"

#include <memory>
#include <optional>
#include <utility>

#include "base/files/scoped_temp_dir.h"
#include "base/test/task_environment.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_test_utils.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_transaction.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_tx_meta.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/browser/test_utils.h"
#include "brave/components/brave_wallet/browser/tx_storage.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

class CardanoTxStateManagerUnitTest : public testing::Test {
 public:
  CardanoTxStateManagerUnitTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}
  ~CardanoTxStateManagerUnitTest() override = default;

 protected:
  void SetUp() override {
    RegisterProfilePrefs(prefs_.registry());
    RegisterProfilePrefsForMigration(prefs_.registry());
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    tx_storage_ = CreateTxStorageForTest(temp_dir_.GetPath());
    account_resolver_delegate_ =
        std::make_unique<AccountResolverDelegateForTest>();
    cardano_tx_state_manager_ = std::make_unique<CardanoTxStateManager>(
        *tx_storage_, *account_resolver_delegate_);
  }

  PrefService* GetPrefs() { return &prefs_; }

  base::test::TaskEnvironment task_environment_;
  base::ScopedTempDir temp_dir_;
  std::unique_ptr<TxStorage> tx_storage_;
  std::unique_ptr<AccountResolverDelegateForTest> account_resolver_delegate_;
  sync_preferences::TestingPrefServiceSyncable prefs_;
  std::unique_ptr<CardanoTxStateManager> cardano_tx_state_manager_;
};

TEST_F(CardanoTxStateManagerUnitTest, CardanoTxMetaAndValue) {
  auto cardano_account_id =
      account_resolver_delegate_->RegisterAccount(MakeIndexBasedAccountId(
          mojom::CoinType::ADA, mojom::KeyringId::kCardanoMainnet,
          mojom::AccountKind::kDerived, 1));

  std::unique_ptr<CardanoTransaction> tx =
      std::make_unique<CardanoTransaction>();
  CardanoTransaction::TxInput input(
      *CardanoAddress::FromString(kMockCardanoAddress1));
  input.utxo_value = 200000;
  tx->AddInput(std::move(input));

  CardanoTransaction::TxOutput output(
      *CardanoAddress::FromString(kMockCardanoAddress2));
  output.amount = 200000 - 1000;
  tx->AddOutput(std::move(output));

  CardanoTxMeta meta(cardano_account_id, std::move(tx));
  meta.set_id(TxMeta::GenerateMetaID());
  meta.set_status(mojom::TransactionStatus::Submitted);
  meta.set_created_time(base::Time::Now());
  meta.set_submitted_time(base::Time::Now());
  meta.set_confirmed_time(base::Time::Now());
  meta.set_tx_hash(
      "aa388f50b725767653e150ad8990ec11a2146d75acafbe492af08213849fe2c5");
  meta.set_origin(url::Origin::Create(GURL("https://test.brave.com/")));
  meta.set_chain_id(mojom::kCardanoTestnet);

  base::DictValue meta_value = meta.ToValue();
  auto meta_from_value =
      cardano_tx_state_manager_->ValueToCardanoTxMeta(meta_value);
  ASSERT_TRUE(meta_from_value);
  EXPECT_EQ(*meta_from_value, meta);
}

}  // namespace brave_wallet
