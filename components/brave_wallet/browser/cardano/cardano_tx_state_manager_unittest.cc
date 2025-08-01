/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/cardano/cardano_tx_state_manager.h"

#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include "base/files/scoped_temp_dir.h"
#include "base/test/task_environment.h"
#include "brave/components/brave_wallet/browser/brave_wallet_prefs.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_test_utils.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_transaction.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_transaction_serializer.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_tx_meta.h"
#include "brave/components/brave_wallet/browser/test_utils.h"
#include "brave/components/brave_wallet/browser/tx_storage_delegate_impl.h"
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
    factory_ = GetTestValueStoreFactory(temp_dir_);
    delegate_ = GetTxStorageDelegateForTest(GetPrefs(), factory_);
    account_resolver_delegate_ =
        std::make_unique<AccountResolverDelegateForTest>();
    cardano_tx_state_manager_ = std::make_unique<CardanoTxStateManager>(
        *delegate_, *account_resolver_delegate_);
  }

  PrefService* GetPrefs() { return &prefs_; }

  base::test::TaskEnvironment task_environment_;
  base::ScopedTempDir temp_dir_;
  scoped_refptr<value_store::TestValueStoreFactory> factory_;
  std::unique_ptr<TxStorageDelegateImpl> delegate_;
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
  tx->set_amount(200000);
  tx->set_to(*CardanoAddress::FromString(kMockCardanoAddress2));

  CardanoTransaction::TxInput input;
  input.utxo_address = *CardanoAddress::FromString(kMockCardanoAddress1);
  input.utxo_value = 200000;
  tx->AddInput(std::move(input));

  CardanoTransaction::TxOutput output;
  output.address = *CardanoAddress::FromString(kMockCardanoAddress2);
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

  base::Value::Dict meta_value = meta.ToValue();
  auto meta_from_value =
      cardano_tx_state_manager_->ValueToCardanoTxMeta(meta_value);
  ASSERT_TRUE(meta_from_value);
  EXPECT_EQ(*meta_from_value, meta);
}

}  // namespace brave_wallet
