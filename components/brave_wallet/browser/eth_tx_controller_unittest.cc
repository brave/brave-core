/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/eth_tx_controller.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/callback_helpers.h"
#include "base/json/json_reader.h"
#include "base/test/bind.h"
#include "brave/components/brave_wallet/browser/asset_ratio_controller.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/brave_wallet_prefs.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/eip1559_transaction.h"
#include "brave/components/brave_wallet/browser/eth_json_rpc_controller.h"
#include "brave/components/brave_wallet/browser/eth_nonce_tracker.h"
#include "brave/components/brave_wallet/browser/eth_transaction.h"
#include "brave/components/brave_wallet/browser/eth_tx_state_manager.h"
#include "brave/components/brave_wallet/browser/hd_keyring.h"
#include "brave/components/brave_wallet/browser/keyring_controller.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
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

}  // namespace

class TestEthTxControllerObserver
    : public brave_wallet::mojom::EthTxControllerObserver {
 public:
  TestEthTxControllerObserver(
      const std::string& expected_gas_price,
      const std::string& expected_gas_limit,
      const std::string& expected_max_priority_fee_per_gas = "",
      const std::string& expected_max_fee_per_gas = "",
      mojom::TransactionStatus expected_status =
          mojom::TransactionStatus::Unapproved)
      : expected_gas_price_(expected_gas_price),
        expected_gas_limit_(expected_gas_limit),
        expected_max_priority_fee_per_gas_(expected_max_priority_fee_per_gas),
        expected_max_fee_per_gas_(expected_max_fee_per_gas),
        expected_status_(expected_status) {}

  void OnNewUnapprovedTx(mojom::TransactionInfoPtr tx) override {}

  void OnUnapprovedTxUpdated(mojom::TransactionInfoPtr tx) override {
    EXPECT_EQ(tx->tx_data->base_data->gas_price,
              base::ToLowerASCII(expected_gas_price_));
    EXPECT_EQ(tx->tx_data->base_data->gas_limit,
              base::ToLowerASCII(expected_gas_limit_));
    EXPECT_EQ(tx->tx_data->max_priority_fee_per_gas,
              base::ToLowerASCII(expected_max_priority_fee_per_gas_));
    EXPECT_EQ(tx->tx_data->max_fee_per_gas,
              base::ToLowerASCII(expected_max_fee_per_gas_));
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
  mojo::PendingRemote<brave_wallet::mojom::EthTxControllerObserver>
  GetReceiver() {
    return observer_receiver_.BindNewPipeAndPassRemote();
  }

 private:
  std::string expected_gas_price_;
  std::string expected_gas_limit_;
  std::string expected_max_priority_fee_per_gas_;
  std::string expected_max_fee_per_gas_;
  bool tx_updated_ = false;
  bool tx_status_changed_ = false;
  mojom::TransactionStatus expected_status_;
  mojo::Receiver<brave_wallet::mojom::EthTxControllerObserver>
      observer_receiver_{this};
};

class EthTxControllerUnitTest : public testing::Test {
 public:
  EthTxControllerUnitTest()
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
                "\"0x595698248593c0000\"}");
          } else if (*method == "eth_getTransactionCount") {
            url_loader_factory_.AddResponse(
                request.url.spec(),
                "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":"
                "\"1\"}");
          }
        }));

    user_prefs::UserPrefs::Set(browser_context_.get(), &prefs_);
    brave_wallet::RegisterProfilePrefs(prefs_.registry());
    rpc_controller_.reset(
        new EthJsonRpcController(shared_url_loader_factory_, &prefs_));
    keyring_controller_.reset(new KeyringController(&prefs_));
    asset_ratio_controller_.reset(
        new AssetRatioController(shared_url_loader_factory_));

    auto tx_state_manager = std::make_unique<EthTxStateManager>(
        &prefs_, rpc_controller_->MakeRemote());
    auto nonce_tracker = std::make_unique<EthNonceTracker>(
        tx_state_manager.get(), rpc_controller_.get());
    auto pending_tx_tracker = std::make_unique<EthPendingTxTracker>(
        tx_state_manager.get(), rpc_controller_.get(), nonce_tracker.get());

    eth_tx_controller_.reset(new EthTxController(
        rpc_controller_.get(), keyring_controller_.get(),
        asset_ratio_controller_.get(), std::move(tx_state_manager),
        std::move(nonce_tracker), std::move(pending_tx_tracker), &prefs_));

    base::RunLoop run_loop;
    rpc_controller_->SetNetwork(brave_wallet::mojom::kLocalhostChainId,
                                base::BindLambdaForTesting([&](bool success) {
                                  EXPECT_TRUE(success);
                                  run_loop.Quit();
                                }));
    run_loop.Run();
    keyring_controller_->CreateWallet("testing123", base::DoNothing());
    base::RunLoop().RunUntilIdle();
    keyring_controller_->AddAccount("Account 1", base::DoNothing());
    base::RunLoop().RunUntilIdle();

    ASSERT_TRUE(base::HexStringToBytes(
        "095ea7b3000000000000000000000000BFb30a082f650C2A15D0632f0e87bE4F8e6446"
        "0f0000000000000000000000000000000000000000000000003fffffffffffffff",
        &data_));
  }

  std::string from() {
    return keyring_controller_->default_keyring_->GetAddress(0);
  }

  EthTxController* eth_tx_controller() { return eth_tx_controller_.get(); }

  void SetErrorInterceptor() {
    url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
        [&](const network::ResourceRequest& request) {
          url_loader_factory_.ClearResponses();
          url_loader_factory_.AddResponse(request.url.spec(), "",
                                          net::HTTP_REQUEST_TIMEOUT);
        }));
  }

 protected:
  content::BrowserTaskEnvironment task_environment_;
  std::unique_ptr<content::TestBrowserContext> browser_context_;
  sync_preferences::TestingPrefServiceSyncable prefs_;
  network::TestURLLoaderFactory url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;

  std::unique_ptr<EthJsonRpcController> rpc_controller_;
  std::unique_ptr<KeyringController> keyring_controller_;
  std::unique_ptr<EthTxController> eth_tx_controller_;
  std::unique_ptr<AssetRatioController> asset_ratio_controller_;
  std::vector<uint8_t> data_;
};

TEST_F(EthTxControllerUnitTest,
       AddUnapprovedTransactionWithGasPriceAndGasLimit) {
  std::string gas_price = "0x09184e72a000";
  std::string gas_limit = "0x0974";
  auto tx_data =
      mojom::TxData::New("0x06", gas_price, gas_limit,
                         "0xbe862ad9abfe6f22bcb087716c7d89a26051f74c",
                         "0x016345785d8a0000", data_);
  bool callback_called = false;
  std::string tx_meta_id;

  eth_tx_controller_->AddUnapprovedTransaction(
      std::move(tx_data), from(),
      base::BindOnce(&AddUnapprovedTransactionSuccessCallback, &callback_called,
                     &tx_meta_id));

  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);
  auto tx_meta = eth_tx_controller_->GetTxForTesting(tx_meta_id);
  EXPECT_TRUE(tx_meta);

  uint256_t gas_price_value;
  uint256_t gas_limit_value;
  EXPECT_TRUE(HexValueToUint256(gas_price, &gas_price_value));
  EXPECT_TRUE(HexValueToUint256(gas_limit, &gas_limit_value));
  EXPECT_EQ(tx_meta->tx->gas_price(), gas_price_value);
  EXPECT_EQ(tx_meta->tx->gas_limit(), gas_limit_value);
}

TEST_F(EthTxControllerUnitTest, AddUnapprovedTransactionWithoutGasLimit) {
  std::string gas_price = "0x09184e72a000";
  auto tx_data =
      mojom::TxData::New("0x06", gas_price, "" /* gas_limit */,
                         "0xbe862ad9abfe6f22bcb087716c7d89a26051f74c",
                         "0x016345785d8a0000", data_);
  bool callback_called = false;
  std::string tx_meta_id;

  eth_tx_controller_->AddUnapprovedTransaction(
      tx_data.Clone(), from(),
      base::BindOnce(&AddUnapprovedTransactionSuccessCallback, &callback_called,
                     &tx_meta_id));

  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);
  auto tx_meta = eth_tx_controller_->GetTxForTesting(tx_meta_id);
  EXPECT_TRUE(tx_meta);

  uint256_t gas_price_value;
  uint256_t gas_limit_value;
  EXPECT_TRUE(HexValueToUint256(gas_price, &gas_price_value));
  // Gas limit should be filled by requesting eth_estimateGas.
  EXPECT_TRUE(HexValueToUint256("0x9604", &gas_limit_value));
  EXPECT_EQ(tx_meta->tx->gas_price(), gas_price_value);
  EXPECT_EQ(tx_meta->tx->gas_limit(), gas_limit_value);

  SetErrorInterceptor();
  callback_called = false;
  eth_tx_controller_->AddUnapprovedTransaction(
      std::move(tx_data), from(),
      base::BindOnce(&AddUnapprovedTransactionFailureCallback,
                     &callback_called));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);
}

TEST_F(EthTxControllerUnitTest, AddUnapprovedTransactionWithoutGasPrice) {
  std::string gas_limit = "0x0974";
  auto tx_data =
      mojom::TxData::New("0x06", "" /* gas_price */, gas_limit,
                         "0xbe862ad9abfe6f22bcb087716c7d89a26051f74c",
                         "0x016345785d8a0000", data_);
  bool callback_called = false;
  std::string tx_meta_id;

  eth_tx_controller_->AddUnapprovedTransaction(
      tx_data.Clone(), from(),
      base::BindOnce(&AddUnapprovedTransactionSuccessCallback, &callback_called,
                     &tx_meta_id));

  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);
  auto tx_meta = eth_tx_controller_->GetTxForTesting(tx_meta_id);
  EXPECT_TRUE(tx_meta);

  uint256_t gas_price_value;
  uint256_t gas_limit_value;
  // Gas price should be filled by requesting eth_gasPrice.
  EXPECT_TRUE(HexValueToUint256("0x595698248593c0000", &gas_price_value));
  EXPECT_TRUE(HexValueToUint256(gas_limit, &gas_limit_value));
  EXPECT_EQ(tx_meta->tx->gas_price(), gas_price_value);
  EXPECT_EQ(tx_meta->tx->gas_limit(), gas_limit_value);

  SetErrorInterceptor();
  callback_called = false;
  eth_tx_controller_->AddUnapprovedTransaction(
      std::move(tx_data), from(),
      base::BindOnce(&AddUnapprovedTransactionFailureCallback,
                     &callback_called));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);
}

TEST_F(EthTxControllerUnitTest,
       AddUnapprovedTransactionWithoutGasPriceAndGasLimit) {
  auto tx_data =
      mojom::TxData::New("0x06", "" /* gas_price */, "" /* gas_limit */,
                         "0xbe862ad9abfe6f22bcb087716c7d89a26051f74c",
                         "0x016345785d8a0000", data_);
  bool callback_called = false;
  std::string tx_meta_id;

  eth_tx_controller_->AddUnapprovedTransaction(
      tx_data.Clone(), from(),
      base::BindOnce(&AddUnapprovedTransactionSuccessCallback, &callback_called,
                     &tx_meta_id));

  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);
  auto tx_meta = eth_tx_controller_->GetTxForTesting(tx_meta_id);
  EXPECT_TRUE(tx_meta);

  uint256_t gas_price_value;
  uint256_t gas_limit_value;
  EXPECT_TRUE(HexValueToUint256("0x595698248593c0000", &gas_price_value));
  EXPECT_TRUE(HexValueToUint256("0x9604", &gas_limit_value));
  EXPECT_EQ(tx_meta->tx->gas_price(), gas_price_value);
  EXPECT_EQ(tx_meta->tx->gas_limit(), gas_limit_value);

  SetErrorInterceptor();
  callback_called = false;
  eth_tx_controller_->AddUnapprovedTransaction(
      std::move(tx_data), from(),
      base::BindOnce(&AddUnapprovedTransactionFailureCallback,
                     &callback_called));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);
}

TEST_F(EthTxControllerUnitTest,
       AddUnapprovedTransactionWithoutGasPriceAndGasLimitForEthSend) {
  auto tx_data =
      mojom::TxData::New("0x06", "" /* gas_price*/, "" /* gas_limit */,
                         "0xbe862ad9abfe6f22bcb087716c7d89a26051f74c",
                         "0x016345785d8a0000", std::vector<uint8_t>());
  bool callback_called = false;
  std::string tx_meta_id;

  eth_tx_controller_->AddUnapprovedTransaction(
      std::move(tx_data), from(),
      base::BindOnce(&AddUnapprovedTransactionSuccessCallback, &callback_called,
                     &tx_meta_id));

  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);
  auto tx_meta = eth_tx_controller_->GetTxForTesting(tx_meta_id);
  EXPECT_TRUE(tx_meta);

  // Default value will be used.
  EXPECT_EQ(tx_meta->tx->gas_price(), kDefaultSendEthGasPrice);
  EXPECT_EQ(tx_meta->tx->gas_limit(), kDefaultSendEthGasLimit);
}

TEST_F(EthTxControllerUnitTest, SetGasPriceAndLimitForUnapprovedTransaction) {
  auto tx_data =
      mojom::TxData::New("0x06", "" /* gas_price*/, "" /* gas_limit */,
                         "0xbe862ad9abfe6f22bcb087716c7d89a26051f74c",
                         "0x016345785d8a0000", std::vector<uint8_t>());
  bool callback_called = false;
  std::string tx_meta_id;

  eth_tx_controller_->AddUnapprovedTransaction(
      std::move(tx_data), from(),
      base::BindOnce(&AddUnapprovedTransactionSuccessCallback, &callback_called,
                     &tx_meta_id));

  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);
  auto tx_meta = eth_tx_controller_->GetTxForTesting(tx_meta_id);
  EXPECT_TRUE(tx_meta);

  // Default value will be used.
  EXPECT_EQ(tx_meta->tx->gas_price(), kDefaultSendEthGasPrice);
  EXPECT_EQ(tx_meta->tx->gas_limit(), kDefaultSendEthGasLimit);

  // Fail if transaction is not found.
  callback_called = false;
  eth_tx_controller_->SetGasPriceAndLimitForUnapprovedTransaction(
      "not_exist", Uint256ValueToHex(kDefaultSendEthGasPrice),
      Uint256ValueToHex(kDefaultSendEthGasLimit),
      base::BindLambdaForTesting([&](bool success) {
        EXPECT_FALSE(success);
        callback_called = true;
      }));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);

  // Fail if passing an empty gas limit.
  callback_called = false;
  eth_tx_controller_->SetGasPriceAndLimitForUnapprovedTransaction(
      tx_meta_id, "", Uint256ValueToHex(kDefaultSendEthGasLimit),
      base::BindLambdaForTesting([&](bool success) {
        EXPECT_FALSE(success);
        callback_called = true;
      }));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);

  // Fail if passing an empty gas price.
  callback_called = false;
  eth_tx_controller_->SetGasPriceAndLimitForUnapprovedTransaction(
      tx_meta_id, Uint256ValueToHex(kDefaultSendEthGasPrice), "",
      base::BindLambdaForTesting([&](bool success) {
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

  TestEthTxControllerObserver observer(update_gas_price_hex_string,
                                       update_gas_limit_hex_string);
  eth_tx_controller_->AddObserver(observer.GetReceiver());

  callback_called = false;
  eth_tx_controller_->SetGasPriceAndLimitForUnapprovedTransaction(
      tx_meta_id, update_gas_price_hex_string, update_gas_limit_hex_string,
      base::BindLambdaForTesting([&](bool success) {
        EXPECT_TRUE(success);
        callback_called = true;
      }));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);
  EXPECT_TRUE(observer.TxUpdated());

  // Get the updated TX.
  tx_meta = eth_tx_controller_->GetTxForTesting(tx_meta_id);
  EXPECT_TRUE(tx_meta);
  EXPECT_EQ(tx_meta->tx->gas_price(), update_gas_price);
  EXPECT_EQ(tx_meta->tx->gas_limit(), update_gas_limit);
}

TEST_F(EthTxControllerUnitTest, ValidateTxData) {
  std::string error_message;
  EXPECT_TRUE(EthTxController::ValidateTxData(
      mojom::TxData::New("0x06", "0x09184e72a000", "0x0974",
                         "0xbe862ad9abfe6f22bcb087716c7d89a26051f74c",
                         "0x016345785d8a0000", std::vector<uint8_t>()),
      &error_message));

  // Make sure if params are specified that they are valid hex strings
  EXPECT_FALSE(EthTxController::ValidateTxData(
      mojom::TxData::New("hello", "0x09184e72a000", "0x0974",
                         "0xbe862ad9abfe6f22bcb087716c7d89a26051f74c",
                         "0x016345785d8a0000", std::vector<uint8_t>()),
      &error_message));
  EXPECT_FALSE(EthTxController::ValidateTxData(
      mojom::TxData::New("0x06", "hello", "0x0974",
                         "0xbe862ad9abfe6f22bcb087716c7d89a26051f74c",
                         "0x016345785d8a0000", std::vector<uint8_t>()),
      &error_message));
  EXPECT_FALSE(EthTxController::ValidateTxData(
      mojom::TxData::New("0x06", "0x09184e72a000", "hello",
                         "0xbe862ad9abfe6f22bcb087716c7d89a26051f74c",
                         "0x016345785d8a0000", std::vector<uint8_t>()),
      &error_message));
  EXPECT_FALSE(EthTxController::ValidateTxData(
      mojom::TxData::New("0x06", "0x09184e72a000", "0x0974", "hello",
                         "0x016345785d8a0000", std::vector<uint8_t>()),
      &error_message));
  EXPECT_FALSE(EthTxController::ValidateTxData(
      mojom::TxData::New("0x06", "0x09184e72a000", "0x0974",
                         "0xbe862ad9abfe6f22bcb087716c7d89a26051f74c", "hello",
                         std::vector<uint8_t>()),
      &error_message));
  // to must not only be a valid hex string but also an address
  EXPECT_FALSE(EthTxController::ValidateTxData(
      mojom::TxData::New("0x06", "0x09184e72a000", "0x0974",
                         "0xbe",  // Invalid address
                         "hello", std::vector<uint8_t>()),
      &error_message));

  // To can't be missing if Data is missing
  EXPECT_FALSE(EthTxController::ValidateTxData(
      mojom::TxData::New("0x06", "0x09184e72a000", "0x0974", "",
                         "0x016345785d8a0000", std::vector<uint8_t>()),
      &error_message));
}

TEST_F(EthTxControllerUnitTest, ValidateTxData1559) {
  std::string error_message;
  EXPECT_TRUE(EthTxController::ValidateTxData1559(
      mojom::TxData1559::New(
          mojom::TxData::New("0x00", "", "0x00",
                             "0x0101010101010101010101010101010101010101",
                             "0x00", std::vector<uint8_t>()),
          "0x04", "0x0", "0x1", nullptr),
      &error_message));

  // Can't specify both gas price and max fee per gas
  EXPECT_FALSE(EthTxController::ValidateTxData1559(
      mojom::TxData1559::New(
          mojom::TxData::New("0x00", "0x1", "0x00",
                             "0x0101010101010101010101010101010101010101",
                             "0x00", std::vector<uint8_t>()),
          "0x04", "0x0", "0x1", nullptr),
      &error_message));
}

TEST_F(EthTxControllerUnitTest, ProcessLedgerSignature) {
  auto tx_data =
      mojom::TxData::New("0x06", "" /* gas_price */, "" /* gas_limit */,
                         "0xbe862ad9abfe6f22bcb087716c7d89a26051f74c",
                         "0x016345785d8a0000", data_);
  bool callback_called = false;
  std::string tx_meta_id;

  eth_tx_controller_->AddUnapprovedTransaction(
      tx_data.Clone(), from(),
      base::BindOnce(&AddUnapprovedTransactionSuccessCallback, &callback_called,
                     &tx_meta_id));
  TestEthTxControllerObserver observer("", "");
  eth_tx_controller_->AddObserver(observer.GetReceiver());
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);
  callback_called = false;
  eth_tx_controller_->ProcessLedgerSignature(
      tx_meta_id, "0x00",
      "93b9121e82df014428924df439ff044f89c205dd76a194f8b11f50d2eade744e",
      "7aa705c9144742836b7fbbd0745c57f67b60df7b8d1790fe59f91ed8d2bfc11d",
      base::BindLambdaForTesting([&](bool success) {
        EXPECT_TRUE(success);
        auto tx_meta = eth_tx_controller_->GetTxForTesting(tx_meta_id);
        EXPECT_TRUE(tx_meta);
        EXPECT_EQ(tx_meta->status, mojom::TransactionStatus::Unapproved);
        callback_called = true;
      }));
  base::RunLoop().RunUntilIdle();
  ASSERT_TRUE(callback_called);
  ASSERT_FALSE(observer.TxStatusChanged());
}

TEST_F(EthTxControllerUnitTest, ProcessLedgerSignatureFail) {
  auto tx_data =
      mojom::TxData::New("0x06", "" /* gas_price */, "" /* gas_limit */,
                         "0xbe862ad9abfe6f22bcb087716c7d89a26051f74c",
                         "0x016345785d8a0000", data_);
  bool callback_called = false;
  std::string tx_meta_id;

  eth_tx_controller_->AddUnapprovedTransaction(
      tx_data.Clone(), from(),
      base::BindOnce(&AddUnapprovedTransactionSuccessCallback, &callback_called,
                     &tx_meta_id));
  TestEthTxControllerObserver observer("", "", "", "",
                                       mojom::TransactionStatus::Error);
  eth_tx_controller_->AddObserver(observer.GetReceiver());
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);
  callback_called = false;
  eth_tx_controller_->ProcessLedgerSignature(
      tx_meta_id, "0x00", "9ff044f89c205dd76a194f8b11f50d2eade744e", "",
      base::BindLambdaForTesting([&](bool success) {
        EXPECT_FALSE(success);
        auto tx_meta = eth_tx_controller_->GetTxForTesting(tx_meta_id);
        EXPECT_TRUE(tx_meta);
        EXPECT_EQ(tx_meta->status, mojom::TransactionStatus::Error);
        callback_called = true;
      }));
  base::RunLoop().RunUntilIdle();
  ASSERT_TRUE(callback_called);
  ASSERT_TRUE(observer.TxStatusChanged());
  observer.Reset();
  callback_called = false;
  eth_tx_controller_->ProcessLedgerSignature(
      "-1", "0x00", "9ff044f89c205dd76a194f8b11f50d2eade744e", "",
      base::BindLambdaForTesting([&](bool success) {
        EXPECT_FALSE(success);
        callback_called = true;
      }));
  base::RunLoop().RunUntilIdle();
  ASSERT_TRUE(callback_called);
  ASSERT_FALSE(observer.TxStatusChanged());
}

TEST_F(EthTxControllerUnitTest, ApproveHardwareTransaction) {
  auto tx_data =
      mojom::TxData::New("0x06", "" /* gas_price */, "" /* gas_limit */,
                         "0xbe862ad9abfe6f22bcb087716c7d89a26051f74c",
                         "0x016345785d8a0000", data_);
  bool callback_called = false;
  std::string tx_meta_id;

  eth_tx_controller_->AddUnapprovedTransaction(
      tx_data.Clone(), from(),
      base::BindOnce(&AddUnapprovedTransactionSuccessCallback, &callback_called,
                     &tx_meta_id));

  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);
  TestEthTxControllerObserver observer("", "", "", "",
                                       mojom::TransactionStatus::Approved);
  eth_tx_controller_->AddObserver(observer.GetReceiver());
  callback_called = false;
  eth_tx_controller_->ApproveHardwareTransaction(
      tx_meta_id,
      base::BindLambdaForTesting([&](bool success, const std::string& result) {
        EXPECT_TRUE(success);
        EXPECT_EQ(result,
                  "0xf87780890595698248593c000082960494be862ad9abfe6f22"
                  "bcb087716c7d89a26051f74c88016345785d8a0000b844095ea7b3000000"
                  "000000000000000000bfb30a082f650c2a15d0632f0e87be4f8e64460f00"
                  "00000000000000000000000000000000000000000000003fffffffffffff"
                  "ff8205398080");
        auto tx_meta = eth_tx_controller_->GetTxForTesting(tx_meta_id);
        EXPECT_TRUE(tx_meta);
        EXPECT_EQ(tx_meta->status, mojom::TransactionStatus::Approved);
        callback_called = true;
      }));
  base::RunLoop().RunUntilIdle();
  ASSERT_TRUE(callback_called);
  ASSERT_TRUE(observer.TxStatusChanged());
}

TEST_F(EthTxControllerUnitTest, ApproveHardwareTransaction1559) {
  auto tx_data = mojom::TxData1559::New(
      mojom::TxData::New("0x00", "", "0x01",
                         "0x0101010101010101010101010101010101010101", "0x00",
                         std::vector<uint8_t>()),
      "0x04", "0x1", "0x1", nullptr);

  bool callback_called = false;
  std::string tx_meta_id;

  eth_tx_controller_->AddUnapproved1559Transaction(
      tx_data.Clone(), from(),
      base::BindOnce(&AddUnapprovedTransactionSuccessCallback, &callback_called,
                     &tx_meta_id));

  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);
  TestEthTxControllerObserver observer("", "", "", "",
                                       mojom::TransactionStatus::Approved);
  eth_tx_controller_->AddObserver(observer.GetReceiver());
  callback_called = false;
  eth_tx_controller_->ApproveHardwareTransaction(
      tx_meta_id,
      base::BindLambdaForTesting([&](bool success, const std::string& result) {
        EXPECT_TRUE(success);
        EXPECT_EQ(
            result,
            "0x02dd04800101019401010101010101010101010101010101010101018080c0");
        auto tx_meta = eth_tx_controller_->GetTxForTesting(tx_meta_id);
        EXPECT_TRUE(tx_meta);
        EXPECT_EQ(tx_meta->status, mojom::TransactionStatus::Approved);

        callback_called = true;
      }));
  base::RunLoop().RunUntilIdle();
  ASSERT_TRUE(callback_called);
  ASSERT_TRUE(observer.TxStatusChanged());
}

TEST_F(EthTxControllerUnitTest, ApproveHardwareTransactionFail) {
  bool callback_called = false;
  TestEthTxControllerObserver observer("", "");
  eth_tx_controller_->AddObserver(observer.GetReceiver());
  eth_tx_controller_->ApproveHardwareTransaction(
      std::string(),
      base::BindLambdaForTesting([&](bool success, const std::string& result) {
        EXPECT_FALSE(success);
        ASSERT_TRUE(result.empty());
        callback_called = true;
      }));
  base::RunLoop().RunUntilIdle();
  ASSERT_TRUE(callback_called);
  ASSERT_FALSE(observer.TxStatusChanged());
}

TEST_F(EthTxControllerUnitTest,
       AddUnapproved1559TransactionWithGasFeeAndLimit) {
  const std::string gas_limit = "0x0974";

  auto tx_data = mojom::TxData1559::New(
      mojom::TxData::New("0x1", "", gas_limit,
                         "0xbe862ad9abfe6f22bcb087716c7d89a26051f74c",
                         "0x016345785d8a0000", data_),
      "0x04", "0x77359400" /* 2 Gwei */, "0xb2d05e000" /* 48 Gwei */, nullptr);

  bool callback_called = false;
  std::string tx_meta_id;

  eth_tx_controller_->AddUnapproved1559Transaction(
      std::move(tx_data), from(),
      base::BindOnce(&AddUnapprovedTransactionSuccessCallback, &callback_called,
                     &tx_meta_id));

  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);
  auto tx_meta = eth_tx_controller_->GetTxForTesting(tx_meta_id);
  EXPECT_TRUE(tx_meta);

  uint256_t gas_limit_value;
  EXPECT_TRUE(HexValueToUint256(gas_limit, &gas_limit_value));
  EXPECT_EQ(tx_meta->tx->gas_limit(), gas_limit_value);
  auto* tx1559 = reinterpret_cast<Eip1559Transaction*>(tx_meta->tx.get());
  EXPECT_EQ(tx1559->max_priority_fee_per_gas(), uint256_t(2) * uint256_t(1e9));
  EXPECT_EQ(tx1559->max_fee_per_gas(), uint256_t(48) * uint256_t(1e9));
  EXPECT_EQ(tx1559->gas_estimation(), Eip1559Transaction::GasEstimation());
}

TEST_F(EthTxControllerUnitTest, AddUnapproved1559TransactionWithoutGasLimit) {
  auto tx_data = mojom::TxData1559::New(
      mojom::TxData::New("0x1", "", "",
                         "0xbe862ad9abfe6f22bcb087716c7d89a26051f74c",
                         "0x016345785d8a0000", data_),
      "0x04", "0x77359400" /* 2 Gwei */, "0xb2d05e000" /* 48 Gwei */, nullptr);

  bool callback_called = false;
  std::string tx_meta_id;

  eth_tx_controller_->AddUnapproved1559Transaction(
      std::move(tx_data), from(),
      base::BindOnce(&AddUnapprovedTransactionSuccessCallback, &callback_called,
                     &tx_meta_id));

  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);
  auto tx_meta = eth_tx_controller_->GetTxForTesting(tx_meta_id);
  EXPECT_TRUE(tx_meta);

  uint256_t gas_limit_value;
  EXPECT_TRUE(HexValueToUint256("0x9604", &gas_limit_value));
  EXPECT_EQ(tx_meta->tx->gas_limit(), gas_limit_value);
  auto* tx1559 = reinterpret_cast<Eip1559Transaction*>(tx_meta->tx.get());
  EXPECT_EQ(tx1559->max_priority_fee_per_gas(), uint256_t(2) * uint256_t(1e9));
  EXPECT_EQ(tx1559->max_fee_per_gas(), uint256_t(48) * uint256_t(1e9));
  EXPECT_EQ(tx1559->gas_estimation(), Eip1559Transaction::GasEstimation());
}

TEST_F(EthTxControllerUnitTest, AddUnapproved1559TransactionWithoutGasFee) {
  const std::string gas_limit = "0x0974";
  auto tx_data = mojom::TxData1559::New(
      mojom::TxData::New("0x1", "", gas_limit,
                         "0xbe862ad9abfe6f22bcb087716c7d89a26051f74c",
                         "0x016345785d8a0000", data_),
      "0x04", "", "", nullptr);

  bool callback_called = false;
  std::string tx_meta_id;

  eth_tx_controller_->AddUnapproved1559Transaction(
      std::move(tx_data), from(),
      base::BindOnce(&AddUnapprovedTransactionSuccessCallback, &callback_called,
                     &tx_meta_id));

  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);
  auto tx_meta = eth_tx_controller_->GetTxForTesting(tx_meta_id);
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

TEST_F(EthTxControllerUnitTest,
       AddUnapproved1559TransactionWithoutGasFeeAndLimit) {
  auto tx_data = mojom::TxData1559::New(
      mojom::TxData::New("0x1", "", "",
                         "0xbe862ad9abfe6f22bcb087716c7d89a26051f74c",
                         "0x016345785d8a0000", data_),
      "0x04", "", "", nullptr);

  bool callback_called = false;
  std::string tx_meta_id;

  eth_tx_controller_->AddUnapproved1559Transaction(
      std::move(tx_data), from(),
      base::BindOnce(&AddUnapprovedTransactionSuccessCallback, &callback_called,
                     &tx_meta_id));

  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);
  auto tx_meta = eth_tx_controller_->GetTxForTesting(tx_meta_id);
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

TEST_F(EthTxControllerUnitTest,
       AddUnapproved1559TransactionWithoutGasFeeAndLimitForEthSend) {
  auto tx_data = mojom::TxData1559::New(
      mojom::TxData::New("0x1", "", "",
                         "0xbe862ad9abfe6f22bcb087716c7d89a26051f74c",
                         "0x016345785d8a0000", std::vector<uint8_t>()),
      "0x04", "", "", nullptr);

  bool callback_called = false;
  std::string tx_meta_id;

  eth_tx_controller_->AddUnapproved1559Transaction(
      std::move(tx_data), from(),
      base::BindOnce(&AddUnapprovedTransactionSuccessCallback, &callback_called,
                     &tx_meta_id));

  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);
  auto tx_meta = eth_tx_controller_->GetTxForTesting(tx_meta_id);
  EXPECT_TRUE(tx_meta);

  // Default gas limit value will be used.
  EXPECT_EQ(tx_meta->tx->gas_limit(), kDefaultSendEthGasLimit);

  // Gas fee and estimation should be filled by gas oracle.
  auto* tx1559 = reinterpret_cast<Eip1559Transaction*>(tx_meta->tx.get());
  EXPECT_EQ(tx1559->max_priority_fee_per_gas(), uint256_t(2) * uint256_t(1e9));
  EXPECT_EQ(tx1559->max_fee_per_gas(), uint256_t(48) * uint256_t(1e9));
  EXPECT_EQ(tx1559->gas_estimation(),
            Eip1559Transaction::GasEstimation::FromMojomGasEstimation1559(
                GetMojomGasEstimation()));
}

TEST_F(EthTxControllerUnitTest,
       AddUnapproved1559TransactionWithGasFeeAndLimitForEthSend) {
  const std::string gas_limit = "0x0974";

  auto tx_data = mojom::TxData1559::New(
      mojom::TxData::New("0x1", "", gas_limit,
                         "0xbe862ad9abfe6f22bcb087716c7d89a26051f74c",
                         "0x016345785d8a0000", std::vector<uint8_t>()),
      "0x04", "0x77359400" /* 2 Gwei */, "0xb2d05e000" /* 48 Gwei */, nullptr);

  bool callback_called = false;
  std::string tx_meta_id;

  eth_tx_controller_->AddUnapproved1559Transaction(
      std::move(tx_data), from(),
      base::BindOnce(&AddUnapprovedTransactionSuccessCallback, &callback_called,
                     &tx_meta_id));

  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);
  auto tx_meta = eth_tx_controller_->GetTxForTesting(tx_meta_id);
  EXPECT_TRUE(tx_meta);

  uint256_t gas_limit_value;
  EXPECT_TRUE(HexValueToUint256(gas_limit, &gas_limit_value));
  EXPECT_EQ(tx_meta->tx->gas_limit(), gas_limit_value);
  auto* tx1559 = reinterpret_cast<Eip1559Transaction*>(tx_meta->tx.get());
  EXPECT_EQ(tx1559->max_priority_fee_per_gas(), uint256_t(2) * uint256_t(1e9));
  EXPECT_EQ(tx1559->max_fee_per_gas(), uint256_t(48) * uint256_t(1e9));
  EXPECT_EQ(tx1559->gas_estimation(), Eip1559Transaction::GasEstimation());
}

TEST_F(EthTxControllerUnitTest,
       AddUnapproved1559TransactionWithoutGasLimitForEthSend) {
  auto tx_data = mojom::TxData1559::New(
      mojom::TxData::New("0x1", "", "",
                         "0xbe862ad9abfe6f22bcb087716c7d89a26051f74c",
                         "0x016345785d8a0000", std::vector<uint8_t>()),
      "0x04", "0x77359400" /* 2 Gwei */, "0xb2d05e000" /* 48 Gwei */, nullptr);
  bool callback_called = false;
  std::string tx_meta_id;

  eth_tx_controller_->AddUnapproved1559Transaction(
      std::move(tx_data), from(),
      base::BindOnce(&AddUnapprovedTransactionSuccessCallback, &callback_called,
                     &tx_meta_id));

  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);
  auto tx_meta = eth_tx_controller_->GetTxForTesting(tx_meta_id);
  EXPECT_TRUE(tx_meta);

  // Default gas limit value will be used.
  EXPECT_EQ(tx_meta->tx->gas_limit(), kDefaultSendEthGasLimit);

  auto* tx1559 = reinterpret_cast<Eip1559Transaction*>(tx_meta->tx.get());
  EXPECT_EQ(tx1559->max_priority_fee_per_gas(), uint256_t(2) * uint256_t(1e9));
  EXPECT_EQ(tx1559->max_fee_per_gas(), uint256_t(48) * uint256_t(1e9));
  EXPECT_EQ(tx1559->gas_estimation(), Eip1559Transaction::GasEstimation());
}

TEST_F(EthTxControllerUnitTest,
       AddUnapproved1559TransactionWithoutGasFeeForEthSend) {
  const std::string gas_limit = "0x0974";
  auto tx_data = mojom::TxData1559::New(
      mojom::TxData::New("0x1", "", gas_limit,
                         "0xbe862ad9abfe6f22bcb087716c7d89a26051f74c",
                         "0x016345785d8a0000", std::vector<uint8_t>()),
      "0x04", "", "", nullptr);

  bool callback_called = false;
  std::string tx_meta_id;

  eth_tx_controller_->AddUnapproved1559Transaction(
      std::move(tx_data), from(),
      base::BindOnce(&AddUnapprovedTransactionSuccessCallback, &callback_called,
                     &tx_meta_id));

  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);
  auto tx_meta = eth_tx_controller_->GetTxForTesting(tx_meta_id);
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

TEST_F(EthTxControllerUnitTest, SetGasFeeAndLimitForUnapprovedTransaction) {
  auto tx_data = mojom::TxData1559::New(
      mojom::TxData::New("0x1", "", "",
                         "0xbe862ad9abfe6f22bcb087716c7d89a26051f74c",
                         "0x016345785d8a0000", data_),
      "0x04", "", "", nullptr);
  bool callback_called = false;
  std::string tx_meta_id;

  eth_tx_controller_->AddUnapproved1559Transaction(
      std::move(tx_data), from(),
      base::BindOnce(&AddUnapprovedTransactionSuccessCallback, &callback_called,
                     &tx_meta_id));

  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);
  auto tx_meta = eth_tx_controller_->GetTxForTesting(tx_meta_id);
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
  eth_tx_controller_->SetGasFeeAndLimitForUnapprovedTransaction(
      "not_exist", "0x1", "0x2", "0x3",
      base::BindLambdaForTesting([&](bool success) {
        EXPECT_FALSE(success);
        callback_called = true;
      }));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);

  // Fail if passing an empty gas limit.
  callback_called = false;
  eth_tx_controller_->SetGasFeeAndLimitForUnapprovedTransaction(
      tx_meta_id, "", "0x2", "0x3",
      base::BindLambdaForTesting([&](bool success) {
        EXPECT_FALSE(success);
        callback_called = true;
      }));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);

  // Fail if passing an empty max_priority_fee_per_gas.
  callback_called = false;
  eth_tx_controller_->SetGasFeeAndLimitForUnapprovedTransaction(
      tx_meta_id, "0x1", "", "0x3",
      base::BindLambdaForTesting([&](bool success) {
        EXPECT_FALSE(success);
        callback_called = true;
      }));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);

  // Fail if passing an empty max_fee_per_gas.
  callback_called = false;
  eth_tx_controller_->SetGasFeeAndLimitForUnapprovedTransaction(
      tx_meta_id, "0x1", "0x2", "",
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

  TestEthTxControllerObserver observer(
      "0x0", update_gas_limit_hex_string,
      update_max_priority_fee_per_gas_hex_string,
      update_max_fee_per_gas_hex_string);
  eth_tx_controller_->AddObserver(observer.GetReceiver());

  callback_called = false;
  eth_tx_controller_->SetGasFeeAndLimitForUnapprovedTransaction(
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
  tx_meta = eth_tx_controller_->GetTxForTesting(tx_meta_id);
  EXPECT_TRUE(tx_meta);
  EXPECT_EQ(tx_meta->tx->gas_limit(), update_gas_limit);
  tx1559 = reinterpret_cast<Eip1559Transaction*>(tx_meta->tx.get());
  EXPECT_EQ(tx1559->max_priority_fee_per_gas(),
            update_max_priority_fee_per_gas);
  EXPECT_EQ(tx1559->max_fee_per_gas(), update_max_fee_per_gas);
}

TEST_F(EthTxControllerUnitTest,
       SetGasFeeAndLimitForUnapprovedTransactionRejectNotEip1559) {
  auto tx_data =
      mojom::TxData::New("0x06", "0x09184e72a000", "0x0974",
                         "0xbe862ad9abfe6f22bcb087716c7d89a26051f74c",
                         "0x016345785d8a0000", data_);
  bool callback_called = false;
  std::string tx_meta_id;

  eth_tx_controller_->AddUnapprovedTransaction(
      std::move(tx_data), from(),
      base::BindOnce(&AddUnapprovedTransactionSuccessCallback, &callback_called,
                     &tx_meta_id));

  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);
  auto tx_meta = eth_tx_controller_->GetTxForTesting(tx_meta_id);
  EXPECT_TRUE(tx_meta);

  callback_called = false;
  eth_tx_controller_->SetGasFeeAndLimitForUnapprovedTransaction(
      tx_meta_id, "0x3344", "0x5566", "0xFED8",
      base::BindLambdaForTesting([&](bool success) {
        EXPECT_FALSE(success);
        callback_called = true;
      }));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);
}

TEST_F(EthTxControllerUnitTest, TestSubmittedToConfirmed) {
  EthAddress addr1 =
      EthAddress::FromHex("0x2f015c60e0be116b1f0cd534704db9c92118fb6a");
  EthAddress addr2 =
      EthAddress::FromHex("0x2f015c60e0be116b1f0cd534704db9c92118fb6b");
  base::RunLoop().RunUntilIdle();
  EthTxStateManager::TxMeta meta;
  meta.id = "001";
  meta.from = addr1;
  meta.status = mojom::TransactionStatus::Submitted;
  eth_tx_controller()->tx_state_manager_->AddOrUpdateTx(meta);
  meta.id = "002";
  meta.from = addr2;
  meta.tx->set_nonce(uint256_t(4));
  meta.status = mojom::TransactionStatus::Submitted;
  eth_tx_controller()->tx_state_manager_->AddOrUpdateTx(meta);

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
      base::TimeDelta::FromSeconds(kBlockTrackerDefaultTimeInSeconds - 1));
  auto tx_meta1 = eth_tx_controller()->GetTxForTesting("001");
  EXPECT_EQ(mojom::TransactionStatus::Submitted, tx_meta1->status);
  auto tx_meta2 = eth_tx_controller()->GetTxForTesting("002");
  EXPECT_EQ(mojom::TransactionStatus::Submitted, tx_meta1->status);

  task_environment_.FastForwardBy(base::TimeDelta::FromSeconds(1));
  tx_meta1 = eth_tx_controller()->GetTxForTesting("001");
  EXPECT_EQ(mojom::TransactionStatus::Confirmed, tx_meta1->status);
  tx_meta2 = eth_tx_controller()->GetTxForTesting("002");
  EXPECT_EQ(mojom::TransactionStatus::Confirmed, tx_meta1->status);

  // If the keyring is locked, nothing should update
  meta.id = "001";
  meta.from = addr1;
  meta.status = mojom::TransactionStatus::Submitted;
  eth_tx_controller()->tx_state_manager_->AddOrUpdateTx(meta);
  meta.id = "002";
  meta.from = addr2;
  meta.tx->set_nonce(uint256_t(4));
  meta.status = mojom::TransactionStatus::Submitted;
  eth_tx_controller()->tx_state_manager_->AddOrUpdateTx(meta);
  keyring_controller_->Lock();
  task_environment_.FastForwardBy(
      base::TimeDelta::FromSeconds(kBlockTrackerDefaultTimeInSeconds + 1));
  tx_meta1 = eth_tx_controller()->GetTxForTesting("001");
  EXPECT_EQ(mojom::TransactionStatus::Submitted, tx_meta1->status);
  tx_meta2 = eth_tx_controller()->GetTxForTesting("002");
  EXPECT_EQ(mojom::TransactionStatus::Submitted, tx_meta1->status);
}

}  //  namespace brave_wallet
