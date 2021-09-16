/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/eth_tx_state_manager.h"

#include <utility>

#include "base/strings/string_number_conversions.h"
#include "base/time/time.h"
#include "base/values.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/eip1559_transaction.h"
#include "brave/components/brave_wallet/browser/eip2930_transaction.h"
#include "brave/components/brave_wallet/browser/eth_address.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "chrome/browser/prefs/browser_prefs.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile.h"
#include "components/prefs/pref_service.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "content/public/test/browser_task_environment.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_wallet {

class EthTxStateManagerUnitTest : public testing::Test {
 public:
  EthTxStateManagerUnitTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME),
        shared_url_loader_factory_(
            base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
                &url_loader_factory_)) {}
  ~EthTxStateManagerUnitTest() override {}

 protected:
  void SetUp() override {
    TestingProfile::Builder builder;
    auto prefs =
        std::make_unique<sync_preferences::TestingPrefServiceSyncable>();
    RegisterUserProfilePrefs(prefs->registry());
    builder.SetPrefService(std::move(prefs));
    profile_ = builder.Build();
    rpc_controller_.reset(
        new EthJsonRpcController(shared_url_loader_factory_, GetPrefs()));
  }

  PrefService* GetPrefs() { return profile_->GetPrefs(); }

  content::BrowserTaskEnvironment task_environment_;
  std::unique_ptr<TestingProfile> profile_;
  network::TestURLLoaderFactory url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
  std::unique_ptr<EthJsonRpcController> rpc_controller_;
};

TEST_F(EthTxStateManagerUnitTest, GenerateMetaID) {
  EXPECT_NE(EthTxStateManager::GenerateMetaID(),
            EthTxStateManager::GenerateMetaID());
}

TEST_F(EthTxStateManagerUnitTest, TxMetaAndValue) {
  // type 0
  std::unique_ptr<EthTransaction> tx =
      std::make_unique<EthTransaction>(*EthTransaction::FromTxData(
          mojom::TxData::New("0x09", "0x4a817c800", "0x5208",
                             "0x3535353535353535353535353535353535353535",
                             "0x0de0b6b3a7640000", std::vector<uint8_t>())));
  EthTxStateManager::TxMeta meta(std::move(tx));
  meta.id = EthTxStateManager::GenerateMetaID();
  meta.status = mojom::TransactionStatus::Submitted;
  meta.from = EthAddress::FromHex("0x2f015c60e0be116b1f0cd534704db9c92118fb6a");
  meta.last_gas_price = 0x1234;
  meta.created_time = base::Time::Now();
  meta.submitted_time = base::Time::Now();
  meta.confirmed_time = base::Time::Now();

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

  meta.tx_receipt = tx_receipt;
  meta.tx_hash =
      "0xb903239f8543d04b5dc1ba6579132b143087c68db1b2168786408fcbce568238";

  base::Value meta_value = EthTxStateManager::TxMetaToValue(meta);
  auto meta_from_value = EthTxStateManager::ValueToTxMeta(meta_value);
  ASSERT_NE(meta_from_value, nullptr);
  EXPECT_EQ(meta_from_value->id, meta.id);
  EXPECT_EQ(meta_from_value->status, meta.status);
  EXPECT_EQ(meta_from_value->from, meta.from);
  EXPECT_EQ(meta_from_value->last_gas_price, meta.last_gas_price);
  EXPECT_EQ(meta_from_value->created_time, meta.created_time);
  EXPECT_EQ(meta_from_value->submitted_time, meta.submitted_time);
  EXPECT_EQ(meta_from_value->confirmed_time, meta.confirmed_time);
  EXPECT_EQ(meta_from_value->tx_receipt, meta.tx_receipt);
  EXPECT_EQ(meta_from_value->tx_hash, meta.tx_hash);
  ASSERT_EQ(meta_from_value->tx->type(), 0);
  EXPECT_EQ(*meta_from_value->tx, *meta.tx);

  EXPECT_EQ(*meta_from_value, meta);

  // type 1
  std::unique_ptr<Eip2930Transaction> tx1 =
      std::make_unique<Eip2930Transaction>(*Eip2930Transaction::FromTxData(
          mojom::TxData::New("0x09", "0x4a817c800", "0x5208",
                             "0x3535353535353535353535353535353535353535",
                             "0x0de0b6b3a7640000", std::vector<uint8_t>()),
          0x3));
  auto* access_list = tx1->access_list();
  Eip2930Transaction::AccessListItem item_a;
  item_a.address.fill(0x0a);
  Eip2930Transaction::AccessedStorageKey storage_key_0;
  storage_key_0.fill(0x00);
  item_a.storage_keys.push_back(storage_key_0);
  access_list->push_back(item_a);

  EthTxStateManager::TxMeta meta1(std::move(tx1));
  base::Value value1 = EthTxStateManager::TxMetaToValue(meta1);
  auto meta_from_value1 = EthTxStateManager::ValueToTxMeta(value1);
  ASSERT_NE(meta_from_value1, nullptr);
  EXPECT_EQ(meta_from_value1->tx->type(), 1);
  Eip2930Transaction* tx_from_value1 =
      static_cast<Eip2930Transaction*>(meta_from_value1->tx.get());
  EXPECT_EQ(*tx_from_value1, *static_cast<Eip2930Transaction*>(meta1.tx.get()));

  // type2
  std::unique_ptr<Eip1559Transaction> tx2 =
      std::make_unique<Eip1559Transaction>(
          *Eip1559Transaction::FromTxData(mojom::TxData1559::New(
              mojom::TxData::New("0x09", "0x4a817c800", "0x5208",
                                 "0x3535353535353535353535353535353535353535",
                                 "0x0de0b6b3a7640000", std::vector<uint8_t>()),
              "0x3", "0x1E", "0x32")));
  EthTxStateManager::TxMeta meta2(std::move(tx2));
  base::Value value2 = EthTxStateManager::TxMetaToValue(meta2);
  auto meta_from_value2 = EthTxStateManager::ValueToTxMeta(value2);
  ASSERT_NE(meta_from_value2, nullptr);
  EXPECT_EQ(meta_from_value2->tx->type(), 2);
  Eip1559Transaction* tx_from_value2 =
      static_cast<Eip1559Transaction*>(meta_from_value2->tx.get());
  EXPECT_EQ(*tx_from_value2, *static_cast<Eip1559Transaction*>(meta2.tx.get()));
}

TEST_F(EthTxStateManagerUnitTest, TxOperations) {
  GetPrefs()->ClearPref(kBraveWalletTransactions);
  EthTxStateManager tx_state_manager(GetPrefs(), rpc_controller_->MakeRemote());
  // Wait for network info
  base::RunLoop().RunUntilIdle();

  EthTxStateManager::TxMeta meta;
  meta.id = "001";
  EXPECT_FALSE(GetPrefs()->HasPrefPath(kBraveWalletTransactions));
  // Add
  tx_state_manager.AddOrUpdateTx(meta);
  EXPECT_TRUE(GetPrefs()->HasPrefPath(kBraveWalletTransactions));
  {
    const auto* dict = GetPrefs()->GetDictionary(kBraveWalletTransactions);
    ASSERT_TRUE(dict);
    EXPECT_EQ(dict->DictSize(), 1u);
    const auto* network_dict = dict->FindKey("mainnet");
    ASSERT_TRUE(network_dict);
    EXPECT_EQ(network_dict->DictSize(), 1u);
    const base::Value* value = network_dict->FindKey("001");
    ASSERT_TRUE(value);
    auto meta_from_value = EthTxStateManager::ValueToTxMeta(*value);
    ASSERT_NE(meta_from_value, nullptr);
    EXPECT_EQ(*meta_from_value, meta);
  }

  meta.tx_hash = "0xabcd";
  // Update
  tx_state_manager.AddOrUpdateTx(meta);
  {
    const auto* dict = GetPrefs()->GetDictionary(kBraveWalletTransactions);
    ASSERT_TRUE(dict);
    EXPECT_EQ(dict->DictSize(), 1u);
    const auto* network_dict = dict->FindKey("mainnet");
    ASSERT_TRUE(network_dict);
    EXPECT_EQ(network_dict->DictSize(), 1u);
    const base::Value* value = network_dict->FindKey("001");
    ASSERT_TRUE(value);
    auto meta_from_value = EthTxStateManager::ValueToTxMeta(*value);
    ASSERT_NE(meta_from_value, nullptr);
    EXPECT_EQ(meta_from_value->tx_hash, meta.tx_hash);
  }

  meta.id = "002";
  meta.tx_hash = "0xabff";
  // Add another one
  tx_state_manager.AddOrUpdateTx(meta);
  {
    const auto* dict = GetPrefs()->GetDictionary(kBraveWalletTransactions);
    ASSERT_TRUE(dict);
    EXPECT_EQ(dict->DictSize(), 1u);
    const auto* network_dict = dict->FindKey("mainnet");
    ASSERT_TRUE(network_dict);
    EXPECT_EQ(network_dict->DictSize(), 2u);
  }

  // Get
  {
    auto meta_fetched = tx_state_manager.GetTx("001");
    ASSERT_NE(meta_fetched, nullptr);
    ASSERT_EQ(tx_state_manager.GetTx("003"), nullptr);
    EXPECT_EQ(meta_fetched->id, "001");
    EXPECT_EQ(meta_fetched->tx_hash, "0xabcd");

    auto meta_fetched2 = tx_state_manager.GetTx("002");
    ASSERT_NE(meta_fetched2, nullptr);
    EXPECT_EQ(meta_fetched2->id, "002");
    EXPECT_EQ(meta_fetched2->tx_hash, "0xabff");
  }

  // Delete
  tx_state_manager.DeleteTx("001");
  {
    const auto* dict = GetPrefs()->GetDictionary(kBraveWalletTransactions);
    ASSERT_TRUE(dict);
    EXPECT_EQ(dict->DictSize(), 1u);
  }

  // Purge
  tx_state_manager.WipeTxs();
  EXPECT_FALSE(GetPrefs()->HasPrefPath(kBraveWalletTransactions));
}

TEST_F(EthTxStateManagerUnitTest, GetTransactionsByStatus) {
  GetPrefs()->ClearPref(kBraveWalletTransactions);
  EthTxStateManager tx_state_manager(GetPrefs(), rpc_controller_->MakeRemote());
  // Wait for network info
  base::RunLoop().RunUntilIdle();

  auto addr1 =
      EthAddress::FromHex("0x3535353535353535353535353535353535353535");
  auto addr2 =
      EthAddress::FromHex("0x2f015c60e0be116b1f0cd534704db9c92118fb6a");

  for (size_t i = 0; i < 20; ++i) {
    EthTxStateManager::TxMeta meta;
    meta.from =
        EthAddress::FromHex("0x3333333333333333333333333333333333333333");
    meta.id = base::NumberToString(i);
    if (i % 2 == 0) {
      if (i % 4 == 0)
        meta.from = addr1;
      meta.status = mojom::TransactionStatus::Confirmed;
    } else {
      if (i % 5 == 0)
        meta.from = addr2;
      meta.status = mojom::TransactionStatus::Submitted;
    }
    tx_state_manager.AddOrUpdateTx(meta);
  }

  EXPECT_EQ(tx_state_manager
                .GetTransactionsByStatus(mojom::TransactionStatus::Approved,
                                         absl::nullopt)
                .size(),
            0u);
  EXPECT_EQ(tx_state_manager
                .GetTransactionsByStatus(mojom::TransactionStatus::Confirmed,
                                         absl::nullopt)
                .size(),
            10u);
  EXPECT_EQ(tx_state_manager
                .GetTransactionsByStatus(mojom::TransactionStatus::Submitted,
                                         absl::nullopt)
                .size(),
            10u);

  EXPECT_EQ(
      tx_state_manager
          .GetTransactionsByStatus(mojom::TransactionStatus::Approved, addr1)
          .size(),
      0u);

  EXPECT_EQ(
      tx_state_manager.GetTransactionsByStatus(absl::nullopt, absl::nullopt)
          .size(),
      20u);
  EXPECT_EQ(
      tx_state_manager.GetTransactionsByStatus(absl::nullopt, addr1).size(),
      5u);
  EXPECT_EQ(
      tx_state_manager.GetTransactionsByStatus(absl::nullopt, addr2).size(),
      2u);

  auto confirmed_addr1 = tx_state_manager.GetTransactionsByStatus(
      mojom::TransactionStatus::Confirmed, addr1);
  EXPECT_EQ(confirmed_addr1.size(), 5u);
  for (const auto& meta : confirmed_addr1) {
    unsigned id;
    ASSERT_TRUE(base::StringToUint(meta->id, &id));
    EXPECT_EQ(id % 4, 0u);
  }

  auto submitted_addr2 = tx_state_manager.GetTransactionsByStatus(
      mojom::TransactionStatus::Submitted, addr2);
  EXPECT_EQ(submitted_addr2.size(), 2u);
  for (const auto& meta : submitted_addr2) {
    unsigned id;
    ASSERT_TRUE(base::StringToUint(meta->id, &id));
    EXPECT_EQ(id % 5, 0u);
  }
}

TEST_F(EthTxStateManagerUnitTest, SwitchNetwork) {
  GetPrefs()->ClearPref(kBraveWalletTransactions);
  EthTxStateManager tx_state_manager(GetPrefs(), rpc_controller_->MakeRemote());
  // Wait for network info
  base::RunLoop().RunUntilIdle();

  EthTxStateManager::TxMeta meta;
  meta.id = "001";
  tx_state_manager.AddOrUpdateTx(meta);

  rpc_controller_->SetNetwork("0x3");
  // Wait for network info
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(tx_state_manager.GetTx("001"), nullptr);
  tx_state_manager.AddOrUpdateTx(meta);

  rpc_controller_->SetNetwork(brave_wallet::mojom::kLocalhostChainId);
  // Wait for network info
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(tx_state_manager.GetTx("001"), nullptr);
  tx_state_manager.AddOrUpdateTx(meta);

  const auto* dict = GetPrefs()->GetDictionary(kBraveWalletTransactions);
  ASSERT_TRUE(dict);
  EXPECT_EQ(dict->DictSize(), 3u);
  const auto* mainnet_dict = dict->FindKey("mainnet");
  ASSERT_TRUE(mainnet_dict);
  EXPECT_EQ(mainnet_dict->DictSize(), 1u);
  EXPECT_TRUE(mainnet_dict->FindKey("001"));
  const auto* ropsten_dict = dict->FindKey("ropsten");
  ASSERT_TRUE(ropsten_dict);
  EXPECT_EQ(ropsten_dict->DictSize(), 1u);
  EXPECT_TRUE(ropsten_dict->FindKey("001"));
  const auto* localhost_dict = dict->FindKey("http://localhost:8545/");
  ASSERT_TRUE(localhost_dict);
  EXPECT_EQ(localhost_dict->DictSize(), 1u);
  EXPECT_TRUE(localhost_dict->FindKey("001"));
}

TEST_F(EthTxStateManagerUnitTest, RetireOldTxMeta) {
  GetPrefs()->ClearPref(kBraveWalletTransactions);
  EthTxStateManager tx_state_manager(GetPrefs(), rpc_controller_->MakeRemote());
  // Wait for network info
  base::RunLoop().RunUntilIdle();

  for (size_t i = 0; i < 20; ++i) {
    EthTxStateManager::TxMeta meta;
    meta.id = base::NumberToString(i);
    if (i % 2 == 0) {
      meta.status = mojom::TransactionStatus::Confirmed;
      meta.confirmed_time = base::Time::Now();
    } else {
      meta.status = mojom::TransactionStatus::Rejected;
      meta.created_time = base::Time::Now();
    }
    tx_state_manager.AddOrUpdateTx(meta);
  }

  EXPECT_TRUE(tx_state_manager.GetTx("0"));
  EthTxStateManager::TxMeta meta21;
  meta21.id = "20";
  meta21.status = mojom::TransactionStatus::Confirmed;
  meta21.confirmed_time = base::Time::Now();
  tx_state_manager.AddOrUpdateTx(meta21);
  EXPECT_FALSE(tx_state_manager.GetTx("0"));

  EXPECT_TRUE(tx_state_manager.GetTx("1"));
  EthTxStateManager::TxMeta meta22;
  meta22.id = "21";
  meta22.status = mojom::TransactionStatus::Rejected;
  meta22.created_time = base::Time::Now();
  tx_state_manager.AddOrUpdateTx(meta22);
  EXPECT_FALSE(tx_state_manager.GetTx("1"));

  // Other status doesn't matter
  EXPECT_TRUE(tx_state_manager.GetTx("2"));
  EXPECT_TRUE(tx_state_manager.GetTx("3"));
  EthTxStateManager::TxMeta meta23;
  meta23.id = "22";
  meta23.status = mojom::TransactionStatus::Submitted;
  meta23.created_time = base::Time::Now();
  tx_state_manager.AddOrUpdateTx(meta23);
  EXPECT_TRUE(tx_state_manager.GetTx("2"));
  EXPECT_TRUE(tx_state_manager.GetTx("3"));
}

TEST_F(EthTxStateManagerUnitTest, TxMetaToTransactionInfo) {
  // type 0
  std::unique_ptr<EthTransaction> tx =
      std::make_unique<EthTransaction>(*EthTransaction::FromTxData(
          mojom::TxData::New("0x09", "0x4a817c800", "0x5208",
                             "0x3535353535353535353535353535353535353535",
                             "0x0de0b6b3a7640000", std::vector<uint8_t>())));
  EthTxStateManager::TxMeta meta(std::move(tx));
  mojom::TransactionInfoPtr ti =
      EthTxStateManager::TxMetaToTransactionInfo(meta);
  ASSERT_EQ(ti->id, meta.id);
  ASSERT_EQ(ti->from_address, meta.from.ToHex());
  ASSERT_EQ(ti->tx_hash, meta.tx_hash);
  ASSERT_EQ(ti->tx_status, meta.status);
  ASSERT_EQ(ti->tx_data->base_data->nonce, Uint256ValueToHex(meta.tx->nonce()));
  ASSERT_EQ(ti->tx_data->base_data->gas_price,
            Uint256ValueToHex(meta.tx->gas_price()));
  ASSERT_EQ(ti->tx_data->base_data->gas_limit,
            Uint256ValueToHex(meta.tx->gas_limit()));
  ASSERT_EQ(ti->tx_data->base_data->to, meta.tx->to().ToHex());
  ASSERT_EQ(ti->tx_data->base_data->value, Uint256ValueToHex(meta.tx->value()));
  ASSERT_EQ(ti->tx_data->base_data->data, meta.tx->data());
  ASSERT_EQ(ti->tx_data->chain_id, "");
  ASSERT_EQ(ti->tx_data->max_priority_fee_per_gas, "");
  ASSERT_EQ(ti->tx_data->max_fee_per_gas, "");

  // type 1
  std::unique_ptr<Eip2930Transaction> tx1 =
      std::make_unique<Eip2930Transaction>(*Eip2930Transaction::FromTxData(
          mojom::TxData::New("0x09", "0x4a817c800", "0x5208",
                             "0x3535353535353535353535353535353535353535",
                             "0x0de0b6b3a7640000", std::vector<uint8_t>()),
          0x3));
  auto* access_list = tx1->access_list();
  Eip2930Transaction::AccessListItem item_a;
  item_a.address.fill(0x0a);
  Eip2930Transaction::AccessedStorageKey storage_key_0;
  storage_key_0.fill(0x00);
  item_a.storage_keys.push_back(storage_key_0);
  access_list->push_back(item_a);
  EthTxStateManager::TxMeta meta1(std::move(tx1));
  mojom::TransactionInfoPtr ti1 =
      EthTxStateManager::TxMetaToTransactionInfo(meta1);
  ASSERT_EQ(ti1->id, meta1.id);
  ASSERT_EQ(ti1->from_address, meta1.from.ToHex());
  ASSERT_EQ(ti1->tx_hash, meta1.tx_hash);
  ASSERT_EQ(ti1->tx_status, meta1.status);
  ASSERT_EQ(ti1->tx_data->base_data->nonce,
            Uint256ValueToHex(meta1.tx->nonce()));
  ASSERT_EQ(ti1->tx_data->base_data->gas_price,
            Uint256ValueToHex(meta1.tx->gas_price()));
  ASSERT_EQ(ti1->tx_data->base_data->gas_limit,
            Uint256ValueToHex(meta1.tx->gas_limit()));
  ASSERT_EQ(ti1->tx_data->base_data->to, meta1.tx->to().ToHex());
  ASSERT_EQ(ti1->tx_data->base_data->value,
            Uint256ValueToHex(meta1.tx->value()));
  ASSERT_EQ(ti1->tx_data->base_data->data, meta1.tx->data());
  auto* tx2930 = reinterpret_cast<Eip2930Transaction*>(meta1.tx.get());
  ASSERT_EQ(ti1->tx_data->chain_id, Uint256ValueToHex(tx2930->chain_id()));
  ASSERT_EQ(ti1->tx_data->max_priority_fee_per_gas, "");
  ASSERT_EQ(ti1->tx_data->max_fee_per_gas, "");

  // type2
  std::unique_ptr<Eip1559Transaction> tx2 =
      std::make_unique<Eip1559Transaction>(
          *Eip1559Transaction::FromTxData(mojom::TxData1559::New(
              mojom::TxData::New("0x09", "0x4a817c800", "0x5208",
                                 "0x3535353535353535353535353535353535353535",
                                 "0x0de0b6b3a7640000", std::vector<uint8_t>()),
              "0x3", "0x1E", "0x32")));
  EthTxStateManager::TxMeta meta2(std::move(tx2));
  mojom::TransactionInfoPtr ti2 =
      EthTxStateManager::TxMetaToTransactionInfo(meta2);
  ASSERT_EQ(ti2->id, meta2.id);
  ASSERT_EQ(ti2->from_address, meta2.from.ToHex());
  ASSERT_EQ(ti2->tx_hash, meta2.tx_hash);
  ASSERT_EQ(ti2->tx_status, meta2.status);
  ASSERT_EQ(ti2->tx_data->base_data->nonce,
            Uint256ValueToHex(meta2.tx->nonce()));
  ASSERT_EQ(ti2->tx_data->base_data->gas_price,
            Uint256ValueToHex(meta2.tx->gas_price()));
  ASSERT_EQ(ti2->tx_data->base_data->gas_limit,
            Uint256ValueToHex(meta2.tx->gas_limit()));
  ASSERT_EQ(ti2->tx_data->base_data->to, meta2.tx->to().ToHex());
  ASSERT_EQ(ti2->tx_data->base_data->value,
            Uint256ValueToHex(meta2.tx->value()));
  ASSERT_EQ(ti2->tx_data->base_data->data, meta2.tx->data());
  auto* tx1559 = reinterpret_cast<Eip1559Transaction*>(meta2.tx.get());
  ASSERT_EQ(ti2->tx_data->chain_id, Uint256ValueToHex(tx1559->chain_id()));
  ASSERT_EQ(ti2->tx_data->max_priority_fee_per_gas,
            Uint256ValueToHex(tx1559->max_priority_fee_per_gas()));
  ASSERT_EQ(ti2->tx_data->max_fee_per_gas,
            Uint256ValueToHex(tx1559->max_fee_per_gas()));
}

}  // namespace brave_wallet
