/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>

#include "base/run_loop.h"
#include "brave/browser/brave_rewards/rewards_service_factory.h"
#include "brave/common/brave_paths.h"
#include "brave/components/brave_rewards/browser/rewards_service_impl.h"
#include "brave/components/brave_rewards/browser/test/common/rewards_browsertest_network_util.h"
#include "brave/components/brave_rewards/browser/test/common/rewards_browsertest_response.h"
#include "brave/components/brave_rewards/browser/test/common/rewards_browsertest_util.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/testing_profile.h"
#include "content/public/test/browser_test.h"
#include "net/dns/mock_host_resolver.h"
#include "testing/gmock/include/gmock/gmock.h"

// npm run test -- brave_browser_tests --filter=RewardsFlagBrowserTest.*

namespace rewards_browsertest {

class RewardsFlagBrowserTest : public InProcessBrowserTest {
 public:
  RewardsFlagBrowserTest() {
    response_ = std::make_unique<RewardsBrowserTestResponse>();
  }

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();

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
            &RewardsFlagBrowserTest::GetTestResponse,
            base::Unretained(this)));
    rewards_service_->SetLedgerEnvForTesting();
  }

  void GetTestResponse(
      const std::string& url,
      int32_t method,
      int* response_status_code,
      std::string* response,
      std::map<std::string, std::string>* headers) {
    response_->Get(
        url,
        method,
        response_status_code,
        response);
  }

  void TearDown() override {
    InProcessBrowserTest::TearDown();
  }

  void ResetWaitForCallback() {
    callback_called_ = false;
    wait_for_callback_.reset(new base::RunLoop);
  }

  void WaitForCallback() {
    if (callback_called_) {
      return;
    }
    wait_for_callback_->Run();
  }

  void CallbackCalled() {
    callback_called_ = true;
    wait_for_callback_->Quit();
  }

  void GetReconcileInterval() {
    ResetWaitForCallback();
    rewards_service_->GetReconcileInterval(
        base::Bind(&RewardsFlagBrowserTest::OnGetReconcileIntervalWrapper,
                   base::Unretained(this)));
    WaitForCallback();
  }

  void GetShortRetries() {
    ResetWaitForCallback();
    rewards_service_->GetShortRetries(
        base::Bind(&RewardsFlagBrowserTest::OnGetShortRetriesWrapper,
                   base::Unretained(this)));
    WaitForCallback();
  }

  void GetEnvironment() {
    ResetWaitForCallback();
    rewards_service_->GetEnvironment(
        base::Bind(&RewardsFlagBrowserTest::OnGetEnvironmentWrapper,
                   base::Unretained(this)));
    WaitForCallback();
  }

  void GetDebug() {
    ResetWaitForCallback();
    rewards_service_->GetDebug(base::Bind(
        &RewardsFlagBrowserTest::OnGetDebugWrapper, base::Unretained(this)));
    WaitForCallback();
  }

  void OnGetReconcileIntervalWrapper(int32_t interval) {
    OnGetReconcileInterval(interval);
    CallbackCalled();
  }

  void OnGetShortRetriesWrapper(bool retries) {
    OnGetShortRetries(retries);
    CallbackCalled();
  }

  void OnGetEnvironmentWrapper(ledger::type::Environment environment) {
    OnGetEnvironment(environment);
    CallbackCalled();
  }

  void OnGetDebugWrapper(bool debug) {
    OnGetDebug(debug);
    CallbackCalled();
  }

  MOCK_METHOD1(OnGetEnvironment, void(ledger::type::Environment));
  MOCK_METHOD1(OnGetDebug, void(bool));
  MOCK_METHOD1(OnGetReconcileInterval, void(int32_t));
  MOCK_METHOD1(OnGetShortRetries, void(bool));

  brave_rewards::RewardsServiceImpl* rewards_service_;
  std::unique_ptr<net::EmbeddedTestServer> https_server_;
  std::unique_ptr<RewardsBrowserTestResponse> response_;
  bool callback_called_ = false;
  std::unique_ptr<base::RunLoop> wait_for_callback_;
};

IN_PROC_BROWSER_TEST_F(RewardsFlagBrowserTest, HandleFlagsStaging) {
  rewards_browsertest_util::StartProcess(rewards_service_);
  EXPECT_CALL(*this, OnGetEnvironment(ledger::type::Environment::STAGING))
      .Times(2);
  EXPECT_CALL(*this, OnGetEnvironment(
      ledger::type::Environment::PRODUCTION)).Times(3);

  testing::InSequence s;

  rewards_service_->SetEnvironment(ledger::type::Environment::PRODUCTION);
  GetEnvironment();

  rewards_service_->SetEnvironment(ledger::type::Environment::PRODUCTION);
  rewards_service_->HandleFlags("staging=true");
  GetEnvironment();

  rewards_service_->SetEnvironment(ledger::type::Environment::PRODUCTION);
  rewards_service_->HandleFlags("staging=1");
  GetEnvironment();

  rewards_service_->SetEnvironment(ledger::type::Environment::STAGING);
  rewards_service_->HandleFlags("staging=false");
  GetEnvironment();

  rewards_service_->SetEnvironment(ledger::type::Environment::STAGING);
  rewards_service_->HandleFlags("staging=werwe");
  GetEnvironment();
}

IN_PROC_BROWSER_TEST_F(RewardsFlagBrowserTest, HandleFlagsDebug) {
  rewards_browsertest_util::StartProcess(rewards_service_);
  EXPECT_CALL(*this, OnGetDebug(true)).Times(3);
  EXPECT_CALL(*this, OnGetDebug(false)).Times(2);

  testing::InSequence s;

  rewards_service_->SetDebug(true);
  GetDebug();

  rewards_service_->SetDebug(false);
  rewards_service_->HandleFlags("debug=true");
  GetDebug();

  rewards_service_->SetDebug(false);
  rewards_service_->HandleFlags("debug=1");
  GetDebug();

  rewards_service_->SetDebug(true);
  rewards_service_->HandleFlags("debug=false");
  GetDebug();

  rewards_service_->SetDebug(true);
  rewards_service_->HandleFlags("debug=werwe");
  GetDebug();
}

IN_PROC_BROWSER_TEST_F(RewardsFlagBrowserTest, HandleFlagsDevelopment) {
  rewards_browsertest_util::StartProcess(rewards_service_);
  EXPECT_CALL(*this, OnGetEnvironment(ledger::type::Environment::DEVELOPMENT))
      .Times(2);
  EXPECT_CALL(*this, OnGetEnvironment(ledger::type::Environment::PRODUCTION))
      .Times(3);

  testing::InSequence s;

  rewards_service_->SetEnvironment(ledger::type::Environment::PRODUCTION);
  GetEnvironment();

  rewards_service_->SetEnvironment(ledger::type::Environment::PRODUCTION);
  rewards_service_->HandleFlags("development=true");
  GetEnvironment();

  rewards_service_->SetEnvironment(ledger::type::Environment::PRODUCTION);
  rewards_service_->HandleFlags("development=1");
  GetEnvironment();

  rewards_service_->SetEnvironment(ledger::type::Environment::PRODUCTION);
  rewards_service_->HandleFlags("development=false");
  GetEnvironment();

  rewards_service_->SetEnvironment(ledger::type::Environment::PRODUCTION);
  rewards_service_->HandleFlags("development=werwe");
  GetEnvironment();
}

IN_PROC_BROWSER_TEST_F(RewardsFlagBrowserTest, HandleFlagsReconcile) {
  rewards_browsertest_util::StartProcess(rewards_service_);
  EXPECT_CALL(*this, OnGetReconcileInterval(10));
  EXPECT_CALL(*this, OnGetReconcileInterval(0)).Times(2);

  testing::InSequence s;

  rewards_service_->SetReconcileInterval(0);
  rewards_service_->HandleFlags("reconcile-interval=10");
  GetReconcileInterval();

  rewards_service_->SetReconcileInterval(0);
  rewards_service_->HandleFlags("reconcile-interval=-1");
  GetReconcileInterval();

  rewards_service_->SetReconcileInterval(0);
  rewards_service_->HandleFlags("reconcile-interval=sdf");
  GetReconcileInterval();
}

IN_PROC_BROWSER_TEST_F(RewardsFlagBrowserTest, HandleFlagsShortRetries) {
  rewards_browsertest_util::StartProcess(rewards_service_);
  EXPECT_CALL(*this, OnGetShortRetries(true));
  EXPECT_CALL(*this, OnGetShortRetries(false));

  testing::InSequence s;

  rewards_service_->SetShortRetries(false);
  rewards_service_->HandleFlags("short-retries=true");
  GetShortRetries();

  rewards_service_->SetShortRetries(true);
  rewards_service_->HandleFlags("short-retries=false");
  GetShortRetries();
}

IN_PROC_BROWSER_TEST_F(RewardsFlagBrowserTest, HandleFlagsMultipleFlags) {
  rewards_browsertest_util::StartProcess(rewards_service_);
  EXPECT_CALL(*this, OnGetEnvironment(ledger::type::Environment::STAGING));
  EXPECT_CALL(*this, OnGetDebug(true));
  EXPECT_CALL(*this, OnGetReconcileInterval(10));
  EXPECT_CALL(*this, OnGetShortRetries(true));

  testing::InSequence s;
  rewards_service_->SetEnvironment(ledger::type::Environment::PRODUCTION);
  rewards_service_->SetDebug(true);
  rewards_service_->SetReconcileInterval(0);
  rewards_service_->SetShortRetries(false);

  rewards_service_->HandleFlags(
      "staging=true,debug=true,short-retries=true,reconcile-interval=10");

  GetReconcileInterval();
  GetShortRetries();
  GetEnvironment();
  GetDebug();
}

IN_PROC_BROWSER_TEST_F(RewardsFlagBrowserTest, HandleFlagsWrongInput) {
  rewards_browsertest_util::StartProcess(rewards_service_);
  EXPECT_CALL(*this, OnGetEnvironment(ledger::type::Environment::PRODUCTION));
  EXPECT_CALL(*this, OnGetDebug(false));
  EXPECT_CALL(*this, OnGetReconcileInterval(0));
  EXPECT_CALL(*this, OnGetShortRetries(false));

  testing::InSequence s;
  rewards_service_->SetEnvironment(ledger::type::Environment::PRODUCTION);
  rewards_service_->SetDebug(false);
  rewards_service_->SetReconcileInterval(0);
  rewards_service_->SetShortRetries(false);

  rewards_service_->HandleFlags(
      "staging=,debug=,shortretries=true,reconcile-interval");

  GetReconcileInterval();
  GetShortRetries();
  GetDebug();
  GetEnvironment();
}

}  // namespace rewards_browsertest
