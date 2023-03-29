/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/brave_wallet_p3a.h"
#include "base/test/metrics/histogram_tester.h"
#include "brave/browser/brave_wallet/brave_wallet_service_factory.h"
#include "brave/browser/brave_wallet/keyring_service_factory.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/features.h"
#include "chrome/test/base/scoped_testing_local_state.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile_manager.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

class BraveWalletP3AUnitTest : public testing::Test {
 public:
  BraveWalletP3AUnitTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {
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
    wallet_p3a_ = wallet_service_->GetBraveWalletP3A();
  }
  void WaitForResponse() { task_environment_.RunUntilIdle(); }

  content::BrowserTaskEnvironment task_environment_;
  std::unique_ptr<ScopedTestingLocalState> local_state_;
  std::unique_ptr<TestingProfile> profile_;
  std::unique_ptr<base::HistogramTester> histogram_tester_;
  KeyringService* keyring_service_;
  BraveWalletService* wallet_service_;
  BraveWalletP3A* wallet_p3a_;
};

TEST_F(BraveWalletP3AUnitTest, DefaultEthereumWalletSetting) {
  histogram_tester_->ExpectBucketCount(
      kDefaultWalletHistogramName,
      static_cast<int>(mojom::DefaultWallet::BraveWalletPreferExtension), 1);
  histogram_tester_->ExpectBucketCount(
      kDefaultWalletHistogramName, static_cast<int>(mojom::DefaultWallet::None),
      0);
  wallet_service_->SetDefaultEthereumWallet(mojom::DefaultWallet::None);
  WaitForResponse();
  histogram_tester_->ExpectBucketCount(
      kDefaultWalletHistogramName, static_cast<int>(mojom::DefaultWallet::None),
      1);
  histogram_tester_->ExpectBucketCount(
      kDefaultWalletHistogramName,
      static_cast<int>(mojom::DefaultWallet::CryptoWallets), 0);
  wallet_service_->SetDefaultEthereumWallet(
      mojom::DefaultWallet::CryptoWallets);
  WaitForResponse();
  histogram_tester_->ExpectBucketCount(
      kDefaultWalletHistogramName,
      static_cast<int>(mojom::DefaultWallet::CryptoWallets), 1);
  histogram_tester_->ExpectBucketCount(
      kDefaultWalletHistogramName,
      static_cast<int>(mojom::DefaultWallet::BraveWallet), 0);
  wallet_service_->SetDefaultEthereumWallet(mojom::DefaultWallet::BraveWallet);
  WaitForResponse();
  histogram_tester_->ExpectBucketCount(
      kDefaultWalletHistogramName,
      static_cast<int>(mojom::DefaultWallet::BraveWallet), 1);
  wallet_service_->SetDefaultEthereumWallet(
      mojom::DefaultWallet::BraveWalletPreferExtension);
  WaitForResponse();
  histogram_tester_->ExpectBucketCount(
      kDefaultWalletHistogramName,
      static_cast<int>(mojom::DefaultWallet::BraveWalletPreferExtension), 2);
}

TEST_F(BraveWalletP3AUnitTest, DefaultSolanaWalletSetting) {
  histogram_tester_->ExpectBucketCount(
      kDefaultSolanaWalletHistogramName,
      static_cast<int>(mojom::DefaultWallet::BraveWalletPreferExtension), 1);
  histogram_tester_->ExpectBucketCount(
      kDefaultSolanaWalletHistogramName,
      static_cast<int>(mojom::DefaultWallet::None), 0);
  wallet_service_->SetDefaultSolanaWallet(mojom::DefaultWallet::None);
  WaitForResponse();
  histogram_tester_->ExpectBucketCount(
      kDefaultSolanaWalletHistogramName,
      static_cast<int>(mojom::DefaultWallet::None), 1);
  histogram_tester_->ExpectBucketCount(
      kDefaultSolanaWalletHistogramName,
      static_cast<int>(mojom::DefaultWallet::CryptoWallets), 0);
  wallet_service_->SetDefaultSolanaWallet(mojom::DefaultWallet::BraveWallet);
  WaitForResponse();
  histogram_tester_->ExpectBucketCount(
      kDefaultSolanaWalletHistogramName,
      static_cast<int>(mojom::DefaultWallet::BraveWallet), 1);
  wallet_service_->SetDefaultSolanaWallet(
      mojom::DefaultWallet::BraveWalletPreferExtension);
  WaitForResponse();
  histogram_tester_->ExpectBucketCount(
      kDefaultSolanaWalletHistogramName,
      static_cast<int>(mojom::DefaultWallet::BraveWalletPreferExtension), 2);
}

TEST_F(BraveWalletP3AUnitTest, KeyringCreated) {
  base::test::ScopedFeatureList feature_list;
  base::FieldTrialParams parameters;
  parameters[features::kCreateDefaultSolanaAccount.name] = "false";

  std::vector<base::test::FeatureRefAndParams> enabled_features;
  enabled_features.emplace_back(
      brave_wallet::features::kBraveWalletSolanaFeature, parameters);

  feature_list.InitWithFeaturesAndParameters(enabled_features, {});

  histogram_tester_->ExpectBucketCount(kKeyringCreatedHistogramName, 0, 1);
  keyring_service_->CreateWallet("testing123", base::DoNothing());
  WaitForResponse();
  histogram_tester_->ExpectBucketCount(kKeyringCreatedHistogramName, 1, 1);
}

TEST_F(BraveWalletP3AUnitTest, ReportOnboardingAction) {
  histogram_tester_->ExpectTotalCount(kOnboardingConversionHistogramName, 0);

  wallet_p3a_->ReportOnboardingAction(mojom::OnboardingAction::Shown);
  histogram_tester_->ExpectUniqueSample(kOnboardingConversionHistogramName, 0,
                                        1);

  wallet_p3a_->ReportOnboardingAction(mojom::OnboardingAction::CreatedWallet);
  histogram_tester_->ExpectBucketCount(kOnboardingConversionHistogramName, 1,
                                       1);

  wallet_p3a_->ReportOnboardingAction(mojom::OnboardingAction::RestoredWallet);
  histogram_tester_->ExpectBucketCount(kOnboardingConversionHistogramName, 2,
                                       1);
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

  // Should not record zero to histogram if user never had an active account
  histogram_tester_->ExpectTotalCount(kEthActiveAccountHistogramName, 0);
  histogram_tester_->ExpectTotalCount(kFilActiveAccountHistogramName, 0);
  histogram_tester_->ExpectTotalCount(kSolActiveAccountHistogramName, 0);

  wallet_p3a_->RecordActiveWalletCount(3, mojom::CoinType::ETH);
  wallet_p3a_->RecordActiveWalletCount(9, mojom::CoinType::FIL);
  wallet_p3a_->RecordActiveWalletCount(7, mojom::CoinType::SOL);

  histogram_tester_->ExpectBucketCount(kEthActiveAccountHistogramName, 3, 1);
  histogram_tester_->ExpectBucketCount(kFilActiveAccountHistogramName, 5, 1);
  histogram_tester_->ExpectBucketCount(kSolActiveAccountHistogramName, 4, 1);

  wallet_p3a_->RecordActiveWalletCount(0, mojom::CoinType::ETH);
  wallet_p3a_->RecordActiveWalletCount(1, mojom::CoinType::FIL);
  wallet_p3a_->RecordActiveWalletCount(2, mojom::CoinType::SOL);

  histogram_tester_->ExpectBucketCount(kEthActiveAccountHistogramName, 0, 1);
  histogram_tester_->ExpectBucketCount(kFilActiveAccountHistogramName, 1, 1);
  histogram_tester_->ExpectBucketCount(kSolActiveAccountHistogramName, 2, 1);
}

TEST_F(BraveWalletP3AUnitTest, JSProviders) {
  auto test_func = [&](mojom::CoinType coin_type, const char* histogram_name) {
    histogram_tester_->ExpectTotalCount(histogram_name, 0);

    wallet_p3a_->ReportJSProvider(mojom::JSProviderType::Native, coin_type,
                                  /*use_native_wallet_enabled*/ true,
                                  /*allow_provider_override*/ true);
    histogram_tester_->ExpectUniqueSample(histogram_name, 0, 1);

    wallet_p3a_->ReportJSProvider(mojom::JSProviderType::None, coin_type,
                                  /*use_native_wallet_enabled*/ false,
                                  /*allow_provider_override*/ true);
    histogram_tester_->ExpectBucketCount(histogram_name, 1, 1);

    keyring_service_->CreateWallet("testing123", base::DoNothing());
    WaitForResponse();

    wallet_p3a_->ReportJSProvider(mojom::JSProviderType::Native, coin_type,
                                  /*use_native_wallet_enabled*/ true,
                                  /*allow_provider_override*/ true);
    histogram_tester_->ExpectBucketCount(histogram_name, 2, 1);

    wallet_p3a_->ReportJSProvider(mojom::JSProviderType::Native, coin_type,
                                  /*use_native_wallet_enabled*/ true,
                                  /*allow_provider_override*/ false);
    histogram_tester_->ExpectBucketCount(histogram_name, 3, 1);

    wallet_p3a_->ReportJSProvider(mojom::JSProviderType::ThirdParty, coin_type,
                                  /*use_native_wallet_enabled*/ false,
                                  /*allow_provider_override*/ true);
    histogram_tester_->ExpectBucketCount(histogram_name, 4, 1);

    wallet_p3a_->ReportJSProvider(mojom::JSProviderType::ThirdParty, coin_type,
                                  /*use_native_wallet_enabled*/ true,
                                  /*allow_provider_override*/ true);
    histogram_tester_->ExpectBucketCount(histogram_name, 5, 1);

    keyring_service_->Reset();
    wallet_p3a_->ReportJSProvider(mojom::JSProviderType::Native, coin_type,
                                  /*use_native_wallet_enabled*/ true,
                                  /*allow_provider_override*/ true);
    histogram_tester_->ExpectBucketCount(histogram_name, 0, 2);
  };
  test_func(mojom::CoinType::ETH, kEthProviderHistogramName);
  test_func(mojom::CoinType::SOL, kSolProviderHistogramName);
}

}  // namespace brave_wallet
