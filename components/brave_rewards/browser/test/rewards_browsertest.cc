/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>
#include <string>

#include "base/containers/flat_map.h"
#include "base/test/bind.h"
#include "bat/ledger/internal/uphold/uphold_util.h"
#include "brave/browser/brave_rewards/rewards_service_factory.h"
#include "brave/common/brave_paths.h"
#include "brave/components/brave_rewards/browser/rewards_service_impl.h"
#include "brave/components/brave_rewards/browser/test/common/rewards_browsertest_context_helper.h"
#include "brave/components/brave_rewards/browser/test/common/rewards_browsertest_context_util.h"
#include "brave/components/brave_rewards/browser/test/common/rewards_browsertest_contribution.h"
#include "brave/components/brave_rewards/browser/test/common/rewards_browsertest_network_util.h"
#include "brave/components/brave_rewards/browser/test/common/rewards_browsertest_promotion.h"
#include "brave/components/brave_rewards/browser/test/common/rewards_browsertest_response.h"
#include "brave/components/brave_rewards/browser/test/common/rewards_browsertest_util.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/testing_profile.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/network_session_configurator/common/network_switches.h"
#include "content/public/test/browser_test.h"
#include "net/dns/mock_host_resolver.h"

// npm run test -- brave_browser_tests --filter=RewardsBrowserTest.*

namespace rewards_browsertest {

class RewardsBrowserTest : public InProcessBrowserTest {
 public:
  RewardsBrowserTest() {
    response_ = std::make_unique<RewardsBrowserTestResponse>();
    contribution_ = std::make_unique<RewardsBrowserTestContribution>();
    promotion_ = std::make_unique<RewardsBrowserTestPromotion>();
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

    // Response mock
    base::ScopedAllowBlockingForTesting allow_blocking;
    response_->LoadMocks();
    rewards_service_->ForTestingSetTestResponseCallback(
        base::BindRepeating(
            &RewardsBrowserTest::GetTestResponse,
            base::Unretained(this)));
    rewards_service_->SetLedgerEnvForTesting();

    // Other
    contribution_->Initialize(browser(), rewards_service_);
    promotion_->Initialize(browser(), rewards_service_);

    rewards_browsertest_util::SetOnboardingBypassed(browser());
  }

  void TearDown() override {
    InProcessBrowserTest::TearDown();
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    // HTTPS server only serves a valid cert for localhost, so this is needed
    // to load pages from other hosts without an error
    command_line->AppendSwitch(switches::kIgnoreCertificateErrors);
  }

  void GetTestResponse(
      const std::string& url,
      int32_t method,
      int* response_status_code,
      std::string* response,
      base::flat_map<std::string, std::string>* headers) {
    response_->SetExternalBalance(contribution_->GetExternalBalance());
    response_->Get(
        url,
        method,
        response_status_code,
        response);
  }

  content::WebContents* contents() const {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  GURL uphold_auth_url() {
    GURL url("chrome://rewards/uphold/authorization?"
             "code=0c42b34121f624593ee3b04cbe4cc6ddcd72d&state=123456789");
    return url;
  }

  brave_rewards::RewardsServiceImpl* rewards_service_;
  std::unique_ptr<net::EmbeddedTestServer> https_server_;
  std::unique_ptr<RewardsBrowserTestResponse> response_;
  std::unique_ptr<RewardsBrowserTestContribution> contribution_;
  std::unique_ptr<RewardsBrowserTestPromotion> promotion_;
  std::unique_ptr<RewardsBrowserTestContextHelper> context_helper_;
};

// https://github.com/brave/brave-browser/issues/12632
IN_PROC_BROWSER_TEST_F(RewardsBrowserTest, DISABLED_ActivateSettingsModal) {
  context_helper_->LoadURL(rewards_browsertest_util::GetRewardsUrl());

  rewards_browsertest_util::WaitForElementThenClick(
      contents(),
      "[data-test-id='settingsButton']");
  rewards_browsertest_util::WaitForElementToAppear(
      contents(),
      "#modal");
}

// https://github.com/brave/brave-browser/issues/12988
IN_PROC_BROWSER_TEST_F(RewardsBrowserTest, DISABLED_ToggleAutoContribute) {
  context_helper_->LoadURL(rewards_browsertest_util::GetRewardsUrl());

  // toggle auto contribute back on
  rewards_browsertest_util::WaitForElementThenClick(
      contents(),
      "[data-test-id2='autoContribution']");
  std::string value = rewards_browsertest_util::WaitForElementThenGetAttribute(
      contents(),
      "[data-test-id2='autoContribution']",
      "data-toggled");
  ASSERT_STREQ(value.c_str(), "true");

  // toggle auto contribute off
  rewards_browsertest_util::WaitForElementThenClick(
      contents(),
      "[data-test-id2='autoContribution']");
  value =
      rewards_browsertest_util::WaitForElementThenGetAttribute(
        contents(),
        "[data-test-id2='autoContribution']",
        "data-toggled");
  ASSERT_STREQ(value.c_str(), "false");
}

IN_PROC_BROWSER_TEST_F(RewardsBrowserTest, SiteBannerDefaultTipChoices) {
  rewards_browsertest_util::StartProcess(rewards_service_);
  rewards_browsertest_util::NavigateToPublisherPage(
      browser(),
      https_server_.get(),
      "3zsistemi.si");

  content::WebContents* site_banner =
      context_helper_->OpenSiteBanner(
          rewards_browsertest_util::TipAction::OneTime);
  auto tip_options = rewards_browsertest_util::GetSiteBannerTipOptions(
      site_banner);
  ASSERT_EQ(tip_options, std::vector<double>({ 1, 5, 50 }));

  site_banner = context_helper_->OpenSiteBanner(
      rewards_browsertest_util::TipAction::SetMonthly);
  tip_options = rewards_browsertest_util::GetSiteBannerTipOptions(
      site_banner);
  ASSERT_EQ(tip_options, std::vector<double>({ 1, 10, 100 }));
}

IN_PROC_BROWSER_TEST_F(
    RewardsBrowserTest,
    SiteBannerDefaultPublisherAmounts) {
  rewards_browsertest_util::NavigateToPublisherPage(
      browser(),
      https_server_.get(),
      "laurenwags.github.io");

  content::WebContents* site_banner =
      context_helper_->OpenSiteBanner(
          rewards_browsertest_util::TipAction::OneTime);
  const auto tip_options = rewards_browsertest_util::GetSiteBannerTipOptions(
      site_banner);
  ASSERT_EQ(tip_options, std::vector<double>({ 5, 10, 20 }));
}

// Disabled in https://github.com/brave/brave-browser/issues/10789
IN_PROC_BROWSER_TEST_F(RewardsBrowserTest, DISABLED_NotVerifiedWallet) {
  contribution_->AddBalance(promotion_->ClaimPromotionViaCode());
  contribution_->IsBalanceCorrect();

  // Click on verify button
  rewards_browsertest_util::WaitForElementThenClick(
      contents(),
      "#verify-wallet-button");

  // Click on verify button in on boarding
  rewards_browsertest_util::WaitForElementThenClick(
      contents(),
      "#on-boarding-verify-button");

  // Check if we are redirected to uphold
  {
    const GURL current_url = contents()->GetURL();
    ASSERT_TRUE(base::StartsWith(
        current_url.spec(),
        ledger::uphold::GetUrl() + "/authorize/",
        base::CompareCase::INSENSITIVE_ASCII));
  }

  // Fake successful authentication
  ui_test_utils::NavigateToURLBlockUntilNavigationsComplete(
        browser(),
        uphold_auth_url(), 1);

  // Check if we are redirected to KYC page
  {
    const GURL current_url = contents()->GetURL();
    ASSERT_TRUE(base::StartsWith(
        current_url.spec(),
        ledger::uphold::GetUrl() + "/signup/step2",
        base::CompareCase::INSENSITIVE_ASCII));
  }
}

IN_PROC_BROWSER_TEST_F(RewardsBrowserTest, ShowMonthlyIfACOff) {
  rewards_browsertest_util::NavigateToPublisherPage(
      browser(),
      https_server_.get(),
      "3zsistemi.si");

  // Open the Rewards popup
  content::WebContents* popup_contents = context_helper_->OpenRewardsPopup();
  ASSERT_TRUE(popup_contents);

  rewards_browsertest_util::WaitForElementToAppear(
      popup_contents,
      "#panel-donate-monthly");
}

IN_PROC_BROWSER_TEST_F(RewardsBrowserTest, ShowACPercentInThePanel) {
  rewards_browsertest_util::StartProcess(rewards_service_);
  rewards_service_->SetAutoContributeEnabled(true);
  context_helper_->LoadURL(rewards_browsertest_util::GetRewardsUrl());
  context_helper_->VisitPublisher(
      rewards_browsertest_util::GetUrl(https_server_.get(), "3zsistemi.si"),
      true);

  rewards_browsertest_util::NavigateToPublisherPage(
      browser(),
      https_server_.get(),
      "3zsistemi.si");

  // Open the Rewards popup
  content::WebContents* popup_contents = context_helper_->OpenRewardsPopup();
  ASSERT_TRUE(popup_contents);

  const std::string score =
      rewards_browsertest_util::WaitForElementThenGetContent(
          popup_contents,
          "[data-test-id='attention-score']");
  EXPECT_NE(score.find("100%"), std::string::npos);
}

IN_PROC_BROWSER_TEST_F(RewardsBrowserTest, ZeroBalanceWalletClaimNotCalled) {
  response_->SetVerifiedWallet(true);
  rewards_browsertest_util::StartProcess(rewards_service_);
  contribution_->SetUpUpholdWallet(rewards_service_, 50.0);

  response_->ClearRequests();

  base::RunLoop run_loop;
  auto test_callback =
      [&](
          const ledger::type::Result result,
          ledger::type::UpholdWalletPtr wallet) {
        auto requests = response_->GetRequests();
        EXPECT_EQ(result, ledger::type::Result::LEDGER_OK);
        EXPECT_FALSE(requests.empty());

        // Should not attempt to call /v2/wallet/UUID/claim endpoint
        // since by default the wallet should contain 0 `user_funds`
        auto wallet_claim_call = std::find_if(
            requests.begin(), requests.end(),
            [](const Request& req) {
              return req.url.find("/v2/wallet") != std::string::npos &&
                     req.url.find("/claim") != std::string::npos;
            });

        EXPECT_TRUE(wallet_claim_call == requests.end());
        run_loop.Quit();
      };

  rewards_service_->GetUpholdWallet(base::BindLambdaForTesting(test_callback));
  run_loop.Run();
}

// https://github.com/brave/brave-browser/issues/12987
IN_PROC_BROWSER_TEST_F(RewardsBrowserTest,
                       DISABLED_BackupRestoreModalHasNotice) {
  context_helper_->LoadURL(rewards_browsertest_util::GetRewardsUrl());
  rewards_browsertest_util::CreateWallet(rewards_service_);
  contribution_->AddBalance(promotion_->ClaimPromotionViaCode());

  rewards_browsertest_util::WaitForElementToEqual(
      contents(),
      "[data-test-id='balance']",
      "30.000 BAT");

  // Click the settings button and wait for the backup modal to appear
  rewards_browsertest_util::WaitForElementThenClick(
      contents(),
      "[data-test-id='settingsButton']");
  rewards_browsertest_util::WaitForElementToAppear(
      contents(),
      "#modal");

  // Ensure that verify link exists
  rewards_browsertest_util::WaitForElementToAppear(
      contents(),
      "#backup-verify-link");
}

IN_PROC_BROWSER_TEST_F(RewardsBrowserTest, BackupRestoreModalHasNoNotice) {
  response_->SetUserFundsBalance(true);
  rewards_browsertest_util::StartProcess(rewards_service_);
  rewards_browsertest_util::CreateWallet(rewards_service_);
  context_helper_->LoadURL(rewards_browsertest_util::GetRewardsUrl());

  rewards_browsertest_util::WaitForElementToEqual(
      contents(),
      "[data-test-id='balance']",
      "20.000 BAT");

  // Click the settings button and wait for the backup modal to appear
  rewards_browsertest_util::WaitForElementThenClick(
      contents(),
      "[data-test-id='settingsButton']");
  rewards_browsertest_util::WaitForElementToAppear(
      contents(),
      "#modal");

  // Presence of recovery key textarea indicates notice isn't
  // displayed
  rewards_browsertest_util::WaitForElementToAppear(
      contents(),
      "#backup-recovery-key");
}

IN_PROC_BROWSER_TEST_F(RewardsBrowserTest, ResetRewards) {
  context_helper_->LoadURL(rewards_browsertest_util::GetRewardsUrl());

  rewards_browsertest_util::WaitForElementThenClick(
      contents(),
      "[data-test-id='settingsButton']");

  rewards_browsertest_util::WaitForElementToAppear(
      contents(),
      "#modal");

  rewards_browsertest_util::WaitForElementThenClick(
      contents(),
      "[data-test-id='settings-modal-tabs-2']");

  rewards_browsertest_util::WaitForElementToContain(
      contents(),
      "[data-test-id='reset-text']",
      "Your Rewards data will");
}

// https://github.com/brave/brave-browser/issues/12607
IN_PROC_BROWSER_TEST_F(RewardsBrowserTest, DISABLED_ResetRewardsWithBAT) {
  context_helper_->LoadURL(rewards_browsertest_util::GetRewardsUrl());
  rewards_browsertest_util::CreateWallet(rewards_service_);
  contribution_->AddBalance(promotion_->ClaimPromotionViaCode());

  rewards_browsertest_util::WaitForElementThenClick(
      contents(),
      "[data-test-id='settingsButton']");

  rewards_browsertest_util::WaitForElementToAppear(
      contents(),
      "#modal");

  rewards_browsertest_util::WaitForElementThenClick(
      contents(),
      "[data-test-id='settings-modal-tabs-2']");

  rewards_browsertest_util::WaitForElementToContain(
      contents(),
      "[data-test-id='reset-text']",
      "Your 30 BATs and other Rewards");
}

// https://github.com/brave/brave-browser/issues/12704
IN_PROC_BROWSER_TEST_F(RewardsBrowserTest, DISABLED_UpholdLimitNoBAT) {
  context_helper_->LoadURL(rewards_browsertest_util::GetRewardsUrl());
  rewards_browsertest_util::WaitForElementThenClick(
      contents(),
      "#verify-wallet-button");

  rewards_browsertest_util::WaitForElementThenClick(
      contents(),
      "#cancel-login-button");

  rewards_browsertest_util::WaitForElementThenClick(
      contents(),
      "#verify-wallet-button");

  rewards_browsertest_util::WaitForElementThenClick(
      contents(),
      "#login-button");

  // Check if we are redirected to uphold
  {
    const GURL current_url = contents()->GetURL();

    auto found = current_url.spec().find("intention=login");
    ASSERT_TRUE(found != std::string::npos);
  }
}

}  // namespace rewards_browsertest
