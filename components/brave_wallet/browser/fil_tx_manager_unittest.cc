/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/fil_tx_manager.h"

#include <memory>
#include <optional>
#include <unordered_map>
#include <utility>

#include "base/files/scoped_temp_dir.h"
#include "base/json/json_writer.h"
#include "base/strings/string_util.h"
#include "base/test/bind.h"
#include "base/test/run_until.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "base/test/values_test_util.h"
#include "brave/components/brave_wallet/browser/blockchain_registry.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/fil_transaction.h"
#include "brave/components/brave_wallet/browser/fil_tx_meta.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/network_manager.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/browser/test_utils.h"
#include "brave/components/brave_wallet/browser/tx_service.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "components/grit/brave_components_strings.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/l10n/l10n_util.h"
#include "url/origin.h"

namespace brave_wallet {

namespace {
std::string GetSignedMessage(const std::string& message,
                             const std::string& data) {
  base::DictValue dict;
  dict.Set("Message", "{message}");
  base::DictValue signature;
  signature.Set("Data", data);
  signature.Set("Type", 1);
  dict.Set("Signature", std::move(signature));
  std::string json;
  EXPECT_TRUE(base::JSONWriter::Write(dict, &json));
  base::ReplaceFirstSubstringAfterOffset(&json, 0, "\"{message}\"", message);
  return json;
}

}  // namespace

class FilTxManagerUnitTest : public testing::Test {
 public:
  FilTxManagerUnitTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}

  void SetUp() override {
    brave_wallet::RegisterLocalStatePrefs(local_state_.registry());
    brave_wallet::RegisterProfilePrefs(prefs_.registry());
    brave_wallet::RegisterProfilePrefsForMigration(prefs_.registry());
    network_manager_ = std::make_unique<NetworkManager>(&prefs_);
    json_rpc_service_ = std::make_unique<JsonRpcService>(
        url_loader_factory_.GetSafeWeakWrapper(), network_manager_.get(),
        &prefs_, nullptr);
    keyring_service_ = std::make_unique<KeyringService>(json_rpc_service_.get(),
                                                        &prefs_, &local_state_);
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    tx_service_ = std::make_unique<TxService>(
        json_rpc_service_.get(), nullptr, nullptr, nullptr, nullptr,
        *keyring_service_, &prefs_, CreateTxStorageForTest(temp_dir_.GetPath()),
        base::BindRepeating([](const url::Origin&, const mojom::AccountIdPtr&) {
          return true;
        }));

    GetAccountUtils().CreateWallet(kMnemonicDivideCruise, kTestWalletPassword);
    ASSERT_TRUE(GetAccountUtils().EnsureFilTestAccount(0));
  }

  AccountUtils GetAccountUtils() {
    return AccountUtils(keyring_service_.get());
  }

  mojom::AccountIdPtr FilTestAcc(size_t index) {
    auto account = GetAccountUtils().EnsureFilTestAccount(index);
    CHECK(account);
    return account->account_id->Clone();
  }

  void SetInterceptor(const GURL& expected_url,
                      const std::string& expected_method,
                      const std::string& content) {
    ClearInterceptorResponses();
    AddInterceptorResponse(expected_method, content);
    url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
        [&, expected_url, expected_method,
         content](const network::ResourceRequest& request) {
          EXPECT_EQ(request.url, expected_url);
          auto header_value = request.headers.GetHeader("X-Eth-Method");
          ASSERT_TRUE(header_value);
          EXPECT_TRUE(responses_.count(*header_value));
          url_loader_factory_.ClearResponses();
          auto response = responses_[*header_value];
          url_loader_factory_.AddResponse(request.url.spec(), response);
        }));
  }

  void AddInterceptorResponse(const std::string& expected_method,
                              const std::string& content) {
    responses_[expected_method] = content;
  }

  void ClearInterceptorResponses() {
    responses_.clear();
    url_loader_factory_.ClearResponses();
  }

  void GetFilTransactionMessageToSign(
      const std::string& tx_meta_id,
      std::optional<std::string> expected_message) {
    base::test::TestFuture<const std::optional<std::string>&> future;
    fil_tx_manager()->GetFilTransactionMessageToSign(tx_meta_id,
                                                     future.GetCallback());
    const auto& json_message = future.Get();
    ASSERT_EQ(json_message.has_value(), expected_message.has_value());
    if (expected_message.has_value()) {
      EXPECT_EQ(base::test::ParseJson(*json_message),
                base::test::ParseJson(*expected_message));
    }
  }

  FilTxManager* fil_tx_manager() { return tx_service_->GetFilTxManager(); }

  PrefService* prefs() { return &prefs_; }

  // Returns the new tx meta id, or an empty string (with a recorded failure)
  // if the transaction could not be added.
  std::string AddUnapprovedTransaction(const std::string& chain_id,
                                       mojom::FilTxDataPtr tx_data,
                                       const mojom::AccountIdPtr& from) {
    base::test::TestFuture<bool, const std::string&, const std::string&> future;
    fil_tx_manager()->AddUnapprovedFilecoinTransaction(
        chain_id, std::move(tx_data), from, nullptr, future.GetCallback());
    const auto& [success, id, err_message] = future.Get();
    EXPECT_TRUE(success);
    EXPECT_FALSE(id.empty());
    EXPECT_TRUE(err_message.empty());
    return success ? id : std::string();
  }

  void ApproveTransaction(const std::string& meta_id,
                          bool is_error,
                          mojom::FilecoinProviderError error,
                          const std::string& expected_err_message) {
    base::test::TestFuture<bool, mojom::ProviderErrorUnionPtr,
                           const std::string&>
        future;
    fil_tx_manager()->ApproveTransaction(meta_id, future.GetCallback());
    auto [success, error_union, err_message] = future.Take();
    EXPECT_NE(success, is_error);
    ASSERT_TRUE(error_union->is_filecoin_provider_error());
    EXPECT_EQ(error_union->get_filecoin_provider_error(), error);
    EXPECT_EQ(err_message, expected_err_message);
  }
  GURL GetNetwork(const std::string& chain_id, mojom::CoinType coin) {
    return network_manager_->GetNetworkURL(chain_id, coin);
  }
  void SetGasEstimateInterceptor(const mojom::AccountIdPtr& from_account,
                                 const std::string& to_account) {
    std::string gas_response = R"({
                          "jsonrpc": "2.0",
                          "result": {
                            "Version": 0,
                            "To": "{to}",
                            "From": "{from}",
                            "Nonce": 5,
                            "Value": "42",
                            "GasLimit": 598585,
                            "GasFeeCap": "100820",
                            "GasPremium": "99766",
                            "Method": 0,
                            "Params": "",
                            "CID": {
                              "/": "bafy2bzacedkdoldmztwjwi3jvxhxo4qqp7haufuifpqzregfqkthlyhhf2lfu"
                            }
                          },
                          "id": 1
                        })";
    base::ReplaceSubstringsAfterOffset(&gas_response, 0, "{to}", to_account);
    base::ReplaceSubstringsAfterOffset(&gas_response, 0, "{from}",
                                       from_account->address);
    SetInterceptor(GetNetwork(mojom::kFilecoinTestnet, mojom::CoinType::FIL),
                   "Filecoin.GasEstimateMessageGas", gas_response);
  }

 protected:
  base::test::TaskEnvironment task_environment_;
  base::ScopedTempDir temp_dir_;
  sync_preferences::TestingPrefServiceSyncable prefs_;
  sync_preferences::TestingPrefServiceSyncable local_state_;
  network::TestURLLoaderFactory url_loader_factory_;
  std::unique_ptr<NetworkManager> network_manager_;
  std::unique_ptr<JsonRpcService> json_rpc_service_;
  std::unique_ptr<KeyringService> keyring_service_;
  std::unique_ptr<TxService> tx_service_;
  std::unordered_map<std::string, std::string> responses_;
};

TEST_F(FilTxManagerUnitTest, SubmitTransactions) {
  const auto from_account = FilTestAcc(0);
  std::string to_account = "t1h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q";
  SetGasEstimateInterceptor(from_account, to_account);
  auto tx_data = mojom::FilTxData::New("" /* nonce */, "" /* gas_premium */,
                                       "" /* gas_fee_cap */, "" /* gas_limit */,
                                       "" /* max_fee */, to_account, "11");
  auto tx = FilTransaction::FromTxData(false, tx_data.Clone());

  const std::string meta_id1 = AddUnapprovedTransaction(
      mojom::kFilecoinTestnet, tx_data.Clone(), from_account);
  ASSERT_FALSE(meta_id1.empty());

  auto tx_meta1 = fil_tx_manager()->GetTxForTesting(meta_id1);
  ASSERT_TRUE(tx_meta1);
  EXPECT_EQ(tx_meta1->chain_id(), mojom::kFilecoinTestnet);

  EXPECT_EQ(tx_meta1->tx()->gas_fee_cap(), "100820");
  EXPECT_EQ(tx_meta1->tx()->gas_limit(), 598585);
  EXPECT_EQ(tx_meta1->tx()->gas_premium(), "99766");
  EXPECT_EQ(tx_meta1->from(), from_account);
  EXPECT_EQ(tx_meta1->status(), mojom::TransactionStatus::Unapproved);

  const std::string meta_id2 = AddUnapprovedTransaction(
      mojom::kFilecoinTestnet, tx_data.Clone(), from_account);
  ASSERT_FALSE(meta_id2.empty());
  auto tx_meta2 = fil_tx_manager()->GetTxForTesting(meta_id2);
  ASSERT_TRUE(tx_meta2);
  EXPECT_EQ(tx_meta2->chain_id(), mojom::kFilecoinTestnet);
  EXPECT_EQ(tx_meta2->from(), from_account);
  EXPECT_EQ(tx_meta2->status(), mojom::TransactionStatus::Unapproved);

  SetInterceptor(GetNetwork(mojom::kFilecoinTestnet, mojom::CoinType::FIL),
                 "Filecoin.MpoolGetNonce",
                 R"({ "jsonrpc": "2.0", "id": 1, "result": 1 })");
  AddInterceptorResponse("Filecoin.StateSearchMsgLimited",
                         R"({ "jsonrpc": "2.0", "id": 1, "result": {
  }})");
  AddInterceptorResponse("Filecoin.ChainHead",
                         R"({ "jsonrpc": "2.0", "id": 1, "result": {
  }})");
  AddInterceptorResponse("Filecoin.MpoolPush",
                         R"({ "id": 1, "jsonrpc": "2.0", "result": {
        "/": "bafy2bzacea3wsdh6y3a36tb3skempjoxqpuyompjbmfeyf34fi3uy6uue42v4"
      }
  })");

  ApproveTransaction(meta_id1, false, mojom::FilecoinProviderError::kSuccess,
                     std::string());
  tx_meta1 = fil_tx_manager()->GetTxForTesting(meta_id1);
  ASSERT_TRUE(tx_meta1);
  EXPECT_FALSE(tx_meta1->tx_hash().empty());
  EXPECT_EQ(tx_meta1->from(), from_account);
  EXPECT_EQ(tx_meta1->status(), mojom::TransactionStatus::Submitted);

  // Send another tx.
  ApproveTransaction(meta_id2, false, mojom::FilecoinProviderError::kSuccess,
                     std::string());

  tx_meta2 = fil_tx_manager()->GetTxForTesting(meta_id2);
  ASSERT_TRUE(tx_meta2);
  EXPECT_EQ(tx_meta2->from(), from_account);
  EXPECT_FALSE(tx_meta2->tx_hash().empty());
  EXPECT_EQ(tx_meta2->status(), mojom::TransactionStatus::Submitted);
}

TEST_F(FilTxManagerUnitTest, SubmitTransactionError) {
  const auto from_account = FilTestAcc(0);
  std::string to_account = "t1h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q";
  SetGasEstimateInterceptor(from_account, to_account);
  auto tx_data = mojom::FilTxData::New("" /* nonce */, "" /* gas_premium */,
                                       "" /* gas_fee_cap */, "" /* gas_limit */,
                                       "" /* max_fee */, to_account, "11");
  auto tx = FilTransaction::FromTxData(false, tx_data.Clone());

  const std::string meta_id1 = AddUnapprovedTransaction(
      mojom::kFilecoinTestnet, tx_data.Clone(), from_account);
  ASSERT_FALSE(meta_id1.empty());

  auto tx_meta1 = fil_tx_manager()->GetTxForTesting(meta_id1);
  ASSERT_TRUE(tx_meta1);

  EXPECT_EQ(tx_meta1->tx()->gas_fee_cap(), "100820");
  EXPECT_EQ(tx_meta1->tx()->gas_limit(), 598585);
  EXPECT_EQ(tx_meta1->tx()->gas_premium(), "99766");
  EXPECT_EQ(tx_meta1->from(), from_account);
  EXPECT_EQ(tx_meta1->status(), mojom::TransactionStatus::Unapproved);

  SetInterceptor(GetNetwork(mojom::kFilecoinTestnet, mojom::CoinType::FIL),
                 "Filecoin.MpoolGetNonce",
                 R"({ "jsonrpc": "2.0", "id": 1, "result": 1 })");

  AddInterceptorResponse("Filecoin.StateSearchMsgLimited",
                         R"({ "jsonrpc": "2.0", "id": 1, "result": {
    "Message": {
      "/": "bafy2bzacea3wsdh6y3a36tb3skempjoxqpuyompjbmfeyf34fi3uy6uue42v4"
    },
    "Receipt": {
      "ExitCode": 0
    }
  }})");
  AddInterceptorResponse("Filecoin.MpoolPush",
                         R"({ "id": 1, "jsonrpc": "2.0", "result":{} })");

  ApproveTransaction(meta_id1, true,
                     mojom::FilecoinProviderError::kParsingError,
                     l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR));
  tx_meta1 = fil_tx_manager()->GetTxForTesting(meta_id1);
  ASSERT_TRUE(tx_meta1);
  EXPECT_TRUE(tx_meta1->tx_hash().empty());
  EXPECT_EQ(tx_meta1->from(), from_account);
  EXPECT_EQ(tx_meta1->status(), mojom::TransactionStatus::Error);
}

TEST_F(FilTxManagerUnitTest, SubmitTransactionConfirmed) {
  const auto from_account = FilTestAcc(0);
  std::string to_account = "t1h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q";
  SetGasEstimateInterceptor(from_account, to_account);
  auto tx_data = mojom::FilTxData::New("" /* nonce */, "" /* gas_premium */,
                                       "" /* gas_fee_cap */, "" /* gas_limit */,
                                       "" /* max_fee */, to_account, "11");
  auto tx = FilTransaction::FromTxData(false, tx_data.Clone());

  const std::string meta_id1 = AddUnapprovedTransaction(
      mojom::kFilecoinTestnet, tx_data.Clone(), from_account);
  ASSERT_FALSE(meta_id1.empty());

  auto tx_meta1 = fil_tx_manager()->GetTxForTesting(meta_id1);
  ASSERT_TRUE(tx_meta1);
  EXPECT_EQ(tx_meta1->chain_id(), mojom::kFilecoinTestnet);

  EXPECT_EQ(tx_meta1->tx()->gas_fee_cap(), "100820");
  EXPECT_EQ(tx_meta1->tx()->gas_limit(), 598585);
  EXPECT_EQ(tx_meta1->tx()->gas_premium(), "99766");
  EXPECT_EQ(tx_meta1->from(), from_account);
  EXPECT_EQ(tx_meta1->status(), mojom::TransactionStatus::Unapproved);

  SetInterceptor(GetNetwork(mojom::kFilecoinTestnet, mojom::CoinType::FIL),
                 "Filecoin.MpoolGetNonce",
                 R"({ "jsonrpc": "2.0", "id": 1, "result": 1 })");
  AddInterceptorResponse("Filecoin.StateSearchMsgLimited",
                         R"({ "jsonrpc": "2.0", "id": 1, "result": {
    "Message": {
      "/": "bafy2bzacea3wsdh6y3a36tb3skempjoxqpuyompjbmfeyf34fi3uy6uue42v4"
    },
    "Receipt": {
      "ExitCode": 0
    }
  }})");
  AddInterceptorResponse("Filecoin.ChainHead",
                         R"({ "jsonrpc": "2.0", "id": 1, "result": {
    "Blocks":[],
    "Cids": [{
          "/": "bafy2bzacea3wsdh6y3a36tb3skempjoxqpuyompjbmfeyf34fi3uy6uue42v4"
    }],
    "Height": 22452
  }})");
  AddInterceptorResponse("Filecoin.MpoolPush",
                         R"({ "id": 1, "jsonrpc": "2.0", "result": {
        "/": "bafy2bzacea3wsdh6y3a36tb3skempjoxqpuyompjbmfeyf34fi3uy6uue42v4"
      }
  })");
  ApproveTransaction(meta_id1, false, mojom::FilecoinProviderError::kSuccess,
                     std::string());
  // Wait for the pending tx to be confirmed.
  ASSERT_TRUE(base::test::RunUntil([&] {
    auto meta = fil_tx_manager()->GetTxForTesting(meta_id1);
    return meta && meta->status() == mojom::TransactionStatus::Confirmed;
  }));
  tx_meta1 = fil_tx_manager()->GetTxForTesting(meta_id1);
  ASSERT_TRUE(tx_meta1);
  EXPECT_FALSE(tx_meta1->tx_hash().empty());
  EXPECT_EQ(tx_meta1->from(), from_account);
  EXPECT_EQ(tx_meta1->status(), mojom::TransactionStatus::Confirmed);
}

TEST_F(FilTxManagerUnitTest, NoDappOrigin) {
  const auto from_account = FilTestAcc(0);
  const std::string to_account = "t1lqarsh4nkg545ilaoqdsbtj4uofplt6sto26ziy";
  SetGasEstimateInterceptor(from_account, to_account);
  auto tx_data = mojom::FilTxData::New("" /* nonce */, "" /* gas_premium */,
                                       "" /* gas_fee_cap */, "" /* gas_limit */,
                                       "" /* max_fee */, to_account, "11");
  const std::string meta_id = AddUnapprovedTransaction(
      mojom::kFilecoinTestnet, std::move(tx_data), from_account);
  ASSERT_FALSE(meta_id.empty());

  auto tx_meta = fil_tx_manager()->GetTxForTesting(meta_id);
  ASSERT_TRUE(tx_meta);
  EXPECT_EQ(tx_meta->chain_id(), mojom::kFilecoinTestnet);

  EXPECT_FALSE(tx_meta->dapp_origin());
}

TEST_F(FilTxManagerUnitTest, GetFilTransactionMessageToSign) {
  const auto from_account = FilTestAcc(0);
  EXPECT_EQ(from_account->address, "t1dca7adhz5lbvin5n3qlw67munu6xhn5fpb77nly");
  const std::string to_account = "t1lqarsh4nkg545ilaoqdsbtj4uofplt6sto26ziy";
  // non-empty nonce
  {
    auto tx_data = mojom::FilTxData::New(
        "1" /* nonce */, "2" /* gas_premium */, "3" /* gas_fee_cap */,
        "4" /* gas_limit */, "" /* max_fee */, to_account, "11");
    const std::string meta_id = AddUnapprovedTransaction(
        mojom::kFilecoinTestnet, std::move(tx_data), from_account);
    ASSERT_FALSE(meta_id.empty());
    auto tx_meta = fil_tx_manager()->GetTxForTesting(meta_id);
    ASSERT_TRUE(tx_meta);
    EXPECT_EQ(tx_meta->chain_id(), mojom::kFilecoinTestnet);
    EXPECT_EQ(tx_meta->from(), from_account);
    EXPECT_EQ(tx_meta->status(), mojom::TransactionStatus::Unapproved);
    GetFilTransactionMessageToSign(meta_id, R"(
    {
        "From": "t1dca7adhz5lbvin5n3qlw67munu6xhn5fpb77nly",
        "GasFeeCap": "3",
        "GasLimit": 4,
        "GasPremium": "2",
        "Method": 0,
        "Nonce": 1,
        "Params": "",
        "To": "t1lqarsh4nkg545ilaoqdsbtj4uofplt6sto26ziy",
        "Value": "11",
        "Version": 0
    }
  )");
  }
  // empty nonce
  {
    SetInterceptor(GetNetwork(mojom::kFilecoinTestnet, mojom::CoinType::FIL),
                   "Filecoin.MpoolGetNonce",
                   R"({ "jsonrpc": "2.0", "id": 1, "result": 5 })");

    auto tx_data = mojom::FilTxData::New(
        "" /* nonce */, "2" /* gas_premium */, "3" /* gas_fee_cap */,
        "4" /* gas_limit */, "" /* max_fee */, to_account, "11");
    const std::string meta_id = AddUnapprovedTransaction(
        mojom::kFilecoinTestnet, std::move(tx_data), from_account);
    ASSERT_FALSE(meta_id.empty());
    auto tx_meta = fil_tx_manager()->GetTxForTesting(meta_id);
    ASSERT_TRUE(tx_meta);
    EXPECT_EQ(tx_meta->chain_id(), mojom::kFilecoinTestnet);
    EXPECT_EQ(tx_meta->from(), from_account);
    EXPECT_EQ(tx_meta->status(), mojom::TransactionStatus::Unapproved);
    GetFilTransactionMessageToSign(meta_id, R"(
    {
        "From": "t1dca7adhz5lbvin5n3qlw67munu6xhn5fpb77nly",
        "GasFeeCap": "3",
        "GasLimit": 4,
        "GasPremium": "2",
        "Method": 0,
        "Nonce": 5,
        "Params": "",
        "To": "t1lqarsh4nkg545ilaoqdsbtj4uofplt6sto26ziy",
        "Value": "11",
        "Version": 0
    }
  )");
  }

  GetFilTransactionMessageToSign("unknown id", std::nullopt);
  GetFilTransactionMessageToSign("", std::nullopt);
}

TEST_F(FilTxManagerUnitTest, ProcessHardwareSignature) {
  const auto from_account = FilTestAcc(0);
  const std::string to_account = "t1lqarsh4nkg545ilaoqdsbtj4uofplt6sto26ziy";

  auto tx_data = mojom::FilTxData::New(
      "1" /* nonce */, "2" /* gas_premium */, "3" /* gas_fee_cap */,
      "4" /* gas_limit */, "" /* max_fee */, to_account, "11");
  const std::string meta_id = AddUnapprovedTransaction(
      mojom::kFilecoinTestnet, std::move(tx_data), from_account);
  ASSERT_FALSE(meta_id.empty());
  auto tx_meta = fil_tx_manager()->GetTxForTesting(meta_id);
  ASSERT_TRUE(tx_meta);
  EXPECT_EQ(tx_meta->chain_id(), mojom::kFilecoinTestnet);
  EXPECT_EQ(tx_meta->from(), from_account);
  EXPECT_EQ(tx_meta->status(), mojom::TransactionStatus::Unapproved);
  auto message = tx_meta->tx()->GetMessageToSignJson(
      FilAddress::FromAddress(from_account->address));
  ASSERT_TRUE(message);
  auto signed_message = GetSignedMessage(*message, "data");
  SetInterceptor(GetNetwork(mojom::kFilecoinTestnet, mojom::CoinType::FIL),
                 "Filecoin.MpoolPush",
                 R"({
            "id": 1,
            "jsonrpc": "2.0",
            "result":
            {
                "/": "bafy2bzacea3wsdh6y3a36tb3skempjoxqpuyompjbmfeyf34fi3uy6uue42v4"
            }
        })");
  AddInterceptorResponse("Filecoin.StateSearchMsgLimited",
                         R"({
            "Message":
            {
                "/": "bafy2bzacea3wsdh6y3a36tb3skempjoxqpuyompjbmfeyf34fi3uy6uue42v4"
            },
            "Receipt":
            {
                "ExitCode": 0
            }
        })");
  AddInterceptorResponse("Filecoin..ChainHead",
                         R"({
      "Blocks":[],
      "Cids": [{
            "/": "bafy2bzacea3wsdh6y3a36tb3skempjoxqpuyompjbmfeyf34fi3uy6uue42v4"
      }],
      "Height": 22452
    })");

  base::test::TestFuture<bool, mojom::ProviderErrorUnionPtr, const std::string&>
      future;
  fil_tx_manager()->ProcessFilHardwareSignature(
      meta_id, mojom::FilecoinSignature::New(signed_message),
      future.GetCallback());
  auto [success, error_union, err_message] = future.Take();
  EXPECT_TRUE(success);
  ASSERT_TRUE(error_union->is_filecoin_provider_error());
  EXPECT_EQ(error_union->get_filecoin_provider_error(),
            mojom::FilecoinProviderError::kSuccess);
  EXPECT_TRUE(err_message.empty());
  auto fil_tx_meta = fil_tx_manager()->GetTxForTesting(meta_id);
  ASSERT_TRUE(fil_tx_meta);
  EXPECT_EQ(fil_tx_meta->status(), mojom::TransactionStatus::Submitted);
}

TEST_F(FilTxManagerUnitTest, ProcessHardwareSignatureError) {
  const auto from_account = FilTestAcc(0);
  const std::string to_account = "t1lqarsh4nkg545ilaoqdsbtj4uofplt6sto26ziy";

  auto tx_data = mojom::FilTxData::New(
      "1" /* nonce */, "2" /* gas_premium */, "3" /* gas_fee_cap */,
      "4" /* gas_limit */, "" /* max_fee */, to_account, "11");
  const std::string meta_id = AddUnapprovedTransaction(
      mojom::kFilecoinTestnet, std::move(tx_data), from_account);
  ASSERT_FALSE(meta_id.empty());
  auto tx_meta = fil_tx_manager()->GetTxForTesting(meta_id);
  ASSERT_TRUE(tx_meta);
  EXPECT_EQ(tx_meta->chain_id(), mojom::kFilecoinTestnet);
  EXPECT_EQ(tx_meta->from(), from_account);
  EXPECT_EQ(tx_meta->status(), mojom::TransactionStatus::Unapproved);
  auto message = tx_meta->tx()->GetMessageToSignJson(
      FilAddress::FromAddress(from_account->address));
  ASSERT_TRUE(message);
  auto signed_message = GetSignedMessage(*message, "data");
  base::test::TestFuture<bool, mojom::ProviderErrorUnionPtr, const std::string&>
      future;
  fil_tx_manager()->ProcessFilHardwareSignature(
      "fake", mojom::FilecoinSignature::New(signed_message),
      future.GetCallback());
  auto [success, error_union, err_message] = future.Take();
  EXPECT_FALSE(success);
  ASSERT_TRUE(error_union->is_filecoin_provider_error());
  EXPECT_EQ(error_union->get_filecoin_provider_error(),
            mojom::FilecoinProviderError::kInternalError);
  EXPECT_EQ(err_message,
            l10n_util::GetStringUTF8(IDS_BRAVE_WALLET_TRANSACTION_NOT_FOUND));
}

TEST_F(FilTxManagerUnitTest, RestrictedFromAddress) {
  const auto from_account = FilTestAcc(0);
  BlockchainRegistry::ScopedRestrictedAddressesForTesting scoped_restricted(
      {base::ToLowerASCII(from_account->address)});

  auto fil_tx_data = mojom::FilTxData::New(
      "" /* nonce */, "" /* gas_premium */, "" /* gas_fee_cap */,
      "" /* gas_limit */, "" /* max_fee */,
      "t1h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q", "");

  base::test::TestFuture<bool, const std::string&, const std::string&>
      unapproved_future;

  tx_service_->AddUnapprovedFilecoinTransaction(
      std::move(fil_tx_data), mojom::kFilecoinTestnet, from_account->Clone(),
      nullptr, unapproved_future.GetCallback());

  const auto& [success, tx_meta_id, err_str] = unapproved_future.Take();

  EXPECT_FALSE(success);
  EXPECT_TRUE(tx_meta_id.empty());
  EXPECT_EQ(err_str, WalletInternalErrorMessage());
}

}  //  namespace brave_wallet
