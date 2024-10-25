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

namespace brave_rewards {

// TODO(https://github.com/brave/brave-browser/issues/23185): Move to unit tests
// for RewardsFlags type.
class RewardsFlagBrowserTest : public InProcessBrowserTest {
 public:
  RewardsFlagBrowserTest() {
    response_ = std::make_unique<test_util::RewardsBrowserTestResponse>();
  }

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();

    RewardsFlags::SetForceParsingForTesting(true);

    // HTTP resolver
    https_server_ = std::make_unique<net::EmbeddedTestServer>(
        net::test_server::EmbeddedTestServer::TYPE_HTTPS);
    https_server_->SetSSLConfig(net::EmbeddedTestServer::CERT_OK);
    https_server_->RegisterRequestHandler(
        base::BindRepeating(&test_util::HandleRequest));
    ASSERT_TRUE(https_server_->Start());

    // Rewards service
    auto* profile = browser()->profile();
    rewards_service_ = static_cast<RewardsServiceImpl*>(
        RewardsServiceFactory::GetForProfile(profile));

    // Response mock
    base::ScopedAllowBlockingForTesting allow_blocking;
    response_->LoadMocks();
    rewards_service_->ForTestingSetTestResponseCallback(
        base::BindRepeating(
            &RewardsFlagBrowserTest::GetTestResponse,
            base::Unretained(this)));
    rewards_service_->SetEngineEnvForTesting();

    test_util::SetOnboardingBypassed(browser());
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
    RewardsFlags::SetForceParsingForTesting(false);
    InProcessBrowserTest::TearDownOnMainThread();
  }

  mojom::Environment GetDefaultEnvironment() {
    return rewards_service_->GetDefaultServerEnvironment();
  }

  raw_ptr<RewardsServiceImpl, DanglingUntriaged> rewards_service_ = nullptr;
  std::unique_ptr<net::EmbeddedTestServer> https_server_;
  std::unique_ptr<test_util::RewardsBrowserTestResponse> response_;
};

IN_PROC_BROWSER_TEST_F(RewardsFlagBrowserTest, HandleFlagsStaging) {
  {
    auto options =
        rewards_service_->HandleFlags(RewardsFlags::ForCurrentProcess());
    EXPECT_EQ(options->environment, GetDefaultEnvironment());
  }

  {
    base::test::ScopedCommandLine scoped_command_line;
    base::CommandLine* command_line =
        scoped_command_line.GetProcessCommandLine();
    command_line->AppendSwitchASCII("rewards", "staging=true");
    auto options =
        rewards_service_->HandleFlags(RewardsFlags::ForCurrentProcess());
    EXPECT_EQ(options->environment, mojom::Environment::kStaging);
  }

  {
    base::test::ScopedCommandLine scoped_command_line;
    base::CommandLine* command_line =
        scoped_command_line.GetProcessCommandLine();
    command_line->AppendSwitchASCII("rewards", "staging=false");
    auto options =
        rewards_service_->HandleFlags(RewardsFlags::ForCurrentProcess());
    EXPECT_EQ(options->environment, mojom::Environment::kProduction);
  }

  {
    base::test::ScopedCommandLine scoped_command_line;
    base::CommandLine* command_line =
        scoped_command_line.GetProcessCommandLine();
    command_line->AppendSwitchASCII("rewards", "staging=foobar");
    rewards_service_->HandleFlags(RewardsFlags::ForCurrentProcess());
    auto options =
        rewards_service_->HandleFlags(RewardsFlags::ForCurrentProcess());
    EXPECT_EQ(options->environment, mojom::Environment::kProduction);
  }
}

IN_PROC_BROWSER_TEST_F(RewardsFlagBrowserTest, HandleFlagsDevelopment) {
  {
    auto options =
        rewards_service_->HandleFlags(RewardsFlags::ForCurrentProcess());
    EXPECT_EQ(options->environment, GetDefaultEnvironment());
  }

  {
    base::test::ScopedCommandLine scoped_command_line;
    base::CommandLine* command_line =
        scoped_command_line.GetProcessCommandLine();
    command_line->AppendSwitchASCII("rewards", "development=true");
    auto options =
        rewards_service_->HandleFlags(RewardsFlags::ForCurrentProcess());
    EXPECT_EQ(options->environment, mojom::Environment::kDevelopment);
  }

  {
    base::test::ScopedCommandLine scoped_command_line;
    base::CommandLine* command_line =
        scoped_command_line.GetProcessCommandLine();
    command_line->AppendSwitchASCII("rewards", "development=1");
    auto options =
        rewards_service_->HandleFlags(RewardsFlags::ForCurrentProcess());
    EXPECT_EQ(options->environment, mojom::Environment::kDevelopment);
  }

  {
    base::test::ScopedCommandLine scoped_command_line;
    base::CommandLine* command_line =
        scoped_command_line.GetProcessCommandLine();
    command_line->AppendSwitchASCII("rewards", "development=false");
    auto options =
        rewards_service_->HandleFlags(RewardsFlags::ForCurrentProcess());
    EXPECT_EQ(options->environment, GetDefaultEnvironment());
  }

  {
    base::test::ScopedCommandLine scoped_command_line;
    base::CommandLine* command_line =
        scoped_command_line.GetProcessCommandLine();
    command_line->AppendSwitchASCII("rewards", "development=foobar");
    auto options =
        rewards_service_->HandleFlags(RewardsFlags::ForCurrentProcess());
    EXPECT_EQ(options->environment, GetDefaultEnvironment());
  }
}

IN_PROC_BROWSER_TEST_F(RewardsFlagBrowserTest, HandleFlagsReconcile) {
  {
    base::test::ScopedCommandLine scoped_command_line;
    base::CommandLine* command_line =
        scoped_command_line.GetProcessCommandLine();
    command_line->AppendSwitchASCII("rewards", "reconcile-interval=10");
    auto options =
        rewards_service_->HandleFlags(RewardsFlags::ForCurrentProcess());
    EXPECT_EQ(options->reconcile_interval, 10);
  }

  {
    base::test::ScopedCommandLine scoped_command_line;
    base::CommandLine* command_line =
        scoped_command_line.GetProcessCommandLine();
    command_line->AppendSwitchASCII("rewards", "reconcile-interval=-1");
    auto options =
        rewards_service_->HandleFlags(RewardsFlags::ForCurrentProcess());
    EXPECT_EQ(options->reconcile_interval, 0);
  }

  {
    base::test::ScopedCommandLine scoped_command_line;
    base::CommandLine* command_line =
        scoped_command_line.GetProcessCommandLine();
    command_line->AppendSwitchASCII("rewards", "reconcile-interval=foobar");
    auto options =
        rewards_service_->HandleFlags(RewardsFlags::ForCurrentProcess());
    EXPECT_EQ(options->reconcile_interval, 0);
  }
}

IN_PROC_BROWSER_TEST_F(RewardsFlagBrowserTest, HandleFlagsRetryInterval) {
  {
    base::test::ScopedCommandLine scoped_command_line;
    base::CommandLine* command_line =
        scoped_command_line.GetProcessCommandLine();
    command_line->AppendSwitchASCII("rewards", "retry-interval=10");
    auto options =
        rewards_service_->HandleFlags(RewardsFlags::ForCurrentProcess());
    EXPECT_EQ(options->retry_interval, 10);
  }

  {
    base::test::ScopedCommandLine scoped_command_line;
    base::CommandLine* command_line =
        scoped_command_line.GetProcessCommandLine();
    command_line->AppendSwitchASCII("rewards", "retry-interval=-1");
    auto options =
        rewards_service_->HandleFlags(RewardsFlags::ForCurrentProcess());
    EXPECT_EQ(options->retry_interval, 0);
  }

  {
    base::test::ScopedCommandLine scoped_command_line;
    base::CommandLine* command_line =
        scoped_command_line.GetProcessCommandLine();
    command_line->AppendSwitchASCII("rewards", "retry-interval=foobar");
    auto options =
        rewards_service_->HandleFlags(RewardsFlags::ForCurrentProcess());
    EXPECT_EQ(options->retry_interval, 0);
  }
}

IN_PROC_BROWSER_TEST_F(RewardsFlagBrowserTest, HandleFlagsMultipleFlags) {
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  command_line->AppendSwitchASCII("rewards",
                                  "staging=true,debug=true,retry-interval=1,"
                                  "reconcile-interval=10");
  auto options =
      rewards_service_->HandleFlags(RewardsFlags::ForCurrentProcess());
  EXPECT_EQ(options->environment, mojom::Environment::kStaging);
  EXPECT_EQ(options->reconcile_interval, 10);
  EXPECT_EQ(options->retry_interval, 1);
}

IN_PROC_BROWSER_TEST_F(RewardsFlagBrowserTest, HandleFlagsWrongInput) {
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  command_line->AppendSwitchASCII("rewards",
                                  "staging=,debug=,retryinterval="
                                  "true,reconcile-interval");
  auto options =
      rewards_service_->HandleFlags(RewardsFlags::ForCurrentProcess());
  EXPECT_EQ(options->environment, mojom::Environment::kProduction);
  EXPECT_EQ(options->reconcile_interval, 0);
  EXPECT_EQ(options->retry_interval, 0);
}

}  // namespace brave_rewards
