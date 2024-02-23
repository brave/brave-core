/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/brave_wallet_p3a.h"
#include "base/memory/raw_ptr.h"
#include "base/test/bind.h"
#include "base/test/metrics/histogram_tester.h"
#include "brave/browser/brave_wallet/brave_wallet_service_factory.h"
#include "brave/browser/brave_wallet/json_rpc_service_factory.h"
#include "brave/browser/brave_wallet/keyring_service_factory.h"
#include "brave/browser/brave_wallet/tx_service_factory.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/browser/test_utils.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/switches.h"
#include "chrome/test/base/scoped_testing_local_state.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile_manager.h"
#include "content/public/test/browser_task_environment.h"
#include "services/data_decoder/public/cpp/test_support/in_process_data_decoder.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"

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
    TestingProfile::Builder builder;
    local_state_ = std::make_unique<ScopedTestingLocalState>(
        TestingBrowserProcess::GetGlobal());
    profile_ = builder.Build();
    keyring_service_ =
        KeyringServiceFactory::GetServiceForContext(profile_.get());
    wallet_service_ =
        brave_wallet::BraveWalletServiceFactory::GetServiceForContext(
            profile_.get());
    tx_service_ = TxServiceFactory::GetServiceForContext(profile_.get());
    json_rpc_service_ =
        JsonRpcServiceFactory::GetServiceForContext(profile_.get());
    json_rpc_service_->SetAPIRequestHelperForTesting(
        shared_url_loader_factory_);
    wallet_p3a_ = wallet_service_->GetBraveWalletP3A();
  }
  void WaitForResponse() { task_environment_.RunUntilIdle(); }

  AccountUtils GetAccountUtils() {
    return AccountUtils(keyring_service_.get());
  }

  mojom::AccountIdPtr eth_from() { return EthAccount(0); }

  mojom::AccountIdPtr EthAccount(size_t index) {
    return GetAccountUtils().EnsureEthAccount(index)->account_id->Clone();
  }

  void SetInterceptor(const std::string& content) {
    url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
        [&, content](const network::ResourceRequest& request) {
          url_loader_factory_.ClearResponses();
          url_loader_factory_.AddResponse(request.url.spec(), content);
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
  content::BrowserTaskEnvironment task_environment_;
  std::unique_ptr<ScopedTestingLocalState> local_state_;
  std::unique_ptr<TestingProfile> profile_;
  std::unique_ptr<base::HistogramTester> histogram_tester_;
  raw_ptr<KeyringService> keyring_service_;
  raw_ptr<TxService> tx_service_;
  raw_ptr<BraveWalletService> wallet_service_;
  raw_ptr<BraveWalletP3A> wallet_p3a_;
  raw_ptr<JsonRpcService> json_rpc_service_;
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

TEST_F(BraveWalletP3AUnitTest, TransactionSent) {
  histogram_tester_->ExpectTotalCount(kEthTransactionSentHistogramName, 0);
  histogram_tester_->ExpectTotalCount(kSolTransactionSentHistogramName, 0);
  histogram_tester_->ExpectTotalCount(kFilTransactionSentHistogramName, 0);

  BraveWalletP3A* wallet_p3a = wallet_service_->GetBraveWalletP3A();

  wallet_p3a->ReportTransactionSent(mojom::CoinType::ETH, true);
  histogram_tester_->ExpectUniqueSample(kEthTransactionSentHistogramName, 1, 1);

  wallet_p3a->ReportTransactionSent(mojom::CoinType::SOL, true);
  histogram_tester_->ExpectUniqueSample(kSolTransactionSentHistogramName, 1, 1);

  wallet_p3a->ReportTransactionSent(mojom::CoinType::FIL, true);
  histogram_tester_->ExpectUniqueSample(kFilTransactionSentHistogramName, 1, 1);

  task_environment_.FastForwardBy(base::Days(4));
  histogram_tester_->ExpectUniqueSample(kEthTransactionSentHistogramName, 1, 5);
  histogram_tester_->ExpectUniqueSample(kSolTransactionSentHistogramName, 1, 5);
  histogram_tester_->ExpectUniqueSample(kFilTransactionSentHistogramName, 1, 5);

  task_environment_.FastForwardBy(base::Days(3));
  histogram_tester_->ExpectBucketCount(kEthTransactionSentHistogramName, 0, 1);
  histogram_tester_->ExpectBucketCount(kSolTransactionSentHistogramName, 0, 1);
  histogram_tester_->ExpectBucketCount(kFilTransactionSentHistogramName, 0, 1);
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

  local_state_->Get()->SetTime(kBraveWalletLastUnlockTime, base::Time::Now());
  histogram_tester_->ExpectUniqueSample(
      kBraveWalletNFTDiscoveryEnabledHistogramName, 0, 1);

  profile_->GetPrefs()->SetBoolean(kBraveWalletNftDiscoveryEnabled, true);
  histogram_tester_->ExpectBucketCount(
      kBraveWalletNFTDiscoveryEnabledHistogramName, 1, 1);

  profile_->GetPrefs()->SetBoolean(kBraveWalletNftDiscoveryEnabled, false);
  histogram_tester_->ExpectBucketCount(
      kBraveWalletNFTDiscoveryEnabledHistogramName, 0, 2);
}

TEST_F(BraveWalletP3AUnitTest, EthTransactionSentObservation) {
  histogram_tester_->ExpectTotalCount(kEthTransactionSentHistogramName, 0);

  WaitForTxStorageDelegateInitialized(tx_service_->GetDelegateForTesting());
  keyring_service_->CreateWallet("testing123", base::DoNothing());
  keyring_service_->AddAccountSync(mojom::CoinType::ETH,
                                   mojom::kDefaultKeyringId, "Account 1");

  // Create & add unapproved ETH transaction
  std::vector<uint8_t> data_;
  auto tx_data =
      mojom::TxData::New("0x06", "0x09184e72a000", "0x0974",
                         "0xbe862ad9abfe6f22bcb087716c7d89a26051f74c",
                         "0x016345785d8a0000", data_, false, std::nullopt);
  std::string tx_meta_id;
  EXPECT_TRUE(AddUnapprovedTransaction(
      mojom::TxDataUnion::NewEthTxData(std::move(tx_data)),
      mojom::kMainnetChainId, eth_from(), &tx_meta_id));

  // Set an interceptor and just fake a common repsonse for
  // eth_getTransactionCount and eth_sendRawTransaction
  SetInterceptor("{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":\"0x0\"}");

  // Approve the ETH transaction
  EXPECT_TRUE(ApproveTransaction(mojom::CoinType::ETH, mojom::kMainnetChainId,
                                 tx_meta_id));

  // Verify EthTransactionSent
  histogram_tester_->ExpectUniqueSample(kEthTransactionSentHistogramName, 1, 1);
}

TEST_F(BraveWalletP3AUnitTest, TestnetEthTransactionSentObservation) {
  histogram_tester_->ExpectTotalCount(kEthTransactionSentHistogramName, 0);

  WaitForTxStorageDelegateInitialized(tx_service_->GetDelegateForTesting());
  keyring_service_->CreateWallet("testing123", base::DoNothing());
  keyring_service_->AddAccountSync(mojom::CoinType::ETH,
                                   mojom::kDefaultKeyringId, "Account 1");
  // Set an interceptor and just fake a common repsonse for
  // eth_getTransactionCount and eth_sendRawTransaction
  SetInterceptor("{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":\"0x0\"}");

  // Create & add unapproved ETH transaction on testnet
  std::vector<uint8_t> data_;
  auto tx_data =
      mojom::TxData::New("0x06", "0x09184e72a000", "0x0974",
                         "0xbe862ad9abfe6f22bcb087716c7d89a26051f74c",
                         "0x016345785d8a0000", data_, false, std::nullopt);
  std::string tx_meta_id;
  EXPECT_TRUE(AddUnapprovedTransaction(
      mojom::TxDataUnion::NewEthTxData(std::move(tx_data)),
      mojom::kLocalhostChainId, eth_from(), &tx_meta_id));

  // Approve the ETH transaction on testnet
  EXPECT_TRUE(ApproveTransaction(mojom::CoinType::ETH, mojom::kLocalhostChainId,
                                 tx_meta_id));

  // Verify EthTransactionSent not updated (testnet switch disabled)
  histogram_tester_->ExpectTotalCount(kEthTransactionSentHistogramName, 0);

  base::CommandLine* cmdline = base::CommandLine::ForCurrentProcess();
  cmdline->AppendSwitch(mojom::kP3ACountTestNetworksSwitch);

  // Create & add unapproved ETH transaction on testnet
  tx_data =
      mojom::TxData::New("0x06", "0x09184e72a000", "0x0974",
                         "0xbe862ad9abfe6f22bcb087716c7d89a26051f74c",
                         "0x016345785d8a0000", data_, false, std::nullopt);
  EXPECT_TRUE(AddUnapprovedTransaction(
      mojom::TxDataUnion::NewEthTxData(std::move(tx_data)),
      mojom::kLocalhostChainId, eth_from(), &tx_meta_id));

  // Approve the ETH transaction on testnet
  EXPECT_TRUE(ApproveTransaction(mojom::CoinType::ETH, mojom::kLocalhostChainId,
                                 tx_meta_id));

  // Verify EthTransactionSent
  histogram_tester_->ExpectUniqueSample(kEthTransactionSentHistogramName, 1, 1);

  cmdline->RemoveSwitch(mojom::kP3ACountTestNetworksSwitch);
}

TEST_F(BraveWalletP3AUnitTest, SolTransactionSentObservation) {
  histogram_tester_->ExpectTotalCount(kSolTransactionSentHistogramName, 0);

  WaitForTxStorageDelegateInitialized(tx_service_->GetDelegateForTesting());
  keyring_service_->CreateWallet("testing123", base::DoNothing());
  keyring_service_->AddAccountSync(mojom::CoinType::SOL,
                                   mojom::kSolanaKeyringId, "Account 1");

  // TODO(stephenheaps): Create & add unapproved SOL transaction

  // TODO(stephenheaps): Approve the SOL transaction

  // Verify SolTransactionSent
  histogram_tester_->ExpectUniqueSample(kSolTransactionSentHistogramName, 1, 1);
}

TEST_F(BraveWalletP3AUnitTest, FilTransactionSentObservation) {
  histogram_tester_->ExpectTotalCount(kFilTransactionSentHistogramName, 0);

  WaitForTxStorageDelegateInitialized(tx_service_->GetDelegateForTesting());
  keyring_service_->CreateWallet("testing123", base::DoNothing());
  keyring_service_->AddAccountSync(mojom::CoinType::FIL,
                                   mojom::kFilecoinKeyringId, "Account 1");

  // TODO(stephenheaps): Create & add unapproved FIL transaction

  // TODO(stephenheaps): Approve the FIL transaction

  // Verify FilTransactionSent
  histogram_tester_->ExpectUniqueSample(kFilTransactionSentHistogramName, 1, 1);
}

}  // namespace brave_wallet
