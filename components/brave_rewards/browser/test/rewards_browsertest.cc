/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>
#include <string>

#include "base/containers/flat_map.h"
#include "base/memory/raw_ptr.h"
#include "base/test/bind.h"
#include "base/test/scoped_feature_list.h"
#include "bat/ledger/global_constants.h"
#include "bat/ledger/internal/uphold/uphold_util.h"
#include "brave/browser/brave_rewards/rewards_service_factory.h"
#include "brave/components/brave_rewards/browser/rewards_service_impl.h"
#include "brave/components/brave_rewards/browser/test/common/rewards_browsertest_context_helper.h"
#include "brave/components/brave_rewards/browser/test/common/rewards_browsertest_context_util.h"
#include "brave/components/brave_rewards/browser/test/common/rewards_browsertest_contribution.h"
#include "brave/components/brave_rewards/browser/test/common/rewards_browsertest_network_util.h"
#include "brave/components/brave_rewards/browser/test/common/rewards_browsertest_promotion.h"
#include "brave/components/brave_rewards/browser/test/common/rewards_browsertest_response.h"
#include "brave/components/brave_rewards/browser/test/common/rewards_browsertest_util.h"
#include "brave/components/brave_rewards/common/features.h"
#include "brave/components/brave_rewards/common/pref_names.h"
#include "brave/components/constants/brave_paths.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/testing_profile.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/network_session_configurator/common/network_switches.h"
#include "content/public/browser/notification_types.h"
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
    feature_list_.InitAndEnableFeature(brave_rewards::features::kGeminiFeature);
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

  double FetchBalance() {
    double total = -1.0;
    base::RunLoop run_loop;
    rewards_service_->FetchBalance(base::BindLambdaForTesting(
        [&](ledger::type::Result result, ledger::type::BalancePtr balance) {
          total = balance ? balance->total : -1.0;
          run_loop.Quit();
        }));
    run_loop.Run();
    return total;
  }

  void WaitForNavigation(const std::string& url_substring) {
    content::WindowedNotificationObserver window_observer(
        content::NOTIFICATION_LOAD_COMPLETED_MAIN_FRAME,
        base::BindLambdaForTesting(
            [&](const content::NotificationSource& source,
                const content::NotificationDetails&) {
              auto contents_source =
                  static_cast<const content::Source<content::WebContents>&>(
                      source);
              std::string url = contents_source->GetLastCommittedURL().spec();
              return url.find(url_substring) != std::string::npos;
            }));

    window_observer.Wait();
  }

  base::test::ScopedFeatureList feature_list_;
  raw_ptr<brave_rewards::RewardsServiceImpl> rewards_service_ = nullptr;
  std::unique_ptr<net::EmbeddedTestServer> https_server_;
  std::unique_ptr<RewardsBrowserTestResponse> response_;
  std::unique_ptr<RewardsBrowserTestContribution> contribution_;
  std::unique_ptr<RewardsBrowserTestPromotion> promotion_;
  std::unique_ptr<RewardsBrowserTestContextHelper> context_helper_;
};

IN_PROC_BROWSER_TEST_F(RewardsBrowserTest, ActivateSettingsModal) {
  context_helper_->LoadRewardsPage();

  rewards_browsertest_util::WaitForElementThenClick(
      contents(), "[data-test-id=manage-wallet-button]");
  rewards_browsertest_util::WaitForElementToAppear(
      contents(),
      "#modal");
}

IN_PROC_BROWSER_TEST_F(RewardsBrowserTest, SiteBannerDefaultTipChoices) {
  rewards_browsertest_util::CreateWallet(rewards_service_);
  rewards_browsertest_util::NavigateToPublisherPage(
      browser(),
      https_server_.get(),
      "3zsistemi.si");

  base::WeakPtr<content::WebContents> site_banner =
      context_helper_->OpenSiteBanner(
          rewards_browsertest_util::TipAction::OneTime);
  auto tip_options =
      rewards_browsertest_util::GetSiteBannerTipOptions(site_banner.get());
  ASSERT_EQ(tip_options, std::vector<double>({ 1, 5, 50 }));

  site_banner = context_helper_->OpenSiteBanner(
      rewards_browsertest_util::TipAction::SetMonthly);
  tip_options =
      rewards_browsertest_util::GetSiteBannerTipOptions(site_banner.get());
  ASSERT_EQ(tip_options, std::vector<double>({ 1, 10, 100 }));
}

IN_PROC_BROWSER_TEST_F(RewardsBrowserTest, SiteBannerDefaultPublisherAmounts) {
  rewards_browsertest_util::CreateWallet(rewards_service_);
  rewards_browsertest_util::NavigateToPublisherPage(
      browser(),
      https_server_.get(),
      "laurenwags.github.io");

  base::WeakPtr<content::WebContents> site_banner =
      context_helper_->OpenSiteBanner(
          rewards_browsertest_util::TipAction::OneTime);
  const auto tip_options =
      rewards_browsertest_util::GetSiteBannerTipOptions(site_banner.get());
  ASSERT_EQ(tip_options, std::vector<double>({ 5, 10, 20 }));
}

IN_PROC_BROWSER_TEST_F(RewardsBrowserTest, NotVerifiedWallet) {
  rewards_browsertest_util::CreateWallet(rewards_service_);
  context_helper_->LoadRewardsPage();
  contribution_->AddBalance(promotion_->ClaimPromotionViaCode());
  contribution_->IsBalanceCorrect();

  rewards_browsertest_util::WaitForElementThenClick(
      contents(), "[data-test-id=verify-rewards-button]");

  rewards_browsertest_util::WaitForElementThenClick(
      contents(), "[data-test-id=connect-continue-button]");

  rewards_browsertest_util::WaitForElementThenClick(
      contents(), "[data-test-id=connect-provider-button]");

  // Check if we are redirected to uphold
  WaitForNavigation(ledger::uphold::GetUrl() + "/authorize/");

  response_->SetVerifiedWallet(true);

  // Fake successful authentication
  ui_test_utils::NavigateToURLBlockUntilNavigationsComplete(
        browser(),
        uphold_auth_url(), 1);

  rewards_browsertest_util::WaitForElementToContain(
      contents(), "[data-test-id=external-wallet-status-text]", "Verified");
}

IN_PROC_BROWSER_TEST_F(RewardsBrowserTest, ShowACPercentInThePanel) {
  rewards_browsertest_util::CreateWallet(rewards_service_);
  rewards_service_->SetAutoContributeEnabled(true);
  context_helper_->LoadRewardsPage();
  context_helper_->VisitPublisher(
      rewards_browsertest_util::GetUrl(https_server_.get(), "3zsistemi.si"),
      true);

  rewards_browsertest_util::NavigateToPublisherPage(
      browser(),
      https_server_.get(),
      "3zsistemi.si");

  // Open the Rewards popup
  base::WeakPtr<content::WebContents> popup_contents =
      context_helper_->OpenRewardsPopup();
  ASSERT_TRUE(popup_contents);

  const std::string score =
      rewards_browsertest_util::WaitForElementThenGetContent(
          popup_contents.get(), "[data-test-id=attention-score-text]");
  EXPECT_NE(score.find("100%"), std::string::npos);
}

IN_PROC_BROWSER_TEST_F(RewardsBrowserTest,
                       ZeroBalanceWalletClaimNotCalled_Uphold) {
  response_->SetVerifiedWallet(true);
  rewards_browsertest_util::CreateWallet(rewards_service_);
  contribution_->SetUpUpholdWallet(rewards_service_, 50.0);

  response_->ClearRequests();

  base::RunLoop run_loop;
  auto test_callback = [&](const ledger::type::Result result,
                           ledger::type::ExternalWalletPtr wallet) {
    auto requests = response_->GetRequests();
    EXPECT_EQ(result, ledger::type::Result::LEDGER_OK);
    EXPECT_FALSE(requests.empty());

    // Should not attempt to call /v2/wallet/UUID/claim endpoint
    // since by default the wallet should contain 0 `user_funds`
    auto wallet_claim_call =
        std::find_if(requests.begin(), requests.end(), [](const Request& req) {
          return req.url.find("/v2/wallet") != std::string::npos &&
                 req.url.find("/claim") != std::string::npos;
        });

    EXPECT_TRUE(wallet_claim_call == requests.end());
    run_loop.Quit();
  };

  rewards_service_->GetExternalWallet(
      base::BindLambdaForTesting(test_callback));
  run_loop.Run();
}

IN_PROC_BROWSER_TEST_F(RewardsBrowserTest,
                       ZeroBalanceWalletClaimNotCalled_Gemini) {
  response_->SetVerifiedWallet(true);
  rewards_browsertest_util::CreateWallet(rewards_service_);
  contribution_->SetUpGeminiWallet(rewards_service_, 50.0);

  response_->ClearRequests();

  base::RunLoop run_loop;
  auto test_callback = [&](const ledger::type::Result result,
                           ledger::type::ExternalWalletPtr wallet) {
    auto requests = response_->GetRequests();
    EXPECT_EQ(result, ledger::type::Result::LEDGER_OK);

    // Should not attempt to call /v2/wallet/UUID/claim endpoint
    // since by default the wallet should contain 0 `user_funds`
    auto wallet_claim_call =
        std::find_if(requests.begin(), requests.end(), [](const Request& req) {
          return req.url.find("/v2/wallet") != std::string::npos &&
                 req.url.find("/claim") != std::string::npos;
        });

    EXPECT_TRUE(wallet_claim_call == requests.end());
    run_loop.Quit();
  };

  rewards_service_->GetExternalWallet(
      base::BindLambdaForTesting(test_callback));
  run_loop.Run();
}

IN_PROC_BROWSER_TEST_F(RewardsBrowserTest, ResetRewards) {
  rewards_browsertest_util::CreateWallet(rewards_service_);
  context_helper_->LoadRewardsPage();

  rewards_browsertest_util::WaitForElementThenClick(
      contents(), "[data-test-id=manage-wallet-button]");

  rewards_browsertest_util::WaitForElementToAppear(contents(), "#modal");

  rewards_browsertest_util::WaitForElementThenClick(
      contents(),
      "[data-test-id='settings-modal-tabs-2']");

  rewards_browsertest_util::WaitForElementToContain(
      contents(),
      "[data-test-id='reset-text']",
      "Your Rewards data will");
}

IN_PROC_BROWSER_TEST_F(RewardsBrowserTest, ResetRewardsWithBAT) {
  rewards_browsertest_util::CreateWallet(rewards_service_);
  context_helper_->LoadRewardsPage();
  contribution_->AddBalance(promotion_->ClaimPromotionViaCode());

  rewards_browsertest_util::WaitForElementThenClick(
      contents(), "[data-test-id=manage-wallet-button]");

  rewards_browsertest_util::WaitForElementToAppear(
      contents(),
      "#modal");

  rewards_browsertest_util::WaitForElementThenClick(
      contents(),
      "[data-test-id='settings-modal-tabs-2']");

  rewards_browsertest_util::WaitForElementToContain(
      contents(), "[data-test-id='reset-text']",
      "Your 30 BAT and other Rewards");
}

IN_PROC_BROWSER_TEST_F(RewardsBrowserTest, UpholdLimitNoBAT) {
  rewards_browsertest_util::CreateWallet(rewards_service_);
  context_helper_->LoadRewardsPage();

  rewards_browsertest_util::WaitForElementThenClick(
      contents(), "[data-test-id=verify-rewards-button]");

  rewards_browsertest_util::WaitForElementThenClick(
      contents(), "[data-test-id=connect-continue-button]");

  rewards_browsertest_util::WaitForElementThenClick(
      contents(), "[data-test-id=connect-provider-button]");

  rewards_browsertest_util::WaitForElementThenClick(
      contents(), "[data-test-id=connect-login-button]");

  // Check if we are redirected to uphold
  WaitForNavigation("intention=login");
}

IN_PROC_BROWSER_TEST_F(RewardsBrowserTest, EnableRewardsWithBalance) {
  // Make sure rewards, ads, and AC prefs are off
  auto* prefs = browser()->profile()->GetPrefs();
  prefs->SetBoolean(brave_rewards::prefs::kEnabled, false);
  prefs->SetBoolean(brave_rewards::prefs::kAutoContributeEnabled, false);

  // Load a balance into the user's wallet
  rewards_browsertest_util::CreateWallet(rewards_service_);
  rewards_service_->FetchPromotions();
  promotion_->WaitForPromotionInitialization();
  promotion_->ClaimPromotionViaCode();

  rewards_service_->EnableRewards();
  base::RunLoop().RunUntilIdle();

  // Ensure that AC is not enabled
  EXPECT_TRUE(prefs->GetBoolean(brave_rewards::prefs::kEnabled));
  EXPECT_FALSE(prefs->GetBoolean(brave_rewards::prefs::kAutoContributeEnabled));
}

}  // namespace rewards_browsertest
