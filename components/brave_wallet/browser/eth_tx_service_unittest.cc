/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/eth_tx_service.h"

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/callback_helpers.h"
#include "base/json/json_reader.h"
#include "base/test/bind.h"
#include "brave/components/brave_wallet/browser/asset_ratio_service.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/brave_wallet_prefs.h"
#include "brave/components/brave_wallet/browser/eip1559_transaction.h"
#include "brave/components/brave_wallet/browser/eth_data_parser.h"
#include "brave/components/brave_wallet/browser/eth_nonce_tracker.h"
#include "brave/components/brave_wallet/browser/eth_transaction.h"
#include "brave/components/brave_wallet/browser/eth_tx_state_manager.h"
#include "brave/components/brave_wallet/browser/hd_keyring.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "components/prefs/testing_pref_service.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/test/browser_task_environment.h"
#include "content/public/test/test_browser_context.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

namespace {

void AddUnapprovedTransactionSuccessCallback(bool* callback_called,
                                             std::string* tx_meta_id,
                                             bool success,
                                             const std::string& id,
                                             const std::string& error_message) {
  EXPECT_TRUE(success);
  EXPECT_FALSE(id.empty());
  EXPECT_TRUE(error_message.empty());
  *callback_called = true;
  *tx_meta_id = id;
}

void AddUnapprovedTransactionFailureCallback(bool* callback_called,
                                             bool success,
                                             const std::string& id,
                                             const std::string& error_message) {
  EXPECT_FALSE(success);
  EXPECT_TRUE(id.empty());
  EXPECT_FALSE(error_message.empty());
  *callback_called = true;
}

mojom::GasEstimation1559Ptr GetMojomGasEstimation() {
  return mojom::GasEstimation1559::New(
      "0x3b9aca00" /* Hex of 1 * 1e9 */, "0xaf16b1600" /* Hex of 47 * 1e9 */,
      "0x77359400" /* Hex of 2 * 1e9 */, "0xb2d05e000" /* Hex of 48 * 1e9 */,
      "0xb2d05e00" /* Hex of 3 * 1e9 */, "0xb68a0aa00" /* Hex of 49 * 1e9 */,
      "0xad8075b7a" /* Hex of 46574033786 */);
}

void MakeERC721TransferFromDataCallback(base::RunLoop* run_loop,
                                        bool expected_success,
                                        mojom::TransactionType expected_type,
                                        bool success,
                                        const std::vector<uint8_t>& data) {
  ASSERT_EQ(expected_success, success);

  // Verify tx type.
  if (success) {
    mojom::TransactionType tx_type;
    std::vector<std::string> tx_params;
    std::vector<std::string> tx_args;
    ASSERT_TRUE(
        GetTransactionInfoFromData(ToHex(data), &tx_type, nullptr, nullptr));
    EXPECT_EQ(expected_type, tx_type);
  }

  run_loop->Quit();
}

}  // namespace

class TestEthTxServiceObserver
    : public brave_wallet::mojom::EthTxServiceObserver {
 public:
  TestEthTxServiceObserver(
      const std::string& expected_nonce,
      const std::string& expected_gas_price,
      const std::string& expected_gas_limit,
      const std::string& expected_max_priority_fee_per_gas = "",
      const std::string& expected_max_fee_per_gas = "",
      const std::vector<uint8_t>& expected_data = std::vector<uint8_t>(),
      mojom::TransactionStatus expected_status =
          mojom::TransactionStatus::Unapproved)
      : expected_nonce_(expected_nonce),
        expected_gas_price_(expected_gas_price),
        expected_gas_limit_(expected_gas_limit),
        expected_max_priority_fee_per_gas_(expected_max_priority_fee_per_gas),
        expected_max_fee_per_gas_(expected_max_fee_per_gas),
        expected_status_(expected_status),
        expected_data_(expected_data) {}

  void OnNewUnapprovedTx(mojom::TransactionInfoPtr tx) override {}

  void OnUnapprovedTxUpdated(mojom::TransactionInfoPtr tx) override {
    EXPECT_EQ(tx->tx_data->base_data->nonce,
              base::ToLowerASCII(expected_nonce_));
    EXPECT_EQ(tx->tx_data->base_data->gas_price,
              base::ToLowerASCII(expected_gas_price_));
    EXPECT_EQ(tx->tx_data->base_data->gas_limit,
              base::ToLowerASCII(expected_gas_limit_));
    EXPECT_EQ(tx->tx_data->max_priority_fee_per_gas,
              base::ToLowerASCII(expected_max_priority_fee_per_gas_));
    EXPECT_EQ(tx->tx_data->max_fee_per_gas,
              base::ToLowerASCII(expected_max_fee_per_gas_));
    EXPECT_EQ(tx->tx_data->base_data->data, expected_data_);
    tx_updated_ = true;
  }

  void OnTransactionStatusChanged(mojom::TransactionInfoPtr tx) override {
    tx_status_changed_ = true;
    EXPECT_EQ(tx->tx_status, expected_status_);
  }

  bool TxUpdated() { return tx_updated_; }
  bool TxStatusChanged() { return tx_status_changed_; }
  void Reset() {
    tx_status_changed_ = false;
    tx_updated_ = false;
  }
  mojo::PendingRemote<brave_wallet::mojom::EthTxServiceObserver> GetReceiver() {
    return observer_receiver_.BindNewPipeAndPassRemote();
  }
  void SetExpectedNonce(const std::string& nonce) { expected_nonce_ = nonce; }

 private:
  std::string expected_nonce_;
  std::string expected_gas_price_;
  std::string expected_gas_limit_;
  std::string expected_max_priority_fee_per_gas_;
  std::string expected_max_fee_per_gas_;
  mojom::TransactionStatus expected_status_;
  bool tx_updated_ = false;
  bool tx_status_changed_ = false;
  std::vector<uint8_t> expected_data_;
  mojo::Receiver<brave_wallet::mojom::EthTxServiceObserver> observer_receiver_{
      this};
};

class EthTxServiceUnitTest : public testing::Test {
 public:
  EthTxServiceUnitTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME),
        browser_context_(new content::TestBrowserContext()),
        shared_url_loader_factory_(
            base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
                &url_loader_factory_)) {}

  void SetUp() override {
    url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
        [&](const network::ResourceRequest& request) {
          url_loader_factory_.ClearResponses();

          if (request.url.spec().find("action=gasoracle") !=
              std::string::npos) {
            url_loader_factory_.AddResponse(
                request.url.spec(),
                "{\"payload\": {\"status\": \"1\", \"message\": \"\", "
                "\"result\": {\"LastBlock\": \"13243541\", \"SafeGasPrice\": "
                "\"47\", \"ProposeGasPrice\": \"48\", \"FastGasPrice\": "
                "\"49\", \"suggestBaseFee\": \"46.574033786\", "
                "\"gasUsedRatio\": "
                "\"0.27036175840958,0.0884828740801432,0.0426623303159149,0."
                "972173412918789,0.319781207901446\"}}, \"lastUpdated\": "
                "\"2021-09-22T21:45:40.015Z\"}");
            return;
          }

          base::StringPiece request_string(request.request_body->elements()
                                               ->at(0)
                                               .As<network::DataElementBytes>()
                                               .AsStringPiece());
          absl::optional<base::Value> request_value =
              base::JSONReader::Read(request_string);
          std::string* method = request_value->FindStringKey("method");
          ASSERT_TRUE(method);

          if (*method == "eth_estimateGas") {
            url_loader_factory_.AddResponse(
                request.url.spec(),
                "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":"
                "\"0x00000000000009604\"}");
          } else if (*method == "eth_gasPrice") {
            url_loader_factory_.AddResponse(
                request.url.spec(),
                "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":"
                "\"0x17fcf18321\"}");
          } else if (*method == "eth_getTransactionCount") {
            url_loader_factory_.AddResponse(
                request.url.spec(),
                "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":"
                "\"1\"}");
          }
        }));

    user_prefs::UserPrefs::Set(browser_context_.get(), &prefs_);
    brave_wallet::RegisterProfilePrefs(prefs_.registry());
    json_rpc_service_.reset(
        new JsonRpcService(shared_url_loader_factory_, &prefs_));
    keyring_service_.reset(new KeyringService(&prefs_));
    asset_ratio_service_.reset(
        new AssetRatioService(shared_url_loader_factory_));

    auto tx_state_manager =
        std::make_unique<EthTxStateManager>(&prefs_, json_rpc_service_.get());
    auto nonce_tracker = std::make_unique<EthNonceTracker>(
        tx_state_manager.get(), json_rpc_service_.get());
    auto pending_tx_tracker = std::make_unique<EthPendingTxTracker>(
        tx_state_manager.get(), json_rpc_service_.get(), nonce_tracker.get());

    eth_tx_service_.reset(new EthTxService(
        json_rpc_service_.get(), keyring_service_.get(),
        asset_ratio_service_.get(), std::move(tx_state_manager),
        std::move(nonce_tracker), std::move(pending_tx_tracker), &prefs_));

    base::RunLoop run_loop;
    json_rpc_service_->SetNetwork(brave_wallet::mojom::kLocalhostChainId,
                                  base::BindLambdaForTesting([&](bool success) {
                                    EXPECT_TRUE(success);
                                    run_loop.Quit();
                                  }));
    run_loop.Run();
    keyring_service_->CreateWallet("testing123", base::DoNothing());
    base::RunLoop().RunUntilIdle();
    keyring_service_->AddAccount("Account 1", mojom::CoinType::ETH,
                                 base::DoNothing());
    base::RunLoop().RunUntilIdle();

    ASSERT_TRUE(base::HexStringToBytes(
        "095ea7b3000000000000000000000000BFb30a082f650C2A15D0632f0e87bE4F8e6446"
        "0f0000000000000000000000000000000000000000000000003fffffffffffffff",
        &data_));
  }

  std::string from() {
    return keyring_service_
        ->GetHDKeyringById(brave_wallet::mojom::kDefaultKeyringId)
        ->GetAddress(0);
  }

  EthTxService* eth_tx_service() { return eth_tx_service_.get(); }

  PrefService* GetPrefs() { return &prefs_; }

  void SetInterceptor(const std::string& content) {
    url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
        [&, content](const network::ResourceRequest& request) {
          url_loader_factory_.ClearResponses();
          url_loader_factory_.AddResponse(request.url.spec(), content);
        }));
  }

  void SetErrorInterceptor() {
    url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
        [&](const network::ResourceRequest& request) {
          url_loader_factory_.ClearResponses();
          url_loader_factory_.AddResponse(request.url.spec(), "",
                                          net::HTTP_REQUEST_TIMEOUT);
        }));
  }

  void DoSpeedupOrCancelTransactionSuccess(const std::string& nonce,
                                           const std::string& gas_price,
                                           const std::vector<uint8_t>& data,
                                           const std::string& orig_meta_id,
                                           mojom::TransactionStatus status,
                                           bool cancel,
                                           std::string* tx_meta_id) {
    auto tx_data =
        mojom::TxData::New(nonce, gas_price, "0x0974",
                           "0xbe862ad9abfe6f22bcb087716c7d89a26051f74c",
                           "0x016345785d8a0000", data);
    auto tx = EthTransaction::FromTxData(tx_data, false);
    ASSERT_TRUE(tx);

    EthTxStateManager::TxMeta meta;
    meta.id = orig_meta_id;
    meta.from =
        EthAddress::FromHex("0xbe862ad9abfe6f22bcb087716c7d89a26051f74a");
    meta.status = status;
    meta.tx = std::make_unique<EthTransaction>(*tx);
    eth_tx_service()->tx_state_manager_->AddOrUpdateTx(meta);

    bool callback_called = false;
    eth_tx_service()->SpeedupOrCancelTransaction(
        orig_meta_id, cancel,
        base::BindOnce(&AddUnapprovedTransactionSuccessCallback,
                       &callback_called, tx_meta_id));
    base::RunLoop().RunUntilIdle();
    EXPECT_TRUE(callback_called);
  }

  void DoSpeedupOrCancel1559TransactionSuccess(
      const std::string& nonce,
      const std::vector<uint8_t>& data,
      const std::string& max_priority_fee_per_gas,
      const std::string& max_fee_per_gas,
      const std::string& orig_meta_id,
      mojom::TransactionStatus status,
      bool cancel,
      std::string* tx_meta_id) {
    auto tx_data1559 = mojom::TxData1559::New(
        mojom::TxData::New(nonce, "", "0x0974",
                           "0xbe862ad9abfe6f22bcb087716c7d89a26051f74c",
                           "0x016345785d8a0000", data),
        "0x539", max_priority_fee_per_gas, max_fee_per_gas, nullptr);

    auto tx1559 = Eip1559Transaction::FromTxData(tx_data1559, false);
    ASSERT_TRUE(tx1559);

    EthTxStateManager::TxMeta meta;
    meta.id = orig_meta_id;
    meta.from =
        EthAddress::FromHex("0xbe862ad9abfe6f22bcb087716c7d89a26051f74a");
    meta.status = status;
    meta.tx = std::make_unique<Eip1559Transaction>(*tx1559);
    eth_tx_service()->tx_state_manager_->AddOrUpdateTx(meta);

    bool callback_called = false;
    eth_tx_service()->SpeedupOrCancelTransaction(
        orig_meta_id, cancel,
        base::BindOnce(&AddUnapprovedTransactionSuccessCallback,
                       &callback_called, tx_meta_id));
    base::RunLoop().RunUntilIdle();
    EXPECT_TRUE(callback_called);
  }

  void DoSpeedupOrCancelTransactionFailure(const std::string& orig_meta_id,
                                           bool cancel) {
    bool callback_called = false;
    eth_tx_service()->SpeedupOrCancelTransaction(
        orig_meta_id, false,
        base::BindOnce(&AddUnapprovedTransactionFailureCallback,
                       &callback_called));
    base::RunLoop().RunUntilIdle();
    EXPECT_TRUE(callback_called);
  }

 protected:
  content::BrowserTaskEnvironment task_environment_;
  std::unique_ptr<content::TestBrowserContext> browser_context_;
  sync_preferences::TestingPrefServiceSyncable prefs_;
  network::TestURLLoaderFactory url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;

  std::unique_ptr<JsonRpcService> json_rpc_service_;
  std::unique_ptr<KeyringService> keyring_service_;
  std::unique_ptr<EthTxService> eth_tx_service_;
  std::unique_ptr<AssetRatioService> asset_ratio_service_;
  std::vector<uint8_t> data_;
};

TEST_F(EthTxServiceUnitTest, AddUnapprovedTransactionWithGasPriceAndGasLimit) {
  std::string gas_price = "0x09184e72a000";
  std::string gas_limit = "0x0974";
  auto tx_data =
      mojom::TxData::New("0x06", gas_price, gas_limit,
                         "0xbe862ad9abfe6f22bcb087716c7d89a26051f74c",
                         "0x016345785d8a0000", data_);
  bool callback_called = false;
  std::string tx_meta_id;

  eth_tx_service_->AddUnapprovedTransaction(
      std::move(tx_data), from(),
      base::BindOnce(&AddUnapprovedTransactionSuccessCallback, &callback_called,
                     &tx_meta_id));

  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);
  auto tx_meta = eth_tx_service_->GetTxForTesting(tx_meta_id);
  EXPECT_TRUE(tx_meta);

  uint256_t gas_price_value;
  uint256_t gas_limit_value;
  EXPECT_TRUE(HexValueToUint256(gas_price, &gas_price_value));
  EXPECT_TRUE(HexValueToUint256(gas_limit, &gas_limit_value));
  EXPECT_EQ(tx_meta->tx->gas_price(), gas_price_value);
  EXPECT_EQ(tx_meta->tx->gas_limit(), gas_limit_value);
}

TEST_F(EthTxServiceUnitTest, AddUnapprovedTransactionWithoutGasLimit) {
  std::string gas_price = "0x09184e72a000";
  auto tx_data =
      mojom::TxData::New("0x06", gas_price, "" /* gas_limit */,
                         "0xbe862ad9abfe6f22bcb087716c7d89a26051f74c",
                         "0x016345785d8a0000", data_);
  bool callback_called = false;
  std::string tx_meta_id;

  eth_tx_service_->AddUnapprovedTransaction(
      tx_data.Clone(), from(),
      base::BindOnce(&AddUnapprovedTransactionSuccessCallback, &callback_called,
                     &tx_meta_id));

  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);
  auto tx_meta = eth_tx_service_->GetTxForTesting(tx_meta_id);
  EXPECT_TRUE(tx_meta);

  uint256_t gas_price_value;
  uint256_t gas_limit_value;
  EXPECT_TRUE(HexValueToUint256(gas_price, &gas_price_value));
  // Gas limit should be filled by requesting eth_estimateGas.
  EXPECT_TRUE(HexValueToUint256("0x9604", &gas_limit_value));
  EXPECT_EQ(tx_meta->tx->gas_price(), gas_price_value);
  EXPECT_EQ(tx_meta->tx->gas_limit(), gas_limit_value);

  // Check gas limit for estimation errors of different tx data types
  std::map<std::string, uint256_t> data_to_default_gas = {
      {"", kDefaultSendEthGasLimit},
      {"0xa9059cbb000000000000000000000000BFb30a082f650C2A15D0632f0e87bE4F8e644"
       "60f0000000000000000000000000000000000000000000000000de0b6b3a7640000",
       kDefaultERC20TransferGasLimit},
      {"0x095ea7b3000000000000000000000000BFb30a082f650C2A15D0632f0e87bE4F8e644"
       "60f0000000000000000000000000000000000000000000000000de0b6b3a7640000",
       kDefaultERC20ApproveGasLimit},
      {"0x23b872dd000000000000000000000000BFb30a082f650C2A15D0632f0e87bE4F8e644"
       "60f000000000000000000000000BFb30a082f650C2A15D0632f0e87bE4F8e64460a0000"
       "00000000000000000000000000000000000000000000000000000000000f",
       kDefaultERC721TransferGasLimit},
      {"0x42842e0e000000000000000000000000BFb30a082f650C2A15D0632f0e87bE4F8e644"
       "60f000000000000000000000000BFb30a082f650C2A15D0632f0e87bE4F8e64460a0000"
       "00000000000000000000000000000000000000000000000000000000000f",
       kDefaultERC721TransferGasLimit},
      {"0x70a082310000000000000000000000004e02f254184E904300e0775E4b8eeCB1",
       0}};
  for (const auto& kv : data_to_default_gas) {
    std::vector<uint8_t> data_decoded;
    if (kv.first.length() >= 2) {
      EXPECT_TRUE(PrefixedHexStringToBytes(kv.first, &data_decoded));
    }

    tx_data = mojom::TxData::New("0x06", gas_price, "" /* gas_limit */,
                                 "0xbe862ad9abfe6f22bcb087716c7d89a26051f74c",
                                 "0x016345785d8a0000", data_decoded);

    SetErrorInterceptor();
    callback_called = false;
    eth_tx_service_->AddUnapprovedTransaction(
        std::move(tx_data), from(),
        base::BindOnce(&AddUnapprovedTransactionSuccessCallback,
                       &callback_called, &tx_meta_id));
    base::RunLoop().RunUntilIdle();
    EXPECT_TRUE(callback_called);
    tx_meta = eth_tx_service_->GetTxForTesting(tx_meta_id);
    EXPECT_TRUE(tx_meta);
    EXPECT_TRUE(HexValueToUint256(gas_price, &gas_price_value));
    EXPECT_TRUE(
        HexValueToUint256(Uint256ValueToHex(kv.second), &gas_limit_value));
    EXPECT_EQ(tx_meta->tx->gas_limit(), gas_limit_value);
  }
}

TEST_F(EthTxServiceUnitTest, AddUnapprovedTransactionWithoutGasPrice) {
  std::string gas_limit = "0x0974";
  auto tx_data =
      mojom::TxData::New("0x06", "" /* gas_price */, gas_limit,
                         "0xbe862ad9abfe6f22bcb087716c7d89a26051f74c",
                         "0x016345785d8a0000", data_);
  bool callback_called = false;
  std::string tx_meta_id;

  eth_tx_service_->AddUnapprovedTransaction(
      tx_data.Clone(), from(),
      base::BindOnce(&AddUnapprovedTransactionSuccessCallback, &callback_called,
                     &tx_meta_id));

  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);
  auto tx_meta = eth_tx_service_->GetTxForTesting(tx_meta_id);
  EXPECT_TRUE(tx_meta);

  uint256_t gas_price_value;
  uint256_t gas_limit_value;
  // Gas price should be filled by requesting eth_gasPrice.
  EXPECT_TRUE(HexValueToUint256("0x17fcf18321", &gas_price_value));
  EXPECT_TRUE(HexValueToUint256(gas_limit, &gas_limit_value));
  EXPECT_EQ(tx_meta->tx->gas_price(), gas_price_value);
  EXPECT_EQ(tx_meta->tx->gas_limit(), gas_limit_value);

  SetErrorInterceptor();
  callback_called = false;
  eth_tx_service_->AddUnapprovedTransaction(
      std::move(tx_data), from(),
      base::BindOnce(&AddUnapprovedTransactionFailureCallback,
                     &callback_called));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);
}

TEST_F(EthTxServiceUnitTest,
       AddUnapprovedTransactionWithoutGasPriceAndGasLimit) {
  auto tx_data =
      mojom::TxData::New("0x06", "" /* gas_price */, "" /* gas_limit */,
                         "0xbe862ad9abfe6f22bcb087716c7d89a26051f74c",
                         "0x016345785d8a0000", data_);
  bool callback_called = false;
  std::string tx_meta_id;

  eth_tx_service_->AddUnapprovedTransaction(
      tx_data.Clone(), from(),
      base::BindOnce(&AddUnapprovedTransactionSuccessCallback, &callback_called,
                     &tx_meta_id));

  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);
  auto tx_meta = eth_tx_service_->GetTxForTesting(tx_meta_id);
  EXPECT_TRUE(tx_meta);

  uint256_t gas_price_value;
  uint256_t gas_limit_value;
  EXPECT_TRUE(HexValueToUint256("0x17fcf18321", &gas_price_value));
  EXPECT_TRUE(HexValueToUint256("0x9604", &gas_limit_value));
  EXPECT_EQ(tx_meta->tx->gas_price(), gas_price_value);
  EXPECT_EQ(tx_meta->tx->gas_limit(), gas_limit_value);

  SetErrorInterceptor();
  callback_called = false;
  eth_tx_service_->AddUnapprovedTransaction(
      std::move(tx_data), from(),
      base::BindOnce(&AddUnapprovedTransactionFailureCallback,
                     &callback_called));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);
}

TEST_F(EthTxServiceUnitTest,
       AddUnapprovedTransactionWithoutGasPriceAndGasLimitForEthSend) {
  auto tx_data =
      mojom::TxData::New("0x06", "" /* gas_price*/, "" /* gas_limit */,
                         "0xbe862ad9abfe6f22bcb087716c7d89a26051f74c",
                         "0x016345785d8a0000", std::vector<uint8_t>());
  bool callback_called = false;
  std::string tx_meta_id;

  eth_tx_service_->AddUnapprovedTransaction(
      std::move(tx_data), from(),
      base::BindOnce(&AddUnapprovedTransactionSuccessCallback, &callback_called,
                     &tx_meta_id));

  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);
  auto tx_meta = eth_tx_service_->GetTxForTesting(tx_meta_id);
  EXPECT_TRUE(tx_meta);

  uint256_t gas_price_value;
  EXPECT_TRUE(HexValueToUint256("0x17fcf18321", &gas_price_value));
  EXPECT_EQ(tx_meta->tx->gas_price(), gas_price_value);

  // Gas limit obtained by querying eth_estimateGas.
  EXPECT_EQ(tx_meta->tx->gas_limit(), 38404ULL);
}

TEST_F(EthTxServiceUnitTest, SetGasPriceAndLimitForUnapprovedTransaction) {
  auto tx_data =
      mojom::TxData::New("0x06", "" /* gas_price*/, "" /* gas_limit */,
                         "0xbe862ad9abfe6f22bcb087716c7d89a26051f74c",
                         "0x016345785d8a0000", std::vector<uint8_t>());
  bool callback_called = false;
  std::string tx_meta_id;

  eth_tx_service_->AddUnapprovedTransaction(
      std::move(tx_data), from(),
      base::BindOnce(&AddUnapprovedTransactionSuccessCallback, &callback_called,
                     &tx_meta_id));

  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);
  auto tx_meta = eth_tx_service_->GetTxForTesting(tx_meta_id);
  EXPECT_TRUE(tx_meta);

  uint256_t gas_price_value;
  EXPECT_TRUE(HexValueToUint256("0x17fcf18321", &gas_price_value));
  EXPECT_EQ(tx_meta->tx->gas_price(), gas_price_value);

  // Gas limit obtained by querying eth_estimateGas.
  EXPECT_EQ(tx_meta->tx->gas_limit(), 38404ULL);

  // Fail if transaction is not found.
  callback_called = false;
  eth_tx_service_->SetGasPriceAndLimitForUnapprovedTransaction(
      "not_exist", "0x1", Uint256ValueToHex(kDefaultSendEthGasLimit),
      base::BindLambdaForTesting([&](bool success) {
        EXPECT_FALSE(success);
        callback_called = true;
      }));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);

  // Fail if passing an empty gas price.
  callback_called = false;
  eth_tx_service_->SetGasPriceAndLimitForUnapprovedTransaction(
      tx_meta_id, "", Uint256ValueToHex(kDefaultSendEthGasLimit),
      base::BindLambdaForTesting([&](bool success) {
        EXPECT_FALSE(success);
        callback_called = true;
      }));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);

  // Fail if passing an empty gas limit.
  callback_called = false;
  eth_tx_service_->SetGasPriceAndLimitForUnapprovedTransaction(
      tx_meta_id, "0x1", "", base::BindLambdaForTesting([&](bool success) {
        EXPECT_FALSE(success);
        callback_called = true;
      }));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);

  const std::string update_gas_price_hex_string = "0x20000000000";
  const std::string update_gas_limit_hex_string = "0xFDE8";

  uint256_t update_gas_price;
  EXPECT_TRUE(
      HexValueToUint256(update_gas_price_hex_string, &update_gas_price));
  uint256_t update_gas_limit;
  EXPECT_TRUE(
      HexValueToUint256(update_gas_limit_hex_string, &update_gas_limit));

  TestEthTxServiceObserver observer("0x6", update_gas_price_hex_string,
                                    update_gas_limit_hex_string);
  eth_tx_service_->AddObserver(observer.GetReceiver());

  callback_called = false;
  eth_tx_service_->SetGasPriceAndLimitForUnapprovedTransaction(
      tx_meta_id, update_gas_price_hex_string, update_gas_limit_hex_string,
      base::BindLambdaForTesting([&](bool success) {
        EXPECT_TRUE(success);
        callback_called = true;
      }));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);
  EXPECT_TRUE(observer.TxUpdated());

  // Get the updated TX.
  tx_meta = eth_tx_service_->GetTxForTesting(tx_meta_id);
  EXPECT_TRUE(tx_meta);
  EXPECT_EQ(tx_meta->tx->gas_price(), update_gas_price);
  EXPECT_EQ(tx_meta->tx->gas_limit(), update_gas_limit);
}

TEST_F(EthTxServiceUnitTest, SetDataForUnapprovedTransaction) {
  std::vector<uint8_t> initial_data{0U, 1U};
  auto tx_data =
      mojom::TxData::New("0x06", "0x11" /* gas_price*/, "0x22" /* gas_limit */,
                         "0xbe862ad9abfe6f22bcb087716c7d89a26051f74c",
                         "0x016345785d8a0000", initial_data);
  bool callback_called = false;
  std::string tx_meta_id;
  eth_tx_service_->AddUnapprovedTransaction(
      std::move(tx_data), from(),
      base::BindOnce(&AddUnapprovedTransactionSuccessCallback, &callback_called,
                     &tx_meta_id));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);

  auto tx_meta = eth_tx_service_->GetTxForTesting(tx_meta_id);
  EXPECT_TRUE(tx_meta);

  EXPECT_EQ(tx_meta->tx->data(), initial_data);

  // Invalid tx_meta id should fail
  std::vector<uint8_t> new_data1;
  base::RunLoop run_loop;
  eth_tx_service_->SetDataForUnapprovedTransaction(
      "", new_data1, base::BindLambdaForTesting([&](bool success) {
        EXPECT_FALSE(success);
        run_loop.Quit();
      }));
  run_loop.Run();

  std::vector<uint8_t> new_data2{1U, 3U, 3U, 7U};
  TestEthTxServiceObserver observer("0x6", "0x11", "0x22", "", "", new_data2);
  eth_tx_service_->AddObserver(observer.GetReceiver());

  // Change the data
  base::RunLoop run_loop2;
  eth_tx_service_->SetDataForUnapprovedTransaction(
      tx_meta_id, new_data2, base::BindLambdaForTesting([&](bool success) {
        EXPECT_TRUE(success);
        run_loop2.Quit();
      }));
  run_loop2.Run();

  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(observer.TxUpdated());

  // Get the updated TX.
  tx_meta = eth_tx_service_->GetTxForTesting(tx_meta_id);
  EXPECT_TRUE(tx_meta);
  EXPECT_EQ(tx_meta->tx->data(), new_data2);
}

TEST_F(EthTxServiceUnitTest, SetNonceForUnapprovedTransaction) {
  auto tx_data =
      mojom::TxData::New("0x06", "0x11" /* gas_price*/, "0x22" /* gas_limit */,
                         "0xbe862ad9abfe6f22bcb087716c7d89a26051f74c",
                         "0x016345785d8a0000", std::vector<uint8_t>());
  bool callback_called = false;
  std::string tx_meta_id;
  eth_tx_service_->AddUnapprovedTransaction(
      std::move(tx_data), from(),
      base::BindOnce(&AddUnapprovedTransactionSuccessCallback, &callback_called,
                     &tx_meta_id));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);

  auto tx_meta = eth_tx_service_->GetTxForTesting(tx_meta_id);
  EXPECT_TRUE(tx_meta);

  EXPECT_EQ(tx_meta->tx->nonce(), 6ULL);

  // Invalid tx_meta id should fail
  base::RunLoop run_loop;
  eth_tx_service_->SetNonceForUnapprovedTransaction(
      "", "0x02", base::BindLambdaForTesting([&](bool success) {
        EXPECT_FALSE(success);
        run_loop.Quit();
      }));
  run_loop.Run();
  EXPECT_EQ(tx_meta->tx->nonce(), 6ULL);

  // Invalid nonce value should fail
  base::RunLoop run_loop2;
  eth_tx_service_->SetNonceForUnapprovedTransaction(
      tx_meta_id, "invalid nonce",
      base::BindLambdaForTesting([&](bool success) {
        EXPECT_FALSE(success);
        run_loop2.Quit();
      }));
  run_loop2.Run();
  EXPECT_EQ(tx_meta->tx->nonce(), 6ULL);

  TestEthTxServiceObserver observer("0x3", "0x11", "0x22");
  eth_tx_service_->AddObserver(observer.GetReceiver());

  // Change the nonce
  base::RunLoop run_loop3;
  eth_tx_service_->SetNonceForUnapprovedTransaction(
      tx_meta_id, "0x3", base::BindLambdaForTesting([&](bool success) {
        EXPECT_TRUE(success);
        run_loop3.Quit();
      }));
  run_loop3.Run();

  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(observer.TxUpdated());

  // Get the updated TX.
  tx_meta = eth_tx_service_->GetTxForTesting(tx_meta_id);
  EXPECT_TRUE(tx_meta);
  EXPECT_EQ(tx_meta->tx->nonce(), 3ULL);

  // Change the nonce back to blank
  observer.SetExpectedNonce("");
  base::RunLoop run_loop4;
  eth_tx_service_->SetNonceForUnapprovedTransaction(
      tx_meta_id, "", base::BindLambdaForTesting([&](bool success) {
        EXPECT_TRUE(success);
        run_loop4.Quit();
      }));
  run_loop4.Run();

  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(observer.TxUpdated());

  // Get the updated TX.
  tx_meta = eth_tx_service_->GetTxForTesting(tx_meta_id);
  EXPECT_TRUE(tx_meta);
  EXPECT_EQ(tx_meta->tx->nonce(), absl::nullopt);
}

TEST_F(EthTxServiceUnitTest, ValidateTxData) {
  std::string error_message;
  EXPECT_TRUE(EthTxService::ValidateTxData(
      mojom::TxData::New("0x06", "0x09184e72a000", "0x0974",
                         "0xbe862ad9abfe6f22bcb087716c7d89a26051f74c",
                         "0x016345785d8a0000", std::vector<uint8_t>()),
      &error_message));

  // Make sure if params are specified that they are valid hex strings
  EXPECT_FALSE(EthTxService::ValidateTxData(
      mojom::TxData::New("hello", "0x09184e72a000", "0x0974",
                         "0xbe862ad9abfe6f22bcb087716c7d89a26051f74c",
                         "0x016345785d8a0000", std::vector<uint8_t>()),
      &error_message));
  EXPECT_FALSE(EthTxService::ValidateTxData(
      mojom::TxData::New("0x06", "hello", "0x0974",
                         "0xbe862ad9abfe6f22bcb087716c7d89a26051f74c",
                         "0x016345785d8a0000", std::vector<uint8_t>()),
      &error_message));
  EXPECT_FALSE(EthTxService::ValidateTxData(
      mojom::TxData::New("0x06", "0x09184e72a000", "hello",
                         "0xbe862ad9abfe6f22bcb087716c7d89a26051f74c",
                         "0x016345785d8a0000", std::vector<uint8_t>()),
      &error_message));
  EXPECT_FALSE(EthTxService::ValidateTxData(
      mojom::TxData::New("0x06", "0x09184e72a000", "0x0974", "hello",
                         "0x016345785d8a0000", std::vector<uint8_t>()),
      &error_message));
  EXPECT_FALSE(EthTxService::ValidateTxData(
      mojom::TxData::New("0x06", "0x09184e72a000", "0x0974",
                         "0xbe862ad9abfe6f22bcb087716c7d89a26051f74c", "hello",
                         std::vector<uint8_t>()),
      &error_message));
  // to must not only be a valid hex string but also an address
  EXPECT_FALSE(EthTxService::ValidateTxData(
      mojom::TxData::New("0x06", "0x09184e72a000", "0x0974",
                         "0xbe",  // Invalid address
                         "hello", std::vector<uint8_t>()),
      &error_message));

  // To can't be missing if Data is missing
  EXPECT_FALSE(EthTxService::ValidateTxData(
      mojom::TxData::New("0x06", "0x09184e72a000", "0x0974", "",
                         "0x016345785d8a0000", std::vector<uint8_t>()),
      &error_message));
}

TEST_F(EthTxServiceUnitTest, ValidateTxData1559) {
  std::string error_message;
  EXPECT_TRUE(EthTxService::ValidateTxData1559(
      mojom::TxData1559::New(
          mojom::TxData::New("0x00", "", "0x00",
                             "0x0101010101010101010101010101010101010101",
                             "0x00", std::vector<uint8_t>()),
          "0x04", "0x0", "0x1", nullptr),
      &error_message));

  // Can't specify both gas price and max fee per gas
  EXPECT_FALSE(EthTxService::ValidateTxData1559(
      mojom::TxData1559::New(
          mojom::TxData::New("0x00", "0x1", "0x00",
                             "0x0101010101010101010101010101010101010101",
                             "0x00", std::vector<uint8_t>()),
          "0x04", "0x0", "0x1", nullptr),
      &error_message));
}

TEST_F(EthTxServiceUnitTest, ProcessHardwareSignature) {
  auto tx_data =
      mojom::TxData::New("0x06", "" /* gas_price */, "" /* gas_limit */,
                         "0xbe862ad9abfe6f22bcb087716c7d89a26051f74c",
                         "0x016345785d8a0000", data_);
  bool callback_called = false;
  std::string tx_meta_id;

  eth_tx_service_->AddUnapprovedTransaction(
      tx_data.Clone(), from(),
      base::BindOnce(&AddUnapprovedTransactionSuccessCallback, &callback_called,
                     &tx_meta_id));
  TestEthTxServiceObserver observer("0x6", "", "", "", "",
                                    std::vector<uint8_t>(),
                                    mojom::TransactionStatus::Approved);
  eth_tx_service_->AddObserver(observer.GetReceiver());
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);

  // Set an interceptor and just fake a common repsonse for
  // eth_getTransactionCount and eth_sendRawTransaction
  SetInterceptor("{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":\"0x0\"}");

  base::RunLoop run_loop;
  eth_tx_service_->ProcessHardwareSignature(
      tx_meta_id, "0x00",
      "0x93b9121e82df014428924df439ff044f89c205dd76a194f8b11f50d2eade744e",
      "0x7aa705c9144742836b7fbbd0745c57f67b60df7b8d1790fe59f91ed8d2bfc11d",
      base::BindLambdaForTesting([&](bool success) {
        EXPECT_TRUE(success);
        auto tx_meta = eth_tx_service_->GetTxForTesting(tx_meta_id);
        EXPECT_TRUE(tx_meta);
        EXPECT_EQ(tx_meta->status, mojom::TransactionStatus::Submitted);
        run_loop.Quit();
      }));
  run_loop.Run();
  ASSERT_TRUE(observer.TxStatusChanged());
}

TEST_F(EthTxServiceUnitTest, ProcessHardwareSignatureFail) {
  auto tx_data =
      mojom::TxData::New("0x06", "" /* gas_price */, "" /* gas_limit */,
                         "0xbe862ad9abfe6f22bcb087716c7d89a26051f74c",
                         "0x016345785d8a0000", data_);
  bool callback_called = false;
  std::string tx_meta_id;

  eth_tx_service_->AddUnapprovedTransaction(
      tx_data.Clone(), from(),
      base::BindOnce(&AddUnapprovedTransactionSuccessCallback, &callback_called,
                     &tx_meta_id));
  TestEthTxServiceObserver observer("0x6", "", "", "", "",
                                    std::vector<uint8_t>(),
                                    mojom::TransactionStatus::Error);
  eth_tx_service_->AddObserver(observer.GetReceiver());
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);
  callback_called = false;
  eth_tx_service_->ProcessHardwareSignature(
      tx_meta_id, "0x00", "9ff044f89c205dd76a194f8b11f50d2eade744e", "",
      base::BindLambdaForTesting([&](bool success) {
        EXPECT_FALSE(success);
        auto tx_meta = eth_tx_service_->GetTxForTesting(tx_meta_id);
        EXPECT_TRUE(tx_meta);
        EXPECT_EQ(tx_meta->status, mojom::TransactionStatus::Error);
        callback_called = true;
      }));
  base::RunLoop().RunUntilIdle();
  ASSERT_TRUE(callback_called);
  ASSERT_TRUE(observer.TxStatusChanged());
  observer.Reset();
  callback_called = false;
  eth_tx_service_->ProcessHardwareSignature(
      "-1", "0x00", "9ff044f89c205dd76a194f8b11f50d2eade744e", "",
      base::BindLambdaForTesting([&](bool success) {
        EXPECT_FALSE(success);
        callback_called = true;
      }));
  base::RunLoop().RunUntilIdle();
  ASSERT_TRUE(callback_called);
  ASSERT_FALSE(observer.TxStatusChanged());
}

TEST_F(EthTxServiceUnitTest, GetNonceForHardwareTransaction) {
  auto tx_data =
      mojom::TxData::New("", "" /* gas_price */, "" /* gas_limit */,
                         "0xbe862ad9abfe6f22bcb087716c7d89a26051f74c",
                         "0x016345785d8a0000", data_);
  bool callback_called = false;
  std::string tx_meta_id;

  eth_tx_service_->AddUnapprovedTransaction(
      tx_data.Clone(), from(),
      base::BindOnce(&AddUnapprovedTransactionSuccessCallback, &callback_called,
                     &tx_meta_id));

  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);
  TestEthTxServiceObserver observer("", "", "", "", "", std::vector<uint8_t>(),
                                    mojom::TransactionStatus::Unapproved);
  eth_tx_service_->AddObserver(observer.GetReceiver());
  callback_called = false;
  eth_tx_service_->GetNonceForHardwareTransaction(
      tx_meta_id,
      base::BindLambdaForTesting([&](const absl::optional<std::string>& nonce) {
        EXPECT_TRUE(nonce);
        EXPECT_FALSE(nonce->empty());
        auto tx_meta = eth_tx_service_->GetTxForTesting(tx_meta_id);
        EXPECT_TRUE(tx_meta);
        EXPECT_EQ(tx_meta->status, mojom::TransactionStatus::Unapproved);
        EXPECT_EQ(Uint256ValueToHex(tx_meta->tx->nonce().value()), nonce);
        callback_called = true;
      }));
  base::RunLoop().RunUntilIdle();
  ASSERT_TRUE(callback_called);

  callback_called = false;
  eth_tx_service_->GetTransactionMessageToSign(
      tx_meta_id,
      base::BindLambdaForTesting(
          [&](const absl::optional<std::string>& result) {
            EXPECT_EQ(
                result,
                "0xf873808517fcf1832182960494be862ad9abfe6f22bcb087716c7d89a2"
                "6051f74c88016345785d8a0000b844095ea7b30000000000000000000000"
                "00bfb30a082f650c2a15d0632f0e87be4f8e64460f000000000000000000"
                "0000000000000000000000000000003fffffffffffffff8205398080");
            callback_called = true;
          }));
  base::RunLoop().RunUntilIdle();
  ASSERT_TRUE(callback_called);
  ASSERT_TRUE(observer.TxStatusChanged());
}

TEST_F(EthTxServiceUnitTest, GetNonceForHardwareTransaction1559) {
  auto tx_data = mojom::TxData1559::New(
      mojom::TxData::New("0x00", "", "0x01",
                         "0x0101010101010101010101010101010101010101", "0x00",
                         std::vector<uint8_t>()),
      "0x04", "0x1", "0x1", nullptr);

  bool callback_called = false;
  std::string tx_meta_id;

  eth_tx_service_->AddUnapproved1559Transaction(
      tx_data.Clone(), from(),
      base::BindOnce(&AddUnapprovedTransactionSuccessCallback, &callback_called,
                     &tx_meta_id));

  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);
  TestEthTxServiceObserver observer("0x0", "", "", "", "",
                                    std::vector<uint8_t>(),
                                    mojom::TransactionStatus::Unapproved);
  eth_tx_service_->AddObserver(observer.GetReceiver());
  callback_called = false;
  eth_tx_service_->GetNonceForHardwareTransaction(
      tx_meta_id,
      base::BindLambdaForTesting([&](const absl::optional<std::string>& nonce) {
        EXPECT_TRUE(nonce);
        EXPECT_FALSE(nonce->empty());
        auto tx_meta = eth_tx_service_->GetTxForTesting(tx_meta_id);
        EXPECT_TRUE(tx_meta);
        EXPECT_EQ(tx_meta->status, mojom::TransactionStatus::Unapproved);
        EXPECT_EQ(Uint256ValueToHex(tx_meta->tx->nonce().value()), nonce);
        callback_called = true;
      }));
  base::RunLoop().RunUntilIdle();
  ASSERT_TRUE(callback_called);

  callback_called = false;
  eth_tx_service_->GetTransactionMessageToSign(
      tx_meta_id,
      base::BindLambdaForTesting([&](const absl::optional<std::string>&
                                         result) {
        EXPECT_EQ(
            result,
            "0x02dd04800101019401010101010101010101010101010101010101018080c0");
        callback_called = true;
      }));
  base::RunLoop().RunUntilIdle();
  ASSERT_TRUE(callback_called);
  ASSERT_TRUE(observer.TxStatusChanged());
}

TEST_F(EthTxServiceUnitTest, GetNonceForHardwareTransactionFail) {
  bool callback_called = false;
  TestEthTxServiceObserver observer("0x1", "", "");
  eth_tx_service_->AddObserver(observer.GetReceiver());
  eth_tx_service_->GetNonceForHardwareTransaction(
      std::string(),
      base::BindLambdaForTesting([&](const absl::optional<std::string>& nonce) {
        EXPECT_FALSE(nonce);
        callback_called = true;
      }));
  base::RunLoop().RunUntilIdle();
  ASSERT_TRUE(callback_called);

  callback_called = false;
  eth_tx_service_->GetTransactionMessageToSign(
      std::string(), base::BindLambdaForTesting(
                         [&](const absl::optional<std::string>& result) {
                           ASSERT_FALSE(result);
                           callback_called = true;
                         }));
  base::RunLoop().RunUntilIdle();
  ASSERT_TRUE(callback_called);
  ASSERT_FALSE(observer.TxStatusChanged());
}

TEST_F(EthTxServiceUnitTest, AddUnapproved1559TransactionWithGasFeeAndLimit) {
  const std::string gas_limit = "0x0974";

  auto tx_data = mojom::TxData1559::New(
      mojom::TxData::New("0x1", "", gas_limit,
                         "0xbe862ad9abfe6f22bcb087716c7d89a26051f74c",
                         "0x016345785d8a0000", data_),
      "0x04", "0x77359400" /* 2 Gwei */, "0xb2d05e000" /* 48 Gwei */, nullptr);

  bool callback_called = false;
  std::string tx_meta_id;

  eth_tx_service_->AddUnapproved1559Transaction(
      std::move(tx_data), from(),
      base::BindOnce(&AddUnapprovedTransactionSuccessCallback, &callback_called,
                     &tx_meta_id));

  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);
  auto tx_meta = eth_tx_service_->GetTxForTesting(tx_meta_id);
  EXPECT_TRUE(tx_meta);

  uint256_t gas_limit_value;
  EXPECT_TRUE(HexValueToUint256(gas_limit, &gas_limit_value));
  EXPECT_EQ(tx_meta->tx->gas_limit(), gas_limit_value);
  auto* tx1559 = reinterpret_cast<Eip1559Transaction*>(tx_meta->tx.get());
  EXPECT_EQ(tx1559->max_priority_fee_per_gas(), uint256_t(2) * uint256_t(1e9));
  EXPECT_EQ(tx1559->max_fee_per_gas(), uint256_t(48) * uint256_t(1e9));
  EXPECT_EQ(tx1559->gas_estimation(), Eip1559Transaction::GasEstimation());
}

TEST_F(EthTxServiceUnitTest, AddUnapproved1559TransactionWithoutGasLimit) {
  auto tx_data = mojom::TxData1559::New(
      mojom::TxData::New("0x1", "", "",
                         "0xbe862ad9abfe6f22bcb087716c7d89a26051f74c",
                         "0x016345785d8a0000", data_),
      "0x04", "0x77359400" /* 2 Gwei */, "0xb2d05e000" /* 48 Gwei */, nullptr);

  bool callback_called = false;
  std::string tx_meta_id;

  eth_tx_service_->AddUnapproved1559Transaction(
      std::move(tx_data), from(),
      base::BindOnce(&AddUnapprovedTransactionSuccessCallback, &callback_called,
                     &tx_meta_id));

  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);
  auto tx_meta = eth_tx_service_->GetTxForTesting(tx_meta_id);
  EXPECT_TRUE(tx_meta);

  uint256_t gas_limit_value;
  EXPECT_TRUE(HexValueToUint256("0x9604", &gas_limit_value));
  EXPECT_EQ(tx_meta->tx->gas_limit(), gas_limit_value);
  auto* tx1559 = reinterpret_cast<Eip1559Transaction*>(tx_meta->tx.get());
  EXPECT_EQ(tx1559->max_priority_fee_per_gas(), uint256_t(2) * uint256_t(1e9));
  EXPECT_EQ(tx1559->max_fee_per_gas(), uint256_t(48) * uint256_t(1e9));
  EXPECT_EQ(tx1559->gas_estimation(), Eip1559Transaction::GasEstimation());
}

TEST_F(EthTxServiceUnitTest, AddUnapproved1559TransactionWithoutGasFee) {
  const std::string gas_limit = "0x0974";
  auto tx_data = mojom::TxData1559::New(
      mojom::TxData::New("0x1", "", gas_limit,
                         "0xbe862ad9abfe6f22bcb087716c7d89a26051f74c",
                         "0x016345785d8a0000", data_),
      "0x04", "", "", nullptr);

  bool callback_called = false;
  std::string tx_meta_id;

  eth_tx_service_->AddUnapproved1559Transaction(
      std::move(tx_data), from(),
      base::BindOnce(&AddUnapprovedTransactionSuccessCallback, &callback_called,
                     &tx_meta_id));

  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);
  auto tx_meta = eth_tx_service_->GetTxForTesting(tx_meta_id);
  EXPECT_TRUE(tx_meta);

  uint256_t gas_limit_value;
  EXPECT_TRUE(HexValueToUint256(gas_limit, &gas_limit_value));
  EXPECT_EQ(tx_meta->tx->gas_limit(), gas_limit_value);
  auto* tx1559 = reinterpret_cast<Eip1559Transaction*>(tx_meta->tx.get());
  EXPECT_EQ(tx1559->max_priority_fee_per_gas(), uint256_t(2) * uint256_t(1e9));
  EXPECT_EQ(tx1559->max_fee_per_gas(), uint256_t(48) * uint256_t(1e9));
  EXPECT_EQ(tx1559->gas_estimation(),
            Eip1559Transaction::GasEstimation::FromMojomGasEstimation1559(
                GetMojomGasEstimation()));
}

TEST_F(EthTxServiceUnitTest,
       AddUnapproved1559TransactionWithoutGasFeeAndLimit) {
  auto tx_data = mojom::TxData1559::New(
      mojom::TxData::New("0x1", "", "",
                         "0xbe862ad9abfe6f22bcb087716c7d89a26051f74c",
                         "0x016345785d8a0000", data_),
      "0x04", "", "", nullptr);

  bool callback_called = false;
  std::string tx_meta_id;

  eth_tx_service_->AddUnapproved1559Transaction(
      std::move(tx_data), from(),
      base::BindOnce(&AddUnapprovedTransactionSuccessCallback, &callback_called,
                     &tx_meta_id));

  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);
  auto tx_meta = eth_tx_service_->GetTxForTesting(tx_meta_id);
  EXPECT_TRUE(tx_meta);

  uint256_t gas_limit_value;
  EXPECT_TRUE(HexValueToUint256("0x9604", &gas_limit_value));
  EXPECT_EQ(tx_meta->tx->gas_limit(), gas_limit_value);
  auto* tx1559 = reinterpret_cast<Eip1559Transaction*>(tx_meta->tx.get());
  EXPECT_EQ(tx1559->max_priority_fee_per_gas(), uint256_t(2) * uint256_t(1e9));
  EXPECT_EQ(tx1559->max_fee_per_gas(), uint256_t(48) * uint256_t(1e9));
  EXPECT_EQ(tx1559->gas_estimation(),
            Eip1559Transaction::GasEstimation::FromMojomGasEstimation1559(
                GetMojomGasEstimation()));
}

TEST_F(EthTxServiceUnitTest,
       AddUnapproved1559TransactionWithoutGasFeeAndLimitForEthSend) {
  auto tx_data = mojom::TxData1559::New(
      mojom::TxData::New("0x1", "", "",
                         "0xbe862ad9abfe6f22bcb087716c7d89a26051f74c",
                         "0x016345785d8a0000", std::vector<uint8_t>()),
      "0x04", "", "", nullptr);

  bool callback_called = false;
  std::string tx_meta_id;

  eth_tx_service_->AddUnapproved1559Transaction(
      std::move(tx_data), from(),
      base::BindOnce(&AddUnapprovedTransactionSuccessCallback, &callback_called,
                     &tx_meta_id));

  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);
  auto tx_meta = eth_tx_service_->GetTxForTesting(tx_meta_id);
  EXPECT_TRUE(tx_meta);

  // Gas limit obtained by querying eth_estimateGas.
  EXPECT_EQ(tx_meta->tx->gas_limit(), 38404ULL);

  // Gas fee and estimation should be filled by gas oracle.
  auto* tx1559 = reinterpret_cast<Eip1559Transaction*>(tx_meta->tx.get());
  EXPECT_EQ(tx1559->max_priority_fee_per_gas(), uint256_t(2) * uint256_t(1e9));
  EXPECT_EQ(tx1559->max_fee_per_gas(), uint256_t(48) * uint256_t(1e9));
  EXPECT_EQ(tx1559->gas_estimation(),
            Eip1559Transaction::GasEstimation::FromMojomGasEstimation1559(
                GetMojomGasEstimation()));
}

TEST_F(EthTxServiceUnitTest,
       AddUnapproved1559TransactionWithGasFeeAndLimitForEthSend) {
  const std::string gas_limit = "0x0974";

  auto tx_data = mojom::TxData1559::New(
      mojom::TxData::New("0x1", "", gas_limit,
                         "0xbe862ad9abfe6f22bcb087716c7d89a26051f74c",
                         "0x016345785d8a0000", std::vector<uint8_t>()),
      "0x04", "0x77359400" /* 2 Gwei */, "0xb2d05e000" /* 48 Gwei */, nullptr);

  bool callback_called = false;
  std::string tx_meta_id;

  eth_tx_service_->AddUnapproved1559Transaction(
      std::move(tx_data), from(),
      base::BindOnce(&AddUnapprovedTransactionSuccessCallback, &callback_called,
                     &tx_meta_id));

  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);
  auto tx_meta = eth_tx_service_->GetTxForTesting(tx_meta_id);
  EXPECT_TRUE(tx_meta);

  uint256_t gas_limit_value;
  EXPECT_TRUE(HexValueToUint256(gas_limit, &gas_limit_value));
  EXPECT_EQ(tx_meta->tx->gas_limit(), gas_limit_value);
  auto* tx1559 = reinterpret_cast<Eip1559Transaction*>(tx_meta->tx.get());
  EXPECT_EQ(tx1559->max_priority_fee_per_gas(), uint256_t(2) * uint256_t(1e9));
  EXPECT_EQ(tx1559->max_fee_per_gas(), uint256_t(48) * uint256_t(1e9));
  EXPECT_EQ(tx1559->gas_estimation(), Eip1559Transaction::GasEstimation());
}

TEST_F(EthTxServiceUnitTest,
       AddUnapproved1559TransactionWithoutGasLimitForEthSend) {
  auto tx_data = mojom::TxData1559::New(
      mojom::TxData::New("0x1", "", "",
                         "0xbe862ad9abfe6f22bcb087716c7d89a26051f74c",
                         "0x016345785d8a0000", std::vector<uint8_t>()),
      "0x04", "0x77359400" /* 2 Gwei */, "0xb2d05e000" /* 48 Gwei */, nullptr);
  bool callback_called = false;
  std::string tx_meta_id;

  eth_tx_service_->AddUnapproved1559Transaction(
      std::move(tx_data), from(),
      base::BindOnce(&AddUnapprovedTransactionSuccessCallback, &callback_called,
                     &tx_meta_id));

  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);
  auto tx_meta = eth_tx_service_->GetTxForTesting(tx_meta_id);
  EXPECT_TRUE(tx_meta);

  // Gas limit obtained by querying eth_estimateGas.
  EXPECT_EQ(tx_meta->tx->gas_limit(), 38404ULL);

  auto* tx1559 = reinterpret_cast<Eip1559Transaction*>(tx_meta->tx.get());
  EXPECT_EQ(tx1559->max_priority_fee_per_gas(), uint256_t(2) * uint256_t(1e9));
  EXPECT_EQ(tx1559->max_fee_per_gas(), uint256_t(48) * uint256_t(1e9));
  EXPECT_EQ(tx1559->gas_estimation(), Eip1559Transaction::GasEstimation());
}

TEST_F(EthTxServiceUnitTest,
       AddUnapproved1559TransactionWithoutGasFeeForEthSend) {
  const std::string gas_limit = "0x0974";
  auto tx_data = mojom::TxData1559::New(
      mojom::TxData::New("0x1", "", gas_limit,
                         "0xbe862ad9abfe6f22bcb087716c7d89a26051f74c",
                         "0x016345785d8a0000", std::vector<uint8_t>()),
      "0x04", "", "", nullptr);

  bool callback_called = false;
  std::string tx_meta_id;

  eth_tx_service_->AddUnapproved1559Transaction(
      std::move(tx_data), from(),
      base::BindOnce(&AddUnapprovedTransactionSuccessCallback, &callback_called,
                     &tx_meta_id));

  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);
  auto tx_meta = eth_tx_service_->GetTxForTesting(tx_meta_id);
  EXPECT_TRUE(tx_meta);

  uint256_t gas_limit_value;
  EXPECT_TRUE(HexValueToUint256(gas_limit, &gas_limit_value));
  EXPECT_EQ(tx_meta->tx->gas_limit(), gas_limit_value);

  // Gas fee and estimation should be filled by gas oracle.
  auto* tx1559 = reinterpret_cast<Eip1559Transaction*>(tx_meta->tx.get());
  EXPECT_EQ(tx1559->max_priority_fee_per_gas(), uint256_t(2) * uint256_t(1e9));
  EXPECT_EQ(tx1559->max_fee_per_gas(), uint256_t(48) * uint256_t(1e9));
  EXPECT_EQ(tx1559->gas_estimation(),
            Eip1559Transaction::GasEstimation::FromMojomGasEstimation1559(
                GetMojomGasEstimation()));
}

TEST_F(EthTxServiceUnitTest, SetGasFeeAndLimitForUnapprovedTransaction) {
  auto tx_data = mojom::TxData1559::New(
      mojom::TxData::New("0x1", "", "",
                         "0xbe862ad9abfe6f22bcb087716c7d89a26051f74c",
                         "0x016345785d8a0000", data_),
      "0x04", "", "", nullptr);
  bool callback_called = false;
  std::string tx_meta_id;

  eth_tx_service_->AddUnapproved1559Transaction(
      std::move(tx_data), from(),
      base::BindOnce(&AddUnapprovedTransactionSuccessCallback, &callback_called,
                     &tx_meta_id));

  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);
  auto tx_meta = eth_tx_service_->GetTxForTesting(tx_meta_id);
  EXPECT_TRUE(tx_meta);

  // Gas limit should be filled by requesting eth_estimateGas.
  uint256_t gas_limit_value;
  EXPECT_TRUE(HexValueToUint256("0x9604", &gas_limit_value));
  EXPECT_EQ(tx_meta->tx->gas_limit(), gas_limit_value);

  // Gas fee and estimation should be filled by gas oracle.
  auto* tx1559 = reinterpret_cast<Eip1559Transaction*>(tx_meta->tx.get());
  EXPECT_EQ(tx1559->max_priority_fee_per_gas(), uint256_t(2) * uint256_t(1e9));
  EXPECT_EQ(tx1559->max_fee_per_gas(), uint256_t(48) * uint256_t(1e9));
  EXPECT_EQ(tx1559->gas_estimation(),
            Eip1559Transaction::GasEstimation::FromMojomGasEstimation1559(
                GetMojomGasEstimation()));

  // Fail if transaction is not found.
  callback_called = false;
  eth_tx_service_->SetGasFeeAndLimitForUnapprovedTransaction(
      "not_exist", "0x1", "0x2", "0x3",
      base::BindLambdaForTesting([&](bool success) {
        EXPECT_FALSE(success);
        callback_called = true;
      }));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);

  // Fail if passing an empty gas limit.
  callback_called = false;
  eth_tx_service_->SetGasFeeAndLimitForUnapprovedTransaction(
      tx_meta_id, "0x1", "0x2", "",
      base::BindLambdaForTesting([&](bool success) {
        EXPECT_FALSE(success);
        callback_called = true;
      }));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);

  // Fail if passing an empty max_priority_fee_per_gas.
  callback_called = false;
  eth_tx_service_->SetGasFeeAndLimitForUnapprovedTransaction(
      tx_meta_id, "", "0x2", "0x3",
      base::BindLambdaForTesting([&](bool success) {
        EXPECT_FALSE(success);
        callback_called = true;
      }));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);

  // Fail if passing an empty max_fee_per_gas.
  callback_called = false;
  eth_tx_service_->SetGasFeeAndLimitForUnapprovedTransaction(
      tx_meta_id, "0x1", "", "0x3",
      base::BindLambdaForTesting([&](bool success) {
        EXPECT_FALSE(success);
        callback_called = true;
      }));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);

  const std::string update_max_priority_fee_per_gas_hex_string = "0x3344";
  const std::string update_max_fee_per_gas_hex_string = "0x5566";
  const std::string update_gas_limit_hex_string = "0xFDE8";

  uint256_t update_max_priority_fee_per_gas;
  EXPECT_TRUE(HexValueToUint256(update_max_priority_fee_per_gas_hex_string,
                                &update_max_priority_fee_per_gas));
  uint256_t update_max_fee_per_gas;
  EXPECT_TRUE(HexValueToUint256(update_max_fee_per_gas_hex_string,
                                &update_max_fee_per_gas));
  uint256_t update_gas_limit;
  EXPECT_TRUE(
      HexValueToUint256(update_gas_limit_hex_string, &update_gas_limit));

  TestEthTxServiceObserver observer("0x1", "0x0", update_gas_limit_hex_string,
                                    update_max_priority_fee_per_gas_hex_string,
                                    update_max_fee_per_gas_hex_string, data_);
  eth_tx_service_->AddObserver(observer.GetReceiver());

  callback_called = false;
  eth_tx_service_->SetGasFeeAndLimitForUnapprovedTransaction(
      tx_meta_id, update_max_priority_fee_per_gas_hex_string,
      update_max_fee_per_gas_hex_string, update_gas_limit_hex_string,
      base::BindLambdaForTesting([&](bool success) {
        EXPECT_TRUE(success);
        callback_called = true;
      }));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);
  EXPECT_TRUE(observer.TxUpdated());

  // Get the updated TX.
  tx_meta = eth_tx_service_->GetTxForTesting(tx_meta_id);
  EXPECT_TRUE(tx_meta);
  EXPECT_EQ(tx_meta->tx->gas_limit(), update_gas_limit);
  tx1559 = reinterpret_cast<Eip1559Transaction*>(tx_meta->tx.get());
  EXPECT_EQ(tx1559->max_priority_fee_per_gas(),
            update_max_priority_fee_per_gas);
  EXPECT_EQ(tx1559->max_fee_per_gas(), update_max_fee_per_gas);
}

TEST_F(EthTxServiceUnitTest,
       SetGasFeeAndLimitForUnapprovedTransactionRejectNotEip1559) {
  auto tx_data =
      mojom::TxData::New("0x06", "0x09184e72a000", "0x0974",
                         "0xbe862ad9abfe6f22bcb087716c7d89a26051f74c",
                         "0x016345785d8a0000", data_);
  bool callback_called = false;
  std::string tx_meta_id;

  eth_tx_service_->AddUnapprovedTransaction(
      std::move(tx_data), from(),
      base::BindOnce(&AddUnapprovedTransactionSuccessCallback, &callback_called,
                     &tx_meta_id));

  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);
  auto tx_meta = eth_tx_service_->GetTxForTesting(tx_meta_id);
  EXPECT_TRUE(tx_meta);

  callback_called = false;
  eth_tx_service_->SetGasFeeAndLimitForUnapprovedTransaction(
      tx_meta_id, "0x3344", "0x5566", "0xFED8",
      base::BindLambdaForTesting([&](bool success) {
        EXPECT_FALSE(success);
        callback_called = true;
      }));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);
}

TEST_F(EthTxServiceUnitTest, TestSubmittedToConfirmed) {
  EthAddress addr1 =
      EthAddress::FromHex("0x2f015c60e0be116b1f0cd534704db9c92118fb6a");
  EthAddress addr2 =
      EthAddress::FromHex("0x2f015c60e0be116b1f0cd534704db9c92118fb6b");
  base::RunLoop().RunUntilIdle();
  EthTxStateManager::TxMeta meta;
  meta.id = "001";
  meta.from = addr1;
  meta.status = mojom::TransactionStatus::Submitted;
  eth_tx_service()->tx_state_manager_->AddOrUpdateTx(meta);
  meta.id = "002";
  meta.from = addr2;
  meta.tx->set_nonce(uint256_t(4));
  meta.status = mojom::TransactionStatus::Submitted;
  eth_tx_service()->tx_state_manager_->AddOrUpdateTx(meta);

  url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
      [this](const network::ResourceRequest& request) {
        url_loader_factory_.ClearResponses();
        std::string header_value;
        EXPECT_TRUE(request.headers.GetHeader("X-Eth-Method", &header_value));
        LOG(ERROR) << "Header value is: " << header_value;
        if (header_value == "eth_blockNumber") {
          url_loader_factory_.AddResponse(request.url.spec(), R"(
            {
              "jsonrpc":"2.0",
              "result":"0x65a8db",
              "id":1
            })");
        } else if (header_value == "eth_getTransactionReceipt") {
          url_loader_factory_.AddResponse(request.url.spec(), R"(
            {
              "jsonrpc": "2.0",
              "id":1,
              "result": {
                "transactionHash": "0xb903239f8543d04b5dc1ba6579132b143087c68db1b2168786408fcbce568238",
                "transactionIndex":  "0x1",
                "blockNumber": "0xb",
                "blockHash": "0xc6ef2fc5426d6ad6fd9e2a26abeab0aa2411b7ab17f30a99d3cb96aed1d1055b",
                "cumulativeGasUsed": "0x33bc",
                "gasUsed": "0x4dc",
                "contractAddress": "0xb60e8dd61c5d32be8058bb8eb970870f07233155",
                "logs": [],
                "logsBloom": "0x00...0",
                "status": "0x1"
              }
            })");
        }
      }));

  // Nothing is triggered after 10s
  task_environment_.FastForwardBy(
      base::Seconds(kBlockTrackerDefaultTimeInSeconds - 1));
  auto tx_meta1 = eth_tx_service()->GetTxForTesting("001");
  EXPECT_EQ(mojom::TransactionStatus::Submitted, tx_meta1->status);
  auto tx_meta2 = eth_tx_service()->GetTxForTesting("002");
  EXPECT_EQ(mojom::TransactionStatus::Submitted, tx_meta1->status);

  task_environment_.FastForwardBy(base::Seconds(1));
  tx_meta1 = eth_tx_service()->GetTxForTesting("001");
  EXPECT_EQ(mojom::TransactionStatus::Confirmed, tx_meta1->status);
  tx_meta2 = eth_tx_service()->GetTxForTesting("002");
  EXPECT_EQ(mojom::TransactionStatus::Confirmed, tx_meta1->status);

  // If the keyring is locked, nothing should update
  meta.id = "001";
  meta.from = addr1;
  meta.status = mojom::TransactionStatus::Submitted;
  eth_tx_service()->tx_state_manager_->AddOrUpdateTx(meta);
  meta.id = "002";
  meta.from = addr2;
  meta.tx->set_nonce(uint256_t(4));
  meta.status = mojom::TransactionStatus::Submitted;
  eth_tx_service()->tx_state_manager_->AddOrUpdateTx(meta);
  keyring_service_->Lock();
  task_environment_.FastForwardBy(
      base::Seconds(kBlockTrackerDefaultTimeInSeconds + 1));
  tx_meta1 = eth_tx_service()->GetTxForTesting("001");
  EXPECT_EQ(mojom::TransactionStatus::Submitted, tx_meta1->status);
  tx_meta2 = eth_tx_service()->GetTxForTesting("002");
  EXPECT_EQ(mojom::TransactionStatus::Submitted, tx_meta1->status);
}

TEST_F(EthTxServiceUnitTest, SpeedupTransaction) {
  // Speedup EthSend with gas price + 10% < eth_getGasPrice should use
  // eth_getGasPrice for EthSend.
  //
  //    gas price       => 0xa (10 wei)
  //    gas price + 10% => 0xb (11 wei)
  //    eth_getGasPrice => 0x17fcf18321 (103 Gwei)
  std::string orig_meta_id = "001";
  std::string tx_meta_id;
  DoSpeedupOrCancelTransactionSuccess(
      "0x05", "0xa", std::vector<uint8_t>(), orig_meta_id,
      mojom::TransactionStatus::Submitted, false, &tx_meta_id);

  auto expected_tx_meta = eth_tx_service_->GetTxForTesting(orig_meta_id);
  ASSERT_TRUE(expected_tx_meta);
  expected_tx_meta->tx->set_gas_price(103027933985ULL);  // 0x17fcf18321
  auto tx_meta = eth_tx_service_->GetTxForTesting(tx_meta_id);
  ASSERT_TRUE(tx_meta);
  EXPECT_EQ(*expected_tx_meta->tx, *tx_meta->tx);

  // Speedup with gas price + 10% < eth_getGasPrice, new gas_price should be
  // from eth_getGasPrice
  //
  //    gas price       => 0xa (10 wei)
  //    gas price + 10% => 0xb (11 wei)
  //    eth_getGasPrice => 0x17fcf18321 (103 Gwei)
  orig_meta_id = "002";
  DoSpeedupOrCancelTransactionSuccess("0x06", "0xa", data_, orig_meta_id,
                                      mojom::TransactionStatus::Submitted,
                                      false, &tx_meta_id);

  expected_tx_meta = eth_tx_service_->GetTxForTesting(orig_meta_id);
  ASSERT_TRUE(expected_tx_meta);
  expected_tx_meta->tx->set_gas_price(103027933985ULL);  // 0x17fcf18321
  tx_meta = eth_tx_service_->GetTxForTesting(tx_meta_id);
  ASSERT_TRUE(tx_meta);
  EXPECT_EQ(*expected_tx_meta->tx, *tx_meta->tx);

  // Speedup with original gas price + 10% > eth_getGasPrice should use
  // original gas price + 10% as the new gas price.
  //
  //    gas price       => 0x174876e800 (100 Gwei)
  //    gas price + 10% => 0x199c82cc00 (110 Gwei)
  //    eth_getGasPrice => 0x17fcf18321 (103 Gwei)
  orig_meta_id = "003";
  DoSpeedupOrCancelTransactionSuccess(
      "0x07", "0x174876e800", data_, orig_meta_id,
      mojom::TransactionStatus::Submitted, false, &tx_meta_id);

  expected_tx_meta = eth_tx_service_->GetTxForTesting(orig_meta_id);
  ASSERT_TRUE(expected_tx_meta);
  expected_tx_meta->tx->set_gas_price(110000000000ULL);  // 0x174876e800 * 1.1
  tx_meta = eth_tx_service_->GetTxForTesting(tx_meta_id);
  ASSERT_TRUE(tx_meta);
  EXPECT_EQ(*expected_tx_meta->tx, *tx_meta->tx);

  // Non-exist transaction should fail.
  DoSpeedupOrCancelTransactionFailure("123", false);

  // Unapproved transaction should fail.
  DoSpeedupOrCancelTransactionFailure(tx_meta_id, false);

  SetErrorInterceptor();
  DoSpeedupOrCancelTransactionFailure(orig_meta_id, false);
}

TEST_F(EthTxServiceUnitTest, Speedup1559Transaction) {
  // Speedup with original gas fees + 10% > avg gas fees should use
  // original gas fees + 10%.
  std::string orig_meta_id = "001";
  std::string tx_meta_id;
  DoSpeedupOrCancel1559TransactionSuccess(
      "0x05", data_, "0x77359400" /* 2 Gwei */, "0xb2d05e000" /* 48 Gwei */,
      orig_meta_id, mojom::TransactionStatus::Submitted, false, &tx_meta_id);

  auto expected_tx_meta = eth_tx_service_->GetTxForTesting(orig_meta_id);
  ASSERT_TRUE(expected_tx_meta);
  auto* expected_tx1559_ptr =
      reinterpret_cast<Eip1559Transaction*>(expected_tx_meta->tx.get());
  expected_tx1559_ptr->set_max_priority_fee_per_gas(
      2200000000ULL);                                        // 2 * 1.1 gwei
  expected_tx1559_ptr->set_max_fee_per_gas(52800000000ULL);  // 48 * 1.1 gwei
  auto tx_meta = eth_tx_service_->GetTxForTesting(tx_meta_id);
  ASSERT_TRUE(tx_meta);
  auto* tx1559_ptr = reinterpret_cast<Eip1559Transaction*>(tx_meta->tx.get());
  EXPECT_EQ(*expected_tx1559_ptr, *tx1559_ptr);

  // Speedup with original gas fees + 10% < avg gas fees should use avg
  // gas fees (2 gwei for priority fee and 48 gwei for max fee).
  orig_meta_id = "002";
  DoSpeedupOrCancel1559TransactionSuccess(
      "0x06", data_, "0x7735940" /* 0.125 Gwei */, "0xb2d05e00" /* 3 Gwei */,
      orig_meta_id, mojom::TransactionStatus::Submitted, false, &tx_meta_id);

  expected_tx_meta = eth_tx_service_->GetTxForTesting(orig_meta_id);
  ASSERT_TRUE(expected_tx_meta);
  expected_tx1559_ptr =
      reinterpret_cast<Eip1559Transaction*>(expected_tx_meta->tx.get());
  expected_tx1559_ptr->set_max_priority_fee_per_gas(2000000000ULL);  // 2 Gwei
  expected_tx1559_ptr->set_max_fee_per_gas(48000000000ULL);          // 48 Gwei
  tx_meta = eth_tx_service_->GetTxForTesting(tx_meta_id);
  ASSERT_TRUE(tx_meta);
  tx1559_ptr = reinterpret_cast<Eip1559Transaction*>(tx_meta->tx.get());
  EXPECT_EQ(*expected_tx1559_ptr, *tx1559_ptr);

  // Non-exist transaction should fail.
  DoSpeedupOrCancelTransactionFailure("123", false);

  // Unapproved transaction should fail.
  DoSpeedupOrCancelTransactionFailure(tx_meta_id, false);

  SetErrorInterceptor();
  DoSpeedupOrCancelTransactionFailure(orig_meta_id, false);
}

TEST_F(EthTxServiceUnitTest, CancelTransaction) {
  // Cancel with original gas price + 10% > eth_getGasPrice should use
  // original gas price + 10% as the new gas price.
  //
  //    gas price       => 0x2540BE4000 (160 Gwei)
  //    gas price + 10% => 0x28fa6ae000 (176 Gwei)
  //    eth_getGasPrice => 0x17fcf18321 (103 Gwei)
  std::string orig_meta_id = "001";
  std::string tx_meta_id;
  DoSpeedupOrCancelTransactionSuccess(
      "0x06", "0x2540BE4000" /* 160 gwei */, data_, orig_meta_id,
      mojom::TransactionStatus::Submitted, true, &tx_meta_id);

  auto orig_tx_meta = eth_tx_service_->GetTxForTesting(orig_meta_id);
  ASSERT_TRUE(orig_tx_meta);
  auto tx_meta = eth_tx_service_->GetTxForTesting(tx_meta_id);
  ASSERT_TRUE(tx_meta);
  EXPECT_EQ(tx_meta->tx->nonce(), orig_tx_meta->tx->nonce());
  EXPECT_EQ(Uint256ValueToHex(tx_meta->tx->nonce().value()), "0x6");
  EXPECT_EQ(tx_meta->tx->gas_price(), 176000000000ULL);  // 160*1.1 gwei
  EXPECT_EQ(tx_meta->tx->to(), orig_tx_meta->from);
  EXPECT_EQ(tx_meta->tx->value(), 0u);
  EXPECT_TRUE(tx_meta->tx->data().empty());

  // Cancel with original gas price + 10% < eth_getGasPrice should use
  // eth_getGasPrice as the new gas price.
  //
  //    gas price       => 0xa (10 wei)
  //    gas price + 10% => 0xb (11 wei)
  //    eth_getGasPrice => 0x17fcf18321 (103 Gwei)
  orig_meta_id = "002";
  DoSpeedupOrCancelTransactionSuccess("0x07", "0x1", data_, orig_meta_id,
                                      mojom::TransactionStatus::Submitted, true,
                                      &tx_meta_id);

  orig_tx_meta = eth_tx_service_->GetTxForTesting(orig_meta_id);
  ASSERT_TRUE(orig_tx_meta);
  tx_meta = eth_tx_service_->GetTxForTesting(tx_meta_id);
  ASSERT_TRUE(tx_meta);
  EXPECT_EQ(tx_meta->tx->nonce(), orig_tx_meta->tx->nonce());
  EXPECT_EQ(Uint256ValueToHex(tx_meta->tx->nonce().value()), "0x7");
  EXPECT_EQ(tx_meta->tx->gas_price(), 0x17fcf18321ULL);  // 0x17fcf18321
  EXPECT_EQ(tx_meta->tx->to(), orig_tx_meta->from);
  EXPECT_EQ(tx_meta->tx->value(), 0u);
  EXPECT_TRUE(tx_meta->tx->data().empty());

  // EIP1559
  // Cancel with original gas fees + 10% > avg gas fees should use
  // original gas fees + 10%.
  orig_meta_id = "003";
  DoSpeedupOrCancel1559TransactionSuccess(
      "0x08", data_, "0x77359400" /* 2 Gwei */, "0xb2d05e000" /* 48 Gwei */,
      orig_meta_id, mojom::TransactionStatus::Submitted, true, &tx_meta_id);

  orig_tx_meta = eth_tx_service_->GetTxForTesting(orig_meta_id);
  ASSERT_TRUE(orig_tx_meta);
  tx_meta = eth_tx_service_->GetTxForTesting(tx_meta_id);
  ASSERT_TRUE(tx_meta);
  auto* orig_tx1559_ptr =
      reinterpret_cast<Eip1559Transaction*>(orig_tx_meta->tx.get());
  auto* tx1559_ptr = reinterpret_cast<Eip1559Transaction*>(tx_meta->tx.get());
  EXPECT_EQ(tx1559_ptr->nonce(), orig_tx1559_ptr->nonce());
  EXPECT_EQ(Uint256ValueToHex(tx1559_ptr->nonce().value()), "0x8");
  EXPECT_EQ(tx1559_ptr->max_priority_fee_per_gas(),
            2200000000ULL);                                  // 2*1.1 gwei
  EXPECT_EQ(tx1559_ptr->max_fee_per_gas(), 52800000000ULL);  // 48*1.1 gwei
  EXPECT_EQ(tx_meta->tx->to(), orig_tx_meta->from);
  EXPECT_EQ(tx_meta->tx->value(), 0u);
  EXPECT_TRUE(tx_meta->tx->data().empty());

  // Non-exist transaction should fail.
  DoSpeedupOrCancelTransactionFailure("123", true);

  // Unapproved transaction should fail.
  DoSpeedupOrCancelTransactionFailure(tx_meta_id, true);

  SetErrorInterceptor();
  DoSpeedupOrCancelTransactionFailure(orig_meta_id, true);
}

TEST_F(EthTxServiceUnitTest, RetryTransaction) {
  auto tx_data =
      mojom::TxData::New("0x07", "0x17fcf18322", "0x0974",
                         "0xbe862ad9abfe6f22bcb087716c7d89a26051f74c",
                         "0x016345785d8a0000", data_);
  auto tx = EthTransaction::FromTxData(tx_data, false);
  ASSERT_TRUE(tx);

  EthTxStateManager::TxMeta meta;
  meta.id = "001";
  meta.from = EthAddress::FromHex("0xbe862ad9abfe6f22bcb087716c7d89a26051f74a");
  meta.status = mojom::TransactionStatus::Error;
  meta.tx = std::make_unique<EthTransaction>(*tx);
  eth_tx_service()->tx_state_manager_->AddOrUpdateTx(meta);

  bool callback_called = false;
  std::string tx_meta_id;

  eth_tx_service()->RetryTransaction(
      "001", base::BindOnce(&AddUnapprovedTransactionSuccessCallback,
                            &callback_called, &tx_meta_id));

  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);
  auto tx_meta = eth_tx_service_->GetTxForTesting(tx_meta_id);
  ASSERT_TRUE(tx_meta);
  EXPECT_EQ(*tx_meta->tx, tx.value());

  // EIP1559
  callback_called = false;
  auto tx_data1559 = mojom::TxData1559::New(
      mojom::TxData::New("0x08", "", "0x0974",
                         "0xbe862ad9abfe6f22bcb087716c7d89a26051f74c",
                         "0x016345785d8a0000", data_),
      "0x539", "0x77359400" /* 2 Gwei */, "0xb2d05e000" /* 48 Gwei */, nullptr);

  auto tx1559 = Eip1559Transaction::FromTxData(tx_data1559, false);
  ASSERT_TRUE(tx1559);

  meta.id = "002";
  meta.status = mojom::TransactionStatus::Error;
  meta.tx = std::make_unique<Eip1559Transaction>(*tx1559);
  eth_tx_service()->tx_state_manager_->AddOrUpdateTx(meta);

  eth_tx_service()->RetryTransaction(
      "002", base::BindOnce(&AddUnapprovedTransactionSuccessCallback,
                            &callback_called, &tx_meta_id));

  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);
  tx_meta = eth_tx_service_->GetTxForTesting(tx_meta_id);
  ASSERT_TRUE(tx_meta);
  auto* tx1559_ptr = reinterpret_cast<Eip1559Transaction*>(tx_meta->tx.get());
  EXPECT_EQ(*tx1559_ptr, tx1559.value());

  // Non-exist transaction should fail.
  callback_called = false;
  eth_tx_service()->RetryTransaction(
      "123", base::BindOnce(&AddUnapprovedTransactionFailureCallback,
                            &callback_called));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);

  // Retry unapproved transaction should fail.
  callback_called = false;
  eth_tx_service()->RetryTransaction(
      tx_meta_id, base::BindOnce(&AddUnapprovedTransactionFailureCallback,
                                 &callback_called));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);
}

TEST_F(EthTxServiceUnitTest, MakeERC721TransferFromDataTxType) {
  const std::string contract_safe_transfer_from =
      "0x0d8775f648430679a709e98d2b0cb6250d2887ef";
  const std::string contract_transfer_from =
      "0x0d8775f648430679a709e98d2b0cb6250d2887ee";

  url_loader_factory_.SetInterceptor(
      base::BindLambdaForTesting([&](const network::ResourceRequest& request) {
        base::StringPiece request_string(request.request_body->elements()
                                             ->at(0)
                                             .As<network::DataElementBytes>()
                                             .AsStringPiece());
        if (request_string.find(contract_safe_transfer_from) !=
            std::string::npos) {
          url_loader_factory_.AddResponse(
              request.url.spec(),
              "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":"
              "\"0x0000000000000000000000000000000000000000000000000000000000"
              "000001\"}");
        } else if (request_string.find(contract_transfer_from) !=
                   std::string::npos) {
          url_loader_factory_.AddResponse(
              request.url.spec(),
              "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":"
              "\"0x0000000000000000000000000000000000000000000000000000000000"
              "000000\"}");
        }
      }));

  auto run_loop = std::make_unique<base::RunLoop>();
  eth_tx_service()->MakeERC721TransferFromData(
      "0xBFb30a082f650C2A15D0632f0e87bE4F8e64460f",
      "0xBFb30a082f650C2A15D0632f0e87bE4F8e64460a", "0xf",
      contract_safe_transfer_from,
      base::BindOnce(&MakeERC721TransferFromDataCallback, run_loop.get(), true,
                     mojom::TransactionType::ERC721SafeTransferFrom));
  run_loop->Run();

  run_loop.reset(new base::RunLoop());
  eth_tx_service()->MakeERC721TransferFromData(
      "0xBFb30a082f650C2A15D0632f0e87bE4F8e64460f",
      "0xBFb30a082f650C2A15D0632f0e87bE4F8e64460a", "0xf",
      contract_transfer_from,
      base::BindOnce(&MakeERC721TransferFromDataCallback, run_loop.get(), true,
                     mojom::TransactionType::ERC721TransferFrom));
  run_loop->Run();

  // Invalid token ID should fail.
  run_loop.reset(new base::RunLoop());
  eth_tx_service()->MakeERC721TransferFromData(
      "0xBFb30a082f650C2A15D0632f0e87bE4F8e64460f",
      "0xBFb30a082f650C2A15D0632f0e87bE4F8e64460a", "1",
      "0x0d8775f648430679a709e98d2b0cb6250d2887ee",
      base::BindOnce(&MakeERC721TransferFromDataCallback, run_loop.get(), false,
                     mojom::TransactionType::Other));
  run_loop->Run();
}

TEST_F(EthTxServiceUnitTest, Reset) {
  eth_tx_service()->known_no_pending_tx = true;
  eth_tx_service()->eth_block_tracker_->Start(base::Seconds(10));
  EXPECT_TRUE(eth_tx_service()->eth_block_tracker_->IsRunning());
  EthTxStateManager::TxMeta meta;
  meta.id = "001";
  meta.from = EthAddress::FromHex("0xbe862ad9abfe6f22bcb087716c7d89a26051f74a");
  meta.status = mojom::TransactionStatus::Unapproved;
  auto tx_data = mojom::TxData::New(
      "0x1", "0x1", "0x0974", "0xbe862ad9abfe6f22bcb087716c7d89a26051f74c",
      "0x016345785d8a0000", std::vector<uint8_t>());
  auto tx = EthTransaction::FromTxData(tx_data, false);
  meta.tx = std::make_unique<EthTransaction>(*tx);
  eth_tx_service()->tx_state_manager_->AddOrUpdateTx(meta);
  EXPECT_TRUE(GetPrefs()->HasPrefPath(kBraveWalletTransactions));

  eth_tx_service()->Reset();

  EXPECT_FALSE(eth_tx_service()->known_no_pending_tx);
  EXPECT_FALSE(eth_tx_service()->eth_block_tracker_->IsRunning());
  EXPECT_FALSE(GetPrefs()->HasPrefPath(kBraveWalletTransactions));
}

}  //  namespace brave_wallet
