/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <vector>

#include "base/base_paths.h"
#include "base/memory/raw_ptr.h"
#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/task/single_thread_task_runner.h"
#include "base/test/scoped_run_loop_timeout.h"
#include "brave/browser/brave_rewards/rewards_service_factory.h"
#include "brave/browser/net/brave_network_audit_test_helper.h"
#include "brave/components/brave_rewards/content/rewards_service_impl.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/prefs/pref_service.h"
#include "content/public/test/browser_test.h"
#include "services/network/public/cpp/network_switches.h"

namespace brave {

namespace {

// Max amount of time to wait after getting an URL loaded, in milliseconds. Note
// that the value passed to --ui-test-action-timeout in //brave/package.json, as
// part of the 'network-audit' script, must be big enough to accomodate this.
//
// In particular:
//   --ui-test-action-timeout: should be greater than |kMaxTimeoutPerLoadedURL|.
//   --test-launcher-timeout: should be able to fit the total sum of timeouts.
constexpr int kMaxTimeoutPerLoadedURL = 30;

void WaitForTimeout(int timeout) {
  base::test::ScopedRunLoopTimeout file_download_timeout(
      FROM_HERE, base::Seconds(kMaxTimeoutPerLoadedURL + 1));
  base::RunLoop run_loop;
  base::SingleThreadTaskRunner::GetCurrentDefault()->PostDelayedTask(
      FROM_HERE, run_loop.QuitClosure(), base::Seconds(timeout));
  run_loop.Run();
}

class BraveRewardsNetworkAuditTest : public InProcessBrowserTest {
 public:
  BraveRewardsNetworkAuditTest() = default;

  BraveRewardsNetworkAuditTest(const BraveRewardsNetworkAuditTest&) = delete;
  BraveRewardsNetworkAuditTest& operator=(const BraveRewardsNetworkAuditTest&) =
      delete;

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();

    ASSERT_TRUE(embedded_test_server()->Start());

    // Create and start the Rewards service
    rewards_service_ = static_cast<brave_rewards::RewardsServiceImpl*>(
        brave_rewards::RewardsServiceFactory::GetForProfile(profile()));
    base::RunLoop run_loop;
    rewards_service_->StartProcessForTesting(run_loop.QuitClosure());
    run_loop.Run();
  }

  void TearDownOnMainThread() override {
    rewards_service_->Shutdown();
    InProcessBrowserTest::TearDownOnMainThread();
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    base::FilePath source_root_path =
        base::PathService::CheckedGet(base::DIR_SRC_TEST_DATA_ROOT);

    // Full log containing all the network requests.
    net_log_path_ = source_root_path.AppendASCII("network_rewards_log.json");

    // Log containing the results of the audit only.
    audit_results_path_ =
        source_root_path.AppendASCII("network_audit_rewards_results.json");

    command_line->AppendSwitchPath(network::switches::kLogNetLog,
                                   net_log_path_);
    command_line->AppendSwitchASCII(network::switches::kNetLogCaptureMode,
                                    "Everything");
  }

  void TearDownInProcessBrowserTestFixture() override {
    // Before adding to this list, get approval from the security team.
    VerifyNetworkAuditLog(
        net_log_path_, audit_results_path_,
        /*extra_allowed_prefixes=*/
        {
            // Brave Rewards production.
            "https://api.rewards.brave.com/v1/cards",
            "https://api.rewards.brave.com/v1/parameters",
            "https://rewards.brave.com/publishers/prefix-list",
            "https://grant.rewards.brave.com/v1/promotions",

            // Brave Rewards staging & dev.
            "https://api.rewards.bravesoftware.com/v1/cards",
            "https://api.rewards.bravesoftware.com/v1/parameters",
            "https://rewards-stg.bravesoftware.com/publishers/prefix-list",
            "https://grant.rewards.bravesoftware.com/v1/promotions",

            // Brave Ads production.
            "https://geo.ads.brave.com/v1/getstate",
            "https://static.ads.brave.com/v9/catalog",

            // Brave Ads staging.
            "https://geo.ads.bravesoftware.com/v1/getstate",
            "https://static.ads.bravesoftware.com/v9/catalog",
        });
  }

  bool EnableBraveRewards() {
    PrefService* pref_service = profile()->GetPrefs();
    pref_service->SetInteger("brave.rewards.version", 7);
    pref_service->SetBoolean("brave.rewards.enabled", true);
    return pref_service->GetBoolean("brave.rewards.enabled");
  }

  Profile* profile() { return browser()->profile(); }

 private:
  raw_ptr<brave_rewards::RewardsServiceImpl, DanglingUntriaged>
      rewards_service_ = nullptr;
  base::FilePath net_log_path_;
  base::FilePath audit_results_path_;
};

IN_PROC_BROWSER_TEST_F(BraveRewardsNetworkAuditTest, BasicTests) {
  // Load brave://rewards and enable Brave Rewards.
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), GURL("brave://rewards")));
  ASSERT_TRUE(EnableBraveRewards());
  WaitForTimeout(kMaxTimeoutPerLoadedURL);
}

}  // namespace

}  // namespace brave
