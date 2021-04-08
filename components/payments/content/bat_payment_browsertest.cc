/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/path_service.h"
#include "brave/browser/brave_rewards/rewards_service_factory.h"
#include "brave/common/brave_paths.h"
#include "brave/components/brave_rewards/browser/rewards_service_impl.h"
#include "brave/components/brave_rewards/browser/test/common/rewards_browsertest_context_helper.h"
#include "brave/components/brave_rewards/browser/test/common/rewards_browsertest_context_util.h"
#include "brave/components/brave_rewards/browser/test/common/rewards_browsertest_response.h"
#include "brave/components/brave_rewards/browser/test/common/rewards_browsertest_util.h"
#include "chrome/test/base/testing_profile.h"
#include "chrome/test/base/ui_test_utils.h"
#include "chrome/test/payments/payment_request_platform_browsertest_base.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

using content::WebContentsConsoleObserver;
using rewards_browsertest::RewardsBrowserTestContextHelper;
using rewards_browsertest::RewardsBrowserTestResponse;

namespace payments {

class BatPaymentTest : public PaymentRequestPlatformBrowserTestBase {
 public:
  BatPaymentTest() {
    response_ = std::make_unique<RewardsBrowserTestResponse>();
  }

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();

    context_helper_ =
        std::make_unique<RewardsBrowserTestContextHelper>(browser());

    // HTTP resolver
    brave::RegisterPathProvider();
    base::FilePath test_data_dir;
    base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir);
    test_data_dir = test_data_dir.Append("rewards-data");
    test_data_dir = test_data_dir.Append("payments");
    https_server()->ServeFilesFromDirectory(test_data_dir);

    host_resolver()->AddRule("*", "127.0.0.1");
    https_server()->SetSSLConfig(net::EmbeddedTestServer::CERT_OK);
    ASSERT_TRUE(https_server()->Start());

    // Rewards service
    brave::RegisterPathProvider();
    auto* profile = browser()->profile();
    rewards_service_ = static_cast<brave_rewards::RewardsServiceImpl*>(
        brave_rewards::RewardsServiceFactory::GetForProfile(profile));

    // Response mock
    base::ScopedAllowBlockingForTesting allow_blocking;
    response_->LoadMocks();
    rewards_service_->ForTestingSetTestResponseCallback(base::BindRepeating(
        &BatPaymentTest::GetTestResponse, base::Unretained(this)));
    rewards_service_->SetLedgerEnvForTesting();

    rewards_browsertest_util::SetOnboardingBypassed(browser());

    // For PaymentRequest
    test_controller()->SetUpOnMainThread();
    PlatformBrowserTest::SetUpOnMainThread();
  }

  void TearDown() override { InProcessBrowserTest::TearDown(); }

  void CanMakePaymentWillPass(const std::list<TestEvent> event_sequence,
                              const std::string expected_result,
                              const std::string function) {
    ResetEventWaiterForEventSequence(event_sequence);
    ASSERT_TRUE(content::ExecuteScript(GetActiveWebContents(), function));
    WaitForObservedEvent();
    ExpectBodyContains(expected_result);
  }

  void CanMakePaymentWillFail(const std::string expected_result,
                              const std::string function) {
    ASSERT_TRUE(content::ExecuteScript(GetActiveWebContents(), function));
    ExpectBodyContains(expected_result);
  }

  void GetTestResponse(const std::string& url,
                       int32_t method,
                       int* response_status_code,
                       std::string* response,
                       base::flat_map<std::string, std::string>* headers) {
    response_->Get(url, method, response_status_code, response);
  }

  void LoadVerifiedPublisher(std::string publisher) {
    rewards_browsertest_util::StartProcess(rewards_service());
    rewards_service()->EnableRewards();
    base::RunLoop().RunUntilIdle();

    context_helper()->LoadURL(
        rewards_browsertest_util::GetUrl(https_server(), publisher));
    content::WebContents* popup_contents = context_helper()->OpenRewardsPopup();
    ASSERT_TRUE(popup_contents);

    // Verify if the publisher is verified
    rewards_browsertest_util::WaitForElementToContain(
        popup_contents, "[id='wallet-panel']", "Brave Verified Creator");
  }

  void LoadUnverifiedPublisher(std::string publisher) {
    rewards_browsertest_util::StartProcess(rewards_service());
    context_helper()->LoadURL(
        rewards_browsertest_util::GetUrl(https_server(), publisher));
    rewards_service()->EnableRewards();
    base::RunLoop().RunUntilIdle();

    content::WebContents* popup_contents = context_helper()->OpenRewardsPopup();
    ASSERT_TRUE(popup_contents);

    // Verify if the publisher is unverified
    rewards_browsertest_util::WaitForElementToContain(
        popup_contents, "[id='wallet-panel']", "Not yet verified");
  }

  brave_rewards::RewardsServiceImpl* rewards_service() {
    return rewards_service_;
  }
  RewardsBrowserTestContextHelper* context_helper() {
    return context_helper_.get();
  }

 private:
  brave_rewards::RewardsServiceImpl* rewards_service_;
  std::unique_ptr<RewardsBrowserTestContextHelper> context_helper_;
  std::unique_ptr<RewardsBrowserTestResponse> response_;
};

IN_PROC_BROWSER_TEST_F(BatPaymentTest, BasicTest) {
  std::string publisher = "duckduckgo.com";
  LoadVerifiedPublisher(publisher);
  ui_test_utils::NavigateToURLWithDisposition(
      browser(),
      rewards_browsertest_util::GetUrl(https_server(), publisher,
                                       "/payment_request.html"),
      WindowOpenDisposition::CURRENT_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP);

  CanMakePaymentWillPass(
      {TestEvent::kCanMakePaymentCalled, TestEvent::kCanMakePaymentReturned},
      "true", "batPaymentMethodSupported()");
}

IN_PROC_BROWSER_TEST_F(BatPaymentTest, UnverifiedPublisherCanMakePayment) {
  std::string publisher = "brave.com";
  LoadUnverifiedPublisher(publisher);
  ui_test_utils::NavigateToURLWithDisposition(
      browser(),
      rewards_browsertest_util::GetUrl(https_server(), publisher,
                                       "/payment_request.html"),
      WindowOpenDisposition::CURRENT_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP);

  CanMakePaymentWillPass(
      {TestEvent::kCanMakePaymentCalled, TestEvent::kCanMakePaymentReturned},
      "true", "batPaymentMethodSupported()");
}

IN_PROC_BROWSER_TEST_F(BatPaymentTest, NoDisplayItems) {
  std::string publisher = "duckduckgo.com";
  LoadVerifiedPublisher(publisher);
  ui_test_utils::NavigateToURLWithDisposition(
      browser(),
      rewards_browsertest_util::GetUrl(https_server(), publisher,
                                       "/payment_request.html"),
      WindowOpenDisposition::CURRENT_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP);

  CanMakePaymentWillFail(
      "InvalidStateError: Failed to execute 'canMakePayment'",
      "paymentRequestWithoutDisplayItems()");
}

IN_PROC_BROWSER_TEST_F(BatPaymentTest, MissingSKUTokens) {
  std::string publisher = "duckduckgo.com";
  LoadVerifiedPublisher(publisher);
  ui_test_utils::NavigateToURLWithDisposition(
      browser(),
      rewards_browsertest_util::GetUrl(https_server(), publisher,
                                       "/payment_request.html"),
      WindowOpenDisposition::CURRENT_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP);

  CanMakePaymentWillFail(
      "InvalidStateError: Failed to execute 'canMakePayment'",
      "paymentRequestWithoutSkuTokens()");
}

}  // namespace payments
