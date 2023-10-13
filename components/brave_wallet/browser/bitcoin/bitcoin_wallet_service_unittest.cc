/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/bitcoin/bitcoin_wallet_service.h"

#include <memory>
#include <string>

#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/test/mock_callback.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "base/types/expected.h"
#include "brave/components/brave_wallet/browser/bitcoin/bitcoin_serializer.h"
#include "brave/components/brave_wallet/browser/bitcoin/bitcoin_test_utils.h"
#include "brave/components/brave_wallet/browser/brave_wallet_prefs.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/test_utils.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/features.h"
#include "brave/components/brave_wallet/common/test_utils.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "services/data_decoder/public/cpp/test_support/in_process_data_decoder.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::_;
using testing::Eq;
using testing::SaveArg;
using testing::Truly;
using testing::WithArg;

namespace brave_wallet {

namespace bitcoin_rpc {
bool operator==(const UnspentOutput& l, const UnspentOutput& r) {
  return l.status.confirmed == r.status.confirmed && l.txid == r.txid &&
         l.value == r.value && l.vout == r.vout;
}

}  // namespace bitcoin_rpc

// TOOD(apaymyshev): cover failure scenarios for BitcoinWalletService with tests
class BitcoinWalletServiceUnitTest : public testing::Test {
 public:
  BitcoinWalletServiceUnitTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}

  ~BitcoinWalletServiceUnitTest() override = default;

  void SetUp() override {
    brave_wallet::RegisterProfilePrefs(prefs_.registry());
    brave_wallet::RegisterLocalStatePrefs(local_state_.registry());

    keyring_service_ =
        std::make_unique<KeyringService>(nullptr, &prefs_, &local_state_);
    bitcoin_test_rpc_server_ =
        std::make_unique<BitcoinTestRpcServer>(keyring_service_.get(), &prefs_);
    bitcoin_wallet_service_ = std::make_unique<BitcoinWalletService>(
        keyring_service_.get(), &prefs_,
        bitcoin_test_rpc_server_->GetURLLoaderFactory());

    keyring_service_->CreateWallet(kMnemonicDivideCruise, kTestWalletPassword,
                                   base::DoNothing());

    btc_account_ =
        GetAccountUtils().EnsureAccount(mojom::KeyringId::kBitcoin84, 0);
    ASSERT_TRUE(btc_account_);
    bitcoin_test_rpc_server_->SetUpBitcoinRpc(btc_account_->account_id);
    keyring_service_->UpdateNextUnusedAddressForBitcoinAccount(
        btc_account_->account_id, mojom::BitcoinKeyId::New(0, 5));
    keyring_service_->UpdateNextUnusedAddressForBitcoinAccount(
        btc_account_->account_id, mojom::BitcoinKeyId::New(1, 5));
  }

  AccountUtils GetAccountUtils() {
    return AccountUtils(keyring_service_.get());
  }

  mojom::AccountIdPtr account_id() const {
    return btc_account_->account_id.Clone();
  }

 protected:
  base::test::ScopedFeatureList feature_list_{
      features::kBraveWalletBitcoinFeature};

  mojom::AccountInfoPtr btc_account_;

  base::test::TaskEnvironment task_environment_;
  sync_preferences::TestingPrefServiceSyncable prefs_;
  sync_preferences::TestingPrefServiceSyncable local_state_;

  std::unique_ptr<BitcoinTestRpcServer> bitcoin_test_rpc_server_;
  std::unique_ptr<KeyringService> keyring_service_;
  std::unique_ptr<BitcoinWalletService> bitcoin_wallet_service_;
  data_decoder::test::InProcessDataDecoder in_process_data_decoder_;
};

TEST_F(BitcoinWalletServiceUnitTest, GetBalance) {
  base::MockCallback<BitcoinWalletService::GetBalanceCallback> callback;

  // GetBalance works.
  auto expected_balance = mojom::BitcoinBalance::New();
  auto addresses = keyring_service_->GetBitcoinAddresses(account_id());
  for (const auto& addr : *addresses) {
    expected_balance->balances[addr->address_string] = 0;
  }

  auto address_0 = bitcoin_test_rpc_server_->Address0();
  auto address_6 = bitcoin_test_rpc_server_->Address6();

  expected_balance->balances[address_0] = 10000 - 5000 + 8888 - 2222;
  expected_balance->balances[address_6] = 100000 - 50000 + 88888 - 22222;
  expected_balance->total_balance = expected_balance->balances[address_0] +
                                    expected_balance->balances[address_6];
  EXPECT_CALL(callback,
              Run(EqualsMojo(expected_balance), absl::optional<std::string>()));
  bitcoin_wallet_service_->GetBalance(account_id(), callback.Get());
  base::RunLoop().RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&callback);
}

TEST_F(BitcoinWalletServiceUnitTest, GetBitcoinAccountInfo) {
  base::MockCallback<BitcoinWalletService::GetBitcoinAccountInfoCallback>
      callback;

  auto expected_bitcoin_accont_info = mojom::BitcoinAccountInfo::New();
  expected_bitcoin_accont_info->next_receive_address =
      mojom::BitcoinAddress::New("bc1qe68jzwhglrs9lm0zf8ddqvzrdcxeg8ej5nd0rc",
                                 mojom::BitcoinKeyId::New(0, 5));
  expected_bitcoin_accont_info->next_change_address =
      mojom::BitcoinAddress::New("bc1q9khch2y932xktwxxzplvaxw6r7h0pw2yeelvj7",
                                 mojom::BitcoinKeyId::New(1, 5));
  auto& stats_map = bitcoin_test_rpc_server_->address_stats_map();
  for (auto i = 0; i < 7; ++i) {
    auto addr = keyring_service_->GetBitcoinAddress(
        account_id(), mojom::BitcoinKeyId::New(0, 5 + i));
    if (i == 6) {
      stats_map[addr->address_string] =
          BitcoinTestRpcServer::EmptyAddressStats(addr->address_string);
    } else {
      stats_map[addr->address_string] =
          BitcoinTestRpcServer::TransactedAddressStats(addr->address_string);
    }
  }

  EXPECT_CALL(callback, Run(EqualsMojo(expected_bitcoin_accont_info)));
  bitcoin_wallet_service_->GetBitcoinAccountInfo(account_id(), callback.Get());
  base::RunLoop().RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&callback);
}

TEST_F(BitcoinWalletServiceUnitTest, RunDiscovery) {
  base::MockCallback<BitcoinWalletService::RunDiscoveryCallback> callback;
  auto bitcoin_account_info =
      bitcoin_wallet_service_->GetBitcoinAccountInfoSync(account_id());
  EXPECT_EQ(bitcoin_account_info->next_receive_address->key_id,
            mojom::BitcoinKeyId::New(0, 5));
  EXPECT_EQ(bitcoin_account_info->next_change_address->key_id,
            mojom::BitcoinKeyId::New(1, 5));

  auto& stats_map = bitcoin_test_rpc_server_->address_stats_map();

  for (auto i = 0; i < 23; ++i) {
    auto receive_addr = keyring_service_->GetBitcoinAddress(
        account_id(), mojom::BitcoinKeyId::New(0, 5 + i));
    if (i == 5) {  // (0, 5 + 5) will become next receive address
      stats_map[receive_addr->address_string] =
          BitcoinTestRpcServer::EmptyAddressStats(receive_addr->address_string);
    } else {
      stats_map[receive_addr->address_string] =
          BitcoinTestRpcServer::TransactedAddressStats(
              receive_addr->address_string);
    }

    auto change_addr = keyring_service_->GetBitcoinAddress(
        account_id(), mojom::BitcoinKeyId::New(1, 5 + i));
    if (i == 3) {  // (0, 5 + 3) will become next change address
      stats_map[change_addr->address_string] =
          BitcoinTestRpcServer::EmptyAddressStats(change_addr->address_string);
    } else {
      stats_map[change_addr->address_string] =
          BitcoinTestRpcServer::TransactedAddressStats(
              change_addr->address_string);
    }
  }

  auto expected_receive_address = keyring_service_->GetBitcoinAddress(
      account_id(), mojom::BitcoinKeyId::New(0, 10));
  EXPECT_CALL(callback, Run(Eq(std::ref(expected_receive_address)),
                            absl::optional<std::string>()));
  bitcoin_wallet_service_->RunDiscovery(account_id(), false, callback.Get());
  base::RunLoop().RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&callback);

  bitcoin_account_info =
      bitcoin_wallet_service_->GetBitcoinAccountInfoSync(account_id());
  EXPECT_EQ(bitcoin_account_info->next_receive_address->key_id,
            mojom::BitcoinKeyId::New(0, 10));
  EXPECT_EQ(bitcoin_account_info->next_change_address->key_id,
            mojom::BitcoinKeyId::New(1, 5));

  auto expected_change_address = keyring_service_->GetBitcoinAddress(
      account_id(), mojom::BitcoinKeyId::New(1, 8));
  EXPECT_CALL(callback, Run(Eq(std::ref(expected_change_address)),
                            absl::optional<std::string>()));
  bitcoin_wallet_service_->RunDiscovery(account_id(), true, callback.Get());
  base::RunLoop().RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&callback);

  bitcoin_account_info =
      bitcoin_wallet_service_->GetBitcoinAccountInfoSync(account_id());
  EXPECT_EQ(bitcoin_account_info->next_receive_address->key_id,
            mojom::BitcoinKeyId::New(0, 10));
  EXPECT_EQ(bitcoin_account_info->next_change_address->key_id,
            mojom::BitcoinKeyId::New(1, 8));
}

TEST_F(BitcoinWalletServiceUnitTest, GetUtxos) {
  using GetUtxosResult =
      base::expected<BitcoinWalletService::UtxoMap, std::string>;
  base::MockCallback<BitcoinWalletService::GetUtxosCallback> callback;

  // GetUtxos works.
  BitcoinWalletService::UtxoMap expected_utxos;
  auto addresses = keyring_service_->GetBitcoinAddresses(account_id());
  for (auto& addr : *addresses) {
    expected_utxos[addr->address_string].clear();
  }
  auto& utxo_0 =
      expected_utxos[bitcoin_test_rpc_server_->Address0()].emplace_back();
  utxo_0.txid = kMockBtcTxid1;
  utxo_0.vout = "1";
  utxo_0.value = "5000";
  utxo_0.status.confirmed = true;
  auto& utxo_6 =
      expected_utxos[bitcoin_test_rpc_server_->Address6()].emplace_back();
  utxo_6.txid = kMockBtcTxid2;
  utxo_6.vout = "7";
  utxo_6.value = "50000";
  utxo_6.status.confirmed = true;
  EXPECT_EQ(expected_utxos.size(), 10u);

  GetUtxosResult actual_utxos;
  EXPECT_CALL(callback, Run(Truly([&](const GetUtxosResult& arg) {
                EXPECT_TRUE(arg.has_value());
                EXPECT_EQ(arg.value(), expected_utxos);
                return true;
              })));
  bitcoin_wallet_service_->GetUtxos(account_id(), callback.Get());
  base::RunLoop().RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&callback);
}

TEST_F(BitcoinWalletServiceUnitTest, CreateTransaction) {
  using CreateTransactionResult =
      base::expected<BitcoinTransaction, std::string>;
  base::MockCallback<BitcoinWalletService::CreateTransactionCallback> callback;

  BitcoinTransaction actual_tx;
  EXPECT_CALL(callback, Run(Truly([&](const CreateTransactionResult& arg) {
                EXPECT_TRUE(arg.has_value());
                actual_tx = arg.value().Clone();
                return true;
              })));
  bitcoin_wallet_service_->CreateTransaction(account_id(), kMockBtcAddress,
                                             6000, callback.Get());
  base::RunLoop().RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&callback);

  EXPECT_EQ(actual_tx.locktime(), 12345u);
  EXPECT_EQ(actual_tx.amount(), 6000u);
  EXPECT_EQ(actual_tx.to(), kMockBtcAddress);

  EXPECT_EQ(actual_tx.EffectiveFeeAmount(), 4878u);  // 23.456*208
  EXPECT_EQ(actual_tx.TotalInputsAmount(), 50000u + 5000u);
  EXPECT_EQ(actual_tx.TotalOutputsAmount(), 50000u + 5000u - 4878u);

  EXPECT_EQ(actual_tx.inputs().size(), 2u);
  auto& input_0 = actual_tx.inputs().at(0);
  EXPECT_EQ(input_0.utxo_address, bitcoin_test_rpc_server_->Address0());
  EXPECT_EQ(base::HexEncode(input_0.utxo_outpoint.txid),
            base::ToUpperASCII(kMockBtcTxid1));
  EXPECT_EQ(input_0.utxo_outpoint.index, 1u);
  EXPECT_EQ(input_0.utxo_value, 5000u);
  EXPECT_TRUE(input_0.script_sig.empty());
  EXPECT_TRUE(input_0.witness.empty());

  auto& input_1 = actual_tx.inputs().at(1);
  EXPECT_EQ(input_1.utxo_address, bitcoin_test_rpc_server_->Address6());
  EXPECT_EQ(base::HexEncode(input_1.utxo_outpoint.txid),
            base::ToUpperASCII(kMockBtcTxid2));
  EXPECT_EQ(input_1.utxo_outpoint.index, 7u);
  EXPECT_EQ(input_1.utxo_value, 50000u);
  EXPECT_TRUE(input_1.script_sig.empty());
  EXPECT_TRUE(input_1.witness.empty());

  EXPECT_EQ(actual_tx.outputs().size(), 2u);

  auto& output_0 = actual_tx.outputs().at(0);
  EXPECT_EQ(output_0.address, kMockBtcAddress);
  EXPECT_EQ(base::HexEncode(output_0.script_pubkey),
            "0014751E76E8199196D454941C45D1B3A323F1433BD6");
  EXPECT_EQ(output_0.amount, 6000u);

  auto& output_1 = actual_tx.outputs().at(1);
  EXPECT_EQ(output_1.address,
            keyring_service_->GetBitcoinAccountInfo(account_id())
                ->next_change_address->address_string);
  EXPECT_EQ(output_1.amount, 50000u + 5000u - 6000u - 4878u);
  EXPECT_EQ(base::HexEncode(output_1.script_pubkey),
            "00142DAF8BA8858A8D65B8C6107ECE99DA1FAEF0B944");
}

TEST_F(BitcoinWalletServiceUnitTest, SignAndPostTransaction) {
  using CreateTransactionResult =
      base::expected<BitcoinTransaction, std::string>;
  base::MockCallback<BitcoinWalletService::CreateTransactionCallback> callback;

  base::MockCallback<BitcoinWalletService::SignAndPostTransactionCallback>
      sign_callback;

  BitcoinTransaction initial_tx;
  EXPECT_CALL(callback, Run(Truly([&](const CreateTransactionResult& arg) {
                EXPECT_TRUE(arg.has_value());
                initial_tx = arg.value().Clone();
                return true;
              })));
  bitcoin_wallet_service_->CreateTransaction(account_id(), kMockBtcAddress,
                                             6000, callback.Get());
  base::RunLoop().RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&callback);

  BitcoinTransaction signed_tx;
  EXPECT_CALL(sign_callback, Run(kMockBtcTxid3, _, ""))
      .WillOnce(WithArg<1>(
          [&](const BitcoinTransaction& tx) { signed_tx = tx.Clone(); }));
  bitcoin_wallet_service_->SignAndPostTransaction(
      account_id(), std::move(initial_tx), sign_callback.Get());
  base::RunLoop().RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&sign_callback);

  EXPECT_EQ(BitcoinSerializer::CalcTransactionWeight(signed_tx), 832u);
  EXPECT_EQ(BitcoinSerializer::CalcVSize(signed_tx), 208u);
  EXPECT_EQ(signed_tx.EffectiveFeeAmount(), 4878u);
  EXPECT_EQ(signed_tx.TotalInputsAmount(), 50000u + 5000u);
  EXPECT_EQ(signed_tx.TotalOutputsAmount(), 50000u + 5000u - 4878u);

  EXPECT_EQ(
      bitcoin_test_rpc_server_->captured_raw_tx(),
      "02000000000102C5E29F841382F02A49BEAFAC756D14A211EC9089AD50E153767625B750"
      "8F38AA0100000000FDFFFFFFBEC2C52B2448A8733648E967D2B4559D0F1AA4BBBB93E53E"
      "9F516A12FB9C1CBD0700000000FDFFFFFF027017000000000000160014751E76E8199196"
      "D454941C45D1B3A323F1433BD65AAC0000000000001600142DAF8BA8858A8D65B8C6107E"
      "CE99DA1FAEF0B944024730440220782626C48EBDD79FEB9A015C68727BAF4CB059F9568A"
      "959C6BB3D2B9655566FA02204A7A58F9A52D0D4F1B632A98F481303746DF675CC65D8453"
      "D06EF298774253B20121028256AD805CC35647890DEFD92AE6EF9BE31BA254E7E7D2834F"
      "8C403766C65FE702473044022072F292CD14269608D0D25F068E5D587DCB4CB4FDEB9679"
      "922F95AAEEE1A30EED02202510CA7CDF4D49C448162BF5FB0F39FC4BEF244830CDABA966"
      "D73C315DA8899A0121038F616FB0894BD77263DA0111E3BAB673AB9B77309FD724717797"
      "5698FEB2CDDE39300000");
}

}  // namespace brave_wallet
