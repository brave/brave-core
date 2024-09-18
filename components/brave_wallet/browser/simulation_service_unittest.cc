/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/simulation_service.h"

#include <map>
#include <memory>
#include <optional>
#include <utility>

#include "base/memory/scoped_refptr.h"
#include "base/test/bind.h"
#include "base/test/gtest_util.h"
#include "base/test/mock_callback.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "base/test/values_test_util.h"
#include "brave/components/brave_wallet/browser/brave_wallet_prefs.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/eip1559_transaction.h"
#include "brave/components/brave_wallet/browser/eth_transaction.h"
#include "brave/components/brave_wallet/browser/eth_tx_meta.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/browser/network_manager.h"
#include "brave/components/brave_wallet/browser/simulation_response_parser.h"
#include "brave/components/brave_wallet/browser/solana_transaction.h"
#include "brave/components/brave_wallet/browser/solana_tx_meta.h"
#include "brave/components/brave_wallet/browser/test_utils.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "brave/components/brave_wallet/common/features.h"
#include "brave/components/brave_wallet/common/test_utils.h"
#include "components/grit/brave_components_strings.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "services/data_decoder/public/cpp/test_support/in_process_data_decoder.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/l10n/l10n_util.h"

using base::test::ParseJson;

namespace brave_wallet {

class SimulationServiceUnitTest : public testing::Test {
 public:
  SimulationServiceUnitTest()
      : shared_url_loader_factory_(
            base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
                &url_loader_factory_)) {
    brave_wallet::RegisterLocalStatePrefs(local_state_.registry());
    brave_wallet::RegisterLocalStatePrefsForMigration(local_state_.registry());
    RegisterProfilePrefs(prefs_.registry());
    RegisterProfilePrefsForMigration(prefs_.registry());
    brave_wallet_service_ = std::make_unique<BraveWalletService>(
        shared_url_loader_factory_, TestBraveWalletServiceDelegate::Create(),
        &prefs_, &local_state_);
    network_manager_ = brave_wallet_service_->network_manager();
    json_rpc_service_ = brave_wallet_service_->json_rpc_service();

    simulation_service_ = std::make_unique<SimulationService>(
        shared_url_loader_factory_, brave_wallet_service_.get());

    SetTransactionSimulationOptInStatus(&prefs_,
                                        mojom::BlowfishOptInStatus::kAllowed);
  }

  ~SimulationServiceUnitTest() override = default;

  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory() {
    return shared_url_loader_factory_;
  }

  PrefService* GetPrefs() { return &prefs_; }

  GURL GetNetwork(const std::string& chain_id, mojom::CoinType coin) {
    return network_manager_->GetNetworkURL(chain_id, coin);
  }

  void SetInterceptor(const std::string& content) {
    url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
        [&, content](const network::ResourceRequest& request) {
          url_loader_factory_.ClearResponses();
          url_loader_factory_.AddResponse(request.url.spec(), content);
        }));
  }

  void SetInterceptors(std::map<GURL, std::string> responses) {
    url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
        [&, responses](const network::ResourceRequest& request) {
          // If the request url is in responses, add that response
          auto it = responses.find(request.url);
          if (it != responses.end()) {
            // Get the response string
            std::string response = it->second;
            url_loader_factory_.ClearResponses();
            url_loader_factory_.AddResponse(request.url.spec(), response);
          }
        }));
  }

  void SetErrorInterceptor(const std::string& content) {
    url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
        [&](const network::ResourceRequest& request) {
          url_loader_factory_.ClearResponses();
          url_loader_factory_.AddResponse(request.url.spec(), content,
                                          net::HTTP_REQUEST_TIMEOUT);
        }));
  }

  mojom::TransactionInfoPtr GetCannedScanEVMTransactionParams(
      bool eip1559,
      const std::string& chain_id) {
    auto eth_account =
        MakeAccountId(mojom::CoinType::ETH, mojom::KeyringId::kDefault,
                      mojom::AccountKind::kDerived,
                      "0xa92D461a9a988A7f11ec285d39783A637Fdd6ba4");

    auto base_tx_data = mojom::TxData::New(
        "0x09", "0x4a817c800", "0x5208",
        "0x3535353535353535353535353535353535353535", "0x0de0b6b3a7640000",
        std::vector<uint8_t>(), false, std::nullopt);

    if (eip1559) {
      std::unique_ptr<Eip1559Transaction> tx =
          std::make_unique<Eip1559Transaction>(
              *Eip1559Transaction::FromTxData(mojom::TxData1559::New(
                  std::move(base_tx_data), "0x3", "0x1E", "0x32", nullptr)));
      EthTxMeta meta(eth_account, std::move(tx));
      meta.set_chain_id(chain_id);
      return meta.ToTransactionInfo();
    } else {
      std::unique_ptr<EthTransaction> tx = std::make_unique<EthTransaction>(
          *EthTransaction::FromTxData(std::move(base_tx_data)));
      EthTxMeta meta(eth_account, std::move(tx));
      meta.set_chain_id(chain_id);
      return meta.ToTransactionInfo();
    }
  }

  mojom::TransactionInfoPtr GetCannedScanSolanaTransactionParams(
      std::optional<std::string> origin,
      const std::string& chain_id) {
    std::string from_account = "BrG44HdsEhzapvs8bEqzvkq4egwevS3fRE6ze2ENo6S8";
    auto sol_account =
        MakeAccountId(mojom::CoinType::SOL, mojom::KeyringId::kSolana,
                      mojom::AccountKind::kDerived, from_account);

    std::string to_account = "JDqrvDz8d8tFCADashbUKQDKfJZFobNy13ugN65t1wvV";
    std::string recent_blockhash =
        "9sHcv6xwn9YkB8nxTUGKDwPwNnmqVp5oAXxU8Fdkm4J6";
    uint64_t last_valid_block_height = 3090;
    const std::vector<uint8_t> data = {2,   0, 0, 0, 128, 150,
                                       152, 0, 0, 0, 0,   0};

    SolanaInstruction instruction(
        // Program ID
        mojom::kSolanaSystemProgramId,
        // Accounts
        {SolanaAccountMeta(from_account, std::nullopt, true, true),
         SolanaAccountMeta(to_account, std::nullopt, false, true)},
        data);

    auto msg = SolanaMessage::CreateLegacyMessage(
        recent_blockhash, last_valid_block_height, from_account,
        std::vector<SolanaInstruction>({instruction}));
    auto tx = std::make_unique<SolanaTransaction>(std::move(*msg));
    tx->set_to_wallet_address(to_account);
    tx->set_lamports(10000000u);
    tx->set_tx_type(mojom::TransactionType::SolanaSystemTransfer);

    SolanaTxMeta meta(sol_account, std::move(tx));
    SolanaSignatureStatus status(82, 10, "", "confirmed");
    meta.set_signature_status(status);

    meta.set_id("meta_id");
    meta.set_status(mojom::TransactionStatus::Confirmed);
    base::Time::Exploded x{1981, 3, 0, 1, 2};
    base::Time confirmed_time = meta.confirmed_time();
    EXPECT_TRUE(base::Time::FromUTCExploded(x, &confirmed_time));
    meta.set_confirmed_time(confirmed_time);
    meta.set_submitted_time(confirmed_time - base::Seconds(3));
    meta.set_created_time(confirmed_time - base::Minutes(1));
    meta.set_tx_hash(
        "5VERv8NMvzbJMEkV8xnrLkEaWRtSz9CosKDYjCJjBRnbJLgp8uirBgmQpjKhoR4tjF3ZpR"
        "zr"
        "FmBV6UjKdiSZkQUW");

    if (origin) {
      meta.set_origin(url::Origin::Create(GURL(*origin)));
    }

    meta.set_chain_id(chain_id);

    return meta.ToTransactionInfo();
  }

  void ScanEVMTransaction(
      mojom::TransactionInfoPtr tx_info,
      const std::string& language,
      SimulationService::ScanEVMTransactionCallback callback) {
    simulation_service_->ScanEVMTransactionInternal(
        std::move(tx_info), language, std::move(callback));
  }

  void ScanSolanaTransaction(
      mojom::TransactionInfoPtr tx_info,
      const std::string& language,
      SimulationService::ScanSolanaTransactionCallback callback) {
    simulation_service_->ScanSolanaTransactionInternal(
        std::move(tx_info), language, std::move(callback));
  }

  void ScanSignSolTransactionsRequest(
      mojom::SignSolTransactionsRequestPtr request,
      const std::string& language,
      SimulationService::ScanSignSolTransactionsRequestCallback callback) {
    simulation_service_->ScanSignSolTransactionsRequestInternal(
        std::move(request), language, std::move(callback));
  }

 protected:
  sync_preferences::TestingPrefServiceSyncable local_state_;
  sync_preferences::TestingPrefServiceSyncable prefs_;
  std::unique_ptr<BraveWalletService> brave_wallet_service_;
  raw_ptr<NetworkManager> network_manager_;
  raw_ptr<JsonRpcService> json_rpc_service_;
  std::unique_ptr<SimulationService> simulation_service_;
  base::test::ScopedFeatureList feature_list_{
      features::kBraveWalletTransactionSimulationsFeature};
  base::test::TaskEnvironment task_environment_;

 private:
  network::TestURLLoaderFactory url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
  data_decoder::test::InProcessDataDecoder in_process_data_decoder_;
};

TEST_F(SimulationServiceUnitTest, GetScanTransactionURL) {
  auto url = simulation_service_->GetScanTransactionURL(
      mojom::kMainnetChainId, mojom::CoinType::ETH, "en-US");
  EXPECT_EQ(url,
            "https://blowfish.wallet.brave.com/ethereum/v0/mainnet/scan/"
            "transactions?language=en-US");

  url = simulation_service_->GetScanTransactionURL(
      mojom::kSepoliaChainId, mojom::CoinType::ETH, "en-US");
  EXPECT_EQ(url,
            "https://blowfish.wallet.brave.com/ethereum/v0/sepolia/scan/"
            "transactions?language=en-US");

  url = simulation_service_->GetScanTransactionURL(
      mojom::kPolygonMainnetChainId, mojom::CoinType::ETH, "en-US");
  EXPECT_EQ(url,
            "https://blowfish.wallet.brave.com/polygon/v0/mainnet/scan/"
            "transactions?language=en-US");

  url = simulation_service_->GetScanTransactionURL(
      mojom::kBnbSmartChainMainnetChainId, mojom::CoinType::ETH, "en-US");
  EXPECT_EQ(url,
            "https://blowfish.wallet.brave.com/bnb/v0/mainnet/scan/"
            "transactions?language=en-US");

  url = simulation_service_->GetScanTransactionURL(
      mojom::kArbitrumMainnetChainId, mojom::CoinType::ETH, "en-US");
  EXPECT_EQ(url,
            "https://blowfish.wallet.brave.com/arbitrum/v0/one/scan/"
            "transactions?language=en-US");

  url = simulation_service_->GetScanTransactionURL(
      mojom::kBaseMainnetChainId, mojom::CoinType::ETH, "en-US");
  EXPECT_EQ(url,
            "https://blowfish.wallet.brave.com/base/v0/mainnet/scan/"
            "transactions?language=en-US");

  url = simulation_service_->GetScanTransactionURL(
      mojom::kSolanaMainnet, mojom::CoinType::SOL, "en-US");
  EXPECT_EQ(url,
            "https://blowfish.wallet.brave.com/solana/v0/mainnet/scan/"
            "transactions?language=en-US");

  url = simulation_service_->GetScanTransactionURL(
      mojom::kSolanaDevnet, mojom::CoinType::SOL, "en-US");
  EXPECT_EQ(url,
            "https://blowfish.wallet.brave.com/solana/v0/devnet/scan/"
            "transactions?language=en-US");

  url = simulation_service_->GetScanTransactionURL(
      mojom::kSolanaTestnet, mojom::CoinType::SOL, "en-US");
  EXPECT_EQ(url,
            "https://blowfish.wallet.brave.com/solana/v0/testnet/scan/"
            "transactions?language=en-US");
}

TEST_F(SimulationServiceUnitTest, GetScanMessageURL) {
  auto url = simulation_service_->GetScanMessageURL(
      mojom::kMainnetChainId, mojom::CoinType::ETH, "en-US");
  EXPECT_EQ(url,
            "https://blowfish.wallet.brave.com/ethereum/v0/mainnet/scan/"
            "message?language=en-US");

  url = simulation_service_->GetScanMessageURL(mojom::kSepoliaChainId,
                                               mojom::CoinType::ETH, "en-US");
  EXPECT_EQ(url,
            "https://blowfish.wallet.brave.com/ethereum/v0/sepolia/scan/"
            "message?language=en-US");

  url = simulation_service_->GetScanMessageURL(mojom::kPolygonMainnetChainId,
                                               mojom::CoinType::ETH, "en-US");
  EXPECT_EQ(url,
            "https://blowfish.wallet.brave.com/polygon/v0/mainnet/scan/"
            "message?language=en-US");

  url = simulation_service_->GetScanMessageURL(
      mojom::kBnbSmartChainMainnetChainId, mojom::CoinType::ETH, "en-US");
  EXPECT_EQ(url,
            "https://blowfish.wallet.brave.com/bnb/v0/mainnet/scan/"
            "message?language=en-US");

  url = simulation_service_->GetScanMessageURL(mojom::kArbitrumMainnetChainId,
                                               mojom::CoinType::ETH, "en-US");
  EXPECT_EQ(url,
            "https://blowfish.wallet.brave.com/arbitrum/v0/one/scan/"
            "message?language=en-US");
}

TEST_F(SimulationServiceUnitTest, ScanEvmTransactionValidResponse) {
  SetInterceptor(R"(
    {
      "requestId":"e8cd35ce-f743-4ef2-8e94-f26857744db7",
      "action":"NONE",
      "simulationResults":{
        "aggregated":{
          "error":null,
          "userAccount":"0xa92D461a9a988A7f11ec285d39783A637Fdd6ba4",
          "expectedStateChanges":{
            "0xa92D461a9a988A7f11ec285d39783A637Fdd6ba4":[
              {
                "humanReadableDiff":"Send 0.033 ETH",
                "rawInfo":{
                  "kind":"NATIVE_ASSET_TRANSFER",
                  "data":{
                    "amount":{
                      "after":"71057321770366572",
                      "before":"104057321770366572"
                    },
                    "asset":{
                      "address":"0xeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee",
                      "symbol":"ETH",
                      "name":"Ether",
                      "decimals":"18",
                      "verified":true,
                      "imageUrl":"https://eth.png",
                      "price":{
                        "source":"Coingecko",
                        "updatedAt":"1681958792",
                        "dollarValuePerToken":"1945.92"
                      }
                    },
                    "counterparty":{
                      "kind":"ACCOUNT",
                      "address":"0x06924592cdf28acd3c1d23c37875c6c6a667bdf7"
                    }
                  }
                }
              }
            ]
          }
        }
      },
      "warnings":[]
    }
  )");

  base::RunLoop run_loop;
  ScanEVMTransaction(
      GetCannedScanEVMTransactionParams(false, mojom::kMainnetChainId), "en-US",
      base::BindLambdaForTesting([&](mojom::EVMSimulationResponsePtr response,
                                     const std::string& error_response,
                                     const std::string& error_string) {
        ASSERT_TRUE(response);

        EXPECT_EQ(response->action, mojom::BlowfishSuggestedAction::kNone);
        EXPECT_EQ(response->warnings.size(), 0u);
        EXPECT_FALSE(response->error);
        ASSERT_EQ(response->expected_state_changes.size(), 1u);
        const auto& state_change_0 = response->expected_state_changes.at(0);
        EXPECT_EQ(state_change_0->human_readable_diff, "Send 0.033 ETH");
        EXPECT_EQ(state_change_0->raw_info->kind,
                  mojom::BlowfishEVMRawInfoKind::kNativeAssetTransfer);
        ASSERT_TRUE(
            state_change_0->raw_info->data->is_native_asset_transfer_data());
        const auto& state_change_0_raw_info =
            state_change_0->raw_info->data->get_native_asset_transfer_data();

        EXPECT_EQ(state_change_0_raw_info->amount->after, "71057321770366572");
        EXPECT_EQ(state_change_0_raw_info->amount->before,
                  "104057321770366572");
        EXPECT_EQ(state_change_0_raw_info->asset->address,
                  "0xeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee");
        EXPECT_EQ(state_change_0_raw_info->asset->decimals, 18);
        EXPECT_EQ(state_change_0_raw_info->asset->image_url, "https://eth.png");
        EXPECT_EQ(state_change_0_raw_info->asset->name, "Ether");
        EXPECT_EQ(state_change_0_raw_info->asset->symbol, "ETH");
        EXPECT_TRUE(state_change_0_raw_info->asset->verified);
        EXPECT_EQ(state_change_0_raw_info->asset->lists.size(), 0u);
        EXPECT_EQ(state_change_0_raw_info->asset->price->dollar_value_per_token,
                  "1945.92");
        EXPECT_EQ(state_change_0_raw_info->asset->price->source,
                  mojom::BlowfishAssetPriceSource::kCoingecko);
        EXPECT_EQ(state_change_0_raw_info->asset->price->last_updated_at,
                  "1681958792");

        EXPECT_EQ(error_response, "");
        EXPECT_EQ(error_string, "");
        run_loop.Quit();
      }));
  run_loop.Run();
}

TEST_F(SimulationServiceUnitTest, ScanEVMTransactionUnsupportedNetwork) {
  base::MockCallback<mojom::SimulationService::ScanEVMTransactionCallback>
      callback;
  EXPECT_CALL(
      callback,
      Run(EqualsMojo(mojom::EVMSimulationResponsePtr()), "",
          l10n_util::GetStringUTF8(IDS_BRAVE_WALLET_UNSUPPORTED_NETWORK)));

  ScanEVMTransaction(
      GetCannedScanEVMTransactionParams(false, mojom::kNeonEVMMainnetChainId),
      "en-US", callback.Get());

  task_environment_.RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&callback);
}

TEST_F(SimulationServiceUnitTest, ScanEVMTransactionEmptyNetwork) {
  base::MockCallback<mojom::SimulationService::ScanEVMTransactionCallback>
      callback;
  EXPECT_CALL(
      callback,
      Run(EqualsMojo(mojom::EVMSimulationResponsePtr()), "",
          l10n_util::GetStringUTF8(IDS_BRAVE_WALLET_UNSUPPORTED_NETWORK)));

  ScanEVMTransaction(GetCannedScanEVMTransactionParams(false, ""), "en-US",
                     callback.Get());

  task_environment_.RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&callback);
}

TEST_F(SimulationServiceUnitTest, ScanEVMTransactionValidErrorResponse) {
  std::string error = R"(
    {
      "error": "No transactions to simulate"
    }
  )";
  SetErrorInterceptor(error);

  base::MockCallback<mojom::SimulationService::ScanEVMTransactionCallback>
      callback;
  EXPECT_CALL(callback, Run(EqualsMojo(mojom::EVMSimulationResponsePtr()),
                            "No transactions to simulate", ""));

  ScanEVMTransaction(
      GetCannedScanEVMTransactionParams(false, mojom::kPolygonMainnetChainId),
      "en-US", callback.Get());

  task_environment_.RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&callback);
}

TEST_F(SimulationServiceUnitTest, ScanEVMTransactionUnexpectedErrorResponse) {
  std::string error = "Woot";
  SetErrorInterceptor(error);

  base::MockCallback<mojom::SimulationService::ScanEVMTransactionCallback>
      callback;
  EXPECT_CALL(callback,
              Run(EqualsMojo(mojom::EVMSimulationResponsePtr()), "",
                  l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR)));

  ScanEVMTransaction(
      GetCannedScanEVMTransactionParams(false, mojom::kPolygonMainnetChainId),
      "en-US", callback.Get());

  task_environment_.RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&callback);
}

TEST_F(SimulationServiceUnitTest, ScanEVMTransactionNullParams) {
  base::MockCallback<mojom::SimulationService::ScanEVMTransactionCallback>
      callback;
  EXPECT_CALL(callback,
              Run(EqualsMojo(mojom::EVMSimulationResponsePtr()), "",
                  l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR)));

  simulation_service_->ScanEVMTransaction("bad_id", "en-US", callback.Get());

  task_environment_.RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&callback);
}

TEST_F(SimulationServiceUnitTest, ScanSolanaTransactionValid) {
  SetInterceptor(R"(
    {
      "aggregated": {
        "action": "NONE",
        "warnings": [],
        "expectedStateChanges": {
          "BrG44HdsEhzapvs8bEqzvkq4egwevS3fRE6ze2ENo6S8": [
            {
              "humanReadableDiff": "Send 2 USDT",
              "suggestedColor": "DEBIT",
              "rawInfo": {
                "kind": "SPL_TRANSFER",
                "data": {
                  "asset": {
                    "symbol": "USDT",
                    "name": "USDT",
                    "mint": "Es9vMFrzaCERmJfrF4H2FYD4KCoNkY11McCe8BenwNYB",
                    "decimals": "6",
                    "supply": "1000000000",
                    "metaplexTokenStandard": "unknown",
                    "price": {
                      "source": "Coingecko",
                      "updatedAt": "1679331222",
                      "dollarValuePerToken": "0.99"
                    },
                    "imageUrl": "https://usdt.png"
                  },
                  "diff": {
                    "sign": "MINUS",
                    "digits": "2000000"
                  },
                  "counterparty": "5wytVPbjLb2VCXbynhUQabEZZD2B6Wxrkvwm6v6Cuy5X"
                }
              }
            }
          ]
        },
        "error": null
      }
    }
  )");

  base::RunLoop run_loop;
  auto tx_info =
      GetCannedScanSolanaTransactionParams(std::nullopt, mojom::kSolanaMainnet);

  ScanSolanaTransaction(
      std::move(tx_info), "en-US",
      base::BindLambdaForTesting(
          [&](mojom::SolanaSimulationResponsePtr response,
              const std::string& error_response,
              const std::string& error_string) {
            ASSERT_TRUE(response);

            EXPECT_EQ(response->action, mojom::BlowfishSuggestedAction::kNone);
            EXPECT_EQ(response->warnings.size(), 0u);
            EXPECT_FALSE(response->error);
            ASSERT_EQ(response->expected_state_changes.size(), 1u);

            const auto& state_change = response->expected_state_changes.at(0);
            EXPECT_EQ(state_change->human_readable_diff, "Send 2 USDT");
            EXPECT_EQ(state_change->suggested_color,
                      mojom::BlowfishSuggestedColor::kDebit);
            EXPECT_EQ(state_change->raw_info->kind,
                      mojom::BlowfishSolanaRawInfoKind::kSplTransfer);
            ASSERT_TRUE(state_change->raw_info->data->is_spl_transfer_data());
            const auto& state_change_raw_info =
                state_change->raw_info->data->get_spl_transfer_data();
            EXPECT_EQ(state_change_raw_info->asset->symbol, "USDT");
            EXPECT_EQ(state_change_raw_info->asset->name, "USDT");
            EXPECT_EQ(state_change_raw_info->asset->mint,
                      "Es9vMFrzaCERmJfrF4H2FYD4KCoNkY11McCe8BenwNYB");
            EXPECT_EQ(state_change_raw_info->asset->decimals, 6);
            EXPECT_EQ(state_change_raw_info->asset->metaplex_token_standard,
                      mojom::BlowfishMetaplexTokenStandardKind::kUnknown);
            ASSERT_TRUE(state_change_raw_info->asset->price);
            EXPECT_EQ(state_change_raw_info->asset->price->source,
                      mojom::BlowfishAssetPriceSource::kCoingecko);
            EXPECT_EQ(state_change_raw_info->asset->price->last_updated_at,
                      "1679331222");
            EXPECT_EQ(
                state_change_raw_info->asset->price->dollar_value_per_token,
                "0.99");
            EXPECT_EQ(state_change_raw_info->diff->sign,
                      mojom::BlowfishDiffSign::kMinus);
            EXPECT_EQ(state_change_raw_info->diff->digits, "2000000");
            EXPECT_EQ(state_change_raw_info->counterparty,
                      "5wytVPbjLb2VCXbynhUQabEZZD2B6Wxrkvwm6v6Cuy5X");

            EXPECT_EQ(error_response, "");
            EXPECT_EQ(error_string, "");
            run_loop.Quit();
          }));
  run_loop.Run();
}

TEST_F(SimulationServiceUnitTest, ScanSolanaTransactionEmptyLatestBlockhash) {
  std::map<GURL, std::string> responses;
  responses[GetNetwork(mojom::kSolanaMainnet, mojom::CoinType::SOL)] = R"(
    {
      "jsonrpc":"2.0",
      "id":1,
      "result":{
        "context":{
          "slot":1069
        },
        "value":{
          "blockhash":"FHutN2vCDobUtYbK67KgW2ZriapANe66NcjYuwjac5MW",
          "lastValidBlockHeight":18446744073709551615
        }
      }
    }
  )";

  responses[GURL(
      "https://blowfish.wallet.brave.com/solana/v0/mainnet/scan/"
      "transactions?language=en-US")] = R"(
    {
      "aggregated": {
        "action": "NONE",
        "warnings": [],
        "expectedStateChanges": {
          "BrG44HdsEhzapvs8bEqzvkq4egwevS3fRE6ze2ENo6S8": [
            {
              "humanReadableDiff": "Send 2 USDT",
              "suggestedColor": "DEBIT",
              "rawInfo": {
                "kind": "SPL_TRANSFER",
                "data": {
                  "asset": {
                    "symbol": "USDT",
                    "name": "USDT",
                    "mint": "Es9vMFrzaCERmJfrF4H2FYD4KCoNkY11McCe8BenwNYB",
                    "decimals": "6",
                    "supply": "1000000000",
                    "metaplexTokenStandard": "unknown",
                    "price": {
                      "source": "Coingecko",
                      "updatedAt": "1679331222",
                      "dollarValuePerToken": "0.99"
                    },
                    "imageUrl": "https://usdt.png"
                  },
                  "diff": {
                    "sign": "MINUS",
                    "digits": "2000000"
                  },
                  "counterparty": "5wytVPbjLb2VCXbynhUQabEZZD2B6Wxrkvwm6v6Cuy5X"
                }
              }
            }
          ]
        },
        "error": null
      }
    }
  )";

  SetInterceptors(responses);

  base::RunLoop run_loop;
  auto tx_info =
      GetCannedScanSolanaTransactionParams(std::nullopt, mojom::kSolanaMainnet);

  // Force the latest blockhash in the transaction to be empty. This should
  // trigger fetching of the latest blockhash from the network.
  tx_info->tx_data_union->get_solana_tx_data()->recent_blockhash = "";

  ScanSolanaTransaction(tx_info->Clone(), "en-US",
                        base::BindLambdaForTesting(
                            [&](mojom::SolanaSimulationResponsePtr response,
                                const std::string& error_response,
                                const std::string& error_string) {
                              EXPECT_TRUE(response);
                              EXPECT_EQ(error_response, "");
                              EXPECT_EQ(error_string, "");
                              run_loop.Quit();
                            }));
  run_loop.Run();

  // KO: Simulation should fail if the latest blockhash is empty both in the
  // transaction and in the RPC response.
  responses[GetNetwork(mojom::kSolanaMainnet, mojom::CoinType::SOL)] = R"(
    {
      "jsonrpc":"2.0",
      "id":1,
      "result":{
        "context":{
          "slot":1069
        },
        "value":{
          "blockhash":"",
          "lastValidBlockHeight":18446744073709551615
        }
      }
    }
  )";
  SetInterceptors(responses);
  base::RunLoop run_loop_2;
  ScanSolanaTransaction(
      std::move(tx_info), "en-US",
      base::BindLambdaForTesting(
          [&](mojom::SolanaSimulationResponsePtr response,
              const std::string& error_response,
              const std::string& error_string) {
            EXPECT_FALSE(response);
            EXPECT_EQ(error_response, "");
            EXPECT_EQ(error_string,
                      l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
            run_loop_2.Quit();
          }));
  run_loop_2.Run();
}

TEST_F(SimulationServiceUnitTest, ScanSolanaTransactionUnsupportedNetwork) {
  auto tx_info = GetCannedScanSolanaTransactionParams(std::nullopt,
                                                      mojom::kLocalhostChainId);

  base::MockCallback<mojom::SimulationService::ScanSolanaTransactionCallback>
      callback;
  EXPECT_CALL(
      callback,
      Run(EqualsMojo(mojom::SolanaSimulationResponsePtr()), "",
          l10n_util::GetStringUTF8(IDS_BRAVE_WALLET_UNSUPPORTED_NETWORK)));

  ScanSolanaTransaction(std::move(tx_info), "en-US", callback.Get());

  task_environment_.RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&callback);
}

TEST_F(SimulationServiceUnitTest, ScanSolanaTransactionEmptyNetwork) {
  auto tx_info = GetCannedScanSolanaTransactionParams(std::nullopt, "");

  base::MockCallback<mojom::SimulationService::ScanSolanaTransactionCallback>
      callback;
  EXPECT_CALL(
      callback,
      Run(EqualsMojo(mojom::SolanaSimulationResponsePtr()), "",
          l10n_util::GetStringUTF8(IDS_BRAVE_WALLET_UNSUPPORTED_NETWORK)));

  ScanSolanaTransaction(std::move(tx_info), "en-US", callback.Get());

  task_environment_.RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&callback);
}

TEST_F(SimulationServiceUnitTest, ScanSolanaTransactionValidErrorResponse) {
  std::string error = R"(
    {
      "error": "No transactions to simulate"
    }
  )";
  SetErrorInterceptor(error);

  auto tx_info =
      GetCannedScanSolanaTransactionParams(std::nullopt, mojom::kSolanaMainnet);

  base::MockCallback<mojom::SimulationService::ScanSolanaTransactionCallback>
      callback;
  EXPECT_CALL(callback, Run(EqualsMojo(mojom::SolanaSimulationResponsePtr()),
                            "No transactions to simulate", ""));

  ScanSolanaTransaction(std::move(tx_info), "en-US", callback.Get());

  task_environment_.RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&callback);
}

TEST_F(SimulationServiceUnitTest,
       ScanSolanaTransactionUnexpectedErrorResponse) {
  std::string error = "Woot";
  SetErrorInterceptor(error);

  auto tx_info =
      GetCannedScanSolanaTransactionParams(std::nullopt, mojom::kSolanaMainnet);

  base::MockCallback<mojom::SimulationService::ScanSolanaTransactionCallback>
      callback;
  EXPECT_CALL(callback,
              Run(EqualsMojo(mojom::SolanaSimulationResponsePtr()), "",
                  l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR)));

  ScanSolanaTransaction(std::move(tx_info), "en-US", callback.Get());

  task_environment_.RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&callback);
}

TEST_F(SimulationServiceUnitTest, ScanSolanaTransactionNullParams) {
  base::MockCallback<mojom::SimulationService::ScanSolanaTransactionCallback>
      callback;
  EXPECT_CALL(callback,
              Run(EqualsMojo(mojom::SolanaSimulationResponsePtr()), "",
                  l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR)));

  simulation_service_->ScanSolanaTransaction("bad_id", "en-US", callback.Get());

  task_environment_.RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&callback);
}

}  // namespace brave_wallet
