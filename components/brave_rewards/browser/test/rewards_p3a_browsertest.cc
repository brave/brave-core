/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>
#include <string>

#include "base/containers/flat_map.h"
#include "base/test/bind.h"
#include "base/test/metrics/histogram_tester.h"
#include "brave/browser/brave_rewards/rewards_service_factory.h"
#include "brave/common/brave_paths.h"
#include "brave/components/brave_rewards/browser/rewards_service_impl.h"
#include "brave/components/brave_rewards/browser/rewards_service_observer.h"
#include "brave/components/brave_rewards/browser/test/common/rewards_browsertest_context_util.h"
#include "brave/components/brave_rewards/browser/test/common/rewards_browsertest_contribution.h"
#include "brave/components/brave_rewards/browser/test/common/rewards_browsertest_network_util.h"
#include "brave/components/brave_rewards/browser/test/common/rewards_browsertest_promotion.h"
#include "brave/components/brave_rewards/browser/test/common/rewards_browsertest_response.h"
#include "brave/components/brave_rewards/browser/test/common/rewards_browsertest_util.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "components/network_session_configurator/common/network_switches.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "net/dns/mock_host_resolver.h"

// npm run test -- brave_browser_tests --filter=RewardsP3ABrowserTest.*

namespace rewards_browsertest {

class RewardsP3ABrowserTest : public InProcessBrowserTest,
                              public brave_rewards::RewardsServiceObserver {
 public:
  RewardsP3ABrowserTest() {
    contribution_ = std::make_unique<RewardsBrowserTestContribution>();
    promotion_ = std::make_unique<RewardsBrowserTestPromotion>();
    response_ = std::make_unique<RewardsBrowserTestResponse>();
    histogram_tester_.reset(new base::HistogramTester);
  }

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();

    context_helper_ =
        std::make_unique<RewardsBrowserTestContextHelper>(browser());

    // HTTP resolver
    host_resolver()->AddRule("*", "127.0.0.1");
    https_server_.reset(new net::EmbeddedTestServer(
        net::test_server::EmbeddedTestServer::TYPE_HTTPS));
    https_server_->SetSSLConfig(net::EmbeddedTestServer::CERT_OK);
    https_server_->RegisterRequestHandler(
        base::BindRepeating(&rewards_browsertest_util::HandleRequest));
    ASSERT_TRUE(https_server_->Start());

    // Rewards service
    brave::RegisterPathProvider();
    auto* profile = browser()->profile();
    rewards_service_ = static_cast<brave_rewards::RewardsServiceImpl*>(
        brave_rewards::RewardsServiceFactory::GetForProfile(profile));
    rewards_service_->AddObserver(this);

    // Response mock
    base::ScopedAllowBlockingForTesting allow_blocking;
    response_->LoadMocks();
    rewards_service_->ForTestingSetTestResponseCallback(base::BindRepeating(
        &RewardsP3ABrowserTest::GetTestResponse, base::Unretained(this)));
    rewards_service_->SetLedgerEnvForTesting();

    // Other
    promotion_->Initialize(browser(), rewards_service_);
    contribution_->Initialize(browser(), rewards_service_);

    rewards_browsertest_util::SetOnboardingBypassed(browser());
  }

  void TearDown() override { InProcessBrowserTest::TearDown(); }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    // HTTPS server only serves a valid cert for localhost, so this is needed
    // to load pages from other hosts without an error
    command_line->AppendSwitch(switches::kIgnoreCertificateErrors);
  }

  void GetTestResponse(const std::string& url,
                       int32_t method,
                       int* response_status_code,
                       std::string* response,
                       base::flat_map<std::string, std::string>* headers) {
    response_->SetExternalBalance(contribution_->GetExternalBalance());
    response_->Get(url, method, response_status_code, response);
  }

  void FetchBalance() {
    base::RunLoop run_loop;
    rewards_service_->FetchBalance(base::BindLambdaForTesting(
        [&](ledger::type::Result result, ledger::type::BalancePtr balance) {
          run_loop.Quit();
        }));
    run_loop.Run();
  }

  void WaitForRewardsInitialization() {
    if (rewards_initialized_) {
      return;
    }

    wait_for_rewards_initialization_loop_.reset(new base::RunLoop);
    wait_for_rewards_initialization_loop_->Run();
  }

  content::WebContents* contents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  void OnRewardsInitialized(
      brave_rewards::RewardsService* rewards_service) override {
    rewards_initialized_ = true;
  }

  brave_rewards::RewardsServiceImpl* rewards_service_;
  std::unique_ptr<net::EmbeddedTestServer> https_server_;
  std::unique_ptr<RewardsBrowserTestContribution> contribution_;
  std::unique_ptr<RewardsBrowserTestPromotion> promotion_;
  std::unique_ptr<RewardsBrowserTestResponse> response_;
  std::unique_ptr<RewardsBrowserTestContextHelper> context_helper_;
  std::unique_ptr<base::HistogramTester> histogram_tester_;

  bool rewards_initialized_ = false;
  std::unique_ptr<base::RunLoop> wait_for_rewards_initialization_loop_;
};

IN_PROC_BROWSER_TEST_F(RewardsP3ABrowserTest, RewardsDisabled) {
  rewards_browsertest_util::StartProcess(rewards_service_);

  WaitForRewardsInitialization();

  histogram_tester_->ExpectBucketCount("Brave.Rewards.WalletBalance.2", 1, 1);
  histogram_tester_->ExpectBucketCount("Brave.Rewards.AutoContributionsState.2",
                                       1, 1);
  histogram_tester_->ExpectBucketCount("Brave.Rewards.TipsState.2", 1, 1);
}

IN_PROC_BROWSER_TEST_F(RewardsP3ABrowserTest,
                       WalletStateWalletCreatedNoGrantsClaimedNoFundsAdded) {
  rewards_browsertest_util::StartProcess(rewards_service_);
  rewards_browsertest_util::CreateWallet(rewards_service_);

  rewards_service_->SetAutoContributeEnabled(true);
  rewards_service_->SetAdsEnabled(true);

  FetchBalance();

  histogram_tester_->ExpectBucketCount("Brave.Rewards.WalletState", 1, 1);
}

IN_PROC_BROWSER_TEST_F(RewardsP3ABrowserTest,
                       WalletStateWalletCreatedGrantsClaimedNoFundsAdded) {
  rewards_browsertest_util::StartProcess(rewards_service_);
  rewards_browsertest_util::CreateWallet(rewards_service_);

  context_helper_->LoadURL(rewards_browsertest_util::GetRewardsUrl());

  rewards_service_->SetAutoContributeEnabled(true);
  rewards_service_->SetAdsEnabled(true);

  contribution_->AddBalance(promotion_->ClaimPromotionViaCode());

  FetchBalance();

  histogram_tester_->ExpectBucketCount("Brave.Rewards.WalletState", 2, 1);
}

IN_PROC_BROWSER_TEST_F(RewardsP3ABrowserTest,
                       WalletStateWalletCreatedNoGrantsClaimedFundsAdded) {
  response_->SetUserFundsBalance(20.0);

  rewards_browsertest_util::StartProcess(rewards_service_);
  rewards_browsertest_util::CreateWallet(rewards_service_);

  context_helper_->LoadURL(rewards_browsertest_util::GetRewardsUrl());

  rewards_service_->SetAutoContributeEnabled(true);
  rewards_service_->SetAdsEnabled(true);

  FetchBalance();

  EXPECT_GT(histogram_tester_->GetBucketCount("Brave.Rewards.WalletState", 3),
            0);
}

IN_PROC_BROWSER_TEST_F(RewardsP3ABrowserTest,
                       WalletStateWalletCreatedGrantsClaimedFundsAdded) {
  response_->SetUserFundsBalance(20.0);

  rewards_browsertest_util::StartProcess(rewards_service_);
  rewards_browsertest_util::CreateWallet(rewards_service_);

  context_helper_->LoadURL(rewards_browsertest_util::GetRewardsUrl());

  rewards_service_->SetAutoContributeEnabled(true);
  rewards_service_->SetAdsEnabled(true);

  contribution_->AddBalance(promotion_->ClaimPromotionViaCode());

  FetchBalance();

  EXPECT_GT(histogram_tester_->GetBucketCount("Brave.Rewards.WalletState", 4),
            0);
}

IN_PROC_BROWSER_TEST_F(RewardsP3ABrowserTest,
                       WalletStateWalletDisabledAfterCreation) {
  rewards_browsertest_util::StartProcess(rewards_service_);
  rewards_browsertest_util::CreateWallet(rewards_service_);

  rewards_service_->SetAdsEnabled(false);
  rewards_service_->SetAutoContributeEnabled(false);

  histogram_tester_->ExpectBucketCount("Brave.Rewards.WalletState", 5, 1);
}

IN_PROC_BROWSER_TEST_F(RewardsP3ABrowserTest, WalletBalanceLessThan10BAT) {
  response_->SetUserFundsBalance(9.0);

  rewards_browsertest_util::StartProcess(rewards_service_);
  rewards_browsertest_util::CreateWallet(rewards_service_);

  context_helper_->LoadURL(rewards_browsertest_util::GetRewardsUrl());

  rewards_service_->SetAutoContributeEnabled(true);
  rewards_service_->SetAdsEnabled(true);

  FetchBalance();

  EXPECT_GT(
      histogram_tester_->GetBucketCount("Brave.Rewards.WalletBalance.2", 2), 0);
}

IN_PROC_BROWSER_TEST_F(RewardsP3ABrowserTest, WalletBalanceLessThan50BAT) {
  response_->SetUserFundsBalance(20.0);

  rewards_browsertest_util::StartProcess(rewards_service_);
  rewards_browsertest_util::CreateWallet(rewards_service_);

  context_helper_->LoadURL(rewards_browsertest_util::GetRewardsUrl());

  rewards_service_->SetAutoContributeEnabled(true);
  rewards_service_->SetAdsEnabled(true);

  FetchBalance();

  EXPECT_GT(
      histogram_tester_->GetBucketCount("Brave.Rewards.WalletBalance.2", 3), 0);
}

IN_PROC_BROWSER_TEST_F(RewardsP3ABrowserTest, WalletBalanceMoreThan50BAT) {
  response_->SetUserFundsBalance(60.0);

  rewards_browsertest_util::StartProcess(rewards_service_);
  rewards_browsertest_util::CreateWallet(rewards_service_);

  context_helper_->LoadURL(rewards_browsertest_util::GetRewardsUrl());

  rewards_service_->SetAutoContributeEnabled(true);
  rewards_service_->SetAdsEnabled(true);

  FetchBalance();

  EXPECT_GT(
      histogram_tester_->GetBucketCount("Brave.Rewards.WalletBalance.2", 4), 0);
}

}  // namespace rewards_browsertest
