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
#include "brave/components/brave_wallet/browser/brave_wallet_prefs.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/test_utils.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/features.h"
#include "brave/components/brave_wallet/common/test_utils.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "services/data_decoder/public/cpp/test_support/in_process_data_decoder.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::_;
using testing::SaveArg;
using testing::Truly;
using testing::WithArg;

namespace brave_wallet {
namespace {
const char kTxid1[] =
    "aa388f50b725767653e150ad8990ec11a2146d75acafbe492af08213849fe2c5";
const char kTxid2[] =
    "bd1c9cfb126a519f3ee593bbbba41a0f9d55b4d267e9483673a848242bc5c2be";
const char kTxid3[] =
    "f4024cb219b898ed51a5c2a2d0589c1de4bb35e329ad15ab08b6ac9ffcc95ae2";
const char kAddress[] = "bc1qw508d6qejxtdg4y5r3zarvary0c5xw7kv8f3t4";
}  // namespace

namespace bitcoin_rpc {
bool operator==(const UnspentOutput& l, const UnspentOutput& r) {
  return true;
}

}  // namespace bitcoin_rpc

// TOOD(apaymyshev): cover failure scenarios for BitcoinWalletService with tests
class BitcoinWalletServiceUnitTest : public testing::Test {
 public:
  BitcoinWalletServiceUnitTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME),
        shared_url_loader_factory_(
            base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
                &url_loader_factory_)) {}

  ~BitcoinWalletServiceUnitTest() override = default;

  void SetUp() override {
    brave_wallet::RegisterProfilePrefs(prefs_.registry());
    brave_wallet::RegisterLocalStatePrefs(local_state_.registry());

    keyring_service_ =
        std::make_unique<KeyringService>(nullptr, &prefs_, &local_state_);
    bitcoin_wallet_service_ = std::make_unique<BitcoinWalletService>(
        keyring_service_.get(), &prefs_, shared_url_loader_factory_);

    url_loader_factory_.SetInterceptor(
        base::BindRepeating(&BitcoinWalletServiceUnitTest::RequestInterceptor,
                            base::Unretained(this)));
    keyring_service_->CreateWallet(kMnemonicDivideCruise, kTestWalletPassword,
                                   base::DoNothing());

    auto btc_mainnet =
        GetKnownChain(&prefs_, mojom::kBitcoinMainnet, mojom::CoinType::BTC);
    btc_mainnet->rpc_endpoints[0] = GURL(mainnet_rpc_url_);
    AddCustomNetwork(&prefs_, *btc_mainnet);
    auto btc_testnet =
        GetKnownChain(&prefs_, mojom::kBitcoinTestnet, mojom::CoinType::BTC);
    btc_testnet->rpc_endpoints[0] = GURL(testnet_rpc_url_);
    AddCustomNetwork(&prefs_, *btc_testnet);

    btc_account_ =
        GetAccountUtils().EnsureAccount(mojom::KeyringId::kBitcoin84, 0);
    ASSERT_TRUE(btc_account_);

    SetUpBitcoinRpc();
  }

  void SetUpBitcoinRpc() {
    auto addresses =
        keyring_service_->GetBitcoinAddresses(*btc_account_->account_id);
    ASSERT_TRUE(addresses);
    EXPECT_EQ(addresses->size(), 5u + 5u);
    for (auto& address : *addresses) {
      auto& stats = address_stats_map_[address.first];
      stats.address = address.first;
      stats.chain_stats.tx_count = "0";
      stats.chain_stats.funded_txo_sum = "0";
      stats.chain_stats.spent_txo_sum = "0";
      stats.mempool_stats.tx_count = "0";
      stats.mempool_stats.funded_txo_sum = "0";
      stats.mempool_stats.spent_txo_sum = "0";
    }

    address_0_ = addresses->at(0).first;
    auto& stats_0 = address_stats_map_[address_0_];
    stats_0.address = address_0_;
    stats_0.chain_stats.funded_txo_sum = "10000";
    stats_0.chain_stats.spent_txo_sum = "5000";
    stats_0.mempool_stats.funded_txo_sum = "8888";
    stats_0.mempool_stats.spent_txo_sum = "2222";

    address_6_ = addresses->at(6).first;
    auto& stats_6 = address_stats_map_[address_6_];
    stats_6.address = address_6_;
    stats_6.chain_stats.funded_txo_sum = "100000";
    stats_6.chain_stats.spent_txo_sum = "50000";
    stats_6.mempool_stats.funded_txo_sum = "88888";
    stats_6.mempool_stats.spent_txo_sum = "22222";

    for (auto& address : *addresses) {
      utxos_map_[address.first].clear();
    }
    auto& utxos_0 = utxos_map_[address_0_];
    utxos_0.emplace_back();
    utxos_0.back().txid = kTxid1;
    utxos_0.back().vout = "1";
    utxos_0.back().value = "5000";
    utxos_0.back().status.confirmed = true;
    auto& utxos_6 = utxos_map_[address_6_];
    utxos_6.emplace_back();
    utxos_6.back().txid = kTxid2;
    utxos_6.back().vout = "7";
    utxos_6.back().value = "50000";
    utxos_6.back().status.confirmed = true;
  }

  AccountUtils GetAccountUtils() {
    return AccountUtils(keyring_service_.get());
  }

  void RequestInterceptor(const network::ResourceRequest& request) {
    url_loader_factory_.ClearResponses();

    if (request.method == net::HttpRequestHeaders::kPostMethod &&
        request.url.path_piece() == "/tx") {
      base::StringPiece request_string(request.request_body->elements()
                                           ->at(0)
                                           .As<network::DataElementBytes>()
                                           .AsStringPiece());
      captured_raw_tx_ = request_string;
      url_loader_factory_.AddResponse(request.url.spec(), kTxid3);
      return;
    }

    if (request.url.path_piece() == "/blocks/tip/height") {
      url_loader_factory_.AddResponse(request.url.spec(),
                                      base::ToString(mainnet_height_));
      return;
    }

    for (auto& item : address_stats_map_) {
      if (request.url.path_piece() == "/address/" + item.first) {
        url_loader_factory_.AddResponse(request.url.spec(),
                                        base::ToString(item.second.ToValue()));
        return;
      }
    }
    for (auto& item : utxos_map_) {
      if (request.url.path_piece() == "/address/" + item.first + "/utxo") {
        base::Value::List items;
        for (auto& utxo : item.second) {
          items.Append(utxo.ToValue());
        }
        url_loader_factory_.AddResponse(request.url.spec(),
                                        base::ToString(items));
        return;
      }
    }

    url_loader_factory_.AddResponse(request.url.spec(), "",
                                    net::HTTP_NOT_FOUND);
    // base::StringPiece request_string(request.request_body->elements()
    //                                      ->at(0)
    //                                      .As<network::DataElementBytes>()
    //                                      .AsStringPiece());
    // base::Value::Dict dict = base::test::ParseJsonDict(request_string);
  }

  mojom::AccountIdPtr account_id() const {
    return btc_account_->account_id.Clone();
  }

 protected:
  base::test::ScopedFeatureList feature_list_{
      features::kBraveWalletBitcoinFeature};

  std::string mainnet_rpc_url_ = "https://btc-mainnet.com/";
  std::string testnet_rpc_url_ = "https://btc-testnet.com/";
  mojom::AccountInfoPtr btc_account_;
  uint32_t mainnet_height_ = 12345;
  std::string address_0_;
  std::string address_6_;
  std::map<std::string, bitcoin_rpc::AddressStats> address_stats_map_;
  std::map<std::string, std::vector<bitcoin_rpc::UnspentOutput>> utxos_map_;
  std::string captured_raw_tx_;

  base::test::TaskEnvironment task_environment_;
  sync_preferences::TestingPrefServiceSyncable prefs_;
  sync_preferences::TestingPrefServiceSyncable local_state_;

  network::TestURLLoaderFactory url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
  std::unique_ptr<KeyringService> keyring_service_;
  std::unique_ptr<BitcoinWalletService> bitcoin_wallet_service_;
  data_decoder::test::InProcessDataDecoder in_process_data_decoder_;
};

TEST_F(BitcoinWalletServiceUnitTest, GetBalance) {
  base::MockCallback<BitcoinWalletService::GetBalanceCallback> callback;

  // GetBalance works.
  auto expected_balance = mojom::BitcoinBalance::New();
  expected_balance->total_balance = 128332;
  for (auto& addr : address_stats_map_) {
    expected_balance->balances[addr.first] = 0;
  }
  expected_balance->balances[address_0_] = 11666;
  expected_balance->balances[address_6_] = 116666;
  EXPECT_CALL(callback,
              Run(EqualsMojo(expected_balance), absl::optional<std::string>()));
  bitcoin_wallet_service_->GetBalance(mojom::kBitcoinMainnet, account_id(),
                                      callback.Get());
  base::RunLoop().RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&callback);
}

TEST_F(BitcoinWalletServiceUnitTest, GetBitcoinAccountInfo_DISABLED) {
  // TODO(apaymyshev): test in case we need GetBitcoinAccountInfo.
}

TEST_F(BitcoinWalletServiceUnitTest, SendTo_DISABLED) {
  // TODO(apaymyshev): test in case we need SendTo.
}

TEST_F(BitcoinWalletServiceUnitTest, GetUtxos) {
  using GetUtxosResult =
      base::expected<BitcoinWalletService::UtxoMap, std::string>;
  base::MockCallback<BitcoinWalletService::GetUtxosCallback> callback;

  // GetUtxos works.
  BitcoinWalletService::UtxoMap expected_utxos;
  for (auto& addr : address_stats_map_) {
    expected_utxos[addr.first].clear();
  }
  auto& utxo_0 = expected_utxos[address_0_].emplace_back();
  utxo_0.txid = kTxid1;
  utxo_0.vout = "1";
  utxo_0.value = "5000";
  utxo_0.status.confirmed = true;
  auto& utxo_6 = expected_utxos[address_6_].emplace_back();
  utxo_6.txid = kTxid2;
  utxo_6.vout = "7";
  utxo_6.value = "50000";
  utxo_6.status.confirmed = true;

  GetUtxosResult actual_utxos;
  EXPECT_CALL(callback, Run(Truly([&](const GetUtxosResult& arg) {
                EXPECT_TRUE(arg.has_value());
                EXPECT_EQ(arg.value(), expected_utxos);
                return true;
              })));
  bitcoin_wallet_service_->GetUtxos(mojom::kBitcoinMainnet, account_id(),
                                    callback.Get());
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
  bitcoin_wallet_service_->CreateTransaction(
      mojom::kBitcoinMainnet, account_id(), kAddress, 6000, callback.Get());
  base::RunLoop().RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&callback);

  EXPECT_EQ(actual_tx.locktime(), 12345u);
  EXPECT_EQ(actual_tx.amount(), 6000u);
  EXPECT_EQ(actual_tx.fee(), 2000u);
  EXPECT_EQ(actual_tx.to(), kAddress);

  EXPECT_EQ(actual_tx.inputs().size(), 2u);
  auto& input_0 = actual_tx.inputs().at(0);
  EXPECT_EQ(input_0.utxo_address, address_0_);
  EXPECT_EQ(base::HexEncode(input_0.utxo_outpoint.txid),
            base::ToUpperASCII(kTxid1));
  EXPECT_EQ(input_0.utxo_outpoint.index, 1u);
  EXPECT_EQ(input_0.utxo_value, 5000u);
  EXPECT_TRUE(input_0.script_sig.empty());
  EXPECT_TRUE(input_0.witness.empty());

  auto& input_1 = actual_tx.inputs().at(1);
  EXPECT_EQ(input_1.utxo_address, address_6_);
  EXPECT_EQ(base::HexEncode(input_1.utxo_outpoint.txid),
            base::ToUpperASCII(kTxid2));
  EXPECT_EQ(input_1.utxo_outpoint.index, 7u);
  EXPECT_EQ(input_1.utxo_value, 50000u);
  EXPECT_TRUE(input_1.script_sig.empty());
  EXPECT_TRUE(input_1.witness.empty());

  EXPECT_EQ(actual_tx.outputs().size(), 2u);

  auto& output_0 = actual_tx.outputs().at(0);
  EXPECT_EQ(output_0.address, kAddress);
  EXPECT_EQ(output_0.amount, 6000u);

  auto& output_1 = actual_tx.outputs().at(1);
  EXPECT_EQ(output_1.address, "bc1qrxxcyn4ywzx3u8ph3mra7qkmxsdd67mrtlmu3j");
  EXPECT_EQ(output_1.amount, 50000u + 5000u - 6000u - 2000u);
}

TEST_F(BitcoinWalletServiceUnitTest, SignAndPostTransaction) {
  using CreateTransactionResult =
      base::expected<BitcoinTransaction, std::string>;
  base::MockCallback<BitcoinWalletService::CreateTransactionCallback> callback;

  base::MockCallback<BitcoinWalletService::SignAndPostTransactionCallback>
      sign_callback;

  BitcoinTransaction intial_tx;
  EXPECT_CALL(callback, Run(Truly([&](const CreateTransactionResult& arg) {
                EXPECT_TRUE(arg.has_value());
                intial_tx = arg.value().Clone();
                return true;
              })));
  bitcoin_wallet_service_->CreateTransaction(
      mojom::kBitcoinMainnet, account_id(), kAddress, 6000, callback.Get());
  base::RunLoop().RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&callback);

  BitcoinTransaction signed_tx;
  EXPECT_CALL(sign_callback, Run(kTxid3, _, ""))
      .WillOnce(WithArg<1>(
          [&](const BitcoinTransaction& tx) { signed_tx = tx.Clone(); }));
  bitcoin_wallet_service_->SignAndPostTransaction(
      mojom::kBitcoinMainnet, account_id(), std::move(intial_tx),
      sign_callback.Get());
  base::RunLoop().RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&sign_callback);

  EXPECT_EQ(
      captured_raw_tx_,
      "02000000000102C5E29F841382F02A49BEAFAC756D14A211EC9089AD50E153767625B750"
      "8F38AA0100000000FDFFFFFFBEC2C52B2448A8733648E967D2B4559D0F1AA4BBBB93E53E"
      "9F516A12FB9C1CBD0700000000FDFFFFFF027017000000000000160014751E76E8199196"
      "D454941C45D1B3A323F1433BD698B7000000000000160014198D824EA4708D1E1C378EC7"
      "DF02DB341ADD7B6302473044022007AE882CEE3384B93D7A0A893093A9E959BD78D890BA"
      "42FDFC89084F296CFE4502207D8B089EDCF84117FA2C8BF3FD00C180CFE49495D682AA3B"
      "BF1A594E524AECA70121028256AD805CC35647890DEFD92AE6EF9BE31BA254E7E7D2834F"
      "8C403766C65FE70247304402206121222B8FA0B4A2D75CAE3ADAEB1015929593535536D5"
      "9C1C67FA0AE4D8A960022067AA021D78C104B95A76192BF9E9AE4EFCB3652CC97B608D98"
      "1F3825208CE7760121028096BB833E64D6F3FB05CBB9AE06D27ED008327A837C39794EBA"
      "322BF1A5BBF839300000");
}

}  // namespace brave_wallet
