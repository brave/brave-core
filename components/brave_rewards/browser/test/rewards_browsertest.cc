/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>
#include <string>

#include "base/containers/flat_map.h"
#include "base/memory/raw_ptr.h"
#include "base/ranges/algorithm.h"
#include "base/test/bind.h"
#include "base/test/scoped_feature_list.h"
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
#include "brave/components/brave_rewards/core/global_constants.h"
#include "brave/components/brave_rewards/core/uphold/uphold_util.h"
#include "brave/components/constants/brave_paths.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/testing_profile.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/network_session_configurator/common/network_switches.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/test/browser_test.h"
#include "net/dns/mock_host_resolver.h"

// npm run test -- brave_browser_tests --filter=RewardsBrowserTest.*

namespace rewards_browsertest {

constexpr char kSelectCountryScript[] = R"(
  const select = document.querySelector('[data-test-id=country-select]');
  select.value = 'US';
  select.dispatchEvent(new Event("change", { bubbles: true }));
  true;
)";

class WalletUpdatedWaiter : public brave_rewards::RewardsServiceObserver {
 public:
  explicit WalletUpdatedWaiter(brave_rewards::RewardsService* rewards_service)
      : rewards_service_(rewards_service) {
    rewards_service_->AddObserver(this);
  }

  ~WalletUpdatedWaiter() override { rewards_service_->RemoveObserver(this); }

  void OnRewardsWalletUpdated() override { run_loop_.Quit(); }

  void Wait() { run_loop_.Run(); }

 private:
  base::RunLoop run_loop_;
  raw_ptr<brave_rewards::RewardsService> rewards_service_;
};

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
    https_server_ = std::make_unique<net::EmbeddedTestServer>(
        net::test_server::EmbeddedTestServer::TYPE_HTTPS);
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
    rewards_service_->FetchBalance(
        base::BindLambdaForTesting([&](ledger::FetchBalanceResult result) {
          total = result.has_value() ? result.value()->total : -1.0;
          run_loop.Quit();
        }));
    run_loop.Run();
    return total;
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
  rewards_browsertest_util::SetOnboardingBypassed(browser(), true);
  rewards_browsertest_util::StartProcess(rewards_service_);
  context_helper_->LoadRewardsPage();

  rewards_browsertest_util::WaitForElementThenClick(
      contents(), "[data-test-id=manage-wallet-button]");
  rewards_browsertest_util::WaitForElementToAppear(
      contents(),
      "#modal");
}

IN_PROC_BROWSER_TEST_F(RewardsBrowserTest, SiteBannerDefaultTipChoices) {
  rewards_browsertest_util::CreateRewardsWallet(rewards_service_);
  rewards_browsertest_util::NavigateToPublisherPage(
      browser(),
      https_server_.get(),
      "3zsistemi.si");

  base::WeakPtr<content::WebContents> site_banner =
      context_helper_->OpenSiteBanner();
  auto tip_options =
      rewards_browsertest_util::GetSiteBannerTipOptions(site_banner.get());
  ASSERT_EQ(tip_options, std::vector<double>({1, 5, 50}));
}

IN_PROC_BROWSER_TEST_F(RewardsBrowserTest, SiteBannerDefaultPublisherAmounts) {
  rewards_browsertest_util::CreateRewardsWallet(rewards_service_);
  rewards_browsertest_util::NavigateToPublisherPage(
      browser(),
      https_server_.get(),
      "laurenwags.github.io");

  base::WeakPtr<content::WebContents> site_banner =
      context_helper_->OpenSiteBanner();
  const auto tip_options =
      rewards_browsertest_util::GetSiteBannerTipOptions(site_banner.get());

  // Creator-specific default tip amounts are no longer supported, so just
  // verify that the tip options match the global defaults
  ASSERT_EQ(tip_options, std::vector<double>({1, 5, 50}));
}

IN_PROC_BROWSER_TEST_F(RewardsBrowserTest, NotVerifiedWallet) {
  rewards_browsertest_util::CreateRewardsWallet(rewards_service_);
  context_helper_->LoadRewardsPage();
  contribution_->AddBalance(promotion_->ClaimPromotionViaCode());
  contribution_->IsBalanceCorrect();

  rewards_browsertest_util::WaitForElementThenClick(
      contents(), "[data-test-id=verify-rewards-button]");

  rewards_browsertest_util::WaitForElementThenClick(
      contents(), "[data-test-id=connect-provider-button]");

  // Check if we are redirected to uphold
  content::DidStartNavigationObserver(contents()).Wait();
  content::DidFinishNavigationObserver observer(
      contents(),
      base::BindLambdaForTesting(
          [this](content::NavigationHandle* navigation_handle) {
            DCHECK(navigation_handle->GetURL().spec().find(
                       ledger::uphold::GetUrl() + "/authorize/") ==
                   std::string::npos);

            // Fake successful authentication
            ui_test_utils::NavigateToURLBlockUntilNavigationsComplete(
                browser(), uphold_auth_url(), 1);

            rewards_browsertest_util::WaitForElementToContain(
                contents(), "[data-test-id=external-wallet-status-text]",
                "Connected");
          }));
}

IN_PROC_BROWSER_TEST_F(RewardsBrowserTest, ShowACPercentInThePanel) {
  rewards_browsertest_util::CreateRewardsWallet(rewards_service_);
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

IN_PROC_BROWSER_TEST_F(RewardsBrowserTest, ResetRewards) {
  rewards_browsertest_util::CreateRewardsWallet(rewards_service_);
  context_helper_->LoadRewardsPage();

  rewards_browsertest_util::WaitForElementThenClick(
      contents(), "[data-test-id=manage-wallet-button]");

  rewards_browsertest_util::WaitForElementToAppear(contents(), "#modal");

  rewards_browsertest_util::WaitForElementToContain(
      contents(), "[data-test-id='reset-text']",
      "By resetting, your current Brave Rewards profile will be deleted");
}

IN_PROC_BROWSER_TEST_F(RewardsBrowserTest, EnableRewardsWithBalance) {
  // Load a balance into the user's wallet
  rewards_browsertest_util::CreateRewardsWallet(rewards_service_);
  auto* prefs = browser()->profile()->GetPrefs();
  EXPECT_TRUE(prefs->GetBoolean(brave_rewards::prefs::kEnabled));

  rewards_service_->FetchPromotions(base::DoNothing());
  promotion_->WaitForPromotionInitialization();
  promotion_->ClaimPromotionViaCode();

  // Make sure rewards, ads, and AC prefs are off
  prefs->SetBoolean(brave_rewards::prefs::kEnabled, false);
  prefs->SetBoolean(brave_rewards::prefs::kAutoContributeEnabled, false);

  base::RunLoop run_loop;
  rewards_service_->CreateRewardsWallet(
      "", base::BindLambdaForTesting(
              [&run_loop](ledger::mojom::CreateRewardsWalletResult) {
                run_loop.Quit();
              }));
  run_loop.Run();

  // Ensure that AC is not enabled
  EXPECT_TRUE(prefs->GetBoolean(brave_rewards::prefs::kEnabled));
  EXPECT_FALSE(prefs->GetBoolean(brave_rewards::prefs::kAutoContributeEnabled));
}

IN_PROC_BROWSER_TEST_F(RewardsBrowserTest, GeoDeclarationNewUser) {
  auto* prefs = browser()->profile()->GetPrefs();
  prefs->SetBoolean(brave_rewards::prefs::kEnabled, false);
  EXPECT_EQ(prefs->GetString(brave_rewards::prefs::kDeclaredGeo), "");

  auto popup_contents = context_helper_->OpenRewardsPopup();
  ASSERT_TRUE(popup_contents);

  rewards_browsertest_util::WaitForElementThenClick(
      popup_contents.get(), "[data-test-id=opt-in-button]");

  rewards_browsertest_util::WaitForElementToAppear(
      popup_contents.get(), "[data-test-id=country-select]");

  WalletUpdatedWaiter waiter(rewards_service_);
  EXPECT_EQ(true, content::EvalJs(popup_contents.get(), kSelectCountryScript));
  rewards_browsertest_util::WaitForElementThenClick(
      popup_contents.get(), "[data-test-id=select-country-button]");
  waiter.Wait();

  EXPECT_EQ(prefs->GetString(brave_rewards::prefs::kDeclaredGeo), "US");
  EXPECT_TRUE(prefs->GetBoolean(brave_rewards::prefs::kEnabled));
}

IN_PROC_BROWSER_TEST_F(RewardsBrowserTest, GeoDeclarationExistingUser) {
  rewards_browsertest_util::CreateRewardsWallet(rewards_service_);
  auto* prefs = browser()->profile()->GetPrefs();
  prefs->SetString(brave_rewards::prefs::kDeclaredGeo, "");

  auto popup_contents = context_helper_->OpenRewardsPopup();
  ASSERT_TRUE(popup_contents);

  rewards_browsertest_util::WaitForElementToAppear(
      popup_contents.get(), "[data-test-id=select-country-button]");

  WalletUpdatedWaiter waiter(rewards_service_);
  EXPECT_EQ(true, content::EvalJs(popup_contents.get(), kSelectCountryScript));
  rewards_browsertest_util::WaitForElementThenClick(
      popup_contents.get(), "[data-test-id=select-country-button]");
  waiter.Wait();

  EXPECT_EQ(prefs->GetString(brave_rewards::prefs::kDeclaredGeo), "US");
  EXPECT_TRUE(prefs->GetBoolean(brave_rewards::prefs::kEnabled));
}

}  // namespace rewards_browsertest
