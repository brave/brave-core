/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/brave_wallet_p3a.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/command_line.h"
#include "base/memory/raw_ptr.h"
#include "base/test/bind.h"
#include "base/test/metrics/histogram_tester.h"
#include "base/test/mock_callback.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/values_test_util.h"
#include "brave/components/brave_wallet/browser/bitcoin/bitcoin_test_utils.h"
#include "brave/components/brave_wallet/browser/brave_wallet_prefs.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/browser/test_utils.h"
#include "brave/components/brave_wallet/browser/tx_service.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_tx_manager.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/brave_wallet_types.h"
#include "brave/components/brave_wallet/common/features.h"
#include "brave/components/brave_wallet/common/switches.h"
#include "brave/components/brave_wallet/common/zcash_utils.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "content/public/test/browser_task_environment.h"
#include "services/data_decoder/public/cpp/test_support/in_process_data_decoder.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::_;
using testing::SaveArg;

namespace brave_wallet {

class BraveWalletP3AUnitTest : public testing::Test {
 public:
  BraveWalletP3AUnitTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME),
        shared_url_loader_factory_(
            base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
                &url_loader_factory_)) {
    histogram_tester_ = std::make_unique<base::HistogramTester>();
  }

  void SetUp() override {
    brave_wallet::RegisterProfilePrefs(prefs_.registry());
    brave_wallet::RegisterProfilePrefsForMigration(prefs_.registry());
    brave_wallet::RegisterLocalStatePrefs(local_state_.registry());
    brave_wallet::RegisterLocalStatePrefsForMigration(local_state_.registry());

    bitcoin_test_rpc_server_ = std::make_unique<BitcoinTestRpcServer>();

    brave_wallet_service_ = std::make_unique<BraveWalletService>(
        shared_url_loader_factory_, TestBraveWalletServiceDelegate::Create(),
        &prefs_, &local_state_);
    brave_wallet_service_->json_rpc_service()->SetAPIRequestHelperForTesting(
        shared_url_loader_factory_);
    keyring_service_ = brave_wallet_service_->keyring_service();
    brave_wallet_service_->GetBitcoinWalletService()
        ->SetUrlLoaderFactoryForTesting(
            bitcoin_test_rpc_server_->GetURLLoaderFactory());
    brave_wallet_service_->GetZcashWalletService()->SetZCashRpcForTesting(
        std::make_unique<ZCashRpc>(brave_wallet_service_->network_manager(),
                                   shared_url_loader_factory_));
    tx_service_ = brave_wallet_service_->tx_service();
    WaitForTxStorageDelegateInitialized(tx_service_->GetDelegateForTesting());
    wallet_p3a_ = brave_wallet_service_->GetBraveWalletP3A();
  }
  void WaitForResponse() { task_environment_.RunUntilIdle(); }

  AccountUtils GetAccountUtils() {
    return AccountUtils(keyring_service_.get());
  }

  mojom::AccountIdPtr eth_from() { return EthAccount(0); }

  mojom::AccountIdPtr EthAccount(size_t index) {
    return GetAccountUtils().EnsureEthAccount(index)->account_id->Clone();
  }

  mojom::AccountIdPtr sol_from() { return SolAccount(0); }

  mojom::AccountIdPtr SolAccount(size_t index) {
    return GetAccountUtils().EnsureSolAccount(index)->account_id->Clone();
  }

  mojom::AccountIdPtr fil_from() { return FilAccount(0); }

  mojom::AccountIdPtr FilAccount(size_t index) {
    return GetAccountUtils().EnsureFilAccount(index)->account_id->Clone();
  }

  mojom::AccountIdPtr btc_from() { return BtcAccount(0); }

  mojom::AccountIdPtr BtcAccount(size_t index) {
    return GetAccountUtils().EnsureBtcAccount(index)->account_id->Clone();
  }

  mojom::AccountIdPtr zec_from() { return ZecAccount(0); }

  mojom::AccountIdPtr ZecAccount(size_t index) {
    return GetAccountUtils().EnsureZecAccount(index)->account_id->Clone();
  }

  void ContinueAddUnapprovedZecTransaction(
      const std::string& chain_id,
      const mojom::AccountIdPtr& from,
      const std::optional<url::Origin>& origin,
      TxService::AddUnapprovedTransactionCallback callback,
      base::expected<ZCashTransaction, std::string> zcash_transaction) {
    tx_service_->GetZCashTxManager()->ContinueAddUnapprovedTransaction(
        chain_id, from, origin, std::move(callback),
        std::move(zcash_transaction));
  }

  void SetInterceptor(const std::string& content) {
    url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
        [&, content](const network::ResourceRequest& request) {
          url_loader_factory_.ClearResponses();
          url_loader_factory_.AddResponse(request.url.spec(), content);
        }));
  }

  void SetSolInterceptor(const std::string& latest_blockhash,
                         uint64_t last_valid_block_height,
                         const std::string& tx_hash,
                         uint64_t block_height = 0,
                         bool get_null_signature_statuses = false) {
    url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
        [&, latest_blockhash, tx_hash, last_valid_block_height,
         block_height](const network::ResourceRequest& request) {
          url_loader_factory_.ClearResponses();
          std::string_view request_string(request.request_body->elements()
                                              ->at(0)
                                              .As<network::DataElementBytes>()
                                              .AsStringPiece());
          base::Value::Dict request_root =
              base::test::ParseJsonDict(request_string);

          std::string* method = request_root.FindString("method");
          ASSERT_TRUE(method);

          if (*method == "getLatestBlockhash") {
            url_loader_factory_.AddResponse(
                request.url.spec(),
                "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":"
                "{\"context\":{\"slot\":1069},\"value\":{\"blockhash\":\"" +
                    latest_blockhash + "\", \"lastValidBlockHeight\":" +
                    base::NumberToString(last_valid_block_height) + "}}}");
          } else if (*method == "getBlockHeight") {
            url_loader_factory_.AddResponse(
                request.url.spec(), R"({"jsonrpc":"2.0", "id":1, "result":)" +
                                        base::NumberToString(block_height) +
                                        "}");
          } else if (*method == "sendTransaction") {
            url_loader_factory_.AddResponse(
                request.url.spec(),
                "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":\"" + tx_hash +
                    "\"}");
          } else if (*method == "simulateTransaction") {
            url_loader_factory_.AddResponse(request.url.spec(), R"({
              "jsonrpc": "2.0",
              "result": {
                "context": {
                  "apiVersion": "1.17.25",
                  "slot": 259225005
                },
                "value": {
                  "accounts": null,
                  "err": null,
                  "logs": [
                    "Program BGUMAp9Gq7iTEuizy4pqaxsTyUCBK68MDfK752saRPUY invoke [1]",
                    "Program log: Instruction: Transfer",
                    "Program BGUMAp9Gq7iTEuizy4pqaxsTyUCBK68MDfK752saRPUY success"
                  ],
                  "returnData": null,
                  "unitsConsumed": 69017
                }
              },
              "id": 1
            })");
          } else if (*method == "getRecentPrioritizationFees") {
            url_loader_factory_.AddResponse(request.url.spec(), R"({
              "jsonrpc": "2.0",
              "result": [
                {
                  "prioritizationFee": 100,
                  "slot": 293251906
                },
                {
                  "prioritizationFee": 200,
                  "slot": 293251906
                },
                {
                  "prioritizationFee": 0,
                  "slot": 293251805
                }
              ],
              "id": 1
            })");
          } else if (*method == "getFeeForMessage") {
            url_loader_factory_.AddResponse(request.url.spec(), R"({
              "jsonrpc":"2.0","id":1,
              "result": {
                "context":{"slot":123065869},
                "value": 5000
              }
            })");
          }
        }));
  }

  void SetFilInterceptor(const std::string& from_account,
                         const std::string& to_account) {
    url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
        [&, from_account, to_account](const network::ResourceRequest& request) {
          url_loader_factory_.ClearResponses();
          std::string_view request_string(request.request_body->elements()
                                              ->at(0)
                                              .As<network::DataElementBytes>()
                                              .AsStringPiece());
          base::Value::Dict request_root =
              base::test::ParseJsonDict(request_string);

          std::string* method = request_root.FindString("method");
          ASSERT_TRUE(method);

          if (*method == "Filecoin.GasEstimateMessageGas") {
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
                          "/":
                          "bafy2bzacedkdoldmztwjwi3jvxhxo4qqp7haufuifpqzregfqkthlyhhf2lfu"
                        }
                      },
                      "id": 1
                    })";
            base::ReplaceSubstringsAfterOffset(&gas_response, 0, "{to}",
                                               to_account);
            base::ReplaceSubstringsAfterOffset(&gas_response, 0, "{from}",
                                               from_account);
            url_loader_factory_.AddResponse(request.url.spec(), gas_response);
          } else if (*method == "Filecoin.MpoolGetNonce") {
            url_loader_factory_.AddResponse(
                request.url.spec(),
                R"({ "jsonrpc": "2.0", "id": 1, "result": 1 })");
          } else if (*method == "Filecoin.StateSearchMsgLimited") {
            url_loader_factory_.AddResponse(
                request.url.spec(),
                R"({ "jsonrpc": "2.0", "id": 1, "result": {}})");
          } else if (*method == "Filecoin.ChainHead") {
            url_loader_factory_.AddResponse(
                request.url.spec(),
                R"({ "jsonrpc": "2.0", "id": 1, "result": {}})");
          } else if (*method == "Filecoin.MpoolPush") {
            url_loader_factory_.AddResponse(
                request.url.spec(),
                R"({ "id": 1, "jsonrpc": "2.0", "result": { "/":
                "bafy2bzacea3wsdh6y3a36tb3skempjoxqpuyompjbmfeyf34fi3uy6uue42v4" }
                })");
          }
        }));
  }

  bool AddUnapprovedTransaction(mojom::TxDataUnionPtr tx_data_union,
                                const std::string& chain_id,
                                const mojom::AccountIdPtr& from_account,
                                std::string* tx_meta_id) {
    bool success;
    base::RunLoop run_loop;
    tx_service_->AddUnapprovedTransaction(
        tx_data_union.Clone(), chain_id, from_account.Clone(),
        base::BindLambdaForTesting([&](bool v, const std::string& tx_id,
                                       const std::string& error_message) {
          success = v;
          *tx_meta_id = tx_id;
          ASSERT_TRUE(error_message.empty());
          run_loop.Quit();
        }));
    run_loop.Run();

    return success;
  }

  bool AddUnapprovedEvmTransaction(mojom::NewEvmTransactionParamsPtr params,
                                   std::string* tx_meta_id) {
    bool success;
    base::RunLoop run_loop;
    tx_service_->AddUnapprovedEvmTransaction(
        std::move(params),
        base::BindLambdaForTesting([&](bool v, const std::string& tx_id,
                                       const std::string& error_message) {
          success = v;
          *tx_meta_id = tx_id;
          ASSERT_TRUE(error_message.empty());
          run_loop.Quit();
        }));
    run_loop.Run();

    return success;
  }

  bool ApproveTransaction(const mojom::CoinType coin_type,
                          const std::string& chain_id,
                          const std::string& tx_meta_id) {
    bool success;
    base::RunLoop run_loop;
    tx_service_->ApproveTransaction(
        coin_type, chain_id, tx_meta_id,
        base::BindLambdaForTesting([&](bool v,
                                       mojom::ProviderErrorUnionPtr error,
                                       const std::string& error_message) {
          success = v;
          ASSERT_TRUE(error_message.empty());
          run_loop.Quit();
        }));
    run_loop.Run();
    return success;
  }

 protected:
  base::test::ScopedFeatureList feature_btc_feature_{
      features::kBraveWalletBitcoinFeature};
  base::test::ScopedFeatureList feature_zec_feature_{
      features::kBraveWalletZCashFeature};
  content::BrowserTaskEnvironment task_environment_;
  sync_preferences::TestingPrefServiceSyncable prefs_;
  sync_preferences::TestingPrefServiceSyncable local_state_;
  std::unique_ptr<base::HistogramTester> histogram_tester_;
  std::unique_ptr<BitcoinTestRpcServer> bitcoin_test_rpc_server_;
  raw_ptr<KeyringService, DanglingUntriaged> keyring_service_;
  raw_ptr<TxService, DanglingUntriaged> tx_service_;
  std::unique_ptr<BraveWalletService> brave_wallet_service_;
  raw_ptr<BraveWalletP3A> wallet_p3a_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
  network::TestURLLoaderFactory url_loader_factory_;
  data_decoder::test::InProcessDataDecoder in_process_data_decoder_;
};

TEST_F(BraveWalletP3AUnitTest, KeyringCreated) {
  histogram_tester_->ExpectBucketCount(kKeyringCreatedHistogramName, 0, 1);
  keyring_service_->CreateWallet("testing123", base::DoNothing());
  WaitForResponse();
  histogram_tester_->ExpectBucketCount(kKeyringCreatedHistogramName, 1, 1);
}

TEST_F(BraveWalletP3AUnitTest, ReportOnboardingAction) {
  histogram_tester_->ExpectTotalCount(kOnboardingConversionHistogramName, 0);

  wallet_p3a_->ReportOnboardingAction(mojom::OnboardingAction::Shown);
  // should not record immediately, should delay
  histogram_tester_->ExpectTotalCount(kOnboardingConversionHistogramName, 0);
  task_environment_.FastForwardBy(base::Seconds(110));

  // report new action before 120 seconds deadline, should postpone timer
  wallet_p3a_->ReportOnboardingAction(
      mojom::OnboardingAction::LegalAndPassword);
  histogram_tester_->ExpectTotalCount(kOnboardingConversionHistogramName, 0);
  task_environment_.FastForwardBy(base::Seconds(120));
  histogram_tester_->ExpectUniqueSample(kOnboardingConversionHistogramName, 1,
                                        1);

  // report new action after 120 seconds deadline, should record
  // immediately to correct histogram value
  wallet_p3a_->ReportOnboardingAction(mojom::OnboardingAction::RecoverySetup);
  histogram_tester_->ExpectTotalCount(kOnboardingConversionHistogramName, 2);
  histogram_tester_->ExpectBucketCount(kOnboardingConversionHistogramName, 2,
                                       1);
}

TEST_F(BraveWalletP3AUnitTest, ReportOnboardingActionRestore) {
  histogram_tester_->ExpectTotalCount(kOnboardingConversionHistogramName, 0);

  wallet_p3a_->ReportOnboardingAction(mojom::OnboardingAction::Shown);
  histogram_tester_->ExpectTotalCount(kOnboardingConversionHistogramName, 0);
  task_environment_.FastForwardBy(base::Seconds(50));

  wallet_p3a_->ReportOnboardingAction(mojom::OnboardingAction::StartRestore);
  histogram_tester_->ExpectTotalCount(kOnboardingConversionHistogramName, 0);
  task_environment_.FastForwardBy(base::Seconds(120));
  // should not monitor the wallet restore flow
  histogram_tester_->ExpectTotalCount(kOnboardingConversionHistogramName, 0);
}

TEST_F(BraveWalletP3AUnitTest, ReportTransactionSent) {
  histogram_tester_->ExpectTotalCount(kEthTransactionSentHistogramName, 0);
  histogram_tester_->ExpectTotalCount(kSolTransactionSentHistogramName, 0);
  histogram_tester_->ExpectTotalCount(kFilTransactionSentHistogramName, 0);
  histogram_tester_->ExpectTotalCount(kBtcTransactionSentHistogramName, 0);
  histogram_tester_->ExpectTotalCount(kZecTransactionSentHistogramName, 0);

  wallet_p3a_->ReportTransactionSent(mojom::CoinType::ETH, true);
  histogram_tester_->ExpectUniqueSample(kEthTransactionSentHistogramName, 1, 1);

  wallet_p3a_->ReportTransactionSent(mojom::CoinType::SOL, true);
  histogram_tester_->ExpectUniqueSample(kSolTransactionSentHistogramName, 1, 1);

  wallet_p3a_->ReportTransactionSent(mojom::CoinType::FIL, true);
  histogram_tester_->ExpectUniqueSample(kFilTransactionSentHistogramName, 1, 1);

  wallet_p3a_->ReportTransactionSent(mojom::CoinType::BTC, true);
  histogram_tester_->ExpectUniqueSample(kBtcTransactionSentHistogramName, 1, 1);

  wallet_p3a_->ReportTransactionSent(mojom::CoinType::ZEC, true);
  histogram_tester_->ExpectUniqueSample(kZecTransactionSentHistogramName, 1, 1);

  task_environment_.FastForwardBy(base::Days(4));
  histogram_tester_->ExpectUniqueSample(kEthTransactionSentHistogramName, 1, 5);
  histogram_tester_->ExpectUniqueSample(kSolTransactionSentHistogramName, 1, 5);
  histogram_tester_->ExpectUniqueSample(kFilTransactionSentHistogramName, 1, 5);
  histogram_tester_->ExpectUniqueSample(kBtcTransactionSentHistogramName, 1, 5);
  histogram_tester_->ExpectUniqueSample(kZecTransactionSentHistogramName, 1, 5);

  task_environment_.FastForwardBy(base::Days(3));
  histogram_tester_->ExpectBucketCount(kEthTransactionSentHistogramName, 0, 1);
  histogram_tester_->ExpectBucketCount(kSolTransactionSentHistogramName, 0, 1);
  histogram_tester_->ExpectBucketCount(kFilTransactionSentHistogramName, 0, 1);
  histogram_tester_->ExpectBucketCount(kBtcTransactionSentHistogramName, 0, 1);
  histogram_tester_->ExpectBucketCount(kZecTransactionSentHistogramName, 0, 1);
}

TEST_F(BraveWalletP3AUnitTest, ActiveAccounts) {
  wallet_p3a_->RecordActiveWalletCount(0, mojom::CoinType::ETH);
  wallet_p3a_->RecordActiveWalletCount(0, mojom::CoinType::FIL);
  wallet_p3a_->RecordActiveWalletCount(0, mojom::CoinType::SOL);
  wallet_p3a_->RecordActiveWalletCount(0, mojom::CoinType::BTC);
  wallet_p3a_->RecordActiveWalletCount(0, mojom::CoinType::ZEC);

  // Should not record zero to histogram if user never had an active account
  histogram_tester_->ExpectTotalCount(kEthActiveAccountHistogramName, 0);
  histogram_tester_->ExpectTotalCount(kFilActiveAccountHistogramName, 0);
  histogram_tester_->ExpectTotalCount(kSolActiveAccountHistogramName, 0);
  histogram_tester_->ExpectTotalCount(kBtcActiveAccountHistogramName, 0);
  histogram_tester_->ExpectTotalCount(kZecActiveAccountHistogramName, 0);

  wallet_p3a_->RecordActiveWalletCount(3, mojom::CoinType::ETH);
  wallet_p3a_->RecordActiveWalletCount(9, mojom::CoinType::FIL);
  wallet_p3a_->RecordActiveWalletCount(7, mojom::CoinType::SOL);
  wallet_p3a_->RecordActiveWalletCount(4, mojom::CoinType::BTC);
  wallet_p3a_->RecordActiveWalletCount(2, mojom::CoinType::ZEC);

  histogram_tester_->ExpectBucketCount(kEthActiveAccountHistogramName, 3, 1);
  histogram_tester_->ExpectBucketCount(kFilActiveAccountHistogramName, 5, 1);
  histogram_tester_->ExpectBucketCount(kSolActiveAccountHistogramName, 4, 1);
  histogram_tester_->ExpectBucketCount(kBtcActiveAccountHistogramName, 4, 1);
  histogram_tester_->ExpectBucketCount(kZecActiveAccountHistogramName, 2, 1);

  wallet_p3a_->RecordActiveWalletCount(0, mojom::CoinType::ETH);
  wallet_p3a_->RecordActiveWalletCount(1, mojom::CoinType::FIL);
  wallet_p3a_->RecordActiveWalletCount(2, mojom::CoinType::SOL);
  wallet_p3a_->RecordActiveWalletCount(3, mojom::CoinType::BTC);
  wallet_p3a_->RecordActiveWalletCount(4, mojom::CoinType::ZEC);

  histogram_tester_->ExpectBucketCount(kEthActiveAccountHistogramName, 0, 1);
  histogram_tester_->ExpectBucketCount(kFilActiveAccountHistogramName, 1, 1);
  histogram_tester_->ExpectBucketCount(kSolActiveAccountHistogramName, 2, 1);
  histogram_tester_->ExpectBucketCount(kBtcActiveAccountHistogramName, 3, 1);
  histogram_tester_->ExpectBucketCount(kZecActiveAccountHistogramName, 4, 1);
}

TEST_F(BraveWalletP3AUnitTest, NewUserBalance) {
  // record first usage
  wallet_p3a_->ReportUsage(true);

  task_environment_.FastForwardBy(base::Days(3));
  wallet_p3a_->ReportUsage(true);

  histogram_tester_->ExpectTotalCount(kNewUserBalanceHistogramName, 0);
  wallet_p3a_->RecordActiveWalletCount(1, mojom::CoinType::ETH);
  histogram_tester_->ExpectUniqueSample(kNewUserBalanceHistogramName, 1, 1);

  task_environment_.FastForwardBy(base::Days(2));
  wallet_p3a_->RecordActiveWalletCount(1, mojom::CoinType::ETH);
  // Should not record because we already recorded
  histogram_tester_->ExpectUniqueSample(kNewUserBalanceHistogramName, 1, 1);
}

TEST_F(BraveWalletP3AUnitTest, NewUserBalancePastDeadline) {
  // record first usage
  wallet_p3a_->ReportUsage(true);

  task_environment_.FastForwardBy(base::Days(8));
  wallet_p3a_->ReportUsage(true);

  histogram_tester_->ExpectTotalCount(kNewUserBalanceHistogramName, 0);
  wallet_p3a_->RecordActiveWalletCount(1, mojom::CoinType::ETH);

  // Should not record new value since we are past the deadline
  histogram_tester_->ExpectTotalCount(kNewUserBalanceHistogramName, 0);
}

TEST_F(BraveWalletP3AUnitTest, JSProviders) {
  auto test_func = [&](mojom::CoinType coin_type, const char* histogram_name) {
    histogram_tester_->ExpectTotalCount(histogram_name, 0);

    wallet_p3a_->ReportJSProvider(mojom::JSProviderType::None, coin_type,
                                  /*allow_provider_override*/ true);
    histogram_tester_->ExpectUniqueSample(histogram_name, 0, 1);

    keyring_service_->CreateWallet("testing123", base::DoNothing());
    WaitForResponse();

    wallet_p3a_->ReportJSProvider(mojom::JSProviderType::None, coin_type,
                                  /*allow_provider_override*/ true);
    histogram_tester_->ExpectBucketCount(histogram_name, 1, 1);

    wallet_p3a_->ReportJSProvider(mojom::JSProviderType::Native, coin_type,
                                  /*allow_provider_override*/ true);
    histogram_tester_->ExpectBucketCount(histogram_name, 2, 1);

    wallet_p3a_->ReportJSProvider(mojom::JSProviderType::Native, coin_type,
                                  /*allow_provider_override*/ false);
    histogram_tester_->ExpectBucketCount(histogram_name, 3, 1);

    wallet_p3a_->ReportJSProvider(mojom::JSProviderType::ThirdParty, coin_type,
                                  /*allow_provider_override*/ true);
    histogram_tester_->ExpectBucketCount(histogram_name, 5, 1);

    keyring_service_->Reset();

    wallet_p3a_->ReportJSProvider(mojom::JSProviderType::ThirdParty, coin_type,
                                  /*allow_provider_override*/ true);
    histogram_tester_->ExpectBucketCount(histogram_name, 4, 1);

    keyring_service_->Reset();
    wallet_p3a_->ReportJSProvider(mojom::JSProviderType::Native, coin_type,
                                  /*allow_provider_override*/ true);
    histogram_tester_->ExpectBucketCount(histogram_name, 0, 2);
  };
  test_func(mojom::CoinType::ETH, kEthProviderHistogramName);
  test_func(mojom::CoinType::SOL, kSolProviderHistogramName);
}

TEST_F(BraveWalletP3AUnitTest, NFTGalleryViews) {
  histogram_tester_->ExpectTotalCount(kBraveWalletNFTCountHistogramName, 0);
  histogram_tester_->ExpectTotalCount(kBraveWalletNFTNewUserHistogramName, 0);

  wallet_p3a_->RecordNFTGalleryView(0);
  histogram_tester_->ExpectUniqueSample(kBraveWalletNFTCountHistogramName, 0,
                                        1);
  histogram_tester_->ExpectUniqueSample(kBraveWalletNFTNewUserHistogramName, 1,
                                        1);

  wallet_p3a_->RecordNFTGalleryView(6);
  histogram_tester_->ExpectBucketCount(kBraveWalletNFTCountHistogramName, 2, 1);
  // new user histogram should only be reported once, ever
  histogram_tester_->ExpectUniqueSample(kBraveWalletNFTNewUserHistogramName, 1,
                                        1);
}

TEST_F(BraveWalletP3AUnitTest, NFTDiscoveryEnabled) {
  histogram_tester_->ExpectTotalCount(
      kBraveWalletNFTDiscoveryEnabledHistogramName, 0);

  local_state_.SetTime(kBraveWalletLastUnlockTime, base::Time::Now());
  histogram_tester_->ExpectUniqueSample(
      kBraveWalletNFTDiscoveryEnabledHistogramName, 0, 1);

  prefs_.SetBoolean(kBraveWalletNftDiscoveryEnabled, true);
  histogram_tester_->ExpectBucketCount(
      kBraveWalletNFTDiscoveryEnabledHistogramName, 1, 1);

  prefs_.SetBoolean(kBraveWalletNftDiscoveryEnabled, false);
  histogram_tester_->ExpectBucketCount(
      kBraveWalletNFTDiscoveryEnabledHistogramName, 0, 2);
}

TEST_F(BraveWalletP3AUnitTest, EthTransactionSentObservation) {
  histogram_tester_->ExpectTotalCount(kEthTransactionSentHistogramName, 0);

  keyring_service_->CreateWallet("testing123", base::DoNothing());

  // Set an interceptor and just fake a common response for
  // eth_getTransactionCount and eth_sendRawTransaction
  SetInterceptor("{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":\"0x0\"}");

  // Create & add unapproved ETH transaction
  std::string tx_meta_id;
  EXPECT_TRUE(AddUnapprovedEvmTransaction(
      mojom::NewEvmTransactionParams::New(
          mojom::kBnbSmartChainMainnetChainId, eth_from(),
          "0xbe862ad9abfe6f22bcb087716c7d89a26051f74c", "0x016345785d8a0000",
          "0x0974", std::vector<uint8_t>()),
      &tx_meta_id));

  // Approve the ETH transaction
  EXPECT_TRUE(ApproveTransaction(
      mojom::CoinType::ETH, mojom::kBnbSmartChainMainnetChainId, tx_meta_id));

  // Verify EthTransactionSent
  histogram_tester_->ExpectUniqueSample(kEthTransactionSentHistogramName, 1, 1);
}

TEST_F(BraveWalletP3AUnitTest, TestnetEthTransactionSentObservation) {
  histogram_tester_->ExpectTotalCount(kEthTransactionSentHistogramName, 0);

  keyring_service_->CreateWallet("testing123", base::DoNothing());

  // Set an interceptor and just fake a common response for
  // eth_getTransactionCount and eth_sendRawTransaction
  SetInterceptor("{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":\"0x0\"}");

  // Create & add unapproved ETH transaction on testnet
  std::string tx_meta_id;
  EXPECT_TRUE(AddUnapprovedEvmTransaction(
      mojom::NewEvmTransactionParams::New(
          mojom::kLocalhostChainId, eth_from(),
          "0xbe862ad9abfe6f22bcb087716c7d89a26051f74c", "0x016345785d8a0000",
          "0x0974", std::vector<uint8_t>()),
      &tx_meta_id));

  // Approve the ETH transaction on testnet
  EXPECT_TRUE(ApproveTransaction(mojom::CoinType::ETH, mojom::kLocalhostChainId,
                                 tx_meta_id));

  // Verify EthTransactionSent not updated (testnet switch disabled)
  histogram_tester_->ExpectTotalCount(kEthTransactionSentHistogramName, 0);

  base::CommandLine* cmdline = base::CommandLine::ForCurrentProcess();
  cmdline->AppendSwitch(mojom::kP3ACountTestNetworksSwitch);

  // Create & add unapproved ETH transaction on testnet
  EXPECT_TRUE(AddUnapprovedEvmTransaction(
      mojom::NewEvmTransactionParams::New(
          mojom::kLocalhostChainId, eth_from(),
          "0xbe862ad9abfe6f22bcb087716c7d89a26051f74c", "0x016345785d8a0000",
          "0x0974", std::vector<uint8_t>()),
      &tx_meta_id));

  // Approve the ETH transaction on testnet
  EXPECT_TRUE(ApproveTransaction(mojom::CoinType::ETH, mojom::kLocalhostChainId,
                                 tx_meta_id));

  // Verify EthTransactionSent
  histogram_tester_->ExpectUniqueSample(kEthTransactionSentHistogramName, 1, 1);

  cmdline->RemoveSwitch(mojom::kP3ACountTestNetworksSwitch);
}

TEST_F(BraveWalletP3AUnitTest, SolTransactionSentObservation) {
  histogram_tester_->ExpectTotalCount(kSolTransactionSentHistogramName, 0);

  keyring_service_->CreateWallet("testing123", base::DoNothing());

  // Create & add unapproved SOL transaction
  const auto& from_account = sol_from();
  std::string from_account_address = from_account->address;

  std::string to_account = "JDqrvDz8d8tFCADashbUKQDKfJZFobNy13ugN65t1wvV";
  const std::vector<uint8_t> data = {2, 0, 0, 0, 128, 150, 152, 0, 0, 0, 0, 0};

  std::vector<mojom::SolanaAccountMetaPtr> account_metas;
  auto account_meta1 =
      mojom::SolanaAccountMeta::New(from_account_address, nullptr, true, true);
  auto account_meta2 =
      mojom::SolanaAccountMeta::New(to_account, nullptr, false, true);
  account_metas.push_back(std::move(account_meta1));
  account_metas.push_back(std::move(account_meta2));

  auto instruction = mojom::SolanaInstruction::New(
      mojom::kSolanaSystemProgramId, std::move(account_metas), data, nullptr);
  std::vector<mojom::SolanaInstructionPtr> instructions;
  instructions.push_back(std::move(instruction));

  auto solana_tx_data = mojom::SolanaTxData::New(
      "", 0, from_account_address, to_account, "", 10000000, 0,
      mojom::TransactionType::SolanaSystemTransfer, std::move(instructions),
      mojom::SolanaMessageVersion::kLegacy,
      mojom::SolanaMessageHeader::New(1, 0, 1),
      std::vector<std::string>(
          {from_account_address, to_account, mojom::kSolanaSystemProgramId}),
      std::vector<mojom::SolanaMessageAddressTableLookupPtr>(), nullptr,
      nullptr, nullptr);

  std::string tx_hash1 =
      "5VERv8NMvzbJMEkV8xnrLkEaWRtSz9CosKDYjCJjBRnbJLgp8uirBgmQpjKhoR4tjF3ZpR"
      "zrFmBV6UjKdiSZkQUW";
  auto* latest_blockhash1 = "EkSnNWid2cvwEVnVx9aBqawnmiCNiDgp3gUdkDPTKN1N";
  auto last_valid_block_height1 = 3090;

  SetSolInterceptor(latest_blockhash1, last_valid_block_height1, tx_hash1,
                    last_valid_block_height1);

  std::string tx_meta_id;
  EXPECT_TRUE(AddUnapprovedTransaction(
      mojom::TxDataUnion::NewSolanaTxData(std::move(solana_tx_data)),
      mojom::kSolanaMainnet, sol_from(), &tx_meta_id));

  // Approve the SOL transaction
  EXPECT_TRUE(ApproveTransaction(mojom::CoinType::SOL, mojom::kSolanaMainnet,
                                 tx_meta_id));

  // Verify SolTransactionSent
  histogram_tester_->ExpectUniqueSample(kSolTransactionSentHistogramName, 1, 1);
}

TEST_F(BraveWalletP3AUnitTest, FilTransactionSentObservation) {
  histogram_tester_->ExpectTotalCount(kFilTransactionSentHistogramName, 0);

  keyring_service_->CreateWallet("testing123", base::DoNothing());

  // Create & add unapproved FIL transaction
  std::string to_account = "f1h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q";
  SetFilInterceptor(fil_from()->address, to_account);
  auto tx_data = mojom::FilTxData::New("" /* nonce */, "" /* gas_premium */,
                                       "" /* gas_fee_cap */, "" /* gas_limit */,
                                       "" /* max_fee */, to_account, "11");
  std::string tx_meta_id;
  EXPECT_TRUE(AddUnapprovedTransaction(
      mojom::TxDataUnion::NewFilTxData(std::move(tx_data)),
      mojom::kFilecoinMainnet, fil_from(), &tx_meta_id));

  // Approve the FIL transaction
  EXPECT_TRUE(ApproveTransaction(mojom::CoinType::FIL, mojom::kFilecoinMainnet,
                                 tx_meta_id));

  // Verify FilTransactionSent
  histogram_tester_->ExpectUniqueSample(kFilTransactionSentHistogramName, 1, 1);
}

TEST_F(BraveWalletP3AUnitTest, BtcTransactionSentObservation) {
  histogram_tester_->ExpectTotalCount(kBtcTransactionSentHistogramName, 0);
  AccountUtils(keyring_service_.get())
      .CreateWallet(kMnemonicDivideCruise, kTestWalletPassword);

  bitcoin_test_rpc_server_->SetUpBitcoinRpc(kMnemonicDivideCruise, 0);

  auto tx_data = mojom::BtcTxData::New(kMockBtcAddress, 5000, false, 0,
                                       std::vector<mojom::BtcTxInputPtr>(),
                                       std::vector<mojom::BtcTxOutputPtr>());
  std::string tx_meta_id;
  EXPECT_TRUE(AddUnapprovedTransaction(
      mojom::TxDataUnion::NewBtcTxData(std::move(tx_data)),
      mojom::kBitcoinMainnet, btc_from(), &tx_meta_id));

  // Approve the BTC transaction
  EXPECT_TRUE(ApproveTransaction(mojom::CoinType::BTC, mojom::kBitcoinMainnet,
                                 tx_meta_id));

  // Verify BtcTransactionSent
  histogram_tester_->ExpectUniqueSample(kBtcTransactionSentHistogramName, 1, 1);
}

TEST_F(BraveWalletP3AUnitTest, ZecTransactionSentObservation) {
  histogram_tester_->ExpectTotalCount(kZecTransactionSentHistogramName, 0);

  keyring_service_->CreateWallet("testing123", base::DoNothing());

  auto tx_data = mojom::BtcTxData::New(kMockBtcAddress, 5000, false, 0,
                                       std::vector<mojom::BtcTxInputPtr>(),
                                       std::vector<mojom::BtcTxOutputPtr>());

  ZCashTransaction zcash_transaction;
  zcash_transaction.set_locktime(2286687);
  {
    ZCashTransaction::TxInput input;
    input.utxo_outpoint.index = 0;
    input.utxo_address = "t1c61yifRMgyhMsBYsFDBa5aEQkgU65CGau";
    input.utxo_value = 537000;
    input.script_pub_key =
        ZCashAddressToScriptPubkey(input.utxo_address, false);
    zcash_transaction.transparent_part().inputs.push_back(std::move(input));
  }

  {
    ZCashTransaction::TxOutput output;
    output.address = "t1KrG29yWzoi7Bs2pvsgXozZYPvGG4D3sGi";
    output.amount = 500000;
    output.script_pubkey = ZCashAddressToScriptPubkey(output.address, false);
    zcash_transaction.transparent_part().outputs.push_back(std::move(output));
  }

  std::string tx_meta_id;
  base::MockCallback<TxService::AddUnapprovedTransactionCallback> add_callback;
  EXPECT_CALL(add_callback, Run(true, _, _)).WillOnce(SaveArg<1>(&tx_meta_id));
  ContinueAddUnapprovedZecTransaction(mojom::kZCashMainnet, zec_from(),
                                      std::nullopt, add_callback.Get(),
                                      std::move(zcash_transaction));
  EXPECT_NE(tx_meta_id, "");

  // Approve the ZEC transaction
  tx_service_->ApproveTransaction(mojom::CoinType::ZEC, mojom::kZCashMainnet,
                                  tx_meta_id, base::DoNothing());
  WaitForResponse();

  // Verify ZecTransactionSent
  histogram_tester_->ExpectUniqueSample(kZecTransactionSentHistogramName, 1, 1);
}

}  // namespace brave_wallet
