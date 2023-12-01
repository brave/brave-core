/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/simulation_service.h"

#include <memory>
#include <optional>
#include <utility>

#include "base/memory/ref_counted.h"
#include "base/memory/scoped_refptr.h"
#include "base/test/bind.h"
#include "base/test/gtest_util.h"
#include "base/test/mock_callback.h"
#include "base/test/task_environment.h"
#include "base/test/values_test_util.h"
#include "brave/components/brave_wallet/browser/brave_wallet_prefs.h"
#include "brave/components/brave_wallet/browser/eip1559_transaction.h"
#include "brave/components/brave_wallet/browser/eth_transaction.h"
#include "brave/components/brave_wallet/browser/eth_tx_meta.h"
#include "brave/components/brave_wallet/browser/simulation_response_parser.h"
#include "brave/components/brave_wallet/browser/solana_transaction.h"
#include "brave/components/brave_wallet/browser/solana_tx_meta.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/common_utils.h"
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
    RegisterProfilePrefs(prefs_.registry());
    RegisterProfilePrefsForMigration(prefs_.registry());
    simulation_service_ =
        std::make_unique<SimulationService>(shared_url_loader_factory_);
  }

  ~SimulationServiceUnitTest() override = default;

  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory() {
    return shared_url_loader_factory_;
  }

  void SetInterceptor(const std::string& content) {
    url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
        [&, content](const network::ResourceRequest& request) {
          url_loader_factory_.ClearResponses();
          url_loader_factory_.AddResponse(request.url.spec(), content);
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

 protected:
  sync_preferences::TestingPrefServiceSyncable prefs_;
  std::unique_ptr<SimulationService> simulation_service_;

 private:
  base::test::TaskEnvironment task_environment_;
  network::TestURLLoaderFactory url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
  data_decoder::test::InProcessDataDecoder in_process_data_decoder_;
};

TEST_F(SimulationServiceUnitTest, GetScanTransactionURL) {
  auto url = simulation_service_->GetScanTransactionURL(
      mojom::kMainnetChainId, mojom::CoinType::ETH, "en-US");
  EXPECT_EQ(url,
            "https://blowfish.wallet.brave.com/ethereum/v0/mainnet/scan/"
            "transaction?language=en-US");

  url = simulation_service_->GetScanTransactionURL(
      mojom::kGoerliChainId, mojom::CoinType::ETH, "en-US");
  EXPECT_EQ(url,
            "https://blowfish.wallet.brave.com/ethereum/v0/goerli/scan/"
            "transaction?language=en-US");

  url = simulation_service_->GetScanTransactionURL(
      mojom::kPolygonMainnetChainId, mojom::CoinType::ETH, "en-US");
  EXPECT_EQ(url,
            "https://blowfish.wallet.brave.com/polygon/v0/mainnet/scan/"
            "transaction?language=en-US");

  url = simulation_service_->GetScanTransactionURL(
      mojom::kBinanceSmartChainMainnetChainId, mojom::CoinType::ETH, "en-US");
  EXPECT_EQ(url,
            "https://blowfish.wallet.brave.com/bnb/v0/mainnet/scan/"
            "transaction?language=en-US");

  url = simulation_service_->GetScanTransactionURL(
      mojom::kArbitrumMainnetChainId, mojom::CoinType::ETH, "en-US");
  EXPECT_EQ(url,
            "https://blowfish.wallet.brave.com/arbitrum/v0/one/scan/"
            "transaction?language=en-US");

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

  url = simulation_service_->GetScanMessageURL(mojom::kGoerliChainId,
                                               mojom::CoinType::ETH, "en-US");
  EXPECT_EQ(url,
            "https://blowfish.wallet.brave.com/ethereum/v0/goerli/scan/"
            "message?language=en-US");

  url = simulation_service_->GetScanMessageURL(mojom::kPolygonMainnetChainId,
                                               mojom::CoinType::ETH, "en-US");
  EXPECT_EQ(url,
            "https://blowfish.wallet.brave.com/polygon/v0/mainnet/scan/"
            "message?language=en-US");

  url = simulation_service_->GetScanMessageURL(
      mojom::kBinanceSmartChainMainnetChainId, mojom::CoinType::ETH, "en-US");
  EXPECT_EQ(url,
            "https://blowfish.wallet.brave.com/bnb/v0/mainnet/scan/"
            "message?language=en-US");

  url = simulation_service_->GetScanMessageURL(mojom::kArbitrumMainnetChainId,
                                               mojom::CoinType::ETH, "en-US");
  EXPECT_EQ(url,
            "https://blowfish.wallet.brave.com/arbitrum/v0/one/scan/"
            "message?language=en-US");
}

TEST_F(SimulationServiceUnitTest, ScanEVMTransactionValidResponse) {
  SetInterceptor(R"(
    {
      "action": "NONE",
      "simulationResults": {
        "error": null,
        "gas": {
          "gasLimit": null
        },
        "expectedStateChanges": [
          {
            "humanReadableDiff": "Send 1 ETH",
            "rawInfo": {
              "data": {
                "amount": {
                  "after": "1182957389356504134754",
                  "before": "1183957389356504134754"
                },
                "contract": {
                  "address": "0xeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee",
                  "kind": "ACCOUNT"
                },
                "asset": {
                  "address": "0xeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee",
                  "decimals": 18,
                  "imageUrl": "https://example.com/eth.png",
                  "name": "Ether",
                  "price": {
                     "dollarValuePerToken": "1968.47",
                     "source": "Coingecko",
                     "updatedAt": "1670324557"
                  },
                  "symbol": "ETH",
                  "verified": true
                }
              },
              "kind": "NATIVE_ASSET_TRANSFER"
            }
          }
        ]
      },
      "warnings": []
    }
  )");

  base::RunLoop run_loop;
  simulation_service_->ScanEVMTransaction(
      GetCannedScanEVMTransactionParams(false, mojom::kMainnetChainId), "en-US",
      base::BindLambdaForTesting([&](mojom::EVMSimulationResponsePtr response,
                                     const std::string& error_response,
                                     const std::string& error_string) {
        ASSERT_TRUE(response);
        EXPECT_EQ(response->action, mojom::BlowfishSuggestedAction::kNone);
        EXPECT_EQ(response->warnings.size(), 0u);
        EXPECT_FALSE(response->simulation_results->error);
        ASSERT_EQ(response->simulation_results->expected_state_changes.size(),
                  1u);

        const auto& state_change =
            response->simulation_results->expected_state_changes.at(0).Clone();
        EXPECT_EQ(state_change->human_readable_diff, "Send 1 ETH");
        EXPECT_EQ(state_change->raw_info->kind,
                  mojom::BlowfishEVMRawInfoKind::kNativeAssetTransfer);
        ASSERT_TRUE(
            state_change->raw_info->data->is_native_asset_transfer_data());
        const auto& state_change_raw_info =
            state_change->raw_info->data->get_native_asset_transfer_data();

        EXPECT_EQ(state_change_raw_info->amount->after,
                  "1182957389356504134754");
        EXPECT_EQ(state_change_raw_info->amount->before,
                  "1183957389356504134754");
        EXPECT_EQ(state_change_raw_info->contract->address,
                  "0xeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee");
        EXPECT_EQ(state_change_raw_info->contract->kind,
                  mojom::BlowfishEVMAddressKind::kAccount);
        EXPECT_EQ(state_change_raw_info->asset->address,
                  "0xeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee");
        EXPECT_EQ(state_change_raw_info->asset->decimals, 18);
        EXPECT_EQ(state_change_raw_info->asset->image_url,
                  "https://example.com/eth.png");
        EXPECT_EQ(state_change_raw_info->asset->name, "Ether");
        EXPECT_EQ(state_change_raw_info->asset->price->dollar_value_per_token,
                  "1968.47");
        EXPECT_EQ(state_change_raw_info->asset->price->source,
                  mojom::BlowfishAssetPriceSource::kCoingecko);
        EXPECT_EQ(state_change_raw_info->asset->price->last_updated_at,
                  "1670324557");
        EXPECT_EQ(state_change_raw_info->asset->symbol, "ETH");
        EXPECT_TRUE(state_change_raw_info->asset->verified);

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

  simulation_service_->ScanEVMTransaction(
      GetCannedScanEVMTransactionParams(false, mojom::kOptimismMainnetChainId),
      "en-US", callback.Get());

  base::RunLoop().RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&callback);
}

TEST_F(SimulationServiceUnitTest, ScanEVMTransactionEmptyNetwork) {
  base::MockCallback<mojom::SimulationService::ScanEVMTransactionCallback>
      callback;
  EXPECT_CALL(
      callback,
      Run(EqualsMojo(mojom::EVMSimulationResponsePtr()), "",
          l10n_util::GetStringUTF8(IDS_BRAVE_WALLET_UNSUPPORTED_NETWORK)));

  simulation_service_->ScanEVMTransaction(
      GetCannedScanEVMTransactionParams(false, ""), "en-US", callback.Get());

  base::RunLoop().RunUntilIdle();
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

  simulation_service_->ScanEVMTransaction(
      GetCannedScanEVMTransactionParams(false, mojom::kPolygonMainnetChainId),
      "en-US", callback.Get());

  base::RunLoop().RunUntilIdle();
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

  simulation_service_->ScanEVMTransaction(
      GetCannedScanEVMTransactionParams(false, mojom::kPolygonMainnetChainId),
      "en-US", callback.Get());

  base::RunLoop().RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&callback);
}

TEST_F(SimulationServiceUnitTest, ScanEVMTransactionNullParams) {
  base::MockCallback<mojom::SimulationService::ScanEVMTransactionCallback>
      callback;
  EXPECT_CALL(callback,
              Run(EqualsMojo(mojom::EVMSimulationResponsePtr()), "",
                  l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR)));

  simulation_service_->ScanEVMTransaction(nullptr, "en-US", callback.Get());

  base::RunLoop().RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&callback);
}

TEST_F(SimulationServiceUnitTest, ScanSolanaTransactionValid) {
  SetInterceptor(R"(
    {
      "status": "CHECKS_PASSED",
      "action": "NONE",
      "warnings": [],
      "simulationResults": {
        "isRecentBlockhashExpired": false,
        "expectedStateChanges": [
          {
            "humanReadableDiff": "Send 2 USDT",
            "suggestedColor": "DEBIT",
            "rawInfo": {
              "kind": "SPL_TRANSFER",
              "data": {
                "symbol": "USDT",
                "name": "USDT",
                "mint": "Es9vMFrzaCERmJfrF4H2FYD4KCoNkY11McCe8BenwNYB",
                "decimals": "6",
                "supply": "1000000000",
                "metaplexTokenStandard": "unknown",
                "assetPrice": {
                  "source": "Coingecko",
                  "last_updated_at": "1679331222",
                  "dollar_value_per_token": "0.99"
                },
                "diff": {
                  "sign": "MINUS",
                  "digits": "2000000"
                }
              }
            }
          }
        ],
        "error": null,
        "raw": {
          "err": null,
          "logs": [],
          "accounts": [],
          "returnData": null,
          "unitsConsumed": 148013
        }
      }
    }
  )");

  base::RunLoop run_loop;
  auto tx_info =
      GetCannedScanSolanaTransactionParams(std::nullopt, mojom::kSolanaMainnet);
  auto request = mojom::SolanaTransactionRequestUnion::NewTransactionInfo(
      std::move(tx_info));

  simulation_service_->ScanSolanaTransaction(
      std::move(request), "en-US",
      base::BindLambdaForTesting([&](mojom::SolanaSimulationResponsePtr
                                         response,
                                     const std::string& error_response,
                                     const std::string& error_string) {
        ASSERT_TRUE(response);

        EXPECT_EQ(response->action, mojom::BlowfishSuggestedAction::kNone);
        EXPECT_EQ(response->warnings.size(), 0u);
        EXPECT_FALSE(response->simulation_results->error);
        EXPECT_FALSE(response->simulation_results->is_recent_blockhash_expired);
        ASSERT_EQ(response->simulation_results->expected_state_changes.size(),
                  1u);

        const auto& state_change =
            response->simulation_results->expected_state_changes.at(0);
        EXPECT_EQ(state_change->human_readable_diff, "Send 2 USDT");
        EXPECT_EQ(state_change->suggested_color,
                  mojom::BlowfishSuggestedColor::kDebit);
        EXPECT_EQ(state_change->raw_info->kind,
                  mojom::BlowfishSolanaRawInfoKind::kSplTransfer);
        ASSERT_TRUE(state_change->raw_info->data->is_spl_transfer_data());

        const auto& state_change_raw_info =
            state_change->raw_info->data->get_spl_transfer_data();
        EXPECT_EQ(state_change_raw_info->symbol, "USDT");
        EXPECT_EQ(state_change_raw_info->name, "USDT");
        EXPECT_EQ(state_change_raw_info->mint,
                  "Es9vMFrzaCERmJfrF4H2FYD4KCoNkY11McCe8BenwNYB");
        EXPECT_EQ(state_change_raw_info->decimals, 6);
        EXPECT_EQ(state_change_raw_info->supply, 1000000000ULL);
        EXPECT_EQ(state_change_raw_info->metaplex_token_standard,
                  mojom::BlowfishMetaplexTokenStandardKind::kUnknown);
        EXPECT_EQ(state_change_raw_info->asset_price->source,
                  mojom::BlowfishAssetPriceSource::kCoingecko);
        EXPECT_EQ(state_change_raw_info->asset_price->last_updated_at,
                  "1679331222");
        EXPECT_EQ(state_change_raw_info->asset_price->dollar_value_per_token,
                  "0.99");
        EXPECT_EQ(state_change_raw_info->diff->sign,
                  mojom::BlowfishDiffSign::kMinus);
        EXPECT_EQ(state_change_raw_info->diff->digits, 2000000ULL);

        EXPECT_EQ(error_response, "");
        EXPECT_EQ(error_string, "");
        run_loop.Quit();
      }));
  run_loop.Run();
}

TEST_F(SimulationServiceUnitTest, ScanSolanaTransactionUnsupportedNetwork) {
  auto tx_info = GetCannedScanSolanaTransactionParams(std::nullopt,
                                                      mojom::kLocalhostChainId);
  auto request = mojom::SolanaTransactionRequestUnion::NewTransactionInfo(
      std::move(tx_info));

  base::MockCallback<mojom::SimulationService::ScanSolanaTransactionCallback>
      callback;
  EXPECT_CALL(
      callback,
      Run(EqualsMojo(mojom::SolanaSimulationResponsePtr()), "",
          l10n_util::GetStringUTF8(IDS_BRAVE_WALLET_UNSUPPORTED_NETWORK)));

  simulation_service_->ScanSolanaTransaction(std::move(request), "en-US",
                                             callback.Get());

  base::RunLoop().RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&callback);
}

TEST_F(SimulationServiceUnitTest, ScanSolanaTransactionEmptyNetwork) {
  auto tx_info = GetCannedScanSolanaTransactionParams(std::nullopt, "");
  auto request = mojom::SolanaTransactionRequestUnion::NewTransactionInfo(
      std::move(tx_info));

  base::MockCallback<mojom::SimulationService::ScanSolanaTransactionCallback>
      callback;
  EXPECT_CALL(
      callback,
      Run(EqualsMojo(mojom::SolanaSimulationResponsePtr()), "",
          l10n_util::GetStringUTF8(IDS_BRAVE_WALLET_UNSUPPORTED_NETWORK)));

  simulation_service_->ScanSolanaTransaction(std::move(request), "en-US",
                                             callback.Get());

  base::RunLoop().RunUntilIdle();
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
  auto request = mojom::SolanaTransactionRequestUnion::NewTransactionInfo(
      std::move(tx_info));

  base::MockCallback<mojom::SimulationService::ScanSolanaTransactionCallback>
      callback;
  EXPECT_CALL(callback, Run(EqualsMojo(mojom::SolanaSimulationResponsePtr()),
                            "No transactions to simulate", ""));

  simulation_service_->ScanSolanaTransaction(std::move(request), "en-US",
                                             callback.Get());

  base::RunLoop().RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&callback);
}

TEST_F(SimulationServiceUnitTest,
       ScanSolanaTransactionUnexpectedErrorResponse) {
  std::string error = "Woot";
  SetErrorInterceptor(error);

  auto tx_info =
      GetCannedScanSolanaTransactionParams(std::nullopt, mojom::kSolanaMainnet);
  auto request = mojom::SolanaTransactionRequestUnion::NewTransactionInfo(
      std::move(tx_info));

  base::MockCallback<mojom::SimulationService::ScanSolanaTransactionCallback>
      callback;
  EXPECT_CALL(callback,
              Run(EqualsMojo(mojom::SolanaSimulationResponsePtr()), "",
                  l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR)));

  simulation_service_->ScanSolanaTransaction(std::move(request), "en-US",
                                             callback.Get());

  base::RunLoop().RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&callback);
}

TEST_F(SimulationServiceUnitTest, ScanSolanaTransactionNullParams) {
  base::MockCallback<mojom::SimulationService::ScanSolanaTransactionCallback>
      callback;
  EXPECT_CALL(callback,
              Run(EqualsMojo(mojom::SolanaSimulationResponsePtr()), "",
                  l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR)));

  simulation_service_->ScanSolanaTransaction(nullptr, "en-US", callback.Get());

  base::RunLoop().RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&callback);
}

}  // namespace brave_wallet
