/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/path_service.h"
#include "base/run_loop.h"
#include "bat/ledger/internal/bat_helper.h"
#include "bat/ledger/internal/static_values.h"
#include "bat/ledger/ledger.h"
#include "brave/browser/ui/views/brave_actions/brave_actions_container.h"
#include "brave/browser/ui/views/location_bar/brave_location_bar_view.h"
#include "brave/common/brave_paths.h"
#include "brave/common/extensions/extension_constants.h"
#include "brave/components/brave_rewards/browser/rewards_service_factory.h"
#include "brave/components/brave_rewards/browser/rewards_service_impl.h"
#include "brave/components/brave_rewards/browser/rewards_service_observer.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/notification_types.h"
#include "content/public/test/browser_test_utils.h"
#include "google_apis/gaia/mock_url_fetcher_factory.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/http_response.h"
#include "net/url_request/url_fetcher_delegate.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using braveledger_bat_helper::SERVER_TYPES;

namespace {

std::unique_ptr<net::test_server::HttpResponse> HandleRequest(
    const net::test_server::HttpRequest& request) {
  std::unique_ptr<net::test_server::BasicHttpResponse> http_response(
      new net::test_server::BasicHttpResponse());
  http_response->set_code(net::HTTP_OK);
  http_response->set_content_type("text/html");
  http_response->set_content(
      "<html><head></head><body><div>Hello, world!</div></body></html>");
  return std::move(http_response);
}

}  // namespace

namespace brave_test_resp {
  std::string registrarVK_;
  std::string verification_;
  std::string wallet_;
  std::string grant_;
  std::string grant_v4_;
  std::string captcha_;
  std::string captcha_solution_;
  std::string contribution_;
  std::string reconcile_;
  std::string current_reconcile_;
  std::string register_;
  std::string register_credential_;
  std::string surveyor_voting_;
  std::string surveyor_voting_credential_;
}  // namespace brave_test_resp

namespace brave_net {
class BraveURLFetcher : public net::TestURLFetcher {
 public:
  BraveURLFetcher(bool success,
                  const GURL& url,
                  const std::string& results,
                  net::URLFetcher::RequestType request_type,
                  net::URLFetcherDelegate* d);
  ~BraveURLFetcher() override;

  void Start() override;
  void DetermineURLResponsePath(const std::string& url);

 private:
  void RunDelegate();

  base::WeakPtrFactory<BraveURLFetcher> weak_factory_{this};
  DISALLOW_COPY_AND_ASSIGN(BraveURLFetcher);
};

void split(std::vector<std::string>* tmp,
           const std::string& query,
           char delimiter) {
  DCHECK(tmp);
  std::stringstream ss(query);
  std::string item;
  while (std::getline(ss, item, delimiter)) {
    if (query[0] != '\n') {
      tmp->push_back(item);
    }
  }
}

bool URLMatches(const std::string& url,
                const std::string& path,
                const std::string& prefix,
                const SERVER_TYPES& server) {
  const std::string target_url =
      braveledger_bat_helper::buildURL(path, prefix, server);
  return (url.find(target_url) == 0);
}

void BraveURLFetcher::DetermineURLResponsePath(const std::string& url) {
  std::vector<std::string> tmp;
  brave_net::split(&tmp, url, '/');
  if (url.find(braveledger_bat_helper::buildURL(REGISTER_PERSONA, PREFIX_V2,
    braveledger_bat_helper::SERVER_TYPES::LEDGER)) == 0
    && tmp.size() == 6) {
    SetResponseString(brave_test_resp::registrarVK_);
  } else if (URLMatches(url, REGISTER_PERSONA, PREFIX_V2,
                        SERVER_TYPES::LEDGER) &&
             tmp.size() == 7) {
    SetResponseString(brave_test_resp::verification_);
  } else if (URLMatches(url, WALLET_PROPERTIES, PREFIX_V2,
                        SERVER_TYPES::BALANCE)) {
    SetResponseString(brave_test_resp::wallet_);
  } else if (URLMatches(url, WALLET_PROPERTIES, PREFIX_V2,
                        SERVER_TYPES::LEDGER)) {
    GURL gurl(url);
    if (gurl.has_query())
      SetResponseString(brave_test_resp::reconcile_);
    else
      SetResponseString(brave_test_resp::current_reconcile_);
  } else if (URLMatches(url, GET_SET_PROMOTION, PREFIX_V2,
                        SERVER_TYPES::LEDGER)) {
    GURL gurl(url);
    if (gurl.has_query())
      SetResponseString(brave_test_resp::grant_);
    else
      SetResponseString(brave_test_resp::captcha_solution_);
  } else if (URLMatches(url, GET_SET_PROMOTION, PREFIX_V4,
                        SERVER_TYPES::LEDGER)) {
    SetResponseString(brave_test_resp::grant_v4_);
  } else if (URLMatches(url, GET_PROMOTION_CAPTCHA, PREFIX_V4,
                        SERVER_TYPES::LEDGER)) {
    // The hint we use doesn't matter since we mock the server's
    // responses anyway, but ledger verifies that the response headers contain
    // a hint so we must add one
    scoped_refptr<net::HttpResponseHeaders> http_response_headers(
        new net::HttpResponseHeaders(""));
    http_response_headers->AddHeader("Captcha-Hint: Triangle");
    set_response_headers(http_response_headers);
    SetResponseString(brave_test_resp::captcha_);
  } else if (URLMatches(url, RECONCILE_CONTRIBUTION, PREFIX_V2,
                        SERVER_TYPES::LEDGER)) {
    SetResponseString(brave_test_resp::contribution_);
  } else if (URLMatches(url, REGISTER_VIEWING, PREFIX_V2,
                        SERVER_TYPES::LEDGER)) {
    if (url.find(REGISTER_VIEWING "/") != std::string::npos)
      SetResponseString(brave_test_resp::register_credential_);
    else
      SetResponseString(brave_test_resp::register_);
  } else if (URLMatches(url, SURVEYOR_BATCH_VOTING, PREFIX_V2,
                        SERVER_TYPES::LEDGER)) {
    if (url.find(SURVEYOR_BATCH_VOTING "/") != std::string::npos)
      SetResponseString(brave_test_resp::surveyor_voting_credential_);
    else
      SetResponseString(brave_test_resp::surveyor_voting_);
  } else if (URLMatches(url, GET_PUBLISHERS_LIST_V1, "",
                        SERVER_TYPES::PUBLISHER_DISTRO)) {
    SetResponseString("[[\"duckduckgo.com\",true,false]]");
  }
}

BraveURLFetcher::BraveURLFetcher(bool success,
  const GURL& url,
  const std::string& results,
  net::URLFetcher::RequestType request_type,
  net::URLFetcherDelegate* d)
  : net::TestURLFetcher(0, url, d) {
  set_url(url);
  set_status(net::URLRequestStatus(net::URLRequestStatus::SUCCESS, 0));
  set_response_code(net::HTTP_OK);
  DetermineURLResponsePath(url.spec());
}

BraveURLFetcher::~BraveURLFetcher() = default;

void BraveURLFetcher::Start() {
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE,
      base::BindOnce(&BraveURLFetcher::RunDelegate,
      weak_factory_.GetWeakPtr()));
}

void BraveURLFetcher::RunDelegate() {
  delegate()->OnURLFetchComplete(this);
}

}  // namespace brave_net

class BraveRewardsBrowserTest : public InProcessBrowserTest,
                                public brave_rewards::RewardsServiceObserver {
 public:
  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    host_resolver()->AddRule("*", "127.0.0.1");
    content::SetupCrossSiteRedirector(embedded_test_server());
    InitEmbeddedTestServer();
    brave::RegisterPathProvider();
    ReadTestData();
    rewards_service_ = static_cast<brave_rewards::RewardsServiceImpl*>(
        brave_rewards::RewardsServiceFactory::GetForProfile(
            browser()->profile()));
    rewards_service_->SetLedgerEnvForTesting();
  }

  void TearDown() override {
    InProcessBrowserTest::TearDown();
  }

  void InitEmbeddedTestServer() {
    embedded_test_server()->RegisterRequestHandler(base::Bind(&HandleRequest));
    ASSERT_TRUE(embedded_test_server()->Start());
  }

  void RunUntilIdle() {
    base::RunLoop loop;
    loop.RunUntilIdle();
  }

  void WaitForWalletInitialization() {
    if (wallet_initialized_)
      return;
    wait_for_wallet_initialization_loop_.reset(new base::RunLoop);
    wait_for_wallet_initialization_loop_->Run();
  }

  void WaitForGrantInitialization() {
    if (grant_initialized_)
      return;
    wait_for_grant_initialization_loop_.reset(new base::RunLoop);
    wait_for_grant_initialization_loop_->Run();
  }

  void WaitForGrantFinished() {
    if (grant_finished_)
      return;
    wait_for_grant_finished_loop_.reset(new base::RunLoop);
    wait_for_grant_finished_loop_->Run();
  }

  void WaitForCaptcha() {
    if (captcha_received_)
      return;
    wait_for_captcha_loop_.reset(new base::RunLoop);
    wait_for_captcha_loop_->Run();
  }

  void WaitForPublisherListNormalized() {
    if (publisher_list_normalized_)
      return;
    wait_for_publisher_list_normalized_loop_.reset(new base::RunLoop);
    wait_for_publisher_list_normalized_loop_->Run();
  }

  void WaitForReconcileCompleted() {
    if (reconcile_completed_)
      return;
    wait_for_reconcile_completed_loop_.reset(new base::RunLoop);
    wait_for_reconcile_completed_loop_->Run();
  }

  void GetReconcileTime() {
    rewards_service()->GetReconcileTime(
        base::Bind(&BraveRewardsBrowserTest::OnGetReconcileTime,
          base::Unretained(this)));
  }

  void GetShortRetries() {
    rewards_service()->GetShortRetries(
        base::Bind(&BraveRewardsBrowserTest::OnGetShortRetries,
          base::Unretained(this)));
  }

  void GetProduction() {
    rewards_service()->GetProduction(
        base::Bind(&BraveRewardsBrowserTest::OnGetProduction,
          base::Unretained(this)));
  }

  void GetDebug() {
    rewards_service()->GetDebug(
        base::Bind(&BraveRewardsBrowserTest::OnGetDebug,
          base::Unretained(this)));
  }

  content::WebContents* OpenRewardsPopup() {
    // Construct an observer to wait for the popup to load
    content::WindowedNotificationObserver popup_observer(
        content::NOTIFICATION_LOAD_COMPLETED_MAIN_FRAME,
        content::NotificationService::AllSources());

    // Click on the Rewards button
    BraveLocationBarView* brave_location_bar_view =
        static_cast<BraveLocationBarView*>(
            BrowserView::GetBrowserViewForBrowser(browser())
                ->GetLocationBarView());
    BraveActionsContainer* brave_actions_container =
        brave_location_bar_view->brave_actions_;
    EXPECT_TRUE(brave_actions_container->actions_.at(brave_rewards_extension_id)
                    .view_controller_->ExecuteAction(true));

    // Wait for the popup to load
    popup_observer.Wait();

    // Retrieve the notification source
    const auto& source =
        static_cast<const content::Source<content::WebContents>&>(
            popup_observer.source());

    return source.ptr();
  }

  void GetTestDataDir(base::FilePath* test_data_dir) {
    base::ScopedAllowBlockingForTesting allow_blocking;
    ASSERT_TRUE(base::PathService::Get(brave::DIR_TEST_DATA, test_data_dir));
    *test_data_dir = test_data_dir->AppendASCII("rewards-data");
    ASSERT_TRUE(base::PathExists(*test_data_dir));
  }

  void ReadTestData() {
    base::ScopedAllowBlockingForTesting allow_blocking;
    base::FilePath path;
    GetTestDataDir(&path);
    ASSERT_TRUE(
        base::ReadFileToString(path.AppendASCII("register_persona_resp.json"),
                               &brave_test_resp::registrarVK_));
    ASSERT_TRUE(
        base::ReadFileToString(path.AppendASCII("verify_persona_resp.json"),
                               &brave_test_resp::verification_));
    ASSERT_TRUE(base::ReadFileToString(
        path.AppendASCII("wallet_balance_empty_resp.json"),
        &brave_test_resp::wallet_));
    ASSERT_TRUE(base::ReadFileToString(path.AppendASCII("ugp_grant_resp.json"),
                                       &brave_test_resp::grant_));
    ASSERT_TRUE(
        base::ReadFileToString(path.AppendASCII("ugp_grant_v4_resp.json"),
                               &brave_test_resp::grant_v4_));
    ASSERT_TRUE(base::ReadFileToString(path.AppendASCII("captcha_resp.png"),
                                       &brave_test_resp::captcha_));
    ASSERT_TRUE(
        base::ReadFileToString(path.AppendASCII("captcha_solution_resp.json"),
                               &brave_test_resp::captcha_solution_));
    ASSERT_TRUE(
        base::ReadFileToString(path.AppendASCII("contribution_resp.json"),
                               &brave_test_resp::contribution_));
    ASSERT_TRUE(base::ReadFileToString(path.AppendASCII("reconcile_resp.json"),
                                       &brave_test_resp::reconcile_));
    ASSERT_TRUE(
        base::ReadFileToString(path.AppendASCII("current_reconcile_resp.json"),
                               &brave_test_resp::current_reconcile_));
    ASSERT_TRUE(base::ReadFileToString(path.AppendASCII("register_resp.json"),
                                       &brave_test_resp::register_));
    ASSERT_TRUE(base::ReadFileToString(
        path.AppendASCII("register_credential_resp.json"),
        &brave_test_resp::register_credential_));
    ASSERT_TRUE(
        base::ReadFileToString(path.AppendASCII("surveyor_voting_resp.json"),
                               &brave_test_resp::surveyor_voting_));
    ASSERT_TRUE(base::ReadFileToString(
        path.AppendASCII("surveyor_voting_credential_resp.json"),
        &brave_test_resp::surveyor_voting_credential_));
  }

  void UpdateTestData() {
    base::ScopedAllowBlockingForTesting allow_blocking;
    base::FilePath path;
    GetTestDataDir(&path);
    if (grant_finished_) {
      if (contribution_made_) {
        ASSERT_TRUE(base::ReadFileToString(
            path.AppendASCII("wallet_balance_contributed_resp.json"),
            &brave_test_resp::wallet_));
      } else if (donation_made_) {
        ASSERT_TRUE(base::ReadFileToString(
            path.AppendASCII("wallet_balance_donated_resp.json"),
            &brave_test_resp::wallet_));
      } else {
        ASSERT_TRUE(base::ReadFileToString(
            path.AppendASCII("wallet_balance_funded_resp.json"),
            &brave_test_resp::wallet_));
      }
    } else {
      ASSERT_TRUE(base::ReadFileToString(
          path.AppendASCII("wallet_balance_empty_resp.json"),
          &brave_test_resp::wallet_));
    }
  }

  GURL rewards_url() {
    GURL rewards_url("brave://rewards");
    return rewards_url;
  }

  content::WebContents* contents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  void EnableRewards() {
    // Load rewards page
    ui_test_utils::NavigateToURL(browser(), rewards_url());
    WaitForLoadStop(contents());
    // Opt in and create wallet to enable rewards
    ASSERT_TRUE(ExecJs(contents(),
      "document.querySelector(\"[data-test-id='optInAction']\").click();",
      content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
      content::ISOLATED_WORLD_ID_CONTENT_END));
    content::EvalJsResult jsResult = EvalJs(contents(),
      "new Promise((resolve) => {"
      "var count = 10;"
      "var interval = setInterval(function() {"
      "  if (count == 0) {"
      "    clearInterval(interval);"
      "    resolve(false);"
      "  } else {"
      "    count -= 1;"
      "  }"
      "  if (document.querySelector(\"[data-test-id2='enableMain']\")) {"
      "    clearInterval(interval);"
      "    resolve(true);"
      "  }"
      "}, 500);});",
      content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
      content::ISOLATED_WORLD_ID_CONTENT_END);
    ASSERT_TRUE(jsResult.ExtractBool());
  }

  brave_rewards::RewardsServiceImpl* rewards_service() {
    return rewards_service_;
  }

  void ClaimGrant(bool use_panel) {
    // Wait for grant to initialize
    WaitForGrantInitialization();

    // Use the appropriate WebContents
    content::WebContents* contents =
        use_panel ? OpenRewardsPopup() : BraveRewardsBrowserTest::contents();
    ASSERT_TRUE(contents);

    // Claim grant via settings page or panel, as instructed
    if (use_panel) {
      ASSERT_TRUE(ExecJs(contents,
                         "document.getElementsByTagName('button')[0].click();",
                         content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
                         content::ISOLATED_WORLD_ID_CONTENT_END));
    } else {
      ASSERT_TRUE(ExecJs(
          contents,
          "document.querySelector(\"[data-test-id='claimGrant']\").click();",
          content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
          content::ISOLATED_WORLD_ID_CONTENT_END));
    }

    // Wait for CAPTCHA
    WaitForCaptcha();

    // Solve the CAPTCHA (response is mocked, so the contents of the
    // solution aren't important)
    const std::string promotion_id = "d9033f51-6d83-4d19-a016-1c11f693f147";
    rewards_service_->SolveGrantCaptcha("{\"x\":1,\"y\":1}", promotion_id);

    // Wait for grant to finish
    WaitForGrantFinished();

    // Dismiss the grant notification
    if (use_panel) {
      ASSERT_TRUE(ExecJs(contents,
                         "document.getElementsByTagName('button')[0].click();",
                         content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
                         content::ISOLATED_WORLD_ID_CONTENT_END));
    }

    // Ensure that grant looks as expected
    EXPECT_STREQ(grant_.altcurrency.c_str(), "BAT");
    EXPECT_STREQ(grant_.probi.c_str(), "30000000000000000000");
    EXPECT_STREQ(grant_.promotionId.c_str(), promotion_id.c_str());
    EXPECT_STREQ(grant_.type.c_str(), "ugp");

    // Check that grant notification shows the appropriate amount
    const std::string selector =
        use_panel ? "[id='root']" : "[data-test-id='newTokenGrant']";
    content::EvalJsResult js_result = EvalJs(
        contents,
        content::JsReplace(
            "const delay = t => new Promise(resolve => setTimeout(resolve, t));"
            "delay(0).then(() => "
            "  document.querySelector($1).innerText);",
            selector),
        content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
        content::ISOLATED_WORLD_ID_CONTENT_END);
    EXPECT_NE(js_result.ExtractString().find("Free Token Grant"),
              std::string::npos);
    EXPECT_NE(js_result.ExtractString().find("30.0 BAT"), std::string::npos);
  }

  void VisitPublisher(const std::string& publisher, bool verified) {
    GURL url = embedded_test_server()->GetURL(publisher, "/index.html");
    ui_test_utils::NavigateToURLWithDisposition(
        browser(), url, WindowOpenDisposition::NEW_FOREGROUND_TAB,
        ui_test_utils::BROWSER_TEST_WAIT_FOR_NAVIGATION);

    // The minimum publisher duration when testing is 1 second (and the
    // granularity is seconds), so wait for just over 2 seconds to elapse
    base::PlatformThread::Sleep(base::TimeDelta::FromMilliseconds(2100));

    // Activate the Rewards settings page tab
    browser()->tab_strip_model()->ActivateTabAt(
        0,
        TabStripModel::UserGestureDetails(TabStripModel::GestureType::kOther));

    // Wait for publisher list normalization
    WaitForPublisherListNormalized();

    // Make sure site appears in auto-contribute table
    content::EvalJsResult js_result = EvalJs(
        contents(),
        "const delay = t => new Promise(resolve => setTimeout(resolve, t));"
        "delay(1000).then(() => "
        "  document.querySelector(\"[data-test-id='autoContribute']\")."
        "    getElementsByTagName('a')[0].innerText);",
        content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
        content::ISOLATED_WORLD_ID_CONTENT_END);
    EXPECT_STREQ(js_result.ExtractString().c_str(), publisher.c_str());

    if (verified) {
      // A verified site has two images associated with it, the site's
      // favicon and the verified icon
      content::EvalJsResult js_result =
          EvalJs(contents(),
                 "document.querySelector(\"[data-test-id='autoContribute']\")."
                 "    getElementsByTagName('svg').length === 2;",
                 content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
                 content::ISOLATED_WORLD_ID_CONTENT_END);
      EXPECT_TRUE(js_result.ExtractBool());
    } else {
      // An unverified site has one image associated with it, the site's
      // favicon
      content::EvalJsResult js_result =
          EvalJs(contents(),
                 "document.querySelector(\"[data-test-id='autoContribute']\")."
                 "    getElementsByTagName('svg').length === 1;",
                 content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
                 content::ISOLATED_WORLD_ID_CONTENT_END);
      EXPECT_TRUE(js_result.ExtractBool());
    }
  }

  void TipPublisher(const std::string& publisher, bool verified, bool monthly) {
    // Claim grant using settings page
    const bool use_panel = true;
    ClaimGrant(use_panel);

    // Navigate to a verified site in a new tab
    GURL url = embedded_test_server()->GetURL(publisher, "/index.html");
    ui_test_utils::NavigateToURLWithDisposition(
        browser(), url, WindowOpenDisposition::NEW_FOREGROUND_TAB,
        ui_test_utils::BROWSER_TEST_WAIT_FOR_NAVIGATION);

    // The minimum publisher duration when testing is 1 second (and the
    // granularity is seconds), so wait for just over 2 seconds to elapse
    base::PlatformThread::Sleep(base::TimeDelta::FromMilliseconds(2100));

    // Open the Rewards popup
    content::WebContents* popup_contents = OpenRewardsPopup();
    ASSERT_TRUE(popup_contents);

    // Construct an observer to wait for the site banner to load
    content::WindowedNotificationObserver site_banner_observer(
        content::NOTIFICATION_LOAD_COMPLETED_MAIN_FRAME,
        content::NotificationService::AllSources());

    // Click button to initiate sending a tip
    ASSERT_TRUE(ExecJs(
        popup_contents,
        "const delay = t => new Promise(resolve => setTimeout(resolve, t));"
        "delay(0).then(() => "
        "  document.querySelector(\"[type='tip']\").click());",
        content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
        content::ISOLATED_WORLD_ID_CONTENT_END));

    // Wait for the site banner to load
    site_banner_observer.Wait();

    // Retrieve the notification source
    const auto& site_banner_source =
        static_cast<const content::Source<content::WebContents>&>(
            site_banner_observer.source());

    content::WebContents* site_banner_contents = site_banner_source.ptr();
    ASSERT_TRUE(site_banner_contents);

    // Select the tip amount (1 BAT)
    // TODO(emerick): Better to do this by data-test-id
    ASSERT_TRUE(ExecJs(
        site_banner_contents,
        "const delay = t => new Promise(resolve => setTimeout(resolve, t));"
        "delay(0).then(() => "
        "  document.getElementsByTagName('button')[1].click());",
        content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
        content::ISOLATED_WORLD_ID_CONTENT_END));

    // Make this tip monthly, if requested
    if (monthly) {
      ASSERT_TRUE(ExecJs(
          site_banner_contents,
          "const delay = t => new Promise(resolve => setTimeout(resolve, t));"
          "delay(0).then(() => "
          "  document.querySelector(\"[data-test-id='monthlyCheckbox']\")"
          "    .children[0].click());",
          content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
          content::ISOLATED_WORLD_ID_CONTENT_END));
    }

    // Send the tip
    // TODO(emerick): Better to do this by data-test-id
    ASSERT_TRUE(ExecJs(
        site_banner_contents,
        "const delay = t => new Promise(resolve => setTimeout(resolve, t));"
        "delay(0).then(() => "
        "  document.getElementsByTagName('button')[4].click());",
        content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
        content::ISOLATED_WORLD_ID_CONTENT_END));

    // Signal that direct donation was made and update wallet with new
    // balance
    if (!monthly) {
      donation_made_ = true;
      UpdateTestData();
    }

    // Wait for thank you banner to load
    ASSERT_TRUE(WaitForLoadStop(site_banner_contents));

    // Make sure that thank you banner shows correct publisher data
    // (domain and amount)
    {
      content::EvalJsResult js_result = EvalJs(
          site_banner_contents,
          "const delay = t => new Promise(resolve => setTimeout(resolve, t));"
          "delay(0).then(() => "
          "  document.documentElement.innerText);",
          content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
          content::ISOLATED_WORLD_ID_CONTENT_END);
      EXPECT_NE(js_result.ExtractString().find(publisher), std::string::npos);
      EXPECT_NE(js_result.ExtractString().find("1.0 BAT"), std::string::npos);
      if (monthly) {
        EXPECT_NE(js_result.ExtractString().find(
                      "Your first monthly tip will be sent on"),
                  std::string::npos);
      }
    }

    // Activate the Rewards settings page tab
    browser()->tab_strip_model()->ActivateTabAt(
        0,
        TabStripModel::UserGestureDetails(TabStripModel::GestureType::kOther));

    // Wait for publisher list normalization
    WaitForPublisherListNormalized();

    if (monthly) {
      // Trigger auto contribution now, for monthly
      rewards_service()->StartAutoContributeForTest();

      // Wait for reconciliation to complete
      WaitForReconcileCompleted();
      ASSERT_EQ(reconcile_status_, verified ? ledger::Result::LEDGER_OK
                                            : ledger::Result::AC_TABLE_EMPTY);
    }

    // Signal that monthly contribution was made and update wallet
    // with new balance
    if (monthly) {
      contribution_made_ = true;
      UpdateTestData();
    }

    if (verified) {
      // Make sure that balance is updated correctly
      {
        const std::string balance = monthly ? "10.0 BAT" : "29.0 BAT";
        content::EvalJsResult js_result = EvalJs(
            contents(),
            "const delay = t => new Promise(resolve => setTimeout(resolve, t));"
            "delay(1000).then(() => "
            "  document.querySelector(\"[data-test-id='balance']\")"
            "    .innerText);",
            content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
            content::ISOLATED_WORLD_ID_CONTENT_END);
        EXPECT_NE(js_result.ExtractString().find(balance), std::string::npos);
      }

      // Check that tip table shows the appropriate tip amount
      {
        const std::string selector =
            monthly ? "[color='contribute']" : "[color='donation']";
        content::EvalJsResult js_result = EvalJs(
            contents(),
            content::JsReplace(
                "const delay = t => new Promise(resolve => setTimeout(resolve, "
                "t));"
                "delay(0).then(() => "
                "  document.querySelector($1).innerText);",
                selector),
            content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
            content::ISOLATED_WORLD_ID_CONTENT_END);
        EXPECT_NE(js_result.ExtractString().find("-1.0BAT"), std::string::npos);
      }
    } else {
      // Make sure that balance did not change
      {
        content::EvalJsResult js_result = EvalJs(
            contents(),
            "const delay = t => new Promise(resolve => setTimeout(resolve, t));"
            "delay(1000).then(() => "
            "  document.querySelector(\"[data-test-id='balance']\")"
            "    .innerText);",
            content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
            content::ISOLATED_WORLD_ID_CONTENT_END);
        EXPECT_NE(js_result.ExtractString().find("30.0 BAT"),
                  std::string::npos);
      }

      // Make sure that pending contribution box shows the correct
      // amount
      {
        const std::string amount = monthly ? "20" : "1";
        content::EvalJsResult js_result = EvalJs(
            contents(),
            "const delay = t => new Promise(resolve => setTimeout(resolve, t));"
            "delay(0).then(() => "
            "  document.querySelector(\"[id='root']\").innerText);",
            content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
            content::ISOLATED_WORLD_ID_CONTENT_END);
        EXPECT_NE(js_result.ExtractString().find(
                      "You\u2019ve designated " + amount +
                      " BAT for creators who haven\u2019t yet signed up to "
                      "receive contributions."),
                  std::string::npos);
      }

      // Check that tip table shows no tip
      {
        content::EvalJsResult js_result = EvalJs(
            contents(),
            "const delay = t => new Promise(resolve => setTimeout(resolve, t));"
            "delay(0).then(() => "
            "  document.querySelector(\"[type='donation']\")"
            "    .parentElement.parentElement.innerText);",
            content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
            content::ISOLATED_WORLD_ID_CONTENT_END);
        EXPECT_NE(
            js_result.ExtractString().find("Total tips this month\n0.0BAT"),
            std::string::npos);
      }
    }
  }

  void OnWalletInitialized(brave_rewards::RewardsService* rewards_service,
                           int result) {
    ASSERT_TRUE(result == ledger::Result::WALLET_CREATED ||
                result == ledger::Result::NO_LEDGER_STATE);
    wallet_initialized_ = true;
    if (wait_for_wallet_initialization_loop_)
      wait_for_wallet_initialization_loop_->Quit();
  }

  void OnGrant(brave_rewards::RewardsService* rewards_service,
               unsigned int result,
               brave_rewards::Grant properties) {
    ASSERT_EQ(result, ledger::Result::LEDGER_OK);
    grant_initialized_ = true;
    if (wait_for_grant_initialization_loop_)
      wait_for_grant_initialization_loop_->Quit();
  }

  void OnGrantCaptcha(brave_rewards::RewardsService* rewards_service,
                      std::string image,
                      std::string hint) {
    captcha_received_ = true;
    if (wait_for_captcha_loop_)
      wait_for_captcha_loop_->Quit();
  }

  void OnGrantFinish(brave_rewards::RewardsService* rewards_service,
                     unsigned int result,
                     brave_rewards::Grant grant) {
    ASSERT_EQ(result, ledger::Result::LEDGER_OK);
    grant_finished_ = true;
    grant_ = grant;
    UpdateTestData();
    if (wait_for_grant_finished_loop_)
      wait_for_grant_finished_loop_->Quit();
  }

  void OnPublisherListNormalized(brave_rewards::RewardsService* rewards_service,
                                 brave_rewards::ContentSiteList list) {
    if (list.size() == 0)
      return;
    publisher_list_normalized_ = true;
    if (wait_for_publisher_list_normalized_loop_)
      wait_for_publisher_list_normalized_loop_->Quit();
  }

  void OnReconcileComplete(brave_rewards::RewardsService* rewards_service,
                           unsigned int result,
                           const std::string& viewing_id,
                           const std::string& category,
                           const std::string& probi) {
    reconcile_completed_ = true;
    reconcile_status_ = result;
    if (wait_for_reconcile_completed_loop_)
      wait_for_reconcile_completed_loop_->Quit();
  }

  MOCK_METHOD1(OnGetProduction, void(bool));
  MOCK_METHOD1(OnGetDebug, void(bool));
  MOCK_METHOD1(OnGetReconcileTime, void(int32_t));
  MOCK_METHOD1(OnGetShortRetries, void(bool));

  brave_rewards::RewardsServiceImpl* rewards_service_;

  brave_rewards::Grant grant_;

  MockURLFetcherFactory<brave_net::BraveURLFetcher> factory;

  std::unique_ptr<base::RunLoop> wait_for_wallet_initialization_loop_;
  bool wallet_initialized_ = false;

  std::unique_ptr<base::RunLoop> wait_for_grant_initialization_loop_;
  bool grant_initialized_ = false;

  std::unique_ptr<base::RunLoop> wait_for_grant_finished_loop_;
  bool grant_finished_ = false;

  std::unique_ptr<base::RunLoop> wait_for_captcha_loop_;
  bool captcha_received_ = false;

  std::unique_ptr<base::RunLoop> wait_for_publisher_list_normalized_loop_;
  bool publisher_list_normalized_ = false;

  std::unique_ptr<base::RunLoop> wait_for_reconcile_completed_loop_;
  bool reconcile_completed_ = false;
  unsigned int reconcile_status_ = ledger::LEDGER_ERROR;

  bool contribution_made_ = false;
  bool donation_made_ = false;
};

IN_PROC_BROWSER_TEST_F(BraveRewardsBrowserTest, RenderWelcome) {
  // Enable Rewards
  EnableRewards();
  EXPECT_STREQ(contents()->GetLastCommittedURL().spec().c_str(),
      // actual url is always chrome://
      "chrome://rewards/");
}

IN_PROC_BROWSER_TEST_F(BraveRewardsBrowserTest, ToggleRewards) {
  // Enable Rewards
  EnableRewards();

  // Toggle rewards off
  content::EvalJsResult toggleOffResult = EvalJs(contents(),
      "document.querySelector(\"[data-test-id2='enableMain']\").click();"
      "new Promise((resolve) => {"
      "var count = 10;"
      "var interval = setInterval(function() {"
      "  if (count == 0) {"
      "    clearInterval(interval);"
      "    resolve(false);"
      "  } else {"
      "    count -= 1;"
      "  }"
      "  if (document.querySelector(\"[data-test-id2='enableMain']\")) {"
      "    clearInterval(interval);"
      "    resolve(document.querySelector(\"[data-test-id2='enableMain']\")"
      "      .getAttribute(\"data-toggled\") === 'false');"
      "  }"
      "}, 500);});",
      content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
      content::ISOLATED_WORLD_ID_CONTENT_END);
  ASSERT_TRUE(toggleOffResult.ExtractBool());

  // Toggle rewards back on
  content::EvalJsResult toggleOnResult = EvalJs(contents(),
      "document.querySelector(\"[data-test-id2='enableMain']\").click();"
      "new Promise((resolve) => {"
      "var count = 10;"
      "var interval = setInterval(function() {"
      "  if (count == 0) {"
      "    clearInterval(interval);"
      "    resolve(false);"
      "  } else {"
      "    count -= 1;"
      "  }"
      "  if (document.querySelector(\"[data-test-id2='enableMain']\")) {"
      "    clearInterval(interval);"
      "    resolve(document.querySelector(\"[data-test-id2='enableMain']\")"
      "      .getAttribute(\"data-toggled\") === 'true');"
      "  }"
      "}, 500);});",
      content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
      content::ISOLATED_WORLD_ID_CONTENT_END);
  ASSERT_TRUE(toggleOnResult.ExtractBool());
}

IN_PROC_BROWSER_TEST_F(BraveRewardsBrowserTest, ToggleAutoContribute) {
  EnableRewards();

  // once rewards has loaded, reload page to activate auto-contribute
  contents()->GetController().Reload(content::ReloadType::NORMAL, true);
  EXPECT_TRUE(WaitForLoadStop(contents()));

  // toggle auto contribute off
  content::EvalJsResult toggleOffResult = EvalJs(contents(),
    "let toggleClicked = false;"
    "new Promise((resolve) => {"
    "var count = 10;"
    "var interval = setInterval(function() {"
    "  if (count == 0) {"
    "    clearInterval(interval);"
    "    resolve(false);"
    "  } else {"
    "    count -= 1;"
    "  }"
    "  if (document.querySelector(\"[data-test-id2='autoContribution']\")) {"
    "    if (!toggleClicked) {"
    "      toggleClicked = true;"
    "      document.querySelector("
    "          \"[data-test-id2='autoContribution']\").click();"
    "    } else {"
    "      clearInterval(interval);"
    "      resolve(document.querySelector("
    "          \"[data-test-id2='autoContribution']\")"
    "        .getAttribute(\"data-toggled\") === 'false');"
    "    }"
    "  }"
    "}, 500);});",
    content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
    content::ISOLATED_WORLD_ID_CONTENT_END);
  ASSERT_TRUE(toggleOffResult.ExtractBool());

  // toggle auto contribute back on
  content::EvalJsResult toggleOnResult = EvalJs(
      contents(),
      "document.querySelector(\"[data-test-id2='autoContribution']\").click();"
      "new Promise((resolve) => {"
      "var count = 10;"
      "var interval = setInterval(function() {"
      "  if (count == 0) {"
      "    clearInterval(interval);"
      "    resolve(false);"
      "  } else {"
      "    count -= 1;"
      "  }"
      "  if (document.querySelector(\"[data-test-id2='autoContribution']\")) {"
      "    clearInterval(interval);"
      "    "
      "resolve(document.querySelector(\"[data-test-id2='autoContribution']\")"
      "      .getAttribute(\"data-toggled\") === 'true');"
      "  }"
      "}, 500);});",
      content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
      content::ISOLATED_WORLD_ID_CONTENT_END);
  ASSERT_TRUE(toggleOnResult.ExtractBool());
}

IN_PROC_BROWSER_TEST_F(BraveRewardsBrowserTest, ActivateSettingsModal) {
  EnableRewards();

  content::EvalJsResult modalResult = EvalJs(
      contents(),
      "document.querySelector(\"[data-test-id='settingsButton']\").click();"
      "new Promise((resolve) => {"
      "var count = 10;"
      "var interval = setInterval(function() {"
      "  if (count == 0) {"
      "    clearInterval(interval);"
      "    resolve(false);"
      "  } else {"
      "    count -= 1;"
      "  }"
      "  if (document.getElementById('modal')) {"
      "    clearInterval(interval);"
      "    resolve(document.getElementById('modal') != null);"
      "  }"
      "}, 500);});",
      content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
      content::ISOLATED_WORLD_ID_CONTENT_END);
  ASSERT_TRUE(modalResult.ExtractBool());
}

IN_PROC_BROWSER_TEST_F(BraveRewardsBrowserTest, HandleFlagsSingleArg) {
  testing::InSequence s;
  // SetProduction(true)
  EXPECT_CALL(*this, OnGetProduction(true));
  // Staging - true and 1
  EXPECT_CALL(*this, OnGetProduction(false)).Times(2);
  // Staging - false and random
  EXPECT_CALL(*this, OnGetProduction(true)).Times(2);

  rewards_service()->SetProduction(true);
  GetProduction();
  RunUntilIdle();

  // Staging - true
  rewards_service()->SetProduction(true);
  rewards_service()->HandleFlags("staging=true");
  GetProduction();
  RunUntilIdle();

  // Staging - 1
  rewards_service()->SetProduction(true);
  rewards_service()->HandleFlags("staging=1");
  GetProduction();
  RunUntilIdle();

  // Staging - false
  rewards_service()->SetProduction(false);
  rewards_service()->HandleFlags("staging=false");
  GetProduction();
  RunUntilIdle();

  // Staging - random
  rewards_service()->SetProduction(false);
  rewards_service()->HandleFlags("staging=werwe");
  GetProduction();
  RunUntilIdle();

  // SetDebug(true)
  EXPECT_CALL(*this, OnGetDebug(true));
  // Debug - true and 1
  EXPECT_CALL(*this, OnGetDebug(true)).Times(2);
  // Debug - false and random
  EXPECT_CALL(*this, OnGetDebug(false)).Times(2);

  rewards_service()->SetDebug(true);
  GetDebug();
  RunUntilIdle();

  // Debug - true
  rewards_service()->SetDebug(false);
  rewards_service()->HandleFlags("debug=true");
  GetDebug();
  RunUntilIdle();

  // Debug - 1
  rewards_service()->SetDebug(false);
  rewards_service()->HandleFlags("debug=1");
  GetDebug();
  RunUntilIdle();

  // Debug - false
  rewards_service()->SetDebug(true);
  rewards_service()->HandleFlags("debug=false");
  GetDebug();
  RunUntilIdle();

  // Debug - random
  rewards_service()->SetDebug(true);
  rewards_service()->HandleFlags("debug=werwe");
  GetDebug();
  RunUntilIdle();

  // positive number
  EXPECT_CALL(*this, OnGetReconcileTime(10));
  // negative number and string
  EXPECT_CALL(*this, OnGetReconcileTime(0)).Times(2);

  // Reconcile interval - positive number
  rewards_service()->SetReconcileTime(0);
  rewards_service()->HandleFlags("reconcile-interval=10");
  GetReconcileTime();
  RunUntilIdle();

  // Reconcile interval - negative number
  rewards_service()->SetReconcileTime(0);
  rewards_service()->HandleFlags("reconcile-interval=-1");
  GetReconcileTime();
  RunUntilIdle();

  // Reconcile interval - string
  rewards_service()->SetReconcileTime(0);
  rewards_service()->HandleFlags("reconcile-interval=sdf");
  GetReconcileTime();
  RunUntilIdle();

  EXPECT_CALL(*this, OnGetShortRetries(true));   // on
  EXPECT_CALL(*this, OnGetShortRetries(false));  // off

  // Short retries - on
  rewards_service()->SetShortRetries(false);
  rewards_service()->HandleFlags("short-retries=true");
  GetShortRetries();
  RunUntilIdle();

  // Short retries - off
  rewards_service()->SetShortRetries(true);
  rewards_service()->HandleFlags("short-retries=false");
  GetShortRetries();
  RunUntilIdle();
}

IN_PROC_BROWSER_TEST_F(BraveRewardsBrowserTest, HandleFlagsMultipleFlags) {
  EXPECT_CALL(*this, OnGetProduction(false));
  EXPECT_CALL(*this, OnGetDebug(true));
  EXPECT_CALL(*this, OnGetReconcileTime(10));
  EXPECT_CALL(*this, OnGetShortRetries(true));

  rewards_service()->SetProduction(true);
  rewards_service()->SetDebug(true);
  rewards_service()->SetReconcileTime(0);
  rewards_service()->SetShortRetries(false);

  rewards_service()->HandleFlags(
      "staging=true,debug=true,short-retries=true,reconcile-interval=10");

  GetReconcileTime();
  GetShortRetries();
  GetProduction();
  GetDebug();
  RunUntilIdle();
}

IN_PROC_BROWSER_TEST_F(BraveRewardsBrowserTest, HandleFlagsWrongInput) {
  EXPECT_CALL(*this, OnGetProduction(true));
  EXPECT_CALL(*this, OnGetDebug(false));
  EXPECT_CALL(*this, OnGetReconcileTime(0));
  EXPECT_CALL(*this, OnGetShortRetries(false));

  rewards_service()->SetProduction(true);
  rewards_service()->SetDebug(false);
  rewards_service()->SetReconcileTime(0);
  rewards_service()->SetShortRetries(false);

  rewards_service()->HandleFlags(
      "staging=,debug=,shortretries=true,reconcile-interval");

  GetReconcileTime();
  GetShortRetries();
  GetDebug();
  GetProduction();
  RunUntilIdle();
}

// #1 - Claim grant via settings page
IN_PROC_BROWSER_TEST_F(BraveRewardsBrowserTest, ClaimGrantViaSettingsPage) {
  // Observe the Rewards service
  rewards_service_->AddObserver(this);

  // Enable Rewards
  EnableRewards();

  // Claim and verify grant using settings page
  const bool use_panel = false;
  ClaimGrant(use_panel);

  // Stop observing the Rewards service
  rewards_service_->RemoveObserver(this);
}

// #2 - Claim grant via panel
IN_PROC_BROWSER_TEST_F(BraveRewardsBrowserTest, ClaimGrantViaPanel) {
  // Observe the Rewards service
  rewards_service_->AddObserver(this);

  // Enable Rewards
  EnableRewards();

  // Claim and verify grant using panel
  const bool use_panel = true;
  ClaimGrant(use_panel);

  // Stop observing the Rewards service
  rewards_service_->RemoveObserver(this);
}

// #3 - Panel shows correct publisher data
IN_PROC_BROWSER_TEST_F(BraveRewardsBrowserTest,
                       PanelShowsCorrectPublisherData) {
  // Observe the Rewards service
  rewards_service_->AddObserver(this);

  // Enable Rewards
  EnableRewards();

  // Navigate to a verified site in a new tab
  const std::string publisher = "duckduckgo.com";
  GURL url = embedded_test_server()->GetURL(publisher, "/index.html");
  ui_test_utils::NavigateToURLWithDisposition(
      browser(), url, WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_NAVIGATION);

  // Open the Rewards popup
  content::WebContents* popup_contents = OpenRewardsPopup();
  ASSERT_TRUE(popup_contents);

  // Retrieve the inner text of the wallet panel and verify that it
  // looks as expected
  {
    content::EvalJsResult js_result = EvalJs(
        popup_contents,
        "const delay = t => new Promise(resolve => setTimeout(resolve, t));"
        "delay(0).then(() => "
        "  document.querySelector(\"[id='wallet-panel']\").innerText);",
        content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
        content::ISOLATED_WORLD_ID_CONTENT_END);
    EXPECT_NE(js_result.ExtractString().find("Brave Verified Publisher"),
              std::string::npos);
    EXPECT_NE(js_result.ExtractString().find(publisher), std::string::npos);
  }

  // Retrieve the inner HTML of the wallet panel and verify that it
  // contains the expected favicon
  {
    content::EvalJsResult js_result = EvalJs(
        popup_contents,
        "const delay = t => new Promise(resolve => setTimeout(resolve, t));"
        "delay(0).then(() => "
        "  document.querySelector(\"[id='wallet-panel']\").innerHTML);",
        content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
        content::ISOLATED_WORLD_ID_CONTENT_END);
    const std::string favicon =
        "chrome://favicon/size/48@2x/http://" + publisher;
    EXPECT_NE(js_result.ExtractString().find(favicon), std::string::npos);
  }

  // Stop observing the Rewards service
  rewards_service_->RemoveObserver(this);
}

// #4a - Visit verified publisher
IN_PROC_BROWSER_TEST_F(BraveRewardsBrowserTest, VisitVerifiedPublisher) {
  // Observe the Rewards service
  rewards_service_->AddObserver(this);

  // Enable Rewards
  EnableRewards();

  // Visit verified publisher
  const bool verified = true;
  VisitPublisher("duckduckgo.com", verified);

  // Stop observing the Rewards service
  rewards_service_->RemoveObserver(this);
}

// #4b - Visit unverified publisher
IN_PROC_BROWSER_TEST_F(BraveRewardsBrowserTest, VisitUnverifiedPublisher) {
  // Observe the Rewards service
  rewards_service_->AddObserver(this);

  // Enable Rewards
  EnableRewards();

  // Visit unverified publisher
  const bool verified = false;
  VisitPublisher("google.com", verified);

  // Stop observing the Rewards service
  rewards_service_->RemoveObserver(this);
}

// #5 - Auto contribution
IN_PROC_BROWSER_TEST_F(BraveRewardsBrowserTest, AutoContribution) {
  // Observe the Rewards service
  rewards_service_->AddObserver(this);

  // Enable Rewards
  EnableRewards();

  // Claim grant using panel
  const bool use_panel = true;
  ClaimGrant(use_panel);

  // Visit verified publisher
  const bool verified = true;
  VisitPublisher("duckduckgo.com", verified);

  // Trigger auto contribution now
  rewards_service()->StartAutoContributeForTest();

  // Wait for reconciliation to complete successfully
  WaitForReconcileCompleted();
  ASSERT_EQ(reconcile_status_, ledger::Result::LEDGER_OK);

  // Signal that contribution was made and update wallet with new
  // balance
  contribution_made_ = true;
  UpdateTestData();

  // Make sure that balance is updated correctly
  {
    content::EvalJsResult js_result = EvalJs(
        contents(),
        "const delay = t => new Promise(resolve => setTimeout(resolve, t));"
        "delay(1000).then(() => "
        "  "
        "document.querySelector(\"[data-test-id='balance']\").innerText);",
        content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
        content::ISOLATED_WORLD_ID_CONTENT_END);
    EXPECT_NE(js_result.ExtractString().find("10.0 BAT"), std::string::npos);
  }

  // Check that summary table shows the appropriate contribution
  {
    content::EvalJsResult js_result = EvalJs(
        contents(),
        "const delay = t => new Promise(resolve => setTimeout(resolve, t));"
        "delay(0).then(() => "
        "  document.querySelector(\"[color='contribute']\").innerText);",
        content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
        content::ISOLATED_WORLD_ID_CONTENT_END);
    EXPECT_NE(js_result.ExtractString().find("-1.0BAT"), std::string::npos);
  }

  // Stop observing the Rewards service
  rewards_service_->RemoveObserver(this);
}

// #6 - Tip verified publisher
IN_PROC_BROWSER_TEST_F(BraveRewardsBrowserTest, TipVerifiedPublisher) {
  // Observe the Rewards service
  rewards_service_->AddObserver(this);

  // Enable Rewards
  EnableRewards();

  // Tip verified publisher
  const bool verified = true;
  const bool monthly = false;
  TipPublisher("duckduckgo.com", verified, monthly);

  // Stop observing the Rewards service
  rewards_service_->RemoveObserver(this);
}

// #7 - Tip unverified publisher
IN_PROC_BROWSER_TEST_F(BraveRewardsBrowserTest, TipUnverifiedPublisher) {
  // Observe the Rewards service
  rewards_service_->AddObserver(this);

  // Enable Rewards
  EnableRewards();

  // Tip unverified publisher
  const bool verified = false;
  const bool monthly = false;
  TipPublisher("google.com", verified, monthly);

  // Stop observing the Rewards service
  rewards_service_->RemoveObserver(this);
}

// #8 - Recurring tip for verified publisher
IN_PROC_BROWSER_TEST_F(BraveRewardsBrowserTest,
                       RecurringTipForVerifiedPublisher) {
  // Observe the Rewards service
  rewards_service_->AddObserver(this);

  // Enable Rewards
  EnableRewards();

  // Tip verified publisher
  const bool verified = true;
  const bool monthly = true;
  TipPublisher("duckduckgo.com", verified, monthly);

  // Stop observing the Rewards service
  rewards_service_->RemoveObserver(this);
}

// #9 - Recurring tip for unverified publisher
IN_PROC_BROWSER_TEST_F(BraveRewardsBrowserTest,
                       RecurringTipForUnverifiedPublisher) {
  // Observe the Rewards service
  rewards_service_->AddObserver(this);

  // Enable Rewards
  EnableRewards();

  // Tip verified publisher
  const bool verified = false;
  const bool monthly = true;
  TipPublisher("google.com", verified, monthly);

  // Stop observing the Rewards service
  rewards_service_->RemoveObserver(this);
}
