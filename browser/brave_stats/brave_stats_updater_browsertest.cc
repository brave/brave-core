/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/command_line.h"
#include "base/environment.h"
#include "base/files/file_util.h"
#include "base/path_service.h"
#include "base/time/time.h"
#include "brave/browser/brave_browser_process.h"
#include "brave/browser/brave_stats/brave_stats_updater.h"
#include "brave/browser/brave_stats/brave_stats_updater_params.h"
#include "brave/browser/brave_stats/switches.h"
#include "brave/common/pref_names.h"
#include "brave/components/brave_referrals/browser/brave_referrals_service.h"
#include "brave/components/brave_referrals/common/pref_names.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "components/prefs/testing_pref_service.h"
#include "content/public/test/browser_test.h"
#include "net/base/url_util.h"
#include "net/test/embedded_test_server/http_request.h"
#include "net/test/embedded_test_server/http_response.h"

namespace {

// Request handler for stats and referral updates. The response this returns
// doesn't represent a valid update server response, but it's sufficient for
// testing purposes as we're not interested in the contents of the
// response.
std::unique_ptr<net::test_server::HttpResponse> HandleRequestForStats(
    const net::test_server::HttpRequest& request) {
  std::unique_ptr<net::test_server::BasicHttpResponse> http_response(
      new net::test_server::BasicHttpResponse());
  if (request.relative_url == "/promo/initialize/nonua") {
    // We need a download id to make promo initialization happy
    http_response->set_code(net::HTTP_OK);
    http_response->set_content("{\"download_id\":\"keur123\"}");
    http_response->set_content_type("application/json");
  } else {
    http_response->set_code(net::HTTP_OK);
    http_response->set_content_type("text/html");
    http_response->set_content("<html><head></head></html>");
  }
  return std::move(http_response);
}

}  // anonymous namespace

class BraveStatsUpdaterBrowserTest : public InProcessBrowserTest {
 public:
  void SetUp() override {
    auto referral_initialized_callback = base::BindRepeating(
        &BraveStatsUpdaterBrowserTest::OnReferralInitialized,
        base::Unretained(this));
    brave::BraveReferralsService::SetReferralInitializedCallbackForTesting(
        &referral_initialized_callback);

    auto stats_updated_callback = base::BindRepeating(
        &BraveStatsUpdaterBrowserTest::OnStandardStatsUpdated,
        base::Unretained(this));
    brave_stats::BraveStatsUpdater::SetStatsUpdatedCallbackForTesting(
        &stats_updated_callback);

    auto stats_threshold_callback = base::BindRepeating(
        &BraveStatsUpdaterBrowserTest::OnThresholdStatsUpdated,
        base::Unretained(this));
    brave_stats::BraveStatsUpdater::SetStatsThresholdCallbackForTesting(
        &stats_threshold_callback);
    InProcessBrowserTest::SetUp();
  }

  void TearDown() override {
    brave::BraveReferralsService::SetReferralInitializedCallbackForTesting(
        nullptr);
    brave_stats::BraveStatsUpdater::SetStatsUpdatedCallbackForTesting(nullptr);
    brave_stats::BraveStatsUpdater::SetStatsThresholdCallbackForTesting(
        nullptr);
    InProcessBrowserTest::TearDown();
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    embedded_test_server()->RegisterRequestHandler(
        base::BindRepeating(&HandleRequestForStats));
    ASSERT_TRUE(embedded_test_server()->Start());
    command_line->AppendSwitchASCII(
        brave_stats::switches::kBraveStatsUpdaterServer,
        embedded_test_server()->base_url().spec());
    SetBaseUpdateURLForTest();
  }

  void SetBaseUpdateURLForTest() {
    std::unique_ptr<base::Environment> env(base::Environment::Create());
    env->SetVar("BRAVE_REFERRALS_SERVER",
                embedded_test_server()->host_port_pair().ToString());
    env->SetVar("BRAVE_REFERRALS_LOCAL", "1");  // use http for local testing
  }

  std::string GetUpdateURL() const { return update_url_; }

  void OnReferralInitialized(const std::string& referral_code) {
    if (wait_for_referral_initialized_loop_) {
      wait_for_referral_initialized_loop_->Quit();
    }

    on_referral_initialized_ = true;
    referral_code_ = referral_code;
  }

  void WaitForReferralInitializeCallback() {
    if (wait_for_referral_initialized_loop_ || on_referral_initialized_) {
      return;
    }

    wait_for_referral_initialized_loop_.reset(new base::RunLoop);
    wait_for_referral_initialized_loop_->Run();
  }

  void OnStandardStatsUpdated(const GURL& update_url) {
    if (wait_for_standard_stats_updated_loop_) {
      wait_for_standard_stats_updated_loop_->Quit();
    }

    on_standard_stats_updated_ = true;

    // We get //1/usage/brave-core here, so ignore the first slash.
    EXPECT_STREQ(update_url.path().c_str() + 1, "/1/usage/brave-core");
    update_url_ = update_url.spec();
  }

  void WaitForStandardStatsUpdatedCallback() {
    if (wait_for_standard_stats_updated_loop_ || on_standard_stats_updated_) {
      return;
    }

    wait_for_standard_stats_updated_loop_.reset(new base::RunLoop);
    wait_for_standard_stats_updated_loop_->Run();
  }

  void OnThresholdStatsUpdated(const GURL& update_url) {
    if (wait_for_threshold_stats_updated_loop_) {
      wait_for_threshold_stats_updated_loop_->Quit();
    }

    on_threshold_stats_updated_ = true;

    // We get //1/usage/brave-core-threshold here, so ignore the first slash.
    EXPECT_STREQ(update_url.path().c_str() + 1,
                 "/1/usage/brave-core-threshold");
    update_url_ = update_url.spec();
  }

  void WaitForThresholdStatsUpdatedCallback() {
    if (wait_for_threshold_stats_updated_loop_ || on_threshold_stats_updated_) {
      return;
    }

    wait_for_threshold_stats_updated_loop_.reset(new base::RunLoop);
    wait_for_threshold_stats_updated_loop_->Run();
  }

 private:
  std::unique_ptr<base::RunLoop> wait_for_referral_initialized_loop_;
  std::unique_ptr<base::RunLoop> wait_for_standard_stats_updated_loop_;
  std::unique_ptr<base::RunLoop> wait_for_threshold_stats_updated_loop_;

  std::string referral_code_;
  std::string update_url_;

  bool on_referral_initialized_ = false;
  bool on_standard_stats_updated_ = false;
  bool on_threshold_stats_updated_ = false;
};

// Run the stats updater and verify that it sets the first check preference
IN_PROC_BROWSER_TEST_F(BraveStatsUpdaterBrowserTest,
                       StatsUpdaterSetsFirstCheckPreference) {
  WaitForReferralInitializeCallback();
  WaitForStandardStatsUpdatedCallback();

  // First check preference should now be true
  EXPECT_TRUE(g_browser_process->local_state()->GetBoolean(kFirstCheckMade));
}

// Run the stats updater and verify the threshold endpoint is reached
IN_PROC_BROWSER_TEST_F(BraveStatsUpdaterBrowserTest,
                       StatsUpdaterThresholdSetsFirstCheckPreference) {
  EXPECT_TRUE(
      g_brave_browser_process->brave_stats_updater()->MaybeDoThresholdPing(3));

  WaitForReferralInitializeCallback();
  WaitForThresholdStatsUpdatedCallback();

  // First check and Threshold check should be set.
  EXPECT_TRUE(g_browser_process->local_state()->GetBoolean(kFirstCheckMade));
  EXPECT_TRUE(
      g_browser_process->local_state()->GetBoolean(kThresholdCheckMade));
}

// Run the stats updater with no active referral and verify that the
// update url specifies the default referral code
IN_PROC_BROWSER_TEST_F(BraveStatsUpdaterBrowserTest,
                       StatsUpdaterStartupPingWithDefaultReferralCode) {
  WaitForReferralInitializeCallback();
  WaitForStandardStatsUpdatedCallback();

  // Promo code file preference should now be true
  EXPECT_TRUE(
      g_browser_process->local_state()->GetBoolean(kReferralInitialization));

  // Verify that update url is valid
  const GURL update_url(GetUpdateURL());
  EXPECT_TRUE(update_url.is_valid());

  // Verify that daily parameter is true
  std::string query_value;
  EXPECT_TRUE(net::GetValueForKeyInQuery(update_url, "daily", &query_value));
  EXPECT_STREQ(query_value.c_str(), "true");

  // Verify that there is no referral code
  EXPECT_TRUE(net::GetValueForKeyInQuery(update_url, "ref", &query_value));
  EXPECT_STREQ(query_value.c_str(), "BRV001");
}

// TODO(bridiver) - convert to a unit test
IN_PROC_BROWSER_TEST_F(BraveStatsUpdaterBrowserTest,
                       DISABLED_StatsUpdaterMigration) {
  // Create a pre 1.19 user.
  // Has a download_id, kReferralCheckedForPromoCodeFile is set, has promo code.
  ASSERT_FALSE(
      g_browser_process->local_state()->GetBoolean(kReferralInitialization));
  g_browser_process->local_state()->SetString(kReferralDownloadID, "migration");
  g_browser_process->local_state()->SetString(kReferralPromoCode, "BRV001");
  g_browser_process->local_state()->SetBoolean(kReferralCheckedForPromoCodeFile,
                                               true);

  WaitForStandardStatsUpdatedCallback();
  // Verify that update url is valid
  const GURL update_url(GetUpdateURL());
  EXPECT_TRUE(update_url.is_valid());

  // Verify that daily parameter is true
  std::string query_value;
  EXPECT_TRUE(net::GetValueForKeyInQuery(update_url, "daily", &query_value));
  EXPECT_STREQ(query_value.c_str(), "true");

  // Verify that there is no referral code
  EXPECT_TRUE(net::GetValueForKeyInQuery(update_url, "ref", &query_value));
  EXPECT_STREQ(query_value.c_str(), "BRV001");
}

class BraveStatsUpdaterReferralCodeBrowserTest
    : public BraveStatsUpdaterBrowserTest {
 public:
  void SetUp() override {
    ASSERT_TRUE(dir.CreateUniqueTempDir());
    const base::FilePath promo_code_file =
        dir.GetPath().AppendASCII("promoCode");
    WritePromoCodeFile(promo_code_file, referral_code());
    brave::BraveReferralsService::SetPromoFilePathForTesting(promo_code_file);
    BraveStatsUpdaterBrowserTest::SetUp();
  }

  int WritePromoCodeFile(const base::FilePath& promo_code_file,
                         const std::string& referral_code) {
    base::ScopedAllowBlockingForTesting allow_blocking;
    return base::WriteFile(promo_code_file, referral_code.c_str(),
                           referral_code.size());
  }

  const std::string referral_code() { return "FOO123"; }

 private:
  base::ScopedTempDir dir;
};

// Run the stats updater with an active referral and verify that the
// update url includes the referral code
IN_PROC_BROWSER_TEST_F(BraveStatsUpdaterReferralCodeBrowserTest,
                       StatsUpdaterStartupPingWithReferralCode) {
  WaitForReferralInitializeCallback();
  WaitForStandardStatsUpdatedCallback();

  // Promo code file preference should now be true
  EXPECT_TRUE(
      g_browser_process->local_state()->GetBoolean(kReferralInitialization));

  // Verify that update url is valid
  const GURL update_url(GetUpdateURL());
  EXPECT_TRUE(update_url.is_valid());

  // Verify that daily parameter is true
  std::string query_value;
  EXPECT_TRUE(net::GetValueForKeyInQuery(update_url, "daily", &query_value));
  EXPECT_STREQ(query_value.c_str(), "true");

  // Verify that the expected referral code is present
  EXPECT_TRUE(net::GetValueForKeyInQuery(update_url, "ref", &query_value));
  EXPECT_STREQ(query_value.c_str(), referral_code().c_str());
}
