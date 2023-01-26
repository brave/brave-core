/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */
#include "brave/browser/brave_ads/ads_service_factory.h"
#include "brave/browser/brave_news/brave_news_controller_factory.h"
#include "brave/browser/brave_rewards/rewards_service_factory.h"
#include "brave/browser/perf/brave_perf_switches.h"
#include "brave/browser/speedreader/speedreader_service_factory.h"
#include "brave/components/brave_ads/browser/ads_service.h"
#include "brave/components/brave_rewards/browser/rewards_service.h"
#include "brave/components/brave_today/browser/brave_news_controller.h"
#include "brave/components/speedreader/speedreader_service.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"

class BraveSpeedFeatureProcessorBrowserTest : public InProcessBrowserTest {
 protected:
  void SetUpCommandLine(base::CommandLine* command_line) override {
    InProcessBrowserTest::SetUpCommandLine(command_line);
    command_line->AppendSwitch(
        perf::switches::kEnableBraveFeaturesForPerfTesting);
  }

  bool SpeedreaderIsEnabled() {
    auto* speedreader_service =
        speedreader::SpeedreaderServiceFactory::GetForProfile(
            browser()->profile());
    return speedreader_service->IsEnabled();
  }

  bool BraveNewsAreEnabled() {
    auto* controller = brave_news::BraveNewsControllerFactory::GetForContext(
        browser()->profile());
    return controller->GetIsEnabledForTesting();
  }

  bool AdsServiceIsEnabled() {
    auto* ads_service =
        brave_ads::AdsServiceFactory::GetForProfile(browser()->profile());
    return ads_service->IsEnabled();
  }

  void NonBlockingDelay(const base::TimeDelta& delay) {
    base::RunLoop run_loop(base::RunLoop::Type::kNestableTasksAllowed);
    base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
        FROM_HERE, run_loop.QuitWhenIdleClosure(), delay);
    run_loop.Run();
  }

  void WaitForRewardsServiceInitialized() {
    auto* rewards_service = brave_rewards::RewardsServiceFactory::GetForProfile(
        browser()->profile());
    while (!rewards_service->IsInitialized()) {
      NonBlockingDelay(base::Seconds(1));
    }
  }
};

IN_PROC_BROWSER_TEST_F(BraveSpeedFeatureProcessorBrowserTest, PRE_Default) {
  WaitForRewardsServiceInitialized();
}

IN_PROC_BROWSER_TEST_F(BraveSpeedFeatureProcessorBrowserTest, Default) {
  EXPECT_TRUE(SpeedreaderIsEnabled());
  EXPECT_TRUE(AdsServiceIsEnabled());
  EXPECT_TRUE(BraveNewsAreEnabled());
  WaitForRewardsServiceInitialized();
}
