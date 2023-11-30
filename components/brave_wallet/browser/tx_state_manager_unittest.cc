/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/tx_state_manager.h"

#include <memory>
#include <optional>
#include <utility>

#include "base/files/scoped_temp_dir.h"
#include "base/run_loop.h"
#include "base/scoped_observation.h"
#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "base/test/values_test_util.h"
#include "base/time/time.h"
#include "base/values.h"
#include "brave/components/brave_wallet/browser/brave_wallet_prefs.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/eth_tx_meta.h"
#include "brave/components/brave_wallet/browser/eth_tx_state_manager.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/browser/test_utils.h"
#include "brave/components/brave_wallet/browser/tx_storage_delegate_impl.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "brave/components/brave_wallet/common/test_utils.h"
#include "brave/components/brave_wallet/common/value_conversion_utils.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using base::test::ParseJson;
using base::test::ParseJsonDict;
using testing::_;

namespace brave_wallet {

class MockTxStateManagerObserver : public TxStateManager::Observer {
 public:
  explicit MockTxStateManagerObserver(TxStateManager* tx_state_manager) {
    observation_.Observe(tx_state_manager);
  }

  MOCK_METHOD1(OnTransactionStatusChanged, void(mojom::TransactionInfoPtr));
  MOCK_METHOD1(OnNewUnapprovedTx, void(mojom::TransactionInfoPtr));

 private:
  base::ScopedObservation<TxStateManager, TxStateManager::Observer>
      observation_{this};
};

class TxStateManagerUnitTest : public testing::Test {
 public:
  TxStateManagerUnitTest() = default;

 protected:
  void SetUp() override {
    RegisterProfilePrefs(prefs_.registry());
    RegisterProfilePrefsForMigration(prefs_.registry());
    // The only different between each coin type's tx state manager in these
    // base functions are their pref paths, so here we just use
    // EthTxStateManager to test common methods in TxStateManager.
    factory_ = GetTestValueStoreFactory(temp_dir_);
    delegate_ = GetTxStorageDelegateForTest(&prefs_, factory_);
    account_resolver_delegate_ =
        std::make_unique<AccountResolverDelegateForTest>();
    WaitForTxStorageDelegateInitialized(delegate_.get());
    tx_state_manager_ = std::make_unique<EthTxStateManager>(
        &prefs_, delegate_.get(), account_resolver_delegate_.get());
    eth_account_id_ = account_resolver_delegate_->RegisterAccount(
        MakeAccountId(mojom::CoinType::ETH, mojom::KeyringId::kDefault,
                      mojom::AccountKind::kDerived,
                      "0x2f015c60e0be116b1f0cd534704db9c92118fb6a"));
  }

  void UpdateCustomNetworks(PrefService* prefs,
                            const std::vector<base::Value::Dict>& values,
                            brave_wallet::mojom::CoinType coin) {
    ScopedDictPrefUpdate update(prefs, kBraveWalletCustomNetworks);
    base::Value::List* list = update->EnsureList(GetPrefKeyForCoinType(coin));
    list->clear();
    for (auto& it : values) {
      list->Append(it.Clone());
    }
  }

  std::optional<base::Value> GetTxs() {
    base::RunLoop run_loop;
    std::optional<base::Value> value_out;
    delegate_->store_->Get(
        "transactions",
        base::BindLambdaForTesting([&](std::optional<base::Value> value) {
          value_out = std::move(value);
          run_loop.Quit();
        }));
    run_loop.Run();
    return value_out;
  }

  base::test::TaskEnvironment task_environment_;
  sync_preferences::TestingPrefServiceSyncable prefs_;
  base::ScopedTempDir temp_dir_;
  scoped_refptr<value_store::TestValueStoreFactory> factory_;
  std::unique_ptr<TxStorageDelegateImpl> delegate_;
  std::unique_ptr<AccountResolverDelegateForTest> account_resolver_delegate_;
  mojom::AccountIdPtr eth_account_id_;
  std::unique_ptr<TxStateManager> tx_state_manager_;
};

TEST_F(TxStateManagerUnitTest, ConvertFromAddress) {
  // Setup transaction.
  EthTxMeta meta(eth_account_id_, std::make_unique<EthTransaction>());
  meta.set_id("001");
  meta.set_chain_id(mojom::kMainnetChainId);
  EXPECT_FALSE(GetTxs());
  ASSERT_TRUE(tx_state_manager_->AddOrUpdateTx(meta));
  EXPECT_TRUE(GetTxs());

  auto txs = GetTxs();
  ASSERT_TRUE(txs);
  const base::Value::Dict* value =
      txs->GetDict().FindDictByDottedPath("ethereum.mainnet.001");
  ASSERT_TRUE(value);

  // Transaction is stored with account id.
  EXPECT_FALSE(value->FindString("from"));
  EXPECT_EQ(*value->FindString("from_account_id"), eth_account_id_->unique_key);
  auto meta_from_value = tx_state_manager_->ValueToTxMeta(*value);
  ASSERT_TRUE(meta_from_value);
  EXPECT_EQ(eth_account_id_,
            static_cast<EthTxMeta*>(meta_from_value.get())->from());

  // Make a transaction clone.
  auto legacy_value = value->Clone();

  // Can't convert to meta if has neither from_account_id nor from fields.
  legacy_value.Remove("from_account_id");
  EXPECT_FALSE(tx_state_manager_->ValueToTxMeta(legacy_value));

  // Can't convert to meta if has unknown address.
  legacy_value.Set("from", "0x3535353535353535353535353535353535353535");
  EXPECT_FALSE(tx_state_manager_->ValueToTxMeta(legacy_value));

  // Convert to meta if has a known address.
  legacy_value.Set("from", eth_account_id_->address);
  EXPECT_TRUE(tx_state_manager_->ValueToTxMeta(legacy_value));
  EXPECT_EQ(eth_account_id_,
            tx_state_manager_->ValueToTxMeta(legacy_value)->from());
}

TEST_F(TxStateManagerUnitTest, TxOperations) {
  EthTxMeta meta(eth_account_id_, std::make_unique<EthTransaction>());
  meta.set_id("001");
  meta.set_chain_id(mojom::kMainnetChainId);
  EXPECT_FALSE(GetTxs());
  // Add
  ASSERT_TRUE(tx_state_manager_->AddOrUpdateTx(meta));
  EXPECT_TRUE(GetTxs());
  {
    auto txs = GetTxs();
    ASSERT_TRUE(txs);
    const auto& dict = txs->GetDict();
    EXPECT_EQ(dict.size(), 1u);
    const auto* ethereum_dict = dict.FindDict("ethereum");
    ASSERT_TRUE(ethereum_dict);
    EXPECT_EQ(ethereum_dict->size(), 1u);
    const auto* network_dict = ethereum_dict->FindDict("mainnet");
    ASSERT_TRUE(network_dict);
    EXPECT_EQ(network_dict->size(), 1u);
    const base::Value::Dict* value = network_dict->FindDict("001");
    ASSERT_TRUE(value);
    auto meta_from_value = tx_state_manager_->ValueToTxMeta(*value);
    ASSERT_NE(meta_from_value, nullptr);
    EXPECT_EQ(*static_cast<EthTxMeta*>(meta_from_value.get()), meta);
  }

  meta.set_tx_hash("0xabcd");
  // Update
  ASSERT_TRUE(tx_state_manager_->AddOrUpdateTx(meta));
  {
    auto txs = GetTxs();
    ASSERT_TRUE(txs);
    const auto& dict = txs->GetDict();
    EXPECT_EQ(dict.size(), 1u);
    const auto* ethereum_dict = dict.FindDict("ethereum");
    ASSERT_TRUE(ethereum_dict);
    EXPECT_EQ(ethereum_dict->size(), 1u);
    const auto* network_dict = ethereum_dict->FindDict("mainnet");
    ASSERT_TRUE(network_dict);
    EXPECT_EQ(network_dict->size(), 1u);
    const base::Value::Dict* value = network_dict->FindDict("001");
    ASSERT_TRUE(value);
    auto meta_from_value = tx_state_manager_->ValueToTxMeta(*value);
    ASSERT_NE(meta_from_value, nullptr);
    EXPECT_EQ(meta_from_value->tx_hash(), meta.tx_hash());
  }

  meta.set_id("002");
  meta.set_tx_hash("0xabff");
  // Add another one
  ASSERT_TRUE(tx_state_manager_->AddOrUpdateTx(meta));
  {
    auto txs = GetTxs();
    ASSERT_TRUE(txs);
    const auto& dict = txs->GetDict();
    EXPECT_EQ(dict.size(), 1u);
    const auto* ethereum_dict = dict.FindDict("ethereum");
    ASSERT_TRUE(ethereum_dict);
    EXPECT_EQ(ethereum_dict->size(), 1u);
    const auto* network_dict = ethereum_dict->FindDict("mainnet");
    ASSERT_TRUE(network_dict);
    EXPECT_EQ(network_dict->size(), 2u);
  }

  // Get
  {
    auto meta_fetched = tx_state_manager_->GetTx(mojom::kMainnetChainId, "001");
    ASSERT_NE(meta_fetched, nullptr);
    ASSERT_EQ(tx_state_manager_->GetTx(mojom::kMainnetChainId, "003"), nullptr);
    ASSERT_EQ(tx_state_manager_->GetTx(mojom::kGoerliChainId, "001"), nullptr);
    EXPECT_EQ(meta_fetched->id(), "001");
    EXPECT_EQ(meta_fetched->tx_hash(), "0xabcd");

    auto meta_fetched2 =
        tx_state_manager_->GetTx(mojom::kMainnetChainId, "002");
    ASSERT_NE(meta_fetched2, nullptr);
    EXPECT_EQ(meta_fetched2->id(), "002");
    EXPECT_EQ(meta_fetched2->tx_hash(), "0xabff");

    auto meta_fetched3 = tx_state_manager_->GetTx(mojom::kMainnetChainId, "");
    EXPECT_EQ(meta_fetched3, nullptr);
  }

  // Delete
  ASSERT_TRUE(tx_state_manager_->DeleteTx(mojom::kMainnetChainId, "001"));
  {
    auto txs = GetTxs();
    ASSERT_TRUE(txs);
    const auto& dict = txs->GetDict();
    EXPECT_EQ(dict.size(), 1u);
    const auto* ethereum_dict = dict.FindDict("ethereum");
    ASSERT_TRUE(ethereum_dict);
    EXPECT_EQ(ethereum_dict->size(), 1u);
    const auto* network_dict = ethereum_dict->FindDict("mainnet");
    ASSERT_TRUE(network_dict);
    EXPECT_EQ(network_dict->size(), 1u);
  }

  // Purge
  ASSERT_TRUE(tx_state_manager_->WipeTxs());
  auto txs = GetTxs();
  const auto& dict = txs->GetDict();
  EXPECT_EQ(dict.size(), 0u);
  EXPECT_FALSE(dict.FindByDottedPath("ethereum"));
}

TEST_F(TxStateManagerUnitTest, GetTransactionsByStatus) {
  prefs_.ClearPref(kBraveWalletTransactions);

  std::string addr1 = "0x3535353535353535353535353535353535353535";
  std::string addr2 = "0x2f015c60e0be116b1f0cd534704db9c92118fb6a";
  std::string addr3 = "0x3333333333333333333333333333333333333333";

  auto acc1 = account_resolver_delegate_->RegisterAccount(
      MakeAccountId(mojom::CoinType::ETH, mojom::KeyringId::kDefault,
                    mojom::AccountKind::kDerived, addr1));
  auto acc2 = account_resolver_delegate_->RegisterAccount(
      MakeAccountId(mojom::CoinType::ETH, mojom::KeyringId::kDefault,
                    mojom::AccountKind::kDerived, addr2));
  auto acc3 = account_resolver_delegate_->RegisterAccount(
      MakeAccountId(mojom::CoinType::ETH, mojom::KeyringId::kDefault,
                    mojom::AccountKind::kDerived, addr3));

  for (size_t i = 0; i < 20; ++i) {
    EthTxMeta meta(acc3, std::make_unique<EthTransaction>());
    meta.set_id(base::NumberToString(i));
    if (i % 2 == 0) {
      if (i % 4 == 0) {
        meta.set_from(acc1);
      }
      if (i % 6 == 0) {
        meta.set_chain_id(mojom::kMainnetChainId);
      } else {
        meta.set_chain_id(mojom::kGoerliChainId);
      }
      meta.set_status(mojom::TransactionStatus::Confirmed);
    } else {
      if (i % 5 == 0) {
        meta.set_from(acc2);
      }
      if (i % 7 == 0) {
        meta.set_chain_id(mojom::kMainnetChainId);
      } else {
        meta.set_chain_id(mojom::kGoerliChainId);
      }
      meta.set_status(mojom::TransactionStatus::Submitted);
    }
    ASSERT_TRUE(tx_state_manager_->AddOrUpdateTx(meta));
  }

  EXPECT_EQ(
      tx_state_manager_
          ->GetTransactionsByStatus(
              std::nullopt, mojom::TransactionStatus::Approved, std::nullopt)
          .size(),
      0u);
  EXPECT_EQ(
      tx_state_manager_
          ->GetTransactionsByStatus(
              std::nullopt, mojom::TransactionStatus::Confirmed, std::nullopt)
          .size(),
      10u);
  EXPECT_EQ(tx_state_manager_
                ->GetTransactionsByStatus(mojom::kMainnetChainId,
                                          mojom::TransactionStatus::Confirmed,
                                          std::nullopt)
                .size(),
            4u);
  EXPECT_EQ(tx_state_manager_
                ->GetTransactionsByStatus(mojom::kGoerliChainId,
                                          mojom::TransactionStatus::Confirmed,
                                          std::nullopt)
                .size(),
            6u);
  EXPECT_EQ(
      tx_state_manager_
          ->GetTransactionsByStatus(
              std::nullopt, mojom::TransactionStatus::Submitted, std::nullopt)
          .size(),
      10u);
  EXPECT_EQ(tx_state_manager_
                ->GetTransactionsByStatus(mojom::kMainnetChainId,
                                          mojom::TransactionStatus::Submitted,
                                          std::nullopt)
                .size(),
            1u);
  EXPECT_EQ(tx_state_manager_
                ->GetTransactionsByStatus(mojom::kGoerliChainId,
                                          mojom::TransactionStatus::Submitted,
                                          std::nullopt)
                .size(),
            9u);

  EXPECT_EQ(tx_state_manager_
                ->GetTransactionsByStatus(
                    std::nullopt, mojom::TransactionStatus::Approved, acc1)
                .size(),
            0u);

  EXPECT_EQ(
      tx_state_manager_
          ->GetTransactionsByStatus(std::nullopt, std::nullopt, std::nullopt)
          .size(),
      20u);
  EXPECT_EQ(tx_state_manager_
                ->GetTransactionsByStatus(std::nullopt, std::nullopt, acc1)
                .size(),
            5u);
  EXPECT_EQ(
      tx_state_manager_
          ->GetTransactionsByStatus(mojom::kMainnetChainId, std::nullopt, acc1)
          .size(),
      2u);
  EXPECT_EQ(
      tx_state_manager_
          ->GetTransactionsByStatus(mojom::kGoerliChainId, std::nullopt, acc1)
          .size(),
      3u);
  EXPECT_EQ(tx_state_manager_
                ->GetTransactionsByStatus(std::nullopt, std::nullopt, acc2)
                .size(),
            2u);
  EXPECT_EQ(
      tx_state_manager_
          ->GetTransactionsByStatus(mojom::kMainnetChainId, std::nullopt, acc2)
          .size(),
      0u);
  EXPECT_EQ(
      tx_state_manager_
          ->GetTransactionsByStatus(mojom::kGoerliChainId, std::nullopt, acc2)
          .size(),
      2u);

  auto confirmed_addr1 = tx_state_manager_->GetTransactionsByStatus(
      std::nullopt, mojom::TransactionStatus::Confirmed, acc1);
  EXPECT_EQ(confirmed_addr1.size(), 5u);
  for (const auto& meta : confirmed_addr1) {
    unsigned id;
    ASSERT_TRUE(base::StringToUint(meta->id(), &id));
    EXPECT_EQ(id % 4, 0u);
  }

  auto submitted_addr2 = tx_state_manager_->GetTransactionsByStatus(
      std::nullopt, mojom::TransactionStatus::Submitted, acc2);
  EXPECT_EQ(submitted_addr2.size(), 2u);
  for (const auto& meta : submitted_addr2) {
    unsigned id;
    ASSERT_TRUE(base::StringToUint(meta->id(), &id));
    EXPECT_EQ(id % 5, 0u);
  }

  // Add custom chain to prefs
  std::vector<base::Value::Dict> values;
  mojom::NetworkInfo custom_chain = GetTestNetworkInfo1("0xdeadbeef");
  values.push_back(NetworkInfoToValue(custom_chain));
  UpdateCustomNetworks(&prefs_, std::move(values), custom_chain.coin);

  // Add a transaction on the custom chain
  EthTxMeta meta(acc1, std::make_unique<EthTransaction>());
  meta.set_id("xyz");
  meta.set_chain_id(custom_chain.chain_id);
  meta.set_status(mojom::TransactionStatus::Submitted);
  ASSERT_TRUE(tx_state_manager_->AddOrUpdateTx(meta));

  // OK: no filter
  EXPECT_EQ(
      tx_state_manager_
          ->GetTransactionsByStatus(std::nullopt, std::nullopt, std::nullopt)
          .size(),
      21u);

  // OK: filter by address
  EXPECT_EQ(tx_state_manager_
                ->GetTransactionsByStatus(std::nullopt, std::nullopt, acc1)
                .size(),
            6u);

  // OK: filter by chain_id
  EXPECT_EQ(tx_state_manager_
                ->GetTransactionsByStatus(custom_chain.chain_id, std::nullopt,
                                          std::nullopt)
                .size(),
            1u);

  // OK: filter by chain_id and address
  EXPECT_EQ(
      tx_state_manager_
          ->GetTransactionsByStatus(custom_chain.chain_id, std::nullopt, acc1)
          .size(),
      1u);

  // OK: filter by chain_id and status
  EXPECT_EQ(tx_state_manager_
                ->GetTransactionsByStatus(custom_chain.chain_id,
                                          mojom::TransactionStatus::Submitted,
                                          std::nullopt)
                .size(),
            1u);

  // OK: filter by chain_id and status and address
  EXPECT_EQ(
      tx_state_manager_
          ->GetTransactionsByStatus(custom_chain.chain_id,
                                    mojom::TransactionStatus::Submitted, acc1)
          .size(),
      1u);
}

TEST_F(TxStateManagerUnitTest, MultiChainId) {
  prefs_.ClearPref(kBraveWalletTransactions);

  EthTxMeta meta(eth_account_id_, std::make_unique<EthTransaction>());
  meta.set_id("001");
  meta.set_chain_id(mojom::kMainnetChainId);
  ASSERT_TRUE(tx_state_manager_->AddOrUpdateTx(meta));

  EXPECT_EQ(tx_state_manager_->GetTx(mojom::kGoerliChainId, "001"), nullptr);
  meta.set_chain_id(mojom::kGoerliChainId);
  ASSERT_TRUE(tx_state_manager_->AddOrUpdateTx(meta));

  EXPECT_EQ(tx_state_manager_->GetTx(mojom::kLocalhostChainId, "001"), nullptr);
  meta.set_chain_id(mojom::kLocalhostChainId);
  ASSERT_TRUE(tx_state_manager_->AddOrUpdateTx(meta));

  auto txs = GetTxs();
  ASSERT_TRUE(txs);
  const auto& dict = txs->GetDict();
  EXPECT_EQ(dict.size(), 1u);
  const auto* ethereum_dict = dict.FindDict("ethereum");
  ASSERT_TRUE(ethereum_dict);
  EXPECT_EQ(ethereum_dict->size(), 3u);
  const auto* mainnet_dict = ethereum_dict->FindDict("mainnet");
  ASSERT_TRUE(mainnet_dict);
  EXPECT_EQ(mainnet_dict->size(), 1u);
  EXPECT_TRUE(mainnet_dict->FindDict("001"));
  const auto* goerli_dict = ethereum_dict->FindDict("goerli");
  ASSERT_TRUE(goerli_dict);
  EXPECT_EQ(goerli_dict->size(), 1u);
  EXPECT_TRUE(goerli_dict->FindDict("001"));
  auto localhost_url_spec =
      brave_wallet::GetNetworkURL(&prefs_, mojom::kLocalhostChainId,
                                  mojom::CoinType::ETH)
          .spec();
  const auto* localhost_dict = ethereum_dict->FindDict(localhost_url_spec);
  ASSERT_TRUE(localhost_dict);
  EXPECT_EQ(localhost_dict->size(), 1u);
  EXPECT_TRUE(localhost_dict->FindDict("001"));
}

TEST_F(TxStateManagerUnitTest, RetireOldTxMeta) {
  for (size_t i = 0; i < 1000; ++i) {
    EthTxMeta meta(eth_account_id_, std::make_unique<EthTransaction>());
    meta.set_id(base::NumberToString(i));
    meta.set_chain_id(mojom::kMainnetChainId);

    if (i % 2 == 0) {
      meta.set_status(mojom::TransactionStatus::Confirmed);
      meta.set_confirmed_time(base::Time::Now());
    } else {
      meta.set_status(mojom::TransactionStatus::Rejected);
      meta.set_created_time(base::Time::Now());
    }
    ASSERT_TRUE(tx_state_manager_->AddOrUpdateTx(meta));
  }

  EXPECT_TRUE(tx_state_manager_->GetTx(mojom::kMainnetChainId, "0"));
  EthTxMeta meta1000(eth_account_id_, std::make_unique<EthTransaction>());
  meta1000.set_id("1000");
  meta1000.set_chain_id(mojom::kMainnetChainId);
  meta1000.set_status(mojom::TransactionStatus::Confirmed);
  meta1000.set_confirmed_time(base::Time::Now());
  ASSERT_TRUE(tx_state_manager_->AddOrUpdateTx(meta1000));
  EXPECT_FALSE(tx_state_manager_->GetTx(mojom::kMainnetChainId, "0"));

  EXPECT_TRUE(tx_state_manager_->GetTx(mojom::kMainnetChainId, "1"));
  EthTxMeta meta1001(eth_account_id_, std::make_unique<EthTransaction>());
  meta1001.set_id("1001");
  meta1001.set_chain_id(mojom::kMainnetChainId);
  meta1001.set_status(mojom::TransactionStatus::Rejected);
  meta1001.set_created_time(base::Time::Now());
  ASSERT_TRUE(tx_state_manager_->AddOrUpdateTx(meta1001));
  EXPECT_FALSE(tx_state_manager_->GetTx(mojom::kMainnetChainId, "1"));

  // Other status doesn't matter
  EXPECT_TRUE(tx_state_manager_->GetTx(mojom::kMainnetChainId, "2"));
  EXPECT_TRUE(tx_state_manager_->GetTx(mojom::kMainnetChainId, "3"));
  EthTxMeta meta1002(eth_account_id_, std::make_unique<EthTransaction>());
  meta1002.set_id("1002");
  meta1002.set_chain_id(mojom::kMainnetChainId);
  meta1002.set_status(mojom::TransactionStatus::Submitted);
  meta1002.set_created_time(base::Time::Now());
  ASSERT_TRUE(tx_state_manager_->AddOrUpdateTx(meta1002));
  EXPECT_TRUE(tx_state_manager_->GetTx(mojom::kMainnetChainId, "2"));
  EXPECT_TRUE(tx_state_manager_->GetTx(mojom::kMainnetChainId, "3"));

  // Other chain id doesn't matter
  EthTxMeta meta1003(eth_account_id_, std::make_unique<EthTransaction>());
  meta1003.set_id("1003");
  meta1003.set_chain_id(mojom::kGoerliChainId);
  meta1003.set_status(mojom::TransactionStatus::Confirmed);
  meta1003.set_created_time(base::Time::Now());
  ASSERT_TRUE(tx_state_manager_->AddOrUpdateTx(meta1003));
  EXPECT_TRUE(tx_state_manager_->GetTx(mojom::kMainnetChainId, "2"));
  EXPECT_TRUE(tx_state_manager_->GetTx(mojom::kMainnetChainId, "3"));
}

TEST_F(TxStateManagerUnitTest, Observer) {
  MockTxStateManagerObserver observer(tx_state_manager_.get());

  EthTxMeta meta(eth_account_id_, std::make_unique<EthTransaction>());
  meta.set_id("001");
  // Add
  EXPECT_CALL(observer,
              OnNewUnapprovedTx(EqualsMojo(meta.ToTransactionInfo())));
  EXPECT_CALL(observer, OnTransactionStatusChanged(_)).Times(0);
  ASSERT_TRUE(tx_state_manager_->AddOrUpdateTx(meta));
  EXPECT_TRUE(testing::Mock::VerifyAndClearExpectations(&observer));

  // Modify
  meta.set_status(mojom::TransactionStatus::Approved);
  EXPECT_CALL(observer, OnNewUnapprovedTx(_)).Times(0);
  EXPECT_CALL(observer,
              OnTransactionStatusChanged(EqualsMojo(meta.ToTransactionInfo())))
      .Times(1);
  ASSERT_TRUE(tx_state_manager_->AddOrUpdateTx(meta));
  EXPECT_TRUE(testing::Mock::VerifyAndClearExpectations(&observer));
}

TEST_F(TxStateManagerUnitTest,
       MigrateSolanaTransactionsForV0TransactionsSupport) {
  ASSERT_FALSE(
      prefs_.GetBoolean(kBraveWalletSolanaTransactionsV0SupportMigrated));
  base::Value txs_value = ParseJson(R"(
    {
      "solana": {
        "devnet": {
          "tx_id1": {
            "tx": {
              "message": {
                "fee_payer": "3Lu176FQzbQJCc8iL9PnmALbpMPhZeknoturApnXRDJw",
                "instructions": [
                  {
                    "accounts": [],
                    "data": "SGVsbG8sIGZyb20gdGhlIFNvbGFuYSBXYWxsZXQgQWRhcHRlciBleGFtcGxlIGFwcCE=",
                    "program_id": "MemoSq4gqABAXKb96qnH8TysNcWxMyWCqXgDLGmfcHr"
                  }
                ],
                "last_valid_block_height": "0",
                "recent_blockhash":
                    "GZH3GWCMxU9aZbai9L8pA3aTWsBVaCwYxiWfhtnMhUbb"
              }
            }
          }
        },
        "mainnet": {
          "tx_id2": {
            "tx": {
              "message": {
                "fee_payer": "3Lu176FQzbQJCc8iL9PnmALbpMPhZeknoturApnXRDJw",
                "instructions": [
                  {
                    "accounts": [
                      {
                        "is_signer": true,
                        "is_writable": true,
                        "pubkey": "3Lu176FQzbQJCc8iL9PnmALbpMPhZeknoturApnXRDJw"
                      },
                      {
                        "is_signer": true,
                        "is_writable": true,
                        "pubkey": "3Lu176FQzbQJCc8iL9PnmALbpMPhZeknoturApnXRDJw"
                      }
                    ],
                    "data": "AgAAAGQAAAAAAAAA",
                    "decoded_data": {
                      "account_params": [
                        {
                          "localized_name": "From Account",
                          "name": "from_account"
                        },
                        {
                          "localized_name": "To Account",
                          "name": "to_account"
                        }
                      ],
                      "params": [
                        {
                          "localized_name": "Lamports",
                          "name": "lamports",
                          "type": 2,
                          "value": "100"
                        }
                      ],
                      "sys_ins_type": "2"
                    },
                    "program_id": "11111111111111111111111111111111"
                  }
                ],
                "last_valid_block_height": "0",
                "recent_blockhash":
                    "AARGss1frfvBSKqYXLHuv3i4kzbQrHhabubcF2KFTE2S"
              }
            }
          },
          "tx_id3": {
            "tx": {
              "message": {
                "fee_payer": "3Lu176FQzbQJCc8iL9PnmALbpMPhZeknoturApnXRDJw",
                "instructions": [
                  {
                    "accounts": [],
                    "data": "SGVsbG8sIGZyb20gdGhlIFNvbGFuYSBXYWxsZXQgQWRhcHRlciBleGFtcGxlIGFwcCE=",
                    "program_id": "MemoSq4gqABAXKb96qnH8TysNcWxMyWCqXgDLGmfcHr"
                  }
                ],
                "last_valid_block_height": "0",
                "recent_blockhash":
                    "GZH3GWCMxU9aZbai9L8pA3aTWsBVaCwYxiWfhtnMhUbb"
              }
            }
          }
        }
      }
    })");

  prefs_.Set(kBraveWalletTransactions, txs_value);
  TxStateManager::MigrateSolanaTransactionsForV0TransactionsSupport(&prefs_);
  base::Value::Dict msg1 = ParseJsonDict(R"({
      "version": 0,
      "fee_payer": "3Lu176FQzbQJCc8iL9PnmALbpMPhZeknoturApnXRDJw",
      "message_header": {
        "num_readonly_signed_accounts": "0",
        "num_readonly_unsigned_accounts": "1",
        "num_required_signatures": "1"
      },
      "static_account_keys": [
        "3Lu176FQzbQJCc8iL9PnmALbpMPhZeknoturApnXRDJw",
        "MemoSq4gqABAXKb96qnH8TysNcWxMyWCqXgDLGmfcHr"
      ],
      "address_table_lookups": [],
      "instructions": [
        {
          "accounts": [],
          "data": "SGVsbG8sIGZyb20gdGhlIFNvbGFuYSBXYWxsZXQgQWRhcHRlciBleGFtcGxlIGFwcCE=",
          "program_id": "MemoSq4gqABAXKb96qnH8TysNcWxMyWCqXgDLGmfcHr"
        }
      ],
      "last_valid_block_height": "0",
      "recent_blockhash": "GZH3GWCMxU9aZbai9L8pA3aTWsBVaCwYxiWfhtnMhUbb"
  })");
  base::Value::Dict msg2 = ParseJsonDict(R"({
      "version": 0,
      "fee_payer": "3Lu176FQzbQJCc8iL9PnmALbpMPhZeknoturApnXRDJw",
      "message_header": {
        "num_readonly_signed_accounts": "0",
        "num_readonly_unsigned_accounts": "1",
        "num_required_signatures": "1"
      },
      "static_account_keys": [
        "3Lu176FQzbQJCc8iL9PnmALbpMPhZeknoturApnXRDJw",
        "11111111111111111111111111111111"
      ],
      "address_table_lookups": [],
      "instructions": [
        {
          "accounts": [
            {
              "is_signer": true,
              "is_writable": true,
              "pubkey": "3Lu176FQzbQJCc8iL9PnmALbpMPhZeknoturApnXRDJw"
            },
            {
              "is_signer": true,
              "is_writable": true,
              "pubkey": "3Lu176FQzbQJCc8iL9PnmALbpMPhZeknoturApnXRDJw"
            }
          ],
          "data": "AgAAAGQAAAAAAAAA",
          "decoded_data": {
            "account_params": [
              {
                "localized_name": "From Account",
                "name": "from_account"
              },
              {
                "localized_name": "To Account",
                "name": "to_account"
              }
            ],
            "params": [
              {
                "localized_name": "Lamports",
                "name": "lamports",
                "type": 2,
                "value": "100"
              }
            ],
            "sys_ins_type": "2"
          },
          "program_id": "11111111111111111111111111111111"
        }
      ],
      "last_valid_block_height": "0",
      "recent_blockhash": "AARGss1frfvBSKqYXLHuv3i4kzbQrHhabubcF2KFTE2S"
  })");
  EXPECT_EQ(*prefs_.GetDict(kBraveWalletTransactions)
                 .FindDictByDottedPath("solana.devnet.tx_id1.tx.message"),
            msg1);
  EXPECT_EQ(*prefs_.GetDict(kBraveWalletTransactions)
                 .FindDictByDottedPath("solana.mainnet.tx_id2.tx.message"),
            msg2);
  EXPECT_EQ(*prefs_.GetDict(kBraveWalletTransactions)
                 .FindDictByDottedPath("solana.mainnet.tx_id3.tx.message"),
            msg1);

  EXPECT_TRUE(
      prefs_.GetBoolean(kBraveWalletSolanaTransactionsV0SupportMigrated));
}

}  // namespace brave_wallet
