/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>

#include "base/containers/flat_map.h"
#include "base/memory/raw_ptr.h"
#include "base/test/bind.h"
#include "brave/browser/brave_rewards/rewards_service_factory.h"
#include "brave/components/brave_rewards/browser/rewards_service_impl.h"
#include "brave/components/brave_rewards/browser/test/common/rewards_browsertest_context_helper.h"
#include "brave/components/brave_rewards/browser/test/common/rewards_browsertest_context_util.h"
#include "brave/components/brave_rewards/browser/test/common/rewards_browsertest_network_util.h"
#include "brave/components/brave_rewards/browser/test/common/rewards_browsertest_promotion.h"
#include "brave/components/brave_rewards/browser/test/common/rewards_browsertest_response.h"
#include "brave/components/brave_rewards/browser/test/common/rewards_browsertest_util.h"
#include "brave/components/constants/brave_paths.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/testing_profile.h"
#include "content/public/test/browser_test.h"
#include "net/dns/mock_host_resolver.h"

// npm run test -- brave_browser_tests --filter=RewardsPromotionBrowserTest.*

namespace brave_rewards {

class RewardsPromotionBrowserTest : public InProcessBrowserTest {
 public:
  RewardsPromotionBrowserTest() {
    promotion_ = std::make_unique<test_util::RewardsBrowserTestPromotion>();
    response_ = std::make_unique<test_util::RewardsBrowserTestResponse>();
  }

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();

    context_helper_ =
        std::make_unique<test_util::RewardsBrowserTestContextHelper>(browser());

    // HTTP resolver
    https_server_ = std::make_unique<net::EmbeddedTestServer>(
        net::test_server::EmbeddedTestServer::TYPE_HTTPS);
    https_server_->SetSSLConfig(net::EmbeddedTestServer::CERT_OK);
    https_server_->RegisterRequestHandler(
        base::BindRepeating(&test_util::HandleRequest));
    ASSERT_TRUE(https_server_->Start());

    // Rewards service
    brave::RegisterPathProvider();
    auto* profile = browser()->profile();
    rewards_service_ = static_cast<RewardsServiceImpl*>(
        RewardsServiceFactory::GetForProfile(profile));

    // Response mock
    base::ScopedAllowBlockingForTesting allow_blocking;
    response_->LoadMocks();
    rewards_service_->ForTestingSetTestResponseCallback(
        base::BindRepeating(
            &RewardsPromotionBrowserTest::GetTestResponse,
            base::Unretained(this)));
    rewards_service_->SetEngineEnvForTesting();

    // Other
    promotion_->Initialize(browser(), rewards_service_);

    test_util::SetOnboardingBypassed(browser());
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

  double ClaimPromotion(bool should_finish = true) {
    auto contents = context_helper_->OpenRewardsPopup();

    // Wait for promotion to initialize
    promotion_->WaitForPromotionInitialization();

    test_util::WaitForElementThenClick(
        contents.get(), "[data-test-id=notification-action-button");

    // Wait for CAPTCHA
    test_util::WaitForElementToAppear(contents.get(),
                                      "[data-test-id=grant-captcha-object]");

    test_util::DragAndDrop(contents.get(),
                           "[data-test-id=grant-captcha-object]",
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
    EXPECT_EQ(promotion->type, mojom::PromotionType::UGP);
    EXPECT_EQ(promotion->expires_at, 1740816427ull);

    // Check that promotion notification shows the appropriate amount
    const std::string selector = "[id='root']";

    test_util::WaitForElementToContain(contents.get(), selector,
                                       "Free Token Grant");
    test_util::WaitForElementToContain(contents.get(), selector, "30.000 BAT");

    return 30;
  }

  content::WebContents* contents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  void CheckPromotionStatus(const std::string status) {
    context_helper_->LoadURL(test_util::GetRewardsInternalsUrl());

    test_util::WaitForElementThenClick(
        contents(), "#internals-tabs > div > div:nth-of-type(3)");

    test_util::WaitForElementToContain(contents(), "#internals-tabs", status);
  }

  raw_ptr<RewardsServiceImpl> rewards_service_ = nullptr;
  std::unique_ptr<net::EmbeddedTestServer> https_server_;
  std::unique_ptr<test_util::RewardsBrowserTestPromotion> promotion_;
  std::unique_ptr<test_util::RewardsBrowserTestResponse> response_;
  std::unique_ptr<test_util::RewardsBrowserTestContextHelper> context_helper_;
  bool gone_ = false;
  bool removed_ = false;
};

IN_PROC_BROWSER_TEST_F(RewardsPromotionBrowserTest, ClaimViaPanel) {
  test_util::CreateRewardsWallet(rewards_service_);
  double balance = ClaimPromotion();
  ASSERT_EQ(balance, 30.0);
}

IN_PROC_BROWSER_TEST_F(RewardsPromotionBrowserTest,
                       PromotionHasEmptyPublicKey) {
  response_->SetPromotionEmptyKey(true);
  test_util::CreateRewardsWallet(rewards_service_);

  base::WeakPtr<content::WebContents> popup =
      context_helper_->OpenRewardsPopup();

  promotion_->WaitForPromotionInitialization();
  test_util::WaitForElementToAppear(popup.get(),
                                    "[data-test-id=notification-close]", false);
}

IN_PROC_BROWSER_TEST_F(RewardsPromotionBrowserTest, PromotionGone) {
  gone_ = true;
  test_util::CreateRewardsWallet(rewards_service_);
  ClaimPromotion(false);
  CheckPromotionStatus("Over");
}

IN_PROC_BROWSER_TEST_F(RewardsPromotionBrowserTest,
                       PromotionRemovedFromEndpoint) {
  test_util::CreateRewardsWallet(rewards_service_);
  context_helper_->LoadRewardsPage();
  promotion_->WaitForPromotionInitialization();
  removed_ = true;
  context_helper_->ReloadCurrentSite();

  test_util::WaitForElementToAppear(
      contents(), "[data-test-id='promotion-claim-box']", false);
  CheckPromotionStatus("Over");
}

IN_PROC_BROWSER_TEST_F(RewardsPromotionBrowserTest, PromotionNotQuiteOver) {
  test_util::CreateRewardsWallet(rewards_service_);

  auto fetch_promotions = [this]() {
    base::RunLoop run_loop;
    rewards_service_->FetchPromotions(base::BindLambdaForTesting(
        [&](std::vector<mojom::PromotionPtr>) { run_loop.Quit(); }));
    run_loop.Run();
  };

  fetch_promotions();

  removed_ = true;
  fetch_promotions();
  CheckPromotionStatus("Over");

  removed_ = false;
  fetch_promotions();
  CheckPromotionStatus("Active");
}

}  // namespace brave_rewards
