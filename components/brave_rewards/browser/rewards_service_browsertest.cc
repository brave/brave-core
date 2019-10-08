/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>

#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/strings/string_split.h"
#include "base/strings/stringprintf.h"
#include "base/memory/weak_ptr.h"
#include "bat/ledger/internal/bat_helper.h"
#include "bat/ledger/internal/uphold/uphold_util.h"
#include "bat/ledger/internal/static_values.h"
#include "bat/ledger/ledger.h"
#include "brave/browser/ui/views/brave_actions/brave_actions_container.h"
#include "brave/browser/ui/views/location_bar/brave_location_bar_view.h"
#include "brave/common/brave_paths.h"
#include "brave/common/extensions/extension_constants.h"
#include "brave/components/brave_rewards/browser/rewards_service_factory.h"
#include "brave/components/brave_rewards/browser/rewards_service_impl.h"
#include "brave/components/brave_rewards/browser/rewards_service_observer.h"
#include "brave/components/brave_rewards/browser/rewards_notification_service_impl.h"  // NOLINT
#include "brave/components/brave_rewards/browser/rewards_notification_service_observer.h"  // NOLINT
#include "brave/components/brave_rewards/common/pref_names.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/network_session_configurator/common/network_switches.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/notification_types.h"
#include "content/public/test/browser_test_utils.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/http_request.h"
#include "net/test/embedded_test_server/http_response.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_browser_tests --filter=BraveRewardsBrowserTest.*

using braveledger_bat_helper::SERVER_TYPES;

namespace {

std::unique_ptr<net::test_server::HttpResponse> HandleRequest(
    const net::test_server::HttpRequest& request) {
  std::unique_ptr<net::test_server::BasicHttpResponse> http_response(
      new net::test_server::BasicHttpResponse());
  http_response->set_code(net::HTTP_OK);
  http_response->set_content_type("text/html");
  if (request.relative_url == "/twitter") {
    http_response->set_content(
        "<html>"
        "  <head></head>"
        "  <body>"
        "    <div data-testid='tweet' data-tweet-id='123'>"
        "      <a href='/status/123'></a>"
        "      <div role='group'>Hello, Twitter!</div>"
        "    </div>"
        "  </body>"
        "</html>");
  } else if (request.relative_url == "/oldtwitter") {
    http_response->set_content(
        "<html>"
        "  <head></head>"
        "  <body>"
        "    <div class='tweet' data-tweet-id='123'>"
        "      <div class='js-actions'>Hello, Twitter!</div>"
        "    </div>"
        "  </body>"
        "</html>");
  } else if (request.relative_url == "/reddit") {
    http_response->set_content(
      "<html>"
        "  <head></head>"
        "  <body>"
        "    <div class='Comment'>"
        "      <div>"
        "        <button aria-label='more options'>"
        "        </button>"
        "      </div>"
        "    </div>"
        "  </body>"
        "</html>");
  } else if (request.relative_url == "/github") {
    http_response->set_content(
      "<html>"
        "  <head></head>"
        "  <body>"
        "   <div class='timeline-comment-actions'>"
        "     <div>GitHubCommentReactsButton</div>"
        "     <div>GitHubCommentElipsesButton</div>"
        "   </div>"
        " </body>"
        "</html>");
  } else {
    http_response->set_content(
        "<html>"
        "  <head></head>"
        "  <body>"
        "    <div>Hello, world!</div>"
        "  </body>"
        "</html>");
  }
  return std::move(http_response);
}

bool URLMatches(const std::string& url,
                const std::string& path,
                const std::string& prefix,
                const SERVER_TYPES& server) {
  const std::string target_url =
      braveledger_bat_helper::buildURL(path, prefix, server);
  return (url.find(target_url) == 0);
}

}  // namespace

namespace brave_test_resp {
  std::string registrarVK_;
  std::string verification_;
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
  std::string uphold_auth_resp_;
  std::string uphold_transactions_resp_;
  std::string uphold_commit_resp_;
}  // namespace brave_test_resp

class BraveRewardsBrowserTest :
    public InProcessBrowserTest,
    public brave_rewards::RewardsServiceObserver,
    public brave_rewards::RewardsNotificationServiceObserver,
    public base::SupportsWeakPtr<BraveRewardsBrowserTest> {
 public:
  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();

    host_resolver()->AddRule("*", "127.0.0.1");

    // Setup up embedded test server for HTTPS requests
    https_server_.reset(new net::EmbeddedTestServer(
        net::test_server::EmbeddedTestServer::TYPE_HTTPS));
    https_server_->SetSSLConfig(net::EmbeddedTestServer::CERT_OK);
    https_server_->RegisterRequestHandler(base::BindRepeating(&HandleRequest));
    ASSERT_TRUE(https_server_->Start());

    brave::RegisterPathProvider();
    ReadTestData();
    rewards_service_ = static_cast<brave_rewards::RewardsServiceImpl*>(
        brave_rewards::RewardsServiceFactory::GetForProfile(
            browser()->profile()));
    rewards_service_->test_response_callback_ =
        base::BindRepeating(&BraveRewardsBrowserTest::GetTestResponse,
                            base::Unretained(this));
    rewards_service_->SetLedgerEnvForTesting();
  }

  void TearDown() override {
    InProcessBrowserTest::TearDown();
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    // HTTPS server only serves a valid cert for localhost, so this is needed
    // to load pages from other hosts without an error
    command_line->AppendSwitch(switches::kIgnoreCertificateErrors);
  }

  net::EmbeddedTestServer* https_server() { return https_server_.get(); }

  void RunUntilIdle() {
    base::RunLoop loop;
    loop.RunUntilIdle();
  }

  std::string GetWalletProperties() {
    return
    "{"
      "\"altcurrency\": \"BAT\","
      "\"probi\": \"0\","
      "\"balance\": \"" + GetAnonBalance() + ".0000\","
      "\"unconfirmed\": \"0.0000\","
      "\"rates\": {"
        "\"BTC\": 0.00003105,"
        "\"ETH\": 0.0007520713830465265,"
        "\"XRP\": 0.6385015608740894,"
        "\"BCH\": 0.000398527449465635,"
        "\"LTC\": 0.003563298490127758,"
        "\"DASH\": 0.0011736801836266257,"
        "\"BTG\": 0.009819171067370777,"
        "\"USD\": 0.214100307359946,"
        "\"EUR\": 0.18357217273398782"
      "},"
      "\"parameters\": {"
        "\"adFree\": {"
          "\"currency\": \"BAT\","
          "\"fee\": {"
            "\"BAT\": 20"
          "},"
          "\"choices\": {"
            "\"BAT\": [10,15,20,30,50,100]"
          "},"
          "\"range\": {"
            "\"BAT\": [10,100]"
          "},"
          "\"days\": 30"
        "}"
      "}"
    "}";
  }

  std::string GetUpholdCard() {
    return
    "{"
      "\"available\": \"" + GetExternalBalance() + "\","
      "\"balance\": \"" + GetExternalBalance() + "\","
      "\"currency\": \"BAT\","
      "\"id\": \"" + external_wallet_address_ + "\","
      "\"label\": \"Brave Browser\","
      "\"lastTransactionAt\": null,"
      "\"settings\": {"
        "\"position\": 31,"
        "\"protected\": false,"
        "\"starred\": false"
      "}"
    "}";
  }

  std::string GetUpholdUser() {
    const std::string verified = verified_wallet_
        ? "2018-08-01T09:53:51.258Z"
        : "null";

    const std::string name = "Test User";

    return base::StringPrintf(
      "{"
        "\"name\": \"%s\","
        "\"memberAt\": \"%s\","
        "\"currencies\": [\"BAT\"]"
      "}",
      name.c_str(),
      verified.c_str());
  }

  void GetTestResponse(const std::string& url,
                       int32_t method,
                       int* response_status_code,
                       std::string* response,
                       std::map<std::string, std::string>* headers) {
    std::vector<std::string> tmp = base::SplitString(url,
                                                     "/",
                                                     base::TRIM_WHITESPACE,
                                                     base::SPLIT_WANT_ALL);
    if (url.find(braveledger_bat_helper::buildURL(REGISTER_PERSONA, PREFIX_V2,
      braveledger_bat_helper::SERVER_TYPES::LEDGER)) == 0
      && tmp.size() == 6) {
      *response = brave_test_resp::registrarVK_;
    } else if (URLMatches(url, REGISTER_PERSONA, PREFIX_V2,
                          SERVER_TYPES::LEDGER) &&
               tmp.size() == 7) {
      *response = brave_test_resp::verification_;
    } else if (URLMatches(url, WALLET_PROPERTIES, PREFIX_V2,
                          SERVER_TYPES::BALANCE)) {
      *response = GetWalletProperties();
    } else if (URLMatches(url, WALLET_PROPERTIES, PREFIX_V2,
                          SERVER_TYPES::LEDGER)) {
      GURL gurl(url);
      if (gurl.has_query()) {
        *response = brave_test_resp::reconcile_;
      } else {
        if (ac_low_amount_) {
          *response = "";
          *response_status_code = net::HTTP_REQUESTED_RANGE_NOT_SATISFIABLE;
        } else {
          *response = brave_test_resp::current_reconcile_;
        }
      }

    } else if (URLMatches(url, GET_SET_PROMOTION, PREFIX_V2,
                          SERVER_TYPES::LEDGER)) {
      GURL gurl(url);
      if (gurl.has_query())
        *response = brave_test_resp::grant_;
      else
        *response = brave_test_resp::captcha_solution_;
    } else if (URLMatches(url, GET_SET_PROMOTION, PREFIX_V4,
                          SERVER_TYPES::LEDGER)) {
      *response = brave_test_resp::grant_v4_;
    } else if (URLMatches(url, GET_PROMOTION_CAPTCHA, PREFIX_V4,
                          SERVER_TYPES::LEDGER)) {
      // The hint we use doesn't matter since we mock the server's
      // responses anyway, but ledger verifies that the response headers contain
      // a hint so we must add one
      (*headers)["captcha-hint"] = "Triangle";
      *response = brave_test_resp::captcha_;
    } else if (URLMatches(url, RECONCILE_CONTRIBUTION, PREFIX_V2,
                          SERVER_TYPES::LEDGER)) {
      *response = brave_test_resp::contribution_;
    } else if (URLMatches(url, REGISTER_VIEWING, PREFIX_V2,
                          SERVER_TYPES::LEDGER)) {
      if (url.find(REGISTER_VIEWING "/") != std::string::npos)
        *response = brave_test_resp::register_credential_;
      else
        *response = brave_test_resp::register_;
    } else if (URLMatches(url, SURVEYOR_BATCH_VOTING, PREFIX_V2,
                          SERVER_TYPES::LEDGER)) {
      if (url.find(SURVEYOR_BATCH_VOTING "/") != std::string::npos)
        *response = brave_test_resp::surveyor_voting_credential_;
      else
        *response = brave_test_resp::surveyor_voting_;
    } else if (URLMatches(url, GET_PUBLISHERS_LIST, "",
                          SERVER_TYPES::PUBLISHER_DISTRO)) {
      if (alter_publisher_list_) {
        *response =
            "["
            "[\"bumpsmack.com\",\"publisher_verified\",false,\"address1\",{}],"
            "[\"duckduckgo.com\",\"wallet_connected\",false,\"address2\",{}]"
            "]";
      } else {
        *response =
            "["
            "[\"bumpsmack.com\",\"publisher_verified\",false,\"address1\",{}],"
            "[\"duckduckgo.com\",\"wallet_connected\",false,\"address2\",{}],"
            "[\"3zsistemi.si\",\"wallet_connected\",false,\"address3\",{}]"
            "]";
      }
    } else if (base::StartsWith(
        url,
        braveledger_uphold::GetAPIUrl("/oauth2/token"),
        base::CompareCase::INSENSITIVE_ASCII)) {
      *response = brave_test_resp::uphold_auth_resp_;
    } else if (base::StartsWith(
        url,
        braveledger_uphold::GetAPIUrl("/v0/me/cards"),
        base::CompareCase::INSENSITIVE_ASCII)) {
      if (base::EndsWith(
          url,
          "transactions",
          base::CompareCase::INSENSITIVE_ASCII)) {
        *response = brave_test_resp::uphold_transactions_resp_;
        *response_status_code = net::HTTP_ACCEPTED;
      } else if (base::EndsWith(
          url,
          "commit",
          base::CompareCase::INSENSITIVE_ASCII)) {
        *response = brave_test_resp::uphold_commit_resp_;
      } else {
        *response = GetUpholdCard();
      }
    } else if (base::StartsWith(
        url,
        braveledger_uphold::GetAPIUrl("/v0/me"),
        base::CompareCase::INSENSITIVE_ASCII)) {
      *response = GetUpholdUser();
    }
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

  void WaitForACReconcileCompleted() {
    if (ac_reconcile_completed_) {
      return;
    }
    wait_for_ac_completed_loop_.reset(new base::RunLoop);
    wait_for_ac_completed_loop_->Run();
  }

  void WaitForTipReconcileCompleted() {
    if (tip_reconcile_completed_) {
      return;
    }
    wait_for_tip_completed_loop_.reset(new base::RunLoop);
    wait_for_tip_completed_loop_->Run();
  }

  void WaitForMultipleTipReconcileCompleted(int32_t needed) {
    multiple_tip_reconcile_needed_ = needed;
    if (multiple_tip_reconcile_completed_) {
      return;
    }

    wait_for_multiple_tip_completed_loop_.reset(new base::RunLoop);
    wait_for_multiple_tip_completed_loop_->Run();
  }

  void WaitForInsufficientFundsNotification() {
    if (insufficient_notification_would_have_already_shown_) {
      return;
    }
    wait_for_insufficient_notification_loop_.reset(new base::RunLoop);
    wait_for_insufficient_notification_loop_->Run();
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
    ASSERT_TRUE(base::ReadFileToString(
        path.AppendASCII("uphold_auth_resp.json"),
        &brave_test_resp::uphold_auth_resp_));
    ASSERT_TRUE(base::ReadFileToString(
        path.AppendASCII("uphold_transactions_resp.json"),
        &brave_test_resp::uphold_transactions_resp_));
    ASSERT_TRUE(base::ReadFileToString(
        path.AppendASCII("uphold_commit_resp.json"),
        &brave_test_resp::uphold_commit_resp_));
  }

  void UpdateContributionBalance(double amount, bool verified = false) {
    if (verified) {
      if (balance_ > 0) {
        balance_ -= amount;
        return;
      }

      if (verified_wallet_) {
        external_balance_ -= amount;
        return;
      }

      return;
    }

    pending_balance_ += amount;
  }

  std::string BalanceDoubleToString(double amount) {
    return base::StringPrintf("%.1f", amount);
  }

  std::string GetBalance() {
    return BalanceDoubleToString(balance_ + external_balance_);
  }

  std::string GetPendingBalance() {
    return BalanceDoubleToString(pending_balance_);
  }

  std::string GetExternalBalance() {
    return BalanceDoubleToString(external_balance_);
  }

  std::string GetAnonBalance() {
    return BalanceDoubleToString(balance_);
  }

  GURL rewards_url() {
    GURL rewards_url("brave://rewards");
    return rewards_url;
  }

  GURL uphold_auth_url() {
    GURL url("chrome://rewards/uphold/authorization?"
             "code=0c42b34121f624593ee3b04cbe4cc6ddcd72d&state=123456789");
    return url;
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

    // Goes to final step
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

    // Dismiss the grant notification
    if (use_panel) {
      content::EvalJsResult jsResult = EvalJs(contents,
      "new Promise((resolve) => {"
      "let count = 10;"
      "let interval = setInterval(function() {"
      "  if (count == 0) {"
      "    clearInterval(interval);"
      "    resolve(false);"
      "  } else {"
      "    count -= 1;"
      "  }"
      "  const button = document.getElementById(\"grant-completed-ok\");"
      "  if (button) {"
      "    clearInterval(interval);"
      "    button.click();"
      "    resolve(true);"
      "  }"
      "}, 500);});",
      content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
      content::ISOLATED_WORLD_ID_CONTENT_END);
      ASSERT_TRUE(jsResult.ExtractBool());
    }
  }

  void VisitPublisher(const std::string& publisher,
                      bool verified,
                      bool last_add = false) {
    GURL url = https_server()->GetURL(publisher, "/index.html");
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
        "  document.querySelector(\"[data-test-id='ac_link_" + publisher + "']"
        "\").innerText);",
        content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
        content::ISOLATED_WORLD_ID_CONTENT_END);
    EXPECT_STREQ(js_result.ExtractString().c_str(), publisher.c_str());

    if (verified) {
      // A verified site has two images associated with it, the site's
      // favicon and the verified icon
      content::EvalJsResult js_result =
          EvalJs(contents(),
                "document.querySelector(\"[data-test-id='ac_link_" +
                publisher + "']\").getElementsByTagName('svg').length === 1;",
                content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
                content::ISOLATED_WORLD_ID_CONTENT_END);
      EXPECT_TRUE(js_result.ExtractBool());
    } else {
      // An unverified site has one image associated with it, the site's
      // favicon
      content::EvalJsResult js_result =
          EvalJs(contents(),
                "document.querySelector(\"[data-test-id='ac_link_" +
                publisher + "']\").getElementsByTagName('svg').length === 0;",
                content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
                content::ISOLATED_WORLD_ID_CONTENT_END);
      EXPECT_TRUE(js_result.ExtractBool());
    }

    if (last_add) {
      last_publisher_added_ = true;
    }
  }

  void TipPublisher(
      const std::string& publisher,
      bool should_contribute = false,
      bool monthly = false,
      int32_t selection = 0) {
    // we shouldn't be adding publisher to AC list,
    // so that we can focus only on tipping part
    rewards_service_->SetPublisherMinVisitTime(8);

    // Navigate to a site in a new tab
    GURL url = https_server()->GetURL(publisher, "/index.html");
    ui_test_utils::NavigateToURLWithDisposition(
        browser(), url, WindowOpenDisposition::NEW_FOREGROUND_TAB,
        ui_test_utils::BROWSER_TEST_WAIT_FOR_NAVIGATION);

    // Open the Rewards popup
    content::WebContents* popup_contents = OpenRewardsPopup();
    ASSERT_TRUE(popup_contents);

    // Construct an observer to wait for the site banner to load
    content::WindowedNotificationObserver site_banner_observer(
        content::NOTIFICATION_LOAD_COMPLETED_MAIN_FRAME,
        content::NotificationService::AllSources());

    const std::string buttonSelector = monthly
        ? "[type='tip-monthly']"
        : "[type='tip']";

    // Click button to initiate sending a tip
    content::EvalJsResult js_result = EvalJs(
      popup_contents,
      content::JsReplace(
      "new Promise((resolve) => {"
      "let count = 10;"
      "var interval = setInterval(function() {"
      "  if (count === 0) {"
      "    clearInterval(interval);"
      "    resolve('');"
      "  } else {"
      "    count -= 1;"
      "  }"
      "  const tipButton = document.querySelector($1);"
      "  if (tipButton) {"
      "    clearInterval(interval);"
      "    tipButton.click();"
      "    resolve(true);"
      "  }"
      "}, 500);});",
      buttonSelector),
      content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
      content::ISOLATED_WORLD_ID_CONTENT_END);

    // Wait for the site banner to load
    site_banner_observer.Wait();

    // Retrieve the notification source
    const auto& site_banner_source =
        static_cast<const content::Source<content::WebContents>&>(
            site_banner_observer.source());

    content::WebContents* site_banner_contents = site_banner_source.ptr();
    ASSERT_TRUE(site_banner_contents);

    const double amount = tip_amounts_.at(selection);
    const std::string amount_str = std::to_string(static_cast<int32_t>(amount));

    // Select the tip amount (default is 1.0 BAT)
    ASSERT_TRUE(ExecJs(
        site_banner_contents,
        content::JsReplace(
        "const delay = t => new Promise(resolve => setTimeout(resolve, t));"
        "delay(0).then(() => "
        "  document.querySelectorAll(\"[data-test-id='amount-wrapper']\")[$1]"
        "  .click());",
        selection),
        content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
        content::ISOLATED_WORLD_ID_CONTENT_END));

    // Send the tip
    ASSERT_TRUE(ExecJs(
        site_banner_contents,
        "const delay = t => new Promise(resolve => setTimeout(resolve, t));"
        "delay(0).then(() => "
        "  document.querySelector(\"[data-test-id='send-tip-button'] button\")"
        "  .click());",
        content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
        content::ISOLATED_WORLD_ID_CONTENT_END));

    // Signal that direct tip was made and update wallet with new
    // balance
    if (!monthly && !should_contribute) {
      UpdateContributionBalance(amount, should_contribute);
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
      EXPECT_NE(js_result.ExtractString().find("" + amount_str + ".0 BAT"),
                std::string::npos);

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

    if (monthly) {
      // Trigger contribution process
      rewards_service()->StartMonthlyContributionForTest();

      // Wait for reconciliation to complete
      WaitForTipReconcileCompleted();
      const auto result = should_contribute
          ? ledger::Result::LEDGER_OK
          : ledger::Result::RECURRING_TABLE_EMPTY;
      ASSERT_EQ(tip_reconcile_status_, result);

      // Signal that monthly contribution was made and update wallet
      // with new balance
      if (!should_contribute) {
        UpdateContributionBalance(amount, should_contribute);
      }
    } else if (!monthly && should_contribute) {
      // Wait for reconciliation to complete
      WaitForTipReconcileCompleted();
      ASSERT_EQ(tip_reconcile_status_, ledger::Result::LEDGER_OK);
    }

    if (should_contribute) {
      // Make sure that balance is updated correctly
      {
        content::EvalJsResult js_result = EvalJs(
            contents(),
            "const delay = t => new Promise(resolve => setTimeout(resolve, t));"
            "delay(1000).then(() => "
            "  document.querySelector(\"[data-test-id='balance']\")"
            "    .innerText);",
            content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
            content::ISOLATED_WORLD_ID_CONTENT_END);
        EXPECT_NE(js_result.ExtractString().find(GetBalance()),
                  std::string::npos);
      }

      // Check that tip table shows the appropriate tip amount
      {
        const std::string selector = monthly
            ? "[data-test-id='summary-donation']"
            : "[data-test-id='summary-tips']";
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
        EXPECT_NE(js_result.ExtractString()
                  .find("-" + BalanceDoubleToString(amount) + "BAT"),
                  std::string::npos);
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
        EXPECT_NE(js_result.ExtractString().
                  find(GetBalance() + " BAT"), std::string::npos);
      }

      // Make sure that pending contribution box shows the correct
      // amount
      {
        content::EvalJsResult js_result = EvalJs(
            contents(),
            "const delay = t => new Promise(resolve => setTimeout(resolve, t));"
            "delay(0).then(() => "
            "  document.querySelector"
            "(\"[data-test-id='pending-contribution-box']\").innerText);",
            content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
            content::ISOLATED_WORLD_ID_CONTENT_END);
        EXPECT_NE(js_result.ExtractString().find(GetPendingBalance()),
                  std::string::npos);
      }

      // Check that tip table shows no tip
      {
        content::EvalJsResult js_result = EvalJs(
            contents(),
            "const delay = t => new Promise(resolve => setTimeout(resolve, t));"
            "delay(0).then(() => "
            "  document.querySelectorAll(\"[type='donation']\")[1]"
            "    .parentElement.parentElement.innerText);",
            content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
            content::ISOLATED_WORLD_ID_CONTENT_END);
        EXPECT_NE(
            js_result.ExtractString().find("Total tips this month\n0.0BAT"),
            std::string::npos);
      }
    }
  }

  bool IsMediaTipsInjected() {
    content::EvalJsResult js_result =
        EvalJs(contents(),
               "new Promise((resolve) => {"
               "let count = 10;"
               "var interval = setInterval(function() {"
               "  if (count === 0) {"
               "    clearInterval(interval);"
               "    resolve(false);"
               "  } else {"
               "    count -= 1;"
               "  }"
               "  const braveTipActions"
               "    = document.querySelectorAll(\".action-brave-tip\");"
               "  if (braveTipActions && braveTipActions.length === 1) {"
               "    clearInterval(interval);"
               "    resolve(true);"
               "  }"
               "}, 500);});",
               content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
               content::ISOLATED_WORLD_ID_CONTENT_END);
    return js_result.ExtractBool();
  }

  void OnWalletInitialized(brave_rewards::RewardsService* rewards_service,
                           int32_t result) {
    const auto converted_result = static_cast<ledger::Result>(result);
    ASSERT_TRUE(converted_result == ledger::Result::WALLET_CREATED ||
                converted_result == ledger::Result::NO_LEDGER_STATE);
    wallet_initialized_ = true;
    if (wait_for_wallet_initialization_loop_)
      wait_for_wallet_initialization_loop_->Quit();
  }

  void OnGrant(brave_rewards::RewardsService* rewards_service,
               unsigned int result,
               brave_rewards::Grant properties) {
    ASSERT_EQ(static_cast<ledger::Result>(result), ledger::Result::LEDGER_OK);
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
    ASSERT_EQ(static_cast<ledger::Result>(result), ledger::Result::LEDGER_OK);
    grant_finished_ = true;
    grant_ = grant;
    balance_ += 30.0;
    if (wait_for_grant_finished_loop_)
      wait_for_grant_finished_loop_->Quit();
  }

  void OnPublisherListNormalized(brave_rewards::RewardsService* rewards_service,
                                 const brave_rewards::ContentSiteList& list) {
    if (list.size() == 0)
      return;
    publisher_list_normalized_ = true;
    if (wait_for_publisher_list_normalized_loop_)
      wait_for_publisher_list_normalized_loop_->Quit();
  }

  void OnReconcileComplete(brave_rewards::RewardsService* rewards_service,
                           unsigned int result,
                           const std::string& viewing_id,
                           const std::string& probi,
                           const int32_t type) {
    const size_t size = probi.size();
    std::string amount = "0";
    if (size > 18) {
      amount = probi.substr(0, size - 18);
    }

    UpdateContributionBalance(std::stod(amount), true);

    const auto converted_result = static_cast<ledger::Result>(result);
    const auto converted_type =
        static_cast<ledger::RewardsType>(type);

    if (converted_type == ledger::RewardsType::AUTO_CONTRIBUTE) {
      ac_reconcile_completed_ = true;
      ac_reconcile_status_ = converted_result;
      if (wait_for_ac_completed_loop_) {
        wait_for_ac_completed_loop_->Quit();
      }
    }

    if (converted_type == ledger::RewardsType::ONE_TIME_TIP ||
        converted_type == ledger::RewardsType::RECURRING_TIP) {
      // Single tip tracking
      tip_reconcile_completed_ = true;
      tip_reconcile_status_ = converted_result;
      if (wait_for_tip_completed_loop_) {
        wait_for_tip_completed_loop_->Quit();
      }

      // Multiple tips
      multiple_tip_reconcile_count_++;
      multiple_tip_reconcile_status_ = converted_result;

      if (multiple_tip_reconcile_count_ == multiple_tip_reconcile_needed_) {
        multiple_tip_reconcile_completed_ = true;
        if (wait_for_multiple_tip_completed_loop_) {
          wait_for_multiple_tip_completed_loop_->Quit();
        }
      }
    }
  }

  void ACLowAmount() {
    ac_low_amount_ = true;
  }

  void OnNotificationAdded(
      brave_rewards::RewardsNotificationService* rewards_notification_service,
      const brave_rewards::RewardsNotificationService::RewardsNotification&
      notification) {
    const brave_rewards::RewardsNotificationService::RewardsNotificationsMap&
        notifications = rewards_notification_service->GetAllNotifications();
    for (const auto& notification : notifications) {
      if (notification.second.type_ ==
          brave_rewards::RewardsNotificationService::RewardsNotificationType
          ::REWARDS_NOTIFICATION_INSUFFICIENT_FUNDS) {
        insufficient_notification_would_have_already_shown_ = true;
        if (wait_for_insufficient_notification_loop_) {
          wait_for_insufficient_notification_loop_->Quit();
        }
      }
    }
  }

  /**
   * When using notification observer for insufficient funds, tests will fail
   * for sufficient funds because observer will never be called for
   * notification. Use this as callback to know when we come back with
   * sufficient funds to prevent inf loop
   * */
  void ShowNotificationAddFundsForTesting(bool sufficient) {
    if (sufficient) {
      insufficient_notification_would_have_already_shown_ = true;
      if (wait_for_insufficient_notification_loop_) {
        wait_for_insufficient_notification_loop_->Quit();
      }
    }
  }

  void CheckInsufficientFundsForTesting() {
    rewards_service_->MaybeShowNotificationAddFundsForTesting(
        base::BindOnce(
            &BraveRewardsBrowserTest::ShowNotificationAddFundsForTesting,
            AsWeakPtr()));
  }

  const std::vector<double> tip_amounts_ = {1.0, 5.0, 10.0};

  MOCK_METHOD1(OnGetProduction, void(bool));
  MOCK_METHOD1(OnGetDebug, void(bool));
  MOCK_METHOD1(OnGetReconcileTime, void(int32_t));
  MOCK_METHOD1(OnGetShortRetries, void(bool));

  std::unique_ptr<net::EmbeddedTestServer> https_server_;

  brave_rewards::RewardsServiceImpl* rewards_service_;

  brave_rewards::Grant grant_;

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

  std::unique_ptr<base::RunLoop> wait_for_ac_completed_loop_;
  bool ac_reconcile_completed_ = false;
  ledger::Result ac_reconcile_status_ = ledger::Result::LEDGER_ERROR;
  std::unique_ptr<base::RunLoop> wait_for_tip_completed_loop_;
  bool tip_reconcile_completed_ = false;
  ledger::Result tip_reconcile_status_ = ledger::Result::LEDGER_ERROR;
  std::unique_ptr<base::RunLoop> wait_for_multiple_tip_completed_loop_;
  bool multiple_tip_reconcile_completed_ = false;
  int32_t multiple_tip_reconcile_count_ = 0;
  int32_t multiple_tip_reconcile_needed_ = 0;
  ledger::Result multiple_tip_reconcile_status_ = ledger::Result::LEDGER_ERROR;

  std::unique_ptr<base::RunLoop> wait_for_insufficient_notification_loop_;
  bool insufficient_notification_would_have_already_shown_ = false;

  bool ac_low_amount_ = false;
  bool last_publisher_added_ = false;
  bool alter_publisher_list_ = false;
  double balance_ = 0;
  double pending_balance_ = 0;
  double external_balance_ = 0;
  double verified_wallet_ = false;
  const std::string external_wallet_address_ =
      "abe5f454-fedd-4ea9-9203-470ae7315bb3";
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
  GURL url = https_server()->GetURL(publisher, "/index.html");
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
      "new Promise((resolve) => {"
      "let count = 10;"
      "var interval = setInterval(function() {"
      "  if (count === 0) {"
      "    clearInterval(interval);"
      "    resolve('');"
      "  } else {"
      "    count -= 1;"
      "  }"
      "  const walletPanel = document.querySelector(\"[id='wallet-panel']\");"
      "  if (walletPanel) {"
      "    clearInterval(interval);"
      "    resolve(walletPanel.innerText);"
      "  }"
      "}, 500);});",
      content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
      content::ISOLATED_WORLD_ID_CONTENT_END);
    EXPECT_NE(js_result.ExtractString().find("Brave Verified Creator"),
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
        "chrome://favicon/size/48@2x/https://" + publisher;
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
  VisitPublisher("brave.com", verified);

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

  // Trigger contribution process
  rewards_service()->StartMonthlyContributionForTest();

  // Wait for reconciliation to complete successfully
  WaitForACReconcileCompleted();
  ASSERT_EQ(ac_reconcile_status_, ledger::Result::LEDGER_OK);

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
    EXPECT_NE(js_result.ExtractString().find(GetBalance() + " BAT"),
              std::string::npos);
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
    EXPECT_NE(js_result.ExtractString().find("-20.0BAT"),
              std::string::npos);
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

  // Claim grant using settings page
  const bool use_panel = true;
  ClaimGrant(use_panel);

  // Tip verified publisher
  TipPublisher("duckduckgo.com", true);

  // Stop observing the Rewards service
  rewards_service_->RemoveObserver(this);
}

// #7 - Tip unverified publisher
IN_PROC_BROWSER_TEST_F(BraveRewardsBrowserTest, TipUnverifiedPublisher) {
  // Observe the Rewards service
  rewards_service_->AddObserver(this);

  // Enable Rewards
  EnableRewards();

  // Claim grant using settings page
  const bool use_panel = true;
  ClaimGrant(use_panel);

  // Tip unverified publisher
  TipPublisher("brave.com");

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

  // Claim grant using settings page
  const bool use_panel = true;
  ClaimGrant(use_panel);

  // Tip verified publisher
  const bool monthly = true;
  TipPublisher("duckduckgo.com", true, monthly);

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

  // Claim grant using settings page
  const bool use_panel = true;
  ClaimGrant(use_panel);

  // Tip verified publisher
  const bool monthly = true;
  TipPublisher("brave.com", false, monthly);

  // Stop observing the Rewards service
  rewards_service_->RemoveObserver(this);
}

// Tip is below server threshold
IN_PROC_BROWSER_TEST_F(BraveRewardsBrowserTest,
                       ContributionWithLowAmount) {
  // Observe the Rewards service
  rewards_service_->AddObserver(this);

  // Enable Rewards
  EnableRewards();

  // Claim grant using panel
  const bool use_panel = true;
  ClaimGrant(use_panel);

  // Set monthly to 0.2 BAT
  rewards_service()->SetContributionAmount(0.2);

  // Visit verified publisher
  const bool verified = true;
  VisitPublisher("duckduckgo.com", verified);

  ACLowAmount();

  // Trigger contribution process
  rewards_service()->StartMonthlyContributionForTest();

  // Wait for reconciliation to complete successfully
  WaitForACReconcileCompleted();
  ASSERT_EQ(ac_reconcile_status_, ledger::Result::CONTRIBUTION_AMOUNT_TOO_LOW);

  // Stop observing the Rewards service
  rewards_service_->RemoveObserver(this);
}

// Brave tip icon is injected when visiting Twitter
IN_PROC_BROWSER_TEST_F(BraveRewardsBrowserTest, TwitterTipsInjectedOnTwitter) {
  // Enable Rewards
  EnableRewards();

  // Navigate to Twitter in a new tab
  GURL url = https_server()->GetURL("twitter.com", "/twitter");
  ui_test_utils::NavigateToURLWithDisposition(
      browser(), url, WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_NAVIGATION);

  // Ensure that Media tips injection is active
  EXPECT_TRUE(IsMediaTipsInjected());
}

// Brave tip icon is not injected when visiting Twitter while Brave
// Rewards is disabled
IN_PROC_BROWSER_TEST_F(BraveRewardsBrowserTest,
                       TwitterTipsNotInjectedWhenRewardsDisabled) {
  // Navigate to Twitter in a new tab
  GURL url = https_server()->GetURL("twitter.com", "/twitter");
  ui_test_utils::NavigateToURLWithDisposition(
      browser(), url, WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_NAVIGATION);

  // Ensure that Media tips injection is not active
  EXPECT_FALSE(IsMediaTipsInjected());
}

// Brave tip icon is injected when visiting old Twitter
IN_PROC_BROWSER_TEST_F(BraveRewardsBrowserTest,
                       TwitterTipsInjectedOnOldTwitter) {
  // Enable Rewards
  EnableRewards();

  // Navigate to Twitter in a new tab
  GURL url = https_server()->GetURL("twitter.com", "/oldtwitter");
  ui_test_utils::NavigateToURLWithDisposition(
      browser(), url, WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_NAVIGATION);

  // Ensure that Media tips injection is active
  EXPECT_TRUE(IsMediaTipsInjected());
}

// Brave tip icon is not injected when visiting old Twitter while
// Brave Rewards is disabled
IN_PROC_BROWSER_TEST_F(BraveRewardsBrowserTest,
                       TwitterTipsNotInjectedWhenRewardsDisabledOldTwitter) {
  // Navigate to Twitter in a new tab
  GURL url = https_server()->GetURL("twitter.com", "/oldtwitter");
  ui_test_utils::NavigateToURLWithDisposition(
      browser(), url, WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_NAVIGATION);

  // Ensure that Media tips injection is not active
  EXPECT_FALSE(IsMediaTipsInjected());
}

// Brave tip icon is not injected into non-Twitter sites
IN_PROC_BROWSER_TEST_F(BraveRewardsBrowserTest,
                       TwitterTipsNotInjectedOnNonTwitter) {
  // Enable Rewards
  EnableRewards();

  // Navigate to a non-Twitter site in a new tab
  GURL url = https_server()->GetURL("brave.com", "/twitter");
  ui_test_utils::NavigateToURLWithDisposition(
      browser(), url, WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_NAVIGATION);

  // Ensure that Media tips injection is not active
  EXPECT_FALSE(IsMediaTipsInjected());
}

// Brave tip icon is injected when visiting Reddit
IN_PROC_BROWSER_TEST_F(BraveRewardsBrowserTest, RedditTipsInjectedOnReddit) {
  // Enable Rewards
  EnableRewards();

  // Navigate to Reddit in a new tab
  GURL url = https_server()->GetURL("reddit.com", "/reddit");
  ui_test_utils::NavigateToURLWithDisposition(
      browser(), url, WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_NAVIGATION);

  // Ensure that Media Tips injection is active
  EXPECT_TRUE(IsMediaTipsInjected());
}

// Brave tip icon is not injected when visiting Reddit
IN_PROC_BROWSER_TEST_F(BraveRewardsBrowserTest,
                       RedditTipsNotInjectedWhenRewardsDisabled) {
  // Navigate to Reddit in a new tab
  GURL url = https_server()->GetURL("reddit.com", "/reddit");
  ui_test_utils::NavigateToURLWithDisposition(
      browser(), url, WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_NAVIGATION);

  // Ensure that Media Tips injection is not active
  EXPECT_FALSE(IsMediaTipsInjected());
}

// Brave tip icon is not injected when visiting Reddit
IN_PROC_BROWSER_TEST_F(BraveRewardsBrowserTest,
                       RedditTipsNotInjectedOnNonReddit) {
  // Enable Rewards
  EnableRewards();

  // Navigate to Reddit in a new tab
  GURL url = https_server()->GetURL("brave.com", "/reddit");
  ui_test_utils::NavigateToURLWithDisposition(
      browser(), url, WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_NAVIGATION);

  // Ensure that Media Tips injection is not active
  EXPECT_FALSE(IsMediaTipsInjected());
}

// Brave tip icon is injected when visiting GitHub
IN_PROC_BROWSER_TEST_F(BraveRewardsBrowserTest, GitHubTipsInjectedOnGitHub) {
  // Enable Rewards
  EnableRewards();

  // Navigate to GitHub in a new tab
  GURL url = https_server()->GetURL("github.com", "/github");
  ui_test_utils::NavigateToURLWithDisposition(
      browser(), url, WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_NAVIGATION);

  // Ensure that Media Tips injection is active
  EXPECT_TRUE(IsMediaTipsInjected());
}

// Brave tip icon is not injected when visiting GitHub while Brave
// Rewards is disabled
IN_PROC_BROWSER_TEST_F(BraveRewardsBrowserTest,
                       GitHubTipsNotInjectedWhenRewardsDisabled) {
  // Navigate to GitHub in a new tab
  GURL url = https_server()->GetURL("github.com", "/github");
  ui_test_utils::NavigateToURLWithDisposition(
      browser(), url, WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_NAVIGATION);

  // Ensure that Media Tips injection is not active
  EXPECT_FALSE(IsMediaTipsInjected());
}

// Brave tip icon is not injected when not visiting GitHub
IN_PROC_BROWSER_TEST_F(BraveRewardsBrowserTest,
                       GitHubTipsNotInjectedOnNonGitHub) {
  // Enable Rewards
  EnableRewards();

  // Navigate to GitHub in a new tab
  GURL url = https_server()->GetURL("brave.com", "/github");
  ui_test_utils::NavigateToURLWithDisposition(
      browser(), url, WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_NAVIGATION);

  // Ensure that Media Tips injection is not active
  EXPECT_FALSE(IsMediaTipsInjected());
}

// Check pending contributions
IN_PROC_BROWSER_TEST_F(BraveRewardsBrowserTest,
                       PendingContributionTip) {
  const std::string publisher = "example.com";

  // Observe the Rewards service
  rewards_service_->AddObserver(this);

  // Enable Rewards
  EnableRewards();

  // Claim grant using settings page
  const bool use_panel = true;
  ClaimGrant(use_panel);

  // Tip unverified publisher
  TipPublisher(publisher);

  // Check that link for pending is shown
  {
    content::EvalJsResult js_result = EvalJs(
        contents(),
        "const delay = t => new Promise(resolve => setTimeout(resolve, t));"
        "delay(0).then(() => "
        "  document.querySelector(\"[data-test-id='reservedAllLink']\")"
        "    .parentElement.parentElement.innerText);",
        content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
        content::ISOLATED_WORLD_ID_CONTENT_END);
    EXPECT_NE(
        js_result.ExtractString().find("Show all pending contributions"),
        std::string::npos);
  }

  // Open modal
  {
    ASSERT_TRUE(ExecJs(contents(),
        "if (document.querySelector(\"[data-test-id='reservedAllLink']\")) {"
        "  document.querySelector("
        "      \"[data-test-id='reservedAllLink']\").click();"
        "}",
        content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
        content::ISOLATED_WORLD_ID_CONTENT_END));
  }

  // Make sure that table is populated
  {
    content::EvalJsResult js_result = EvalJs(
        contents(),
        "const delay = t => new Promise(resolve => setTimeout(resolve, t));"
        "delay(0).then(() => "
        " document.querySelector(\"[id='pendingContributionTable']\")"
        "    .getElementsByTagName('a')[0].innerText);",
        content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
        content::ISOLATED_WORLD_ID_CONTENT_END);
    EXPECT_NE(
        js_result.ExtractString().find(publisher),
        std::string::npos);
  }

  // Stop observing the Rewards service
  rewards_service_->RemoveObserver(this);
}

IN_PROC_BROWSER_TEST_F(BraveRewardsBrowserTest,
    InsufficientNotificationForVerifiedsZeroAmountZeroPublishers) {
  rewards_service_->GetNotificationService()->AddObserver(this);
  EnableRewards();
  CheckInsufficientFundsForTesting();
  WaitForInsufficientFundsNotification();
  const brave_rewards::RewardsNotificationService::RewardsNotificationsMap&
      notifications = rewards_service_->GetAllNotifications();

  if (notifications.empty()) {
    SUCCEED();
  }
  bool notification_shown = false;
  for (const auto& notification : notifications) {
    if (notification.second.type_ ==
        brave_rewards::RewardsNotificationService::RewardsNotificationType
        ::REWARDS_NOTIFICATION_INSUFFICIENT_FUNDS) {
      notification_shown = true;
      break;
    }
  }
  EXPECT_FALSE(notification_shown);

  // Stop observing the Rewards service
  rewards_service_->RemoveObserver(this);
}

IN_PROC_BROWSER_TEST_F(BraveRewardsBrowserTest,
                       InsufficientNotificationForVerifiedsDefaultAmount) {
  rewards_service_->AddObserver(this);
  rewards_service_->GetNotificationService()->AddObserver(this);
  EnableRewards();
  // Claim grant using panel
  const bool use_panel = true;
  ClaimGrant(use_panel);

  // Visit publishers
  const bool verified = true;
  while (!last_publisher_added_) {
    VisitPublisher("duckduckgo.com", verified);
    VisitPublisher("bumpsmack.com", verified);
    VisitPublisher("brave.com", !verified, true);
  }

  CheckInsufficientFundsForTesting();
  WaitForInsufficientFundsNotification();
  const brave_rewards::RewardsNotificationService::RewardsNotificationsMap&
      notifications = rewards_service_->GetAllNotifications();

  if (notifications.empty()) {
    SUCCEED();
  }
  bool notification_shown = false;
  for (const auto& notification : notifications) {
    if (notification.second.type_ ==
        brave_rewards::RewardsNotificationService::RewardsNotificationType
        ::REWARDS_NOTIFICATION_INSUFFICIENT_FUNDS) {
      notification_shown = true;
      break;
    }
  }
  EXPECT_FALSE(notification_shown);

  // Stop observing the Rewards service
  rewards_service_->RemoveObserver(this);
}

IN_PROC_BROWSER_TEST_F(BraveRewardsBrowserTest,
                       InsufficientNotificationForVerifiedsSufficientAmount) {
  rewards_service_->AddObserver(this);
  rewards_service_->GetNotificationService()->AddObserver(this);

  EnableRewards();
  // Claim grant using panel
  const bool use_panel = true;
  ClaimGrant(use_panel);

  // Visit publishers
  const bool verified = true;
  while (!last_publisher_added_) {
    VisitPublisher("duckduckgo.com", verified);
    VisitPublisher("bumpsmack.com", verified);
    VisitPublisher("brave.com", !verified, true);
  }

  rewards_service_->SetContributionAmount(40.0);

  CheckInsufficientFundsForTesting();
  WaitForInsufficientFundsNotification();
  const brave_rewards::RewardsNotificationService::RewardsNotificationsMap&
      notifications = rewards_service_->GetAllNotifications();
  if (notifications.empty()) {
    SUCCEED();
  }
  bool notification_shown = false;
  for (const auto& notification : notifications) {
    if (notification.second.type_ ==
        brave_rewards::RewardsNotificationService::RewardsNotificationType
        ::REWARDS_NOTIFICATION_INSUFFICIENT_FUNDS) {
      notification_shown = true;
      break;
    }
  }
  EXPECT_FALSE(notification_shown);

  // Stop observing the Rewards service
  rewards_service_->RemoveObserver(this);
}

IN_PROC_BROWSER_TEST_F(BraveRewardsBrowserTest,
                       InsufficientNotificationForVerifiedsInsufficientAmount) {
  rewards_service_->AddObserver(this);
  rewards_service_->GetNotificationService()->AddObserver(this);
  EnableRewards();
  // Claim grant using panel
  const bool use_panel = true;
  ClaimGrant(use_panel);

  // Visit publishers
  const bool verified = true;
  while (!last_publisher_added_) {
    VisitPublisher("duckduckgo.com", verified);
    VisitPublisher("bumpsmack.com", verified);
    VisitPublisher("brave.com", !verified, true);
  }
  rewards_service_->SetContributionAmount(100.0);

  rewards_service_->CheckInsufficientFundsForTesting();
  WaitForInsufficientFundsNotification();
  const brave_rewards::RewardsNotificationService::RewardsNotificationsMap&
      notifications = rewards_service_->GetAllNotifications();

  if (notifications.empty()) {
    FAIL() << "Should see Insufficient Funds notification";
  }
  bool notification_shown = false;
  for (const auto& notification : notifications) {
    if (notification.second.type_ ==
        brave_rewards::RewardsNotificationService::RewardsNotificationType
        ::REWARDS_NOTIFICATION_INSUFFICIENT_FUNDS) {
      notification_shown = true;
      break;
    }
  }
  EXPECT_TRUE(notification_shown);


  // Stop observing the Rewards service
  rewards_service_->RemoveObserver(this);
}

// Test whether rewards is diabled in private profile.
IN_PROC_BROWSER_TEST_F(BraveRewardsBrowserTest, PrefsTestInPrivateWindow) {
  EnableRewards();
  auto* profile = browser()->profile();
  EXPECT_TRUE(profile->GetPrefs()->GetBoolean(
      brave_rewards::prefs::kBraveRewardsEnabled));

  Profile* private_profile = profile->GetOffTheRecordProfile();
  EXPECT_FALSE(private_profile->GetPrefs()->GetBoolean(
      brave_rewards::prefs::kBraveRewardsEnabled));
}

IN_PROC_BROWSER_TEST_F(BraveRewardsBrowserTest,
                       ProcessPendingContributions) {
  rewards_service_->AddObserver(this);
  rewards_service_->GetNotificationService()->AddObserver(this);

  alter_publisher_list_ = true;

  EnableRewards();

  // Claim grant using panel
  ClaimGrant(true);

  // Tip unverified publisher
  TipPublisher("brave.com");
  rewards_service_->OnTip("brave.com", 5.0, false);
  UpdateContributionBalance(5.0, false);  // update pending balance
  TipPublisher("3zsistemi.si", false, false, 2);
  TipPublisher("3zsistemi.si", false, false, 1);
  TipPublisher("3zsistemi.si", false, false, 2);
  TipPublisher("3zsistemi.si", false, false, 2);

  // Make sure that pending contribution box shows the correct
  // amount
  {
    content::EvalJsResult js_result = EvalJs(
        contents(),
        "const delay = t => new Promise(resolve => setTimeout(resolve, t));"
        "delay(500).then(() => "
        "  document.querySelector"
        "(\"[data-test-id='pending-contribution-box']\").innerText);",
        content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
        content::ISOLATED_WORLD_ID_CONTENT_END);
    EXPECT_NE(js_result.ExtractString().find(GetPendingBalance()),
              std::string::npos);
  }

  alter_publisher_list_ = false;

  browser()->tab_strip_model()->ActivateTabAt(
      2,
      TabStripModel::UserGestureDetails(TabStripModel::GestureType::kOther));

  // Open the Rewards popup
  content::WebContents* popup_contents = OpenRewardsPopup();
  ASSERT_TRUE(popup_contents);

  // Refresh publisher list
  {
    content::EvalJsResult jsResult = EvalJs(popup_contents,
      "new Promise((resolve) => {"
      "let count = 10;"
      "let interval = setInterval(function() {"
      "  if (count == 0) {"
      "    clearInterval(interval);"
      "    resolve(false);"
      "  } else {"
      "    count -= 1;"
      "  }"
      "  const button = "
      "document.querySelector(\"[data-test-id='unverified-check-button']\");"
      "  if (button) {"
      "    clearInterval(interval);"
      "    button.click();"
      "    resolve(true);"
      "  }"
      "}, 500);});",
      content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
      content::ISOLATED_WORLD_ID_CONTENT_END);
      ASSERT_TRUE(jsResult.ExtractBool());
  }

  // Activate the Rewards settings page tab
  browser()->tab_strip_model()->ActivateTabAt(
      0,
      TabStripModel::UserGestureDetails(TabStripModel::GestureType::kOther));

  // Wait for new verified publisher to be processed
  WaitForMultipleTipReconcileCompleted(3);
  ASSERT_EQ(multiple_tip_reconcile_status_, ledger::Result::LEDGER_OK);
  UpdateContributionBalance(-25.0, false);  // update pending balance

  // Make sure that balance is updated correctly
  {
    content::EvalJsResult js_result = EvalJs(
        contents(),
        "const delay = t => new Promise(resolve => setTimeout(resolve, t));"
        "delay(1000).then(() => "
        "  document.querySelector(\"[data-test-id='balance']\")"
        "    .innerText);",
        content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
        content::ISOLATED_WORLD_ID_CONTENT_END);
    EXPECT_NE(js_result.ExtractString().find(GetBalance()),
              std::string::npos);
  }

  // Check that wallet summary shows the appropriate tip amount
  {
    const std::string selector = "[data-test-id='summary-tips']";
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
    EXPECT_NE(js_result.ExtractString().find("-25.0BAT"),
              std::string::npos);
  }

  // Make sure that pending contribution box shows the correct
  // amount
  {
    content::EvalJsResult js_result = EvalJs(
        contents(),
        "const delay = t => new Promise(resolve => setTimeout(resolve, t));"
        "delay(0).then(() => "
        "  document.querySelector"
        "(\"[data-test-id='pending-contribution-box']\").innerText);",
        content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
        content::ISOLATED_WORLD_ID_CONTENT_END);
    EXPECT_NE(js_result.ExtractString().find(GetPendingBalance()),
              std::string::npos);
  }

  // Open the Rewards popup
  {
    content::WebContents* popup_contents = OpenRewardsPopup();
    ASSERT_TRUE(popup_contents);

    // Check if verified notification is shown
    content::EvalJsResult js_result = EvalJs(
        popup_contents,
        "const delay = t => new Promise(resolve => setTimeout(resolve, t));"
        "delay(500).then(() => "
        "  document.querySelector(\"[id='root']\").innerText);",
        content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
        content::ISOLATED_WORLD_ID_CONTENT_END);
    EXPECT_NE(js_result.ExtractString().find("3zsistemi.si"),
          std::string::npos);

    // Close notification
    ASSERT_TRUE(ExecJs(popup_contents,
        "  document.querySelector("
        "      \"[data-test-id='notification-close']\").click();",
        content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
        content::ISOLATED_WORLD_ID_CONTENT_END));

    // Check if insufficient funds notification is shown
    content::EvalJsResult js_result2 = EvalJs(
        popup_contents,
        "const delay = t => new Promise(resolve => setTimeout(resolve, t));"
        "delay(500).then(() => "
        "  document.querySelector(\"[id='root']\").innerText);",
        content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
        content::ISOLATED_WORLD_ID_CONTENT_END);
    EXPECT_NE(js_result2.ExtractString().find("Insufficient Funds"),
          std::string::npos);
  }

  // Stop observing the Rewards service
  rewards_service_->RemoveObserver(this);
}

IN_PROC_BROWSER_TEST_F(BraveRewardsBrowserTest,
                       NotVerifedWallet) {
  EnableRewards();

  // Click on verify button
  {
    ASSERT_TRUE(ExecJs(contents(),
        "  document.getElementById(\"verify-wallet-button\").click();",
        content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
        content::ISOLATED_WORLD_ID_CONTENT_END));
  }

  // Click on verify button in on boarding
  {
    ASSERT_TRUE(ExecJs(contents(),
        "  document.getElementById(\"on-boarding-verify-button\").click();",
        content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
        content::ISOLATED_WORLD_ID_CONTENT_END));
  }

  // Check if we are redirected to uphold
  {
    const GURL current_url = contents()->GetURL();
    ASSERT_TRUE(base::StartsWith(
        current_url.spec(),
        braveledger_uphold::GetUrl() + "/authorize/",
        base::CompareCase::INSENSITIVE_ASCII));
  }

  // Fake successful authentication
  ui_test_utils::NavigateToURLBlockUntilNavigationsComplete(
        browser(),
        uphold_auth_url(), 1);

  // Check if we are redirected to KYC page
  {
    const GURL current_url = contents()->GetURL();
    ASSERT_TRUE(base::StartsWith(
        current_url.spec(),
        braveledger_uphold::GetUrl() + "/signup/step2",
        base::CompareCase::INSENSITIVE_ASCII));
  }
}

IN_PROC_BROWSER_TEST_F(BraveRewardsBrowserTest,
                       TipWithVerifiedWallet) {
  rewards_service()->AddObserver(this);
  verified_wallet_ = true;
  external_balance_ = 50.0;

  auto wallet = ledger::ExternalWallet::New();
  wallet->token = "token";
  wallet->address = external_wallet_address_;
  wallet->status = 2;
  wallet->one_time_string = "";
  wallet->user_name = "Brave Test";
  wallet->transferred = true;
  rewards_service()->SaveExternalWallet("uphold", std::move(wallet));

  // Enable Rewards
  EnableRewards();

  // Tip verified publisher
  TipPublisher("duckduckgo.com", true);

  // Stop observing the Rewards service
  rewards_service()->RemoveObserver(this);
}

IN_PROC_BROWSER_TEST_F(BraveRewardsBrowserTest, TipConnectedPublisherAnon) {
  // Observe the Rewards service
  rewards_service_->AddObserver(this);

  // Enable Rewards
  EnableRewards();

  // Claim grant using settings page
  const bool use_panel = true;
  ClaimGrant(use_panel);

  // Tip verified publisher
  TipPublisher("bumpsmack.com", true);

  // Stop observing the Rewards service
  rewards_service_->RemoveObserver(this);
}

IN_PROC_BROWSER_TEST_F(
    BraveRewardsBrowserTest,
    TipConnectedPublisherAnonAndConnected) {
  // Observe the Rewards service
  rewards_service()->AddObserver(this);
  verified_wallet_ = true;
  external_balance_ = 50.0;

  auto wallet = ledger::ExternalWallet::New();
  wallet->token = "token";
  wallet->address = external_wallet_address_;
  wallet->status = 1;
  wallet->one_time_string = "";
  wallet->user_name = "Brave Test";
  wallet->transferred = true;
  rewards_service()->SaveExternalWallet("uphold", std::move(wallet));

  // Enable Rewards
  EnableRewards();

  // Claim grant using settings page
  const bool use_panel = true;
  ClaimGrant(use_panel);

  // Tip verified publisher
  TipPublisher("bumpsmack.com", true);

  // Stop observing the Rewards service
  rewards_service_->RemoveObserver(this);
}

IN_PROC_BROWSER_TEST_F(
    BraveRewardsBrowserTest,
    TipConnectedPublisherConnected) {
  // Observe the Rewards service
  rewards_service()->AddObserver(this);
  verified_wallet_ = true;
  external_balance_ = 50.0;

  auto wallet = ledger::ExternalWallet::New();
  wallet->token = "token";
  wallet->address = external_wallet_address_;
  wallet->status = 1;
  wallet->one_time_string = "";
  wallet->user_name = "Brave Test";
  wallet->transferred = true;
  rewards_service()->SaveExternalWallet("uphold", std::move(wallet));

  // Enable Rewards
  EnableRewards();

  // Tip verified publisher
  TipPublisher("bumpsmack.com", false);

  // Stop observing the Rewards service
  rewards_service_->RemoveObserver(this);
}

IN_PROC_BROWSER_TEST_F(
    BraveRewardsBrowserTest,
    TipConnectedPublisherVerified) {
  // Observe the Rewards service
  rewards_service()->AddObserver(this);
  verified_wallet_ = true;
  external_balance_ = 50.0;

  auto wallet = ledger::ExternalWallet::New();
  wallet->token = "token";
  wallet->address = external_wallet_address_;
  wallet->status = 2;
  wallet->one_time_string = "";
  wallet->user_name = "Brave Test";
  wallet->transferred = true;
  rewards_service()->SaveExternalWallet("uphold", std::move(wallet));

  // Enable Rewards
  EnableRewards();

  // Tip verified publisher
  TipPublisher("bumpsmack.com", false);

  // Stop observing the Rewards service
  rewards_service_->RemoveObserver(this);
}
