/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/eth_tx_state_manager.h"

#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include "base/files/scoped_temp_dir.h"
#include "base/test/task_environment.h"
#include "base/time/time.h"
#include "base/values.h"
#include "brave/components/brave_wallet/browser/brave_wallet_prefs.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/eip1559_transaction.h"
#include "brave/components/brave_wallet/browser/eip2930_transaction.h"
#include "brave/components/brave_wallet/browser/eth_tx_meta.h"
#include "brave/components/brave_wallet/browser/test_utils.h"
#include "brave/components/brave_wallet/browser/tx_storage_delegate_impl.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "components/prefs/pref_service.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/origin.h"

namespace brave_wallet {

class EthTxStateManagerUnitTest : public testing::Test {
 public:
  EthTxStateManagerUnitTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}
  ~EthTxStateManagerUnitTest() override = default;

 protected:
  void SetUp() override {
    RegisterProfilePrefs(prefs_.registry());
    RegisterProfilePrefsForMigration(prefs_.registry());
    factory_ = GetTestValueStoreFactory(temp_dir_);
    delegate_ = GetTxStorageDelegateForTest(GetPrefs(), factory_);
    account_resolver_delegate_ =
        std::make_unique<AccountResolverDelegateForTest>();
    eth_tx_state_manager_ = std::make_unique<EthTxStateManager>(
        *delegate_, *account_resolver_delegate_);
  }

  PrefService* GetPrefs() { return &prefs_; }

  base::test::TaskEnvironment task_environment_;
  base::ScopedTempDir temp_dir_;
  scoped_refptr<value_store::TestValueStoreFactory> factory_;
  std::unique_ptr<value_store::ValueStoreFrontend> storage_;
  std::unique_ptr<TxStorageDelegateImpl> delegate_;
  std::unique_ptr<AccountResolverDelegateForTest> account_resolver_delegate_;
  sync_preferences::TestingPrefServiceSyncable prefs_;
  std::unique_ptr<EthTxStateManager> eth_tx_state_manager_;
};

TEST_F(EthTxStateManagerUnitTest, TxMetaAndValue) {
  auto eth_account_id = account_resolver_delegate_->RegisterAccount(
      MakeAccountId(mojom::CoinType::ETH, mojom::KeyringId::kDefault,
                    mojom::AccountKind::kDerived,
                    "0x2f015c60e0be116b1f0cd534704db9c92118fb6a"));

  // type 0
  std::unique_ptr<EthTransaction> tx = std::make_unique<EthTransaction>(
      *EthTransaction::FromTxData(mojom::TxData::New(
          "0x09", "0x4a817c800", "0x5208",
          "0x3535353535353535353535353535353535353535", "0x0de0b6b3a7640000",
          std::vector<uint8_t>(), false, std::nullopt)));
  EthTxMeta meta(eth_account_id, std::move(tx));
  meta.set_id(TxMeta::GenerateMetaID());
  meta.set_status(mojom::TransactionStatus::Submitted);
  meta.set_created_time(base::Time::Now());
  meta.set_submitted_time(base::Time::Now());
  meta.set_confirmed_time(base::Time::Now());

  TransactionReceipt tx_receipt;
  tx_receipt.transaction_hash =
      "0xb903239f8543d04b5dc1ba6579132b143087c68db1b2168786408fcbce568238";
  tx_receipt.transaction_index = 0x1;
  tx_receipt.block_number = 0xb;
  tx_receipt.block_hash =
      "0xc6ef2fc5426d6ad6fd9e2a26abeab0aa2411b7ab17f30a99d3cb96aed1d1055b";
  tx_receipt.cumulative_gas_used = 0x33bc;
  tx_receipt.gas_used = 0x4dc;
  tx_receipt.contract_address = "0xb60e8dd61c5d32be8058bb8eb970870f07233155";
  tx_receipt.status = true;

  meta.set_tx_receipt(tx_receipt);
  meta.set_tx_hash(
      "0xb903239f8543d04b5dc1ba6579132b143087c68db1b2168786408fcbce568238");
  meta.set_origin(url::Origin::Create(GURL("https://test.brave.com")));
  meta.set_chain_id(mojom::kMainnetChainId);

  base::Value::Dict meta_value = meta.ToValue();
  EXPECT_FALSE(meta_value.FindString("from"));
  const std::string* from_account_id = meta_value.FindString("from_account_id");
  ASSERT_TRUE(from_account_id);
  EXPECT_EQ(*from_account_id, eth_account_id->unique_key);
  auto meta_from_value = eth_tx_state_manager_->ValueToEthTxMeta(meta_value);
  ASSERT_NE(meta_from_value, nullptr);
  EXPECT_EQ(meta_from_value->id(), meta.id());
  EXPECT_EQ(meta_from_value->status(), meta.status());
  EXPECT_EQ(meta_from_value->from(), meta.from());
  EXPECT_EQ(meta_from_value->created_time(), meta.created_time());
  EXPECT_EQ(meta_from_value->submitted_time(), meta.submitted_time());
  EXPECT_EQ(meta_from_value->confirmed_time(), meta.confirmed_time());
  EXPECT_EQ(meta_from_value->tx_receipt(), meta.tx_receipt());
  EXPECT_EQ(meta_from_value->tx_hash(), meta.tx_hash());
  EXPECT_EQ(meta_from_value->origin(), meta.origin());
  EXPECT_EQ(meta_from_value->chain_id(), meta.chain_id());
  ASSERT_EQ(meta_from_value->tx()->type(), 0);
  EXPECT_EQ(*meta_from_value->tx(), *meta.tx());
  // optional sign_only will be false by default
  EXPECT_FALSE(meta_from_value->sign_only());

  EXPECT_EQ(*meta_from_value, meta);

  // type 1
  std::unique_ptr<Eip2930Transaction> tx1 =
      std::make_unique<Eip2930Transaction>(*Eip2930Transaction::FromTxData(
          mojom::TxData::New("0x09", "0x4a817c800", "0x5208",
                             "0x3535353535353535353535353535353535353535",
                             "0x0de0b6b3a7640000", std::vector<uint8_t>(),
                             false, std::nullopt),
          0x3));
  auto* access_list = tx1->access_list();
  Eip2930Transaction::AccessListItem item_a;
  item_a.address.fill(0x0a);
  Eip2930Transaction::AccessedStorageKey storage_key_0;
  storage_key_0.fill(0x00);
  item_a.storage_keys.push_back(storage_key_0);
  access_list->push_back(item_a);

  EthTxMeta meta1(eth_account_id, std::move(tx1));
  base::Value::Dict value1 = meta1.ToValue();
  auto meta_from_value1 = eth_tx_state_manager_->ValueToEthTxMeta(value1);
  ASSERT_NE(meta_from_value1, nullptr);
  EXPECT_EQ(meta_from_value1->tx()->type(), 1);
  Eip2930Transaction* tx_from_value1 =
      static_cast<Eip2930Transaction*>(meta_from_value1->tx());
  EXPECT_EQ(*tx_from_value1, *static_cast<Eip2930Transaction*>(meta1.tx()));

  // type2
  std::unique_ptr<Eip1559Transaction> tx2 =
      std::make_unique<Eip1559Transaction>(
          *Eip1559Transaction::FromTxData(mojom::TxData1559::New(
              mojom::TxData::New("0x09", "0x4a817c800", "0x5208",
                                 "0x3535353535353535353535353535353535353535",
                                 "0x0de0b6b3a7640000", std::vector<uint8_t>(),
                                 false, std::nullopt),
              "0x3", "0x1E", "0x32",
              mojom::GasEstimation1559::New(
                  "0x3b9aca00" /* Hex of 1 * 1e9 */,
                  "0xaf16b1600" /* Hex of 47 * 1e9 */,
                  "0x77359400" /* Hex of 2 * 1e9 */,
                  "0xb2d05e000" /* Hex of 48 * 1e9 */,
                  "0xb2d05e00" /* Hex of 3 * 1e9 */,
                  "0xb68a0aa00" /* Hex of 49 * 1e9 */,
                  "0xad8075b7a" /* Hex of 46574033786 */))));
  EthTxMeta meta2(eth_account_id, std::move(tx2));
  base::Value::Dict value2 = meta2.ToValue();
  auto meta_from_value2 = eth_tx_state_manager_->ValueToEthTxMeta(value2);
  ASSERT_NE(meta_from_value2, nullptr);
  EXPECT_EQ(meta_from_value2->tx()->type(), 2);
  Eip1559Transaction* tx_from_value2 =
      static_cast<Eip1559Transaction*>(meta_from_value2->tx());
  EXPECT_EQ(*tx_from_value2, *static_cast<Eip1559Transaction*>(meta2.tx()));

  // test sign_only
  std::unique_ptr<EthTransaction> tx3 = std::make_unique<EthTransaction>(
      *EthTransaction::FromTxData(mojom::TxData::New(
          "0x09", "0x4a817c800", "0x5208",
          "0x3535353535353535353535353535353535353535", "0x0de0b6b3a7640000",
          std::vector<uint8_t>(), false, std::nullopt)));
  EthTxMeta meta3(eth_account_id, std::move(tx3));
  meta3.set_sign_only(true);
  base::Value::Dict meta_value3 = meta3.ToValue();
  auto meta_from_value3 = eth_tx_state_manager_->ValueToEthTxMeta(meta_value3);
  ASSERT_NE(meta_from_value3, nullptr);
  EXPECT_TRUE(meta_from_value3->sign_only());
  meta3.set_sign_only(false);
  meta_value3 = meta3.ToValue();
  meta_from_value3 = eth_tx_state_manager_->ValueToEthTxMeta(meta_value3);
  ASSERT_NE(meta_from_value3, nullptr);
  EXPECT_FALSE(meta_from_value3->sign_only());
}

}  // namespace brave_wallet
