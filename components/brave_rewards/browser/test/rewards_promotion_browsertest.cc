/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/containers/flat_map.h"
#include "base/memory/raw_ptr.h"
#include "brave/browser/brave_rewards/rewards_service_factory.h"
#include "brave/common/brave_paths.h"
#include "brave/components/brave_rewards/browser/rewards_service_impl.h"
#include "brave/components/brave_rewards/browser/test/common/rewards_browsertest_context_helper.h"
#include "brave/components/brave_rewards/browser/test/common/rewards_browsertest_context_util.h"
#include "brave/components/brave_rewards/browser/test/common/rewards_browsertest_network_util.h"
#include "brave/components/brave_rewards/browser/test/common/rewards_browsertest_promotion.h"
#include "brave/components/brave_rewards/browser/test/common/rewards_browsertest_response.h"
#include "brave/components/brave_rewards/browser/test/common/rewards_browsertest_util.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/testing_profile.h"
#include "content/public/test/browser_test.h"
#include "net/dns/mock_host_resolver.h"

// npm run test -- brave_browser_tests --filter=RewardsPromotionBrowserTest.*

namespace rewards_browsertest {

class RewardsPromotionBrowserTest : public InProcessBrowserTest {
 public:
  RewardsPromotionBrowserTest() {
    promotion_ = std::make_unique<RewardsBrowserTestPromotion>();
    response_ = std::make_unique<RewardsBrowserTestResponse>();
  }

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();

    context_helper_ =
        std::make_unique<RewardsBrowserTestContextHelper>(browser());

    // HTTP resolver
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
            &RewardsPromotionBrowserTest::GetTestResponse,
            base::Unretained(this)));
    rewards_service_->SetLedgerEnvForTesting();

    // Other
    promotion_->Initialize(browser(), rewards_service_);

    rewards_browsertest_util::SetOnboardingBypassed(browser());
  }

  void TearDown() override {
    InProcessBrowserTest::TearDown();
  }

  void GetTestResponse(
      const std::string& url,
      int32_t method,
      int* response_status_code,
      std::string* response,
      base::flat_map<std::string, std::string>* headers) {
    if (gone_) {
      if (url.find("/v1/promotions/") != std::string::npos && method == 2) {
        *response_status_code = 410;
        return;
      }
    }

    if (removed_ && url.find("/v1/promotions?") != std::string::npos) {
      *response = "{}";
      return;
    }

    response_->Get(
        url,
        method,
        response_status_code,
        response);
  }

  double ClaimPromotion(bool use_panel, const bool should_finish = true) {
    // Use the appropriate WebContents
    base::WeakPtr<content::WebContents> contents =
        use_panel ? context_helper_->OpenRewardsPopup()
                  : browser()
                        ->tab_strip_model()
                        ->GetActiveWebContents()
                        ->GetWeakPtr();

    // Wait for promotion to initialize
    promotion_->WaitForPromotionInitialization();

    // Claim promotion via settings page or panel, as instructed
    if (use_panel) {
      rewards_browsertest_util::WaitForElementThenClick(
          contents.get(), "[data-test-id=notification-action-button");
    } else {
      rewards_browsertest_util::WaitForElementThenClick(
          contents.get(), "[data-test-id='claimGrant']");
    }

    // Wait for CAPTCHA
    rewards_browsertest_util::WaitForElementToAppear(
        contents.get(), "[data-test-id=grant-captcha-object]");

    rewards_browsertest_util::DragAndDrop(
        contents.get(), "[data-test-id=grant-captcha-object]",
        "[data-test-id=grant-captcha-target]");

    if (!should_finish) {
      promotion_->WaitForPromotionFinished(false);
      return 0.0;
    }

    promotion_->WaitForPromotionFinished();

    // Ensure that promotion looks as expected
    auto promotion = promotion_->GetPromotion();
    EXPECT_STREQ(
        std::to_string(promotion->approximate_value).c_str(),
        "30.000000");
    EXPECT_STREQ(
        promotion->id.c_str(),
        promotion_->GetPromotionId().c_str());
    EXPECT_EQ(promotion->type, ledger::type::PromotionType::UGP);
    EXPECT_EQ(promotion->expires_at, 1740816427ull);

    // Check that promotion notification shows the appropriate amount
    const std::string selector = use_panel
        ? "[id='root']"
        : "[data-test-id='newTokenGrant']";
    rewards_browsertest_util::WaitForElementToContain(contents.get(), selector,
                                                      "Free Token Grant");
    rewards_browsertest_util::WaitForElementToContain(contents.get(), selector,
                                                      "30.000 BAT");

    // Dismiss the promotion notification
    if (use_panel) {
      rewards_browsertest_util::WaitForElementThenClick(contents.get(),
                                                        "#"
                                                        "grant-completed-ok");
    }

    return 30;
  }

  content::WebContents* contents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  void CheckPromotionStatus(const std::string status) {
    context_helper_->LoadURL(
        rewards_browsertest_util::GetRewardsInternalsUrl());

    rewards_browsertest_util::WaitForElementThenClick(
        contents(),
        "#internals-tabs > div > div:nth-of-type(3)");

    rewards_browsertest_util::WaitForElementToContain(
        contents(),
        "#internals-tabs",
        status);
  }

  raw_ptr<brave_rewards::RewardsServiceImpl> rewards_service_ = nullptr;
  std::unique_ptr<net::EmbeddedTestServer> https_server_;
  std::unique_ptr<RewardsBrowserTestPromotion> promotion_;
  std::unique_ptr<RewardsBrowserTestResponse> response_;
  std::unique_ptr<RewardsBrowserTestContextHelper> context_helper_;
  bool gone_ = false;
  bool removed_ = false;
};

// https://github.com/brave/brave-browser/issues/12605
IN_PROC_BROWSER_TEST_F(RewardsPromotionBrowserTest,
                       DISABLED_ClaimViaSettingsPage) {
  context_helper_->LoadURL(rewards_browsertest_util::GetRewardsUrl());
  double balance = ClaimPromotion(false);
  ASSERT_EQ(balance, 30.0);
}

// https://github.com/brave/brave-browser/issues/12605
IN_PROC_BROWSER_TEST_F(RewardsPromotionBrowserTest, DISABLED_ClaimViaPanel) {
  double balance = ClaimPromotion(true);
  ASSERT_EQ(balance, 30.0);
}

IN_PROC_BROWSER_TEST_F(
    RewardsPromotionBrowserTest,
    PromotionHasEmptyPublicKey) {
  response_->SetPromotionEmptyKey(true);
  rewards_browsertest_util::StartProcess(rewards_service_);

  base::WeakPtr<content::WebContents> popup =
      context_helper_->OpenRewardsPopup();

  promotion_->WaitForPromotionInitialization();
  rewards_browsertest_util::WaitForElementToAppear(
      popup.get(), "[data-test-id=notification-close]", false);
}

IN_PROC_BROWSER_TEST_F(RewardsPromotionBrowserTest, PromotionGone) {
  gone_ = true;
  rewards_browsertest_util::StartProcess(rewards_service_);
  ClaimPromotion(true, false);
  CheckPromotionStatus("Over");
}

// https://github.com/brave/brave-browser/issues/12632
IN_PROC_BROWSER_TEST_F(
    RewardsPromotionBrowserTest,
    DISABLED_PromotionRemovedFromEndpoint) {
  context_helper_->LoadURL(rewards_browsertest_util::GetRewardsUrl());
  promotion_->WaitForPromotionInitialization();
  removed_ = true;
  context_helper_->ReloadCurrentSite();

  rewards_browsertest_util::WaitForElementToAppear(
      contents(),
      "[data-test-id='promotion-claim-box']",
      false);
  CheckPromotionStatus("Over");
}

IN_PROC_BROWSER_TEST_F(RewardsPromotionBrowserTest, PromotionNotQuiteOver) {
  rewards_browsertest_util::StartProcess(rewards_service_);
  rewards_service_->FetchPromotions();
  promotion_->WaitForPromotionInitialization();

  removed_ = true;
  rewards_service_->FetchPromotions();
  promotion_->WaitForPromotionInitialization();

  CheckPromotionStatus("Over");

  removed_ = false;
  rewards_service_->FetchPromotions();
  promotion_->WaitForPromotionInitialization();

  CheckPromotionStatus("Active");
}

}  // namespace rewards_browsertest
