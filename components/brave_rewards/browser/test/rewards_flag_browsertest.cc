/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>

#include "base/command_line.h"
#include "base/containers/flat_map.h"
#include "base/memory/raw_ptr.h"
#include "base/run_loop.h"
#include "base/test/scoped_command_line.h"
#include "brave/browser/brave_rewards/rewards_service_factory.h"
#include "brave/components/brave_rewards/browser/rewards_service_impl.h"
#include "brave/components/brave_rewards/browser/test/common/rewards_browsertest_network_util.h"
#include "brave/components/brave_rewards/browser/test/common/rewards_browsertest_response.h"
#include "brave/components/brave_rewards/browser/test/common/rewards_browsertest_util.h"
#include "brave/components/brave_rewards/common/rewards_flags.h"
#include "brave/components/constants/brave_paths.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/testing_profile.h"
#include "content/public/test/browser_test.h"
#include "net/dns/mock_host_resolver.h"
#include "testing/gmock/include/gmock/gmock.h"

// npm run test -- brave_browser_tests --filter=RewardsFlagBrowserTest.*

namespace rewards_browsertest {

// TODO(https://github.com/brave/brave-browser/issues/23185): Move to unit tests
// for RewardsFlags type.
class RewardsFlagBrowserTest : public InProcessBrowserTest {
 public:
  RewardsFlagBrowserTest() {
    response_ = std::make_unique<RewardsBrowserTestResponse>();
  }

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();

    brave_rewards::RewardsFlags::SetForceParsingForTesting(true);

    // HTTP resolver
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
            &RewardsFlagBrowserTest::GetTestResponse,
            base::Unretained(this)));
    rewards_service_->SetLedgerEnvForTesting();

    rewards_browsertest_util::SetOnboardingBypassed(browser());
  }

  void GetTestResponse(
      const std::string& url,
      int32_t method,
      int* response_status_code,
      std::string* response,
      base::flat_map<std::string, std::string>* headers) {
    response_->Get(
        url,
        method,
        response_status_code,
        response);
  }

  void TearDownOnMainThread() override {
    brave_rewards::RewardsFlags::SetForceParsingForTesting(false);

    InProcessBrowserTest::TearDownOnMainThread();
  }

  void ResetWaitForCallback() {
    callback_called_ = false;
    wait_for_callback_ = std::make_unique<base::RunLoop>();
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
        base::BindOnce(&RewardsFlagBrowserTest::OnGetReconcileIntervalWrapper,
                       base::Unretained(this)));
    WaitForCallback();
  }

  void GetRetryInterval() {
    ResetWaitForCallback();
    rewards_service_->GetRetryInterval(
        base::BindOnce(&RewardsFlagBrowserTest::OnGetRetryIntervalWrapper,
                       base::Unretained(this)));
    WaitForCallback();
  }

  void GetEnvironment() {
    ResetWaitForCallback();
    rewards_service_->GetEnvironment(
        base::BindOnce(&RewardsFlagBrowserTest::OnGetEnvironmentWrapper,
                       base::Unretained(this)));
    WaitForCallback();
  }

  void GetDebug() {
    ResetWaitForCallback();
    rewards_service_->GetDebug(base::BindOnce(
        &RewardsFlagBrowserTest::OnGetDebugWrapper, base::Unretained(this)));
    WaitForCallback();
  }

  void GetGeminiRetries() {
    ResetWaitForCallback();
    rewards_service_->GetGeminiRetries(
        base::BindOnce(&RewardsFlagBrowserTest::OnGetGeminiRetriesWrapper,
                       base::Unretained(this)));
    WaitForCallback();
  }

  void OnGetReconcileIntervalWrapper(int32_t interval) {
    OnGetReconcileInterval(interval);
    CallbackCalled();
  }

  void OnGetRetryIntervalWrapper(int32_t interval) {
    OnGetRetryInterval(interval);
    CallbackCalled();
  }

  void OnGetEnvironmentWrapper(ledger::mojom::Environment environment) {
    OnGetEnvironment(environment);
    CallbackCalled();
  }

  void OnGetDebugWrapper(bool debug) {
    OnGetDebug(debug);
    CallbackCalled();
  }

  void OnGetGeminiRetriesWrapper(int32_t retries) {
    OnGetGeminiRetries(retries);
    CallbackCalled();
  }

  MOCK_METHOD1(OnGetEnvironment, void(ledger::mojom::Environment));
  MOCK_METHOD1(OnGetDebug, void(bool));
  MOCK_METHOD1(OnGetReconcileInterval, void(int32_t));
  MOCK_METHOD1(OnGetRetryInterval, void(int32_t));
  MOCK_METHOD1(OnGetGeminiRetries, void(int32_t));

  raw_ptr<brave_rewards::RewardsServiceImpl> rewards_service_ = nullptr;
  std::unique_ptr<net::EmbeddedTestServer> https_server_;
  std::unique_ptr<RewardsBrowserTestResponse> response_;
  bool callback_called_ = false;
  std::unique_ptr<base::RunLoop> wait_for_callback_;
};

IN_PROC_BROWSER_TEST_F(RewardsFlagBrowserTest, HandleFlagsStaging) {
  rewards_browsertest_util::StartProcess(rewards_service_);
  EXPECT_CALL(*this, OnGetEnvironment(ledger::mojom::Environment::STAGING))
      .Times(2);
  EXPECT_CALL(*this, OnGetEnvironment(ledger::mojom::Environment::PRODUCTION))
      .Times(3);

  testing::InSequence s;

  rewards_service_->SetEnvironment(ledger::mojom::Environment::PRODUCTION);
  GetEnvironment();

  {
    rewards_service_->SetEnvironment(ledger::mojom::Environment::PRODUCTION);
    base::test::ScopedCommandLine scoped_command_line;
    base::CommandLine* command_line =
        scoped_command_line.GetProcessCommandLine();
    command_line->AppendSwitchASCII("rewards", "staging=true");
    rewards_service_->HandleFlags(
        brave_rewards::RewardsFlags::ForCurrentProcess());
    GetEnvironment();
  }

  {
    rewards_service_->SetEnvironment(ledger::mojom::Environment::PRODUCTION);
    base::test::ScopedCommandLine scoped_command_line;
    base::CommandLine* command_line =
        scoped_command_line.GetProcessCommandLine();
    command_line->AppendSwitchASCII("rewards", "staging=1");
    rewards_service_->HandleFlags(
        brave_rewards::RewardsFlags::ForCurrentProcess());
    GetEnvironment();
  }

  {
    rewards_service_->SetEnvironment(ledger::mojom::Environment::STAGING);
    base::test::ScopedCommandLine scoped_command_line;
    base::CommandLine* command_line =
        scoped_command_line.GetProcessCommandLine();
    command_line->AppendSwitchASCII("rewards", "staging=false");
    rewards_service_->HandleFlags(
        brave_rewards::RewardsFlags::ForCurrentProcess());
    GetEnvironment();
  }

  {
    rewards_service_->SetEnvironment(ledger::mojom::Environment::STAGING);
    base::test::ScopedCommandLine scoped_command_line;
    base::CommandLine* command_line =
        scoped_command_line.GetProcessCommandLine();
    command_line->AppendSwitchASCII("rewards", "staging=foobar");
    rewards_service_->HandleFlags(
        brave_rewards::RewardsFlags::ForCurrentProcess());
    GetEnvironment();
  }
}

IN_PROC_BROWSER_TEST_F(RewardsFlagBrowserTest, HandleFlagsDebug) {
  rewards_browsertest_util::StartProcess(rewards_service_);
  EXPECT_CALL(*this, OnGetDebug(true)).Times(3);
  EXPECT_CALL(*this, OnGetDebug(false)).Times(2);

  testing::InSequence s;

  rewards_service_->SetDebug(true);
  GetDebug();

  {
    rewards_service_->SetDebug(false);
    base::test::ScopedCommandLine scoped_command_line;
    base::CommandLine* command_line =
        scoped_command_line.GetProcessCommandLine();
    command_line->AppendSwitchASCII("rewards", "debug=true");
    rewards_service_->HandleFlags(
        brave_rewards::RewardsFlags::ForCurrentProcess());
    GetDebug();
  }

  {
    rewards_service_->SetDebug(false);
    base::test::ScopedCommandLine scoped_command_line;
    base::CommandLine* command_line =
        scoped_command_line.GetProcessCommandLine();
    command_line->AppendSwitchASCII("rewards", "debug=1");
    rewards_service_->HandleFlags(
        brave_rewards::RewardsFlags::ForCurrentProcess());
    GetDebug();
  }

  {
    rewards_service_->SetDebug(false);
    base::test::ScopedCommandLine scoped_command_line;
    base::CommandLine* command_line =
        scoped_command_line.GetProcessCommandLine();
    command_line->AppendSwitchASCII("rewards", "debug=false");
    rewards_service_->HandleFlags(
        brave_rewards::RewardsFlags::ForCurrentProcess());
    GetDebug();
  }

  {
    rewards_service_->SetDebug(false);
    base::test::ScopedCommandLine scoped_command_line;
    base::CommandLine* command_line =
        scoped_command_line.GetProcessCommandLine();
    command_line->AppendSwitchASCII("rewards", "debug=foobar");
    rewards_service_->HandleFlags(
        brave_rewards::RewardsFlags::ForCurrentProcess());
    GetDebug();
  }
}

IN_PROC_BROWSER_TEST_F(RewardsFlagBrowserTest, HandleFlagsDevelopment) {
  rewards_browsertest_util::StartProcess(rewards_service_);
  EXPECT_CALL(*this, OnGetEnvironment(ledger::mojom::Environment::DEVELOPMENT))
      .Times(2);
  EXPECT_CALL(*this, OnGetEnvironment(ledger::mojom::Environment::PRODUCTION))
      .Times(3);

  testing::InSequence s;

  rewards_service_->SetEnvironment(ledger::mojom::Environment::PRODUCTION);
  GetEnvironment();

  {
    rewards_service_->SetEnvironment(ledger::mojom::Environment::PRODUCTION);
    base::test::ScopedCommandLine scoped_command_line;
    base::CommandLine* command_line =
        scoped_command_line.GetProcessCommandLine();
    command_line->AppendSwitchASCII("rewards", "development=true");
    rewards_service_->HandleFlags(
        brave_rewards::RewardsFlags::ForCurrentProcess());
    GetEnvironment();
  }

  {
    rewards_service_->SetEnvironment(ledger::mojom::Environment::PRODUCTION);
    base::test::ScopedCommandLine scoped_command_line;
    base::CommandLine* command_line =
        scoped_command_line.GetProcessCommandLine();
    command_line->AppendSwitchASCII("rewards", "development=1");
    rewards_service_->HandleFlags(
        brave_rewards::RewardsFlags::ForCurrentProcess());
    GetEnvironment();
  }

  {
    rewards_service_->SetEnvironment(ledger::mojom::Environment::PRODUCTION);
    base::test::ScopedCommandLine scoped_command_line;
    base::CommandLine* command_line =
        scoped_command_line.GetProcessCommandLine();
    command_line->AppendSwitchASCII("rewards", "development=false");
    rewards_service_->HandleFlags(
        brave_rewards::RewardsFlags::ForCurrentProcess());
    GetEnvironment();
  }

  {
    rewards_service_->SetEnvironment(ledger::mojom::Environment::PRODUCTION);
    base::test::ScopedCommandLine scoped_command_line;
    base::CommandLine* command_line =
        scoped_command_line.GetProcessCommandLine();
    command_line->AppendSwitchASCII("rewards", "development=foobar");
    rewards_service_->HandleFlags(
        brave_rewards::RewardsFlags::ForCurrentProcess());
    GetEnvironment();
  }
}

IN_PROC_BROWSER_TEST_F(RewardsFlagBrowserTest, HandleFlagsReconcile) {
  rewards_browsertest_util::StartProcess(rewards_service_);
  EXPECT_CALL(*this, OnGetReconcileInterval(10));
  EXPECT_CALL(*this, OnGetReconcileInterval(0)).Times(2);

  testing::InSequence s;

  {
    rewards_service_->SetReconcileInterval(0);
    base::test::ScopedCommandLine scoped_command_line;
    base::CommandLine* command_line =
        scoped_command_line.GetProcessCommandLine();
    command_line->AppendSwitchASCII("rewards", "reconcile-interval=10");
    rewards_service_->HandleFlags(
        brave_rewards::RewardsFlags::ForCurrentProcess());
    GetReconcileInterval();
  }

  {
    rewards_service_->SetReconcileInterval(0);
    base::test::ScopedCommandLine scoped_command_line;
    base::CommandLine* command_line =
        scoped_command_line.GetProcessCommandLine();
    command_line->AppendSwitchASCII("rewards", "reconcile-interval=-1");
    rewards_service_->HandleFlags(
        brave_rewards::RewardsFlags::ForCurrentProcess());
    GetReconcileInterval();
  }

  {
    rewards_service_->SetReconcileInterval(0);
    base::test::ScopedCommandLine scoped_command_line;
    base::CommandLine* command_line =
        scoped_command_line.GetProcessCommandLine();
    command_line->AppendSwitchASCII("rewards", "reconcile-interval=foobar");
    rewards_service_->HandleFlags(
        brave_rewards::RewardsFlags::ForCurrentProcess());
    GetReconcileInterval();
  }
}

IN_PROC_BROWSER_TEST_F(RewardsFlagBrowserTest, HandleFlagsRetryInterval) {
  rewards_browsertest_util::StartProcess(rewards_service_);
  EXPECT_CALL(*this, OnGetRetryInterval(10));
  EXPECT_CALL(*this, OnGetRetryInterval(0)).Times(2);

  testing::InSequence s;

  {
    rewards_service_->SetRetryInterval(0);
    base::test::ScopedCommandLine scoped_command_line;
    base::CommandLine* command_line =
        scoped_command_line.GetProcessCommandLine();
    command_line->AppendSwitchASCII("rewards", "retry-interval=10");
    rewards_service_->HandleFlags(
        brave_rewards::RewardsFlags::ForCurrentProcess());
    GetRetryInterval();
  }

  {
    rewards_service_->SetRetryInterval(0);
    base::test::ScopedCommandLine scoped_command_line;
    base::CommandLine* command_line =
        scoped_command_line.GetProcessCommandLine();
    command_line->AppendSwitchASCII("rewards", "retry-interval=-1");
    rewards_service_->HandleFlags(
        brave_rewards::RewardsFlags::ForCurrentProcess());
    GetRetryInterval();
  }

  {
    rewards_service_->SetRetryInterval(0);
    base::test::ScopedCommandLine scoped_command_line;
    base::CommandLine* command_line =
        scoped_command_line.GetProcessCommandLine();
    command_line->AppendSwitchASCII("rewards", "retry-interval=foobar");
    rewards_service_->HandleFlags(
        brave_rewards::RewardsFlags::ForCurrentProcess());
    GetRetryInterval();
  }
}

IN_PROC_BROWSER_TEST_F(RewardsFlagBrowserTest, HandleFlagsGeminiRetries) {
  rewards_browsertest_util::StartProcess(rewards_service_);
  EXPECT_CALL(*this, OnGetGeminiRetries(2));
  EXPECT_CALL(*this, OnGetGeminiRetries(10));
  EXPECT_CALL(*this, OnGetGeminiRetries(0));

  testing::InSequence s;

  {
    rewards_service_->SetGeminiRetries(0);
    base::test::ScopedCommandLine scoped_command_line;
    base::CommandLine* command_line =
        scoped_command_line.GetProcessCommandLine();
    command_line->AppendSwitchASCII("rewards", "gemini-retries=2");
    rewards_service_->HandleFlags(
        brave_rewards::RewardsFlags::ForCurrentProcess());
    GetGeminiRetries();
  }

  {
    rewards_service_->SetGeminiRetries(0);
    base::test::ScopedCommandLine scoped_command_line;
    base::CommandLine* command_line =
        scoped_command_line.GetProcessCommandLine();
    command_line->AppendSwitchASCII("rewards", "gemini-retries=10");
    rewards_service_->HandleFlags(
        brave_rewards::RewardsFlags::ForCurrentProcess());
    GetGeminiRetries();
  }

  {
    rewards_service_->SetGeminiRetries(0);
    base::test::ScopedCommandLine scoped_command_line;
    base::CommandLine* command_line =
        scoped_command_line.GetProcessCommandLine();
    command_line->AppendSwitchASCII("rewards", "gemini-retries=-1");
    rewards_service_->HandleFlags(
        brave_rewards::RewardsFlags::ForCurrentProcess());
    GetGeminiRetries();
  }
}

IN_PROC_BROWSER_TEST_F(RewardsFlagBrowserTest, HandleFlagsMultipleFlags) {
  rewards_browsertest_util::StartProcess(rewards_service_);
  EXPECT_CALL(*this, OnGetEnvironment(ledger::mojom::Environment::STAGING));
  EXPECT_CALL(*this, OnGetDebug(true));
  EXPECT_CALL(*this, OnGetReconcileInterval(10));
  EXPECT_CALL(*this, OnGetRetryInterval(1));
  EXPECT_CALL(*this, OnGetGeminiRetries(2));

  testing::InSequence s;
  rewards_service_->SetEnvironment(ledger::mojom::Environment::PRODUCTION);
  rewards_service_->SetDebug(true);
  rewards_service_->SetReconcileInterval(0);
  rewards_service_->SetRetryInterval(0);

  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  command_line->AppendSwitchASCII("rewards",
                                  "staging=true,debug=true,retry-interval=1,"
                                  "reconcile-interval=10,gemini-retries=2");
  rewards_service_->HandleFlags(
      brave_rewards::RewardsFlags::ForCurrentProcess());

  GetReconcileInterval();
  GetRetryInterval();
  GetEnvironment();
  GetDebug();
  GetGeminiRetries();
}

IN_PROC_BROWSER_TEST_F(RewardsFlagBrowserTest, HandleFlagsWrongInput) {
  rewards_browsertest_util::StartProcess(rewards_service_);
  EXPECT_CALL(*this, OnGetEnvironment(ledger::mojom::Environment::PRODUCTION));
  EXPECT_CALL(*this, OnGetDebug(false));
  EXPECT_CALL(*this, OnGetReconcileInterval(0));
  EXPECT_CALL(*this, OnGetRetryInterval(0));
  EXPECT_CALL(*this, OnGetGeminiRetries(3));

  testing::InSequence s;
  rewards_service_->SetEnvironment(ledger::mojom::Environment::PRODUCTION);
  rewards_service_->SetDebug(false);
  rewards_service_->SetReconcileInterval(0);
  rewards_service_->SetRetryInterval(0);
  rewards_service_->SetGeminiRetries(3);

  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  command_line->AppendSwitchASCII("rewards",
                                  "staging=,debug=,retryinterval="
                                  "true,reconcile-interval,gemini-retries");
  rewards_service_->HandleFlags(
      brave_rewards::RewardsFlags::ForCurrentProcess());

  GetReconcileInterval();
  GetRetryInterval();
  GetDebug();
  GetEnvironment();
  GetGeminiRetries();
}

}  // namespace rewards_browsertest
