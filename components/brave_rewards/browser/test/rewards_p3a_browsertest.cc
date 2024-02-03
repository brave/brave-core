/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>
#include <string>

#include "base/containers/flat_map.h"
#include "base/memory/raw_ptr.h"
#include "base/test/bind.h"
#include "base/test/metrics/histogram_tester.h"
#include "brave/browser/brave_rewards/rewards_service_factory.h"
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"
#include "brave/components/brave_rewards/browser/rewards_p3a.h"
#include "brave/components/brave_rewards/browser/rewards_service_impl.h"
#include "brave/components/brave_rewards/browser/rewards_service_observer.h"
#include "brave/components/brave_rewards/browser/test/common/rewards_browsertest_context_util.h"
#include "brave/components/brave_rewards/browser/test/common/rewards_browsertest_contribution.h"
#include "brave/components/brave_rewards/browser/test/common/rewards_browsertest_network_util.h"
#include "brave/components/brave_rewards/browser/test/common/rewards_browsertest_promotion.h"
#include "brave/components/brave_rewards/browser/test/common/rewards_browsertest_response.h"
#include "brave/components/brave_rewards/browser/test/common/rewards_browsertest_util.h"
#include "brave/components/brave_rewards/common/pref_names.h"
#include "brave/components/constants/brave_paths.h"
#include "brave/components/ntp_background_images/common/pref_names.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "components/network_session_configurator/common/network_switches.h"
#include "components/prefs/pref_service.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "net/dns/mock_host_resolver.h"

// npm run test -- brave_browser_tests --filter=RewardsP3ABrowserTest.*

namespace brave_rewards {

class RewardsP3ABrowserTest : public InProcessBrowserTest,
                              public RewardsServiceObserver {
 public:
  RewardsP3ABrowserTest() {
    contribution_ =
        std::make_unique<test_util::RewardsBrowserTestContribution>();
    promotion_ = std::make_unique<test_util::RewardsBrowserTestPromotion>();
    response_ = std::make_unique<test_util::RewardsBrowserTestResponse>();
    histogram_tester_ = std::make_unique<base::HistogramTester>();
  }

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();

    // HTTP resolver
    host_resolver()->AddRule("*", "127.0.0.1");
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
    rewards_service_->AddObserver(this);

    // Response mock
    base::ScopedAllowBlockingForTesting allow_blocking;
    response_->LoadMocks();
    rewards_service_->ForTestingSetTestResponseCallback(base::BindRepeating(
        &RewardsP3ABrowserTest::GetTestResponse, base::Unretained(this)));
    rewards_service_->SetEngineEnvForTesting();

    // Other
    promotion_->Initialize(browser(), rewards_service_);
    contribution_->Initialize(browser(), rewards_service_);

    test_util::SetOnboardingBypassed(browser());
  }

  void TearDown() override { InProcessBrowserTest::TearDown(); }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    // HTTPS server only serves a valid cert for localhost, so this is needed
    // to load pages from other hosts without an error
    command_line->AppendSwitch(switches::kIgnoreCertificateErrors);
  }

  void GetTestResponse(const std::string& url,
                       int32_t method,
                       int* response_status_code,
                       std::string* response,
                       base::flat_map<std::string, std::string>* headers) {
    response_->SetExternalBalance(contribution_->GetExternalBalance());
    response_->Get(url, method, response_status_code, response);
  }

  void WaitForRewardsInitialization() {
    if (rewards_initialized_) {
      return;
    }

    wait_for_rewards_initialization_loop_ = std::make_unique<base::RunLoop>();
    wait_for_rewards_initialization_loop_->Run();
  }

  void TurnOnRewards() {
    // Set the enabled pref to false so that wallet creation will automatically
    // turn on Ads and AC.
    browser()->profile()->GetPrefs()->SetBoolean(prefs::kEnabled, false);
    test_util::CreateRewardsWallet(rewards_service_);
  }

  content::WebContents* contents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  void OnRewardsInitialized(RewardsService* rewards_service) override {
    rewards_initialized_ = true;
    if (wait_for_rewards_initialization_loop_) {
      wait_for_rewards_initialization_loop_->Quit();
    }
  }

  raw_ptr<RewardsServiceImpl> rewards_service_ = nullptr;
  std::unique_ptr<net::EmbeddedTestServer> https_server_;
  std::unique_ptr<test_util::RewardsBrowserTestContribution> contribution_;
  std::unique_ptr<test_util::RewardsBrowserTestPromotion> promotion_;
  std::unique_ptr<test_util::RewardsBrowserTestResponse> response_;
  std::unique_ptr<base::HistogramTester> histogram_tester_;

  bool rewards_initialized_ = false;
  std::unique_ptr<base::RunLoop> wait_for_rewards_initialization_loop_;
};

IN_PROC_BROWSER_TEST_F(RewardsP3ABrowserTest, RewardsDisabled) {
  test_util::StartProcess(rewards_service_);
  WaitForRewardsInitialization();

  histogram_tester_->ExpectTotalCount(p3a::kAutoContributionsStateHistogramName,
                                      0);
  histogram_tester_->ExpectTotalCount(p3a::kTipsSentHistogramName, 0);
  histogram_tester_->ExpectUniqueSample(p3a::kAdTypesEnabledHistogramName,
                                        INT_MAX - 1, 1);
}

IN_PROC_BROWSER_TEST_F(RewardsP3ABrowserTest, RewardsReset) {
  test_util::StartProcess(rewards_service_);
  WaitForRewardsInitialization();

  histogram_tester_->ExpectUniqueSample(p3a::kAdTypesEnabledHistogramName,
                                        INT_MAX - 1, 1);
  TurnOnRewards();

  histogram_tester_->ExpectUniqueSample(
      p3a::kAutoContributionsStateHistogramName, INT_MAX - 1, 1);
  histogram_tester_->ExpectUniqueSample(p3a::kTipsSentHistogramName,
                                        INT_MAX - 1, 1);
}

IN_PROC_BROWSER_TEST_F(RewardsP3ABrowserTest, ToggleAdTypes) {
  test_util::StartProcess(rewards_service_);
  WaitForRewardsInitialization();

  PrefService* prefs = browser()->profile()->GetPrefs();

  TurnOnRewards();

  prefs->SetBoolean(brave_ads::prefs::kOptedInToNotificationAds, false);
  histogram_tester_->ExpectBucketCount(p3a::kAdTypesEnabledHistogramName,
                                       p3a::AdTypesEnabled::kNTP, 1);

  prefs->SetBoolean(ntp_background_images::prefs::
                        kNewTabPageShowSponsoredImagesBackgroundImage,
                    false);
  histogram_tester_->ExpectBucketCount(p3a::kAdTypesEnabledHistogramName,
                                       p3a::AdTypesEnabled::kNone, 1);

  prefs->SetBoolean(brave_ads::prefs::kOptedInToNotificationAds, true);
  histogram_tester_->ExpectBucketCount(p3a::kAdTypesEnabledHistogramName,
                                       p3a::AdTypesEnabled::kNotification, 1);
}

#if !BUILDFLAG(IS_ANDROID)
IN_PROC_BROWSER_TEST_F(RewardsP3ABrowserTest, Conversion) {
  PrefService* prefs = browser()->profile()->GetPrefs();
  prefs->SetBoolean(prefs::kEnabled, false);

  p3a::ConversionMonitor conversion_monitor(prefs);

  histogram_tester_->ExpectTotalCount(p3a::kEnabledSourceHistogramName, 0);

  conversion_monitor.RecordPanelTrigger(p3a::PanelTrigger::kToolbarButton);

  histogram_tester_->ExpectBucketCount(p3a::kToolbarButtonTriggerHistogramName,
                                       1, 1);

  prefs->SetBoolean(prefs::kEnabled, true);
  conversion_monitor.RecordRewardsEnable();

  histogram_tester_->ExpectBucketCount(p3a::kEnabledSourceHistogramName, 1, 1);

  prefs->SetBoolean(prefs::kEnabled, false);
  conversion_monitor.RecordPanelTrigger(p3a::PanelTrigger::kNTP);

  histogram_tester_->ExpectBucketCount(p3a::kToolbarButtonTriggerHistogramName,
                                       1, 1);

  conversion_monitor.RecordRewardsEnable();

  histogram_tester_->ExpectBucketCount(p3a::kEnabledSourceHistogramName, 2, 1);
}
#endif  // !BUILDFLAG(IS_ANDROID)

}  // namespace brave_rewards
