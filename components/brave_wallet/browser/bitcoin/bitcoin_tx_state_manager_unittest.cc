/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/bitcoin/bitcoin_tx_state_manager.h"

#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include "base/files/scoped_temp_dir.h"
#include "base/test/task_environment.h"
#include "brave/components/brave_wallet/browser/bitcoin/bitcoin_serializer.h"
#include "brave/components/brave_wallet/browser/bitcoin/bitcoin_transaction.h"
#include "brave/components/brave_wallet/browser/bitcoin/bitcoin_tx_meta.h"
#include "brave/components/brave_wallet/browser/brave_wallet_prefs.h"
#include "brave/components/brave_wallet/browser/test_utils.h"
#include "brave/components/brave_wallet/browser/tx_storage_delegate_impl.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

class BitcoinTxStateManagerUnitTest : public testing::Test {
 public:
  BitcoinTxStateManagerUnitTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}
  ~BitcoinTxStateManagerUnitTest() override = default;

 protected:
  void SetUp() override {
    RegisterProfilePrefs(prefs_.registry());
    RegisterProfilePrefsForMigration(prefs_.registry());
    factory_ = GetTestValueStoreFactory(temp_dir_);
    delegate_ = GetTxStorageDelegateForTest(GetPrefs(), factory_);
    account_resolver_delegate_ =
        std::make_unique<AccountResolverDelegateForTest>();
    bitcoin_tx_state_manager_ = std::make_unique<BitcoinTxStateManager>(
        *delegate_, *account_resolver_delegate_);
  }

  PrefService* GetPrefs() { return &prefs_; }

  base::test::TaskEnvironment task_environment_;
  base::ScopedTempDir temp_dir_;
  scoped_refptr<value_store::TestValueStoreFactory> factory_;
  std::unique_ptr<TxStorageDelegateImpl> delegate_;
  std::unique_ptr<AccountResolverDelegateForTest> account_resolver_delegate_;
  sync_preferences::TestingPrefServiceSyncable prefs_;
  std::unique_ptr<BitcoinTxStateManager> bitcoin_tx_state_manager_;
};

TEST_F(BitcoinTxStateManagerUnitTest, BitcoinTxMetaAndValue) {
  auto btc_account_id =
      account_resolver_delegate_->RegisterAccount(MakeIndexBasedAccountId(
          mojom::CoinType::BTC, mojom::KeyringId::kBitcoin84,
          mojom::AccountKind::kDerived, 1));

  std::unique_ptr<BitcoinTransaction> tx =
      std::make_unique<BitcoinTransaction>();
  tx->set_amount(200000);
  tx->set_to("tb1qva8clyftt2fstawn5dy0nvrfmygpzulf3lwulm");

  BitcoinTransaction::TxInput input;
  input.utxo_address = "tb1q56kslnp386v43wpp6wkpx072ryud5gu865efx8";
  input.utxo_value = 200000;
  tx->AddInput(std::move(input));

  BitcoinTransaction::TxOutput output;
  output.address = "tb1qva8clyftt2fstawn5dy0nvrfmygpzulf3lwulm";
  output.script_pubkey = BitcoinSerializer::AddressToScriptPubkey(
      "tb1qva8clyftt2fstawn5dy0nvrfmygpzulf3lwulm", true);
  output.amount = 200000 - 1000;
  tx->AddOutput(std::move(output));

  BitcoinTxMeta meta(btc_account_id, std::move(tx));
  meta.set_id(TxMeta::GenerateMetaID());
  meta.set_status(mojom::TransactionStatus::Submitted);
  meta.set_created_time(base::Time::Now());
  meta.set_submitted_time(base::Time::Now());
  meta.set_confirmed_time(base::Time::Now());
  meta.set_tx_hash(
      "aa388f50b725767653e150ad8990ec11a2146d75acafbe492af08213849fe2c5");
  meta.set_origin(url::Origin::Create(GURL("https://test.brave.com/")));
  meta.set_chain_id(mojom::kBitcoinTestnet);

  base::Value::Dict meta_value = meta.ToValue();
  auto meta_from_value =
      bitcoin_tx_state_manager_->ValueToBitcoinTxMeta(meta_value);
  ASSERT_TRUE(meta_from_value);
  EXPECT_EQ(*meta_from_value, meta);
}

}  // namespace brave_wallet
