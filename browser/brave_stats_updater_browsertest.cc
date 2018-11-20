/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_stats_updater.h"
#include "brave/common/pref_names.h"
#include "brave/components/brave_referrals/browser/brave_referrals_service.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "components/prefs/testing_pref_service.h"
#include "net/test/embedded_test_server/http_response.h"

namespace {

// Request handler for stats updates. The response this returns doesn't
// represent a valid update server response, but it's sufficient for
// testing purposes as we're not interested in the contents of the
// response.
std::unique_ptr<net::test_server::HttpResponse> HandleRequestForStats(
    const net::test_server::HttpRequest& request) {
  std::unique_ptr<net::test_server::BasicHttpResponse> http_response(
      new net::test_server::BasicHttpResponse());
  http_response->set_code(net::HTTP_OK);
  http_response->set_content_type("text/html");
  http_response->set_content("<html><head></head></html>");
  return std::move(http_response);
}

}  // anonymous namespace

class BraveStatsUpdaterBrowserTest : public InProcessBrowserTest {
 public:
  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    brave::RegisterPrefsForBraveStatsUpdater(testing_local_state_.registry());
    brave::RegisterPrefsForBraveReferralsService(testing_local_state_.registry());
    InitEmbeddedTestServer();
  }

  void InitEmbeddedTestServer() {
    embedded_test_server()->RegisterRequestHandler(
        base::Bind(&HandleRequestForStats));
    ASSERT_TRUE(embedded_test_server()->Start());
  }

  PrefService* GetLocalState() { return &testing_local_state_; }

  void SetBaseUpdateURLForTest(const GURL& base_update_url) {
    brave::BraveStatsUpdater::SetBaseUpdateURLForTest(base_update_url);
  }

  void StatsUpdated() {
    was_called_ = true;
    wait_for_callback_loop_->Quit();
  }

  void WaitForStatsUpdatedCallback() {
    if (was_called_)
      return;
    wait_for_callback_loop_.reset(new base::RunLoop);
    wait_for_callback_loop_->Run();
  }

 private:
  TestingPrefServiceSimple testing_local_state_;
  std::unique_ptr<base::RunLoop> wait_for_callback_loop_;
  bool was_called_ = false;
};

// Run the stats updater and verify that it sets the first check preference
IN_PROC_BROWSER_TEST_F(BraveStatsUpdaterBrowserTest, StatsUpdaterSetsFirstCheckPreference) {
  GURL base_update_url = embedded_test_server()->GetURL("/1/usage/brave-core");
  SetBaseUpdateURLForTest(base_update_url);

  brave::BraveStatsUpdater stats_updater(GetLocalState());
  stats_updater.SetStatsUpdatedCallback(base::BindRepeating(
      &BraveStatsUpdaterBrowserTest::StatsUpdated, base::Unretained(this)));

  // Ensure that first check preference is false
  ASSERT_FALSE(GetLocalState()->GetBoolean(kFirstCheckMade));

  // Start the stats updater, wait for it to perform its startup ping,
  // and then shut it down
  stats_updater.Start();
  WaitForStatsUpdatedCallback();
  stats_updater.Stop();

  // First check preference should now be true
  EXPECT_TRUE(GetLocalState()->GetBoolean(kFirstCheckMade));
}
