/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/strings/string_split.h"
#include "base/strings/stringprintf.h"
#include "base/memory/weak_ptr.h"
#include "base/test/bind_test_util.h"
#include "bat/ledger/internal/bat_helper.h"
#include "bat/ledger/internal/uphold/uphold_util.h"
#include "bat/ledger/internal/request/request_util.h"
#include "bat/ledger/internal/static_values.h"
#include "bat/ledger/ledger.h"
#include "brave/browser/extensions/api/brave_action_api.h"
#include "brave/common/brave_paths.h"
#include "brave/common/extensions/extension_constants.h"
#include "brave/components/brave_rewards/browser/rewards_service_browsertest_utils.h"
#include "brave/browser/brave_rewards/rewards_service_factory.h"
#include "brave/components/brave_rewards/browser/rewards_service_impl.h"
#include "brave/components/brave_rewards/browser/rewards_service_observer.h"
#include "brave/components/brave_rewards/browser/rewards_notification_service_impl.h"  // NOLINT
#include "brave/components/brave_rewards/browser/rewards_notification_service_observer.h"  // NOLINT
#include "brave/components/brave_rewards/common/pref_names.h"
#include "brave/browser/ui/views/brave_actions/brave_actions_container.h"
#include "brave/browser/ui/views/location_bar/brave_location_bar_view.h"
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

using braveledger_request_util::ServerTypes;

using RewardsNotificationType =
    brave_rewards::RewardsNotificationService::RewardsNotificationType;

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
                const ServerTypes& server) {
  const std::string target_url =
      braveledger_request_util::BuildUrl(path, prefix, server);
  return (url.find(target_url) == 0);
}

enum class ContributionType { OneTimeTip, MonthlyTip };

}  // namespace

namespace brave_test_resp {
  std::string registrarVK_;
  std::string verification_;
  std::string promotions_;
  std::string promotion_claim_;
  std::string promotion_tokens_;
  std::string captcha_;
  std::string wallet_properties_;
  std::string wallet_properties_defaults_;
  std::string uphold_auth_resp_;
  std::string uphold_transactions_resp_;
  std::string uphold_commit_resp_;
}  // namespace brave_test_resp

class BraveRewardsBrowserTest
    : public InProcessBrowserTest,
      public brave_rewards::RewardsServiceObserver,
      public brave_rewards::RewardsNotificationServiceObserver,
      public base::SupportsWeakPtr<BraveRewardsBrowserTest> {
 public:
  BraveRewardsBrowserTest() {
    // You can do set-up work for each test here
  }

  ~BraveRewardsBrowserTest() override {
    // You can do clean-up work that doesn't throw exceptions here
  }

  void SetUpOnMainThread() override {
    // Code here will be called immediately after the constructor (right before
    // each test)

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

    auto* browser_profile = browser()->profile();

    rewards_service_ = static_cast<brave_rewards::RewardsServiceImpl*>(
        brave_rewards::RewardsServiceFactory::GetForProfile(browser_profile));
    rewards_service_->ForTestingSetTestResponseCallback(
        base::BindRepeating(&BraveRewardsBrowserTest::GetTestResponse,
                            base::Unretained(this)));
    rewards_service_->AddObserver(this);
    if (!rewards_service_->IsWalletInitialized()) {
      WaitForWalletInitialization();
    }
    rewards_service_->SetLedgerEnvForTesting();
  }

  void TearDown() override {
    // Code here will be called immediately after each test (right before the
    // destructor)

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

  PrefService* GetPrefs() const {
    return browser()->profile()->GetPrefs();
  }

  bool IsRewardsEnabled() const {
    return GetPrefs()->GetBoolean(brave_rewards::prefs::kBraveRewardsEnabled);
  }

  std::string GetPromotionId() {
    return "6820f6a4-c6ef-481d-879c-d2c30c8928c3";
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

    const std::string status = verified_wallet_
        ? "ok"
        : "pending";

    const std::string name = "Test User";

    return base::StringPrintf(
      "{"
        "\"name\": \"%s\","
        "\"memberAt\": \"%s\","
        "\"status\": \"%s\","
        "\"currencies\": [\"BAT\"]"
      "}",
      name.c_str(),
      verified.c_str(),
      status.c_str());
  }

  std::vector<double> GetSiteBannerTipOptions(
      content::WebContents* site_banner) {
    rewards_service_browsertest_utils::WaitForElementToAppear(
        site_banner,
        "[data-test-id=amount-wrapper] div span");
    auto options = content::EvalJs(
        site_banner,
        R"(
            const delay = t => new Promise(resolve => setTimeout(resolve, t));
            delay(500).then(() => Array.prototype.map.call(
                document.querySelectorAll(
                    "[data-test-id=amount-wrapper] div span"),
                node => parseFloat(node.innerText)))
        )",
        content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
        content::ISOLATED_WORLD_ID_CONTENT_END).ExtractList();

    std::vector<double> result;
    for (const auto& value : options.GetList()) {
      result.push_back(value.GetDouble());
    }
    return result;
  }

  static std::vector<double> GetRewardsPopupTipOptions(
      content::WebContents* popup) {
    rewards_service_browsertest_utils::WaitForElementToAppear(
        popup,
        "option:not(:disabled)");
    auto options = content::EvalJs(
        popup,
        R"_(
          const delay = t => new Promise(resolve => setTimeout(resolve, t));
          delay(0).then(() =>
              Array.prototype.map.call(
                  document.querySelectorAll("option:not(:disabled)"),
                  node => parseFloat(node.value)))
        )_",
        content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
        content::ISOLATED_WORLD_ID_CONTENT_END).ExtractList();

    std::vector<double> result;
    for (const auto& value : options.GetList()) {
      result.push_back(value.GetDouble());
    }
    return result;
  }


  void GetTestResponse(const std::string& url,
                       int32_t method,
                       int* response_status_code,
                       std::string* response,
                       std::map<std::string, std::string>* headers) {
    request_made_ = true;
    std::vector<std::string> tmp = base::SplitString(url,
                                                     "/",
                                                     base::TRIM_WHITESPACE,
                                                     base::SPLIT_WANT_ALL);
    const std::string persona_url =
        braveledger_request_util::BuildUrl(REGISTER_PERSONA, PREFIX_V2);
    if (url.find(persona_url) == 0 && tmp.size() == 6) {
      *response = brave_test_resp::registrarVK_;
    } else if (URLMatches(url, REGISTER_PERSONA, PREFIX_V2,
                          ServerTypes::LEDGER) &&
               tmp.size() == 7) {
      *response = brave_test_resp::verification_;
    } else if (URLMatches(url, WALLET_PROPERTIES, PREFIX_V2,
                          ServerTypes::BALANCE)) {
      if (show_defaults_in_properties_) {
        *response = brave_test_resp::wallet_properties_defaults_;
      } else {
        *response = brave_test_resp::wallet_properties_;
      }
    } else if (URLMatches(url, "/promotions?", PREFIX_V1,
                          ServerTypes::kPromotion)) {
      *response = brave_test_resp::promotions_;
    } else if (URLMatches(url, "/promotions/", PREFIX_V1,
                          ServerTypes::kPromotion)) {
      if (url.find("claims") != std::string::npos) {
        *response = brave_test_resp::promotion_tokens_;
      } else {
        *response = brave_test_resp::promotion_claim_;
      }
    } else if (URLMatches(url, "/captchas", PREFIX_V1,
                          ServerTypes::kPromotion)) {
      *response = brave_test_resp::captcha_;
    } else if (URLMatches(url, GET_PUBLISHERS_LIST, "",
                          ServerTypes::PUBLISHER_DISTRO)) {
      if (alter_publisher_list_) {
        *response =
            "["
            "[\"bumpsmack.com\",\"publisher_verified\",false,\"address1\",{}],"
            "[\"duckduckgo.com\",\"wallet_connected\",false,\"address2\",{}],"
            "[\"laurenwags.github.io\",\"wallet_connected\",false,\"address2\","
              "{\"donationAmounts\": [5,10,20]}]"
            "]";
      } else {
        *response =
            "["
            "[\"bumpsmack.com\",\"publisher_verified\",false,\"address1\",{}],"
            "[\"duckduckgo.com\",\"wallet_connected\",false,\"address2\",{}],"
            "[\"3zsistemi.si\",\"wallet_connected\",false,\"address3\",{}],"
            "[\"site1.com\",\"wallet_connected\",false,\"address4\",{}],"
            "[\"site2.com\",\"wallet_connected\",false,\"address5\",{}],"
            "[\"site3.com\",\"wallet_connected\",false,\"address6\",{}],"
            "[\"laurenwags.github.io\",\"wallet_connected\",false,\"address2\","
              "{\"donationAmounts\": [5,10,20]}]"
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

  void WaitForPromotionInitialization() {
    if (promotion_initialized_)
      return;
    wait_for_promotion_initialization_loop_.reset(new base::RunLoop);
    wait_for_promotion_initialization_loop_->Run();
  }

  void WaitForPromotionFinished() {
    if (promotion_finished_)
      return;
    wait_for_promotion_finished_loop_.reset(new base::RunLoop);
    wait_for_promotion_finished_loop_->Run();
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

  void WaitForPendingTipToBeSaved() {
    if (pending_tip_saved_) {
      return;
    }
    wait_for_pending_tip_saved_loop_.reset(new base::RunLoop);
    wait_for_pending_tip_saved_loop_->Run();
  }

  void WaitForMultipleTipReconcileCompleted(int32_t needed) {
    multiple_tip_reconcile_needed_ = needed;
    if (multiple_tip_reconcile_completed_) {
      return;
    }

    wait_for_multiple_tip_completed_loop_.reset(new base::RunLoop);
    wait_for_multiple_tip_completed_loop_->Run();
  }

  void WaitForMultipleACReconcileCompleted(int32_t needed) {
    multiple_ac_reconcile_needed_ = needed;
    if (multiple_ac_reconcile_completed_) {
      return;
    }

    wait_for_multiple_ac_completed_loop_.reset(new base::RunLoop);
    wait_for_multiple_ac_completed_loop_->Run();
  }

  void WaitForInsufficientFundsNotification() {
    if (insufficient_notification_would_have_already_shown_) {
      return;
    }
    wait_for_insufficient_notification_loop_.reset(new base::RunLoop);
    wait_for_insufficient_notification_loop_->Run();
  }

  void WaitForRecurringTipToBeSaved() {
    if (recurring_tip_saved_) {
      return;
    }
    wait_for_recurring_tip_saved_loop_.reset(new base::RunLoop);
    wait_for_recurring_tip_saved_loop_->Run();
  }

  void AddNotificationServiceObserver() {
    rewards_service_->GetNotificationService()->AddObserver(this);
  }

  bool IsShowingNotificationForType(
      const RewardsNotificationType type) const {
    const auto& notifications = rewards_service_->GetAllNotifications();
    for (const auto& notification : notifications) {
      if (notification.second.type_ == type) {
        return true;
      }
    }

    return false;
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

  void GetEnvironment() {
    rewards_service()->GetEnvironment(
        base::Bind(&BraveRewardsBrowserTest::OnGetEnvironment,
          base::Unretained(this)));
  }

  void GetDebug() {
    rewards_service()->GetDebug(
        base::Bind(&BraveRewardsBrowserTest::OnGetDebug,
          base::Unretained(this)));
  }

  void OpenRewardsPopupRewardsEnabled() const {
    // Ask the popup to open
    std::string error;
    bool popup_shown = extensions::BraveActionAPI::ShowActionUI(
      browser(), brave_rewards_extension_id, nullptr, &error);
    if (!popup_shown) {
      LOG(ERROR) << "Could not open rewards popup: " << error;
    }
    EXPECT_TRUE(popup_shown);
  }

  void OpenRewardsPopupRewardsDisabled() const {
    BrowserView* browser_view =
      BrowserView::GetBrowserViewForBrowser(browser());
    BraveLocationBarView* brave_location_bar_view =
        static_cast<BraveLocationBarView*>(browser_view->GetLocationBarView());
    ASSERT_NE(brave_location_bar_view, nullptr);
    auto* brave_actions = brave_location_bar_view->brave_actions_;
    ASSERT_NE(brave_actions, nullptr);

    brave_actions->OnRewardsStubButtonClicked();
  }

  content::WebContents* OpenRewardsPopup() const {
    // Construct an observer to wait for the popup to load
    content::WebContents* popup_contents = nullptr;
    auto check_load_is_rewards_panel =
        [&](const content::NotificationSource& source,
            const content::NotificationDetails&) -> bool {
          auto web_contents_source =
              static_cast<const content::Source<content::WebContents>&>(source);
          popup_contents = web_contents_source.ptr();

          // Check that this notification is for the Rewards panel and not, say,
          // the extension background page.
          std::string url = popup_contents->GetLastCommittedURL().spec();
          std::string rewards_panel_url = std::string("chrome-extension://") +
              brave_rewards_extension_id + "/brave_rewards_panel.html";
          return url == rewards_panel_url;
        };
     content::WindowedNotificationObserver popup_observer(
         content::NOTIFICATION_LOAD_COMPLETED_MAIN_FRAME,
         base::BindLambdaForTesting(check_load_is_rewards_panel));

    if (IsRewardsEnabled()) {
      OpenRewardsPopupRewardsEnabled();
    } else {
      OpenRewardsPopupRewardsDisabled();
    }

    // Wait for the popup to load
    popup_observer.Wait();
    rewards_service_browsertest_utils::WaitForElementToAppear(
        popup_contents,
        "[data-test-id='rewards-panel']");

    return popup_contents;
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

    ASSERT_TRUE(base::ReadFileToString(path.AppendASCII("promotions_resp.json"),
                                       &brave_test_resp::promotions_));

    ASSERT_TRUE(base::ReadFileToString(path.AppendASCII("captcha_resp.json"),
                                       &brave_test_resp::captcha_));
    ASSERT_TRUE(
        base::ReadFileToString(path.AppendASCII("promotion_claim_resp.json"),
                               &brave_test_resp::promotion_claim_));
    ASSERT_TRUE(
        base::ReadFileToString(path.AppendASCII("promotion_tokens_resp.json"),
                               &brave_test_resp::promotion_tokens_));
    ASSERT_TRUE(
        base::ReadFileToString(path.AppendASCII("wallet_properties_resp.json"),
                               &brave_test_resp::wallet_properties_));
    ASSERT_TRUE(base::ReadFileToString(
        path.AppendASCII("wallet_properties_resp_defaults.json"),
        &brave_test_resp::wallet_properties_defaults_));
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

  static std::string BalanceDoubleToString(double amount) {
    return base::StringPrintf("%.1f", amount);
  }

  std::string GetBalance() const {
    return BalanceDoubleToString(balance_ + external_balance_);
  }

  std::string GetPendingBalance() const {
    return BalanceDoubleToString(pending_balance_);
  }

  std::string GetExternalBalance() {
    return BalanceDoubleToString(external_balance_);
  }

  std::string GetAnonBalance() const {
    return BalanceDoubleToString(balance_);
  }

  GURL rewards_url() {
    GURL rewards_url("brave://rewards");
    return rewards_url;
  }

  GURL new_tab_url() {
    GURL new_tab_url("brave://newtab");
    return new_tab_url;
  }

  GURL uphold_auth_url() {
    GURL url("chrome://rewards/uphold/authorization?"
             "code=0c42b34121f624593ee3b04cbe4cc6ddcd72d&state=123456789");
    return url;
  }

  content::WebContents* contents() const {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  void EnableRewards(bool use_new_tab = false) {
    // Load rewards page
    GURL page_url = use_new_tab ? new_tab_url() : rewards_url();
    ui_test_utils::NavigateToURL(browser(), page_url);
    WaitForLoadStop(contents());
    // Opt in and create wallet to enable rewards
    rewards_service_browsertest_utils::WaitForElementThenClick(
        contents(),
        "[data-test-id='optInAction']");
    rewards_service_browsertest_utils::WaitForElementToAppear(
        contents(),
        "[data-test-id2='enableMain']");
  }

  void EnableRewardsViaCode() {
    base::RunLoop run_loop;
    bool wallet_created = false;
    rewards_service_->CreateWallet(
        base::BindLambdaForTesting([&](int32_t result) {
          wallet_created =
              (result == static_cast<int32_t>(ledger::Result::WALLET_CREATED));
          run_loop.Quit();
        }));

    run_loop.Run();

    ASSERT_TRUE(wallet_created);
    ASSERT_TRUE(IsRewardsEnabled());
  }

  brave_rewards::RewardsServiceImpl* rewards_service() {
    return rewards_service_;
  }

  // Use this function only if you are testing claim flow
  // otherwise always use ClaimPromotionViaCode to seep things up
  void ClaimPromotion(bool use_panel) {
    // Wait for promotion to initialize
    WaitForPromotionInitialization();

    // Use the appropriate WebContents
    content::WebContents *contents =
        use_panel ? OpenRewardsPopup() : BraveRewardsBrowserTest::contents();
    ASSERT_TRUE(contents);

    // Claim promotion via settings page or panel, as instructed
    if (use_panel) {
      rewards_service_browsertest_utils::WaitForElementThenClick(
          contents,
          "button");
    } else {
      rewards_service_browsertest_utils::WaitForElementThenClick(
          contents,
          "[data-test-id='claimGrant']");
    }

    // Wait for CAPTCHA
    rewards_service_browsertest_utils::WaitForElementToAppear(
        contents,
        "[data-test-id='captcha']");

    rewards_service_browsertest_utils::DragAndDrop(
        contents,
        "[data-test-id=\"captcha-triangle\"]",
        "[data-test-id=\"captcha-drop\"]");

    WaitForPromotionFinished();

    // Ensure that promotion looks as expected
    EXPECT_STREQ(std::to_string(promotion_.amount).c_str(), "30.000000");
    EXPECT_STREQ(promotion_.promotion_id.c_str(), GetPromotionId().c_str());
    EXPECT_EQ(promotion_.type, 0u);
    EXPECT_EQ(promotion_.expires_at, 1740816427ull);
    balance_ += 30;

    // Check that promotion notification shows the appropriate amount
    const std::string selector =
        use_panel ? "[id='root']" : "[data-test-id='newTokenGrant']";
    rewards_service_browsertest_utils::WaitForElementToContain(
        contents,
        selector,
        "Free Token Grant");
    rewards_service_browsertest_utils::WaitForElementToContain(
        contents,
        selector,
        "30.0 BAT");

    // Dismiss the promotion notification
    if (use_panel) {
      rewards_service_browsertest_utils::WaitForElementThenClick(
          contents, "#"
                    "grant-completed-ok");
    }
  }

  void ClaimPromotionViaCode() {
    // Wait for promotion to initialize
    WaitForPromotionInitialization();

    const std::string solution = R"(
    {
      "captchaId": "a78e549f-904d-425e-9736-54f693117e01",
      "x": 1,
      "y": 1
    })";
    rewards_service_->AttestPromotion(
        GetPromotionId(),
        solution,
        base::DoNothing());
    WaitForPromotionFinished();
    balance_ += 30;
  }

  void VisitPublisher(const std::string& publisher,
                      bool verified,
                      bool last_add = false) {
    GURL url = https_server()->GetURL(publisher, "/index.html");
    ui_test_utils::NavigateToURLWithDisposition(
        browser(), url, WindowOpenDisposition::NEW_FOREGROUND_TAB,
        ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP);

    // The minimum publisher duration when testing is 1 second (and the
    // granularity is seconds), so wait for just over 2 seconds to elapse
    base::PlatformThread::Sleep(base::TimeDelta::FromMilliseconds(2100));

    // Activate the Rewards settings page tab
    ActivateTabAtIndex(0);

    // Wait for publisher list normalization
    WaitForPublisherListNormalized();

    // Make sure site appears in auto-contribute table
    rewards_service_browsertest_utils::WaitForElementToEqual(
        contents(),
        "[data-test-id='ac_link_" + publisher + "']",
        publisher);

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

  std::string RewardsPageTipSummaryAmount() const {
    const std::string amount =
        rewards_service_browsertest_utils::WaitForElementThenGetContent(
        contents(),
        "[data-test-id=summary-tips] [color=contribute] span span");
    return amount + " BAT";
  }

  std::string ExpectedTipSummaryAmountString() const {
    // The tip summary page formats 2.4999 as 2.4, so we do the same here.
    double truncated_amount = floor(reconciled_tip_total_ * 10) / 10;
    return BalanceDoubleToString(-truncated_amount);
  }

  void ActivateTabAtIndex(int index) const {
    browser()->tab_strip_model()->ActivateTabAt(
        index,
        TabStripModel::UserGestureDetails(TabStripModel::GestureType::kOther));
  }

  void RefreshPublisherListUsingRewardsPopup() const {
    rewards_service_browsertest_utils::WaitForElementThenClick(
        OpenRewardsPopup(),
        "[data-test-id='unverified-check-button']");
  }

  content::WebContents* OpenSiteBanner(ContributionType banner_type) const {
    content::WebContents* popup_contents = OpenRewardsPopup();

    // Construct an observer to wait for the site banner to load.
    content::WindowedNotificationObserver site_banner_observer(
        content::NOTIFICATION_LOAD_COMPLETED_MAIN_FRAME,
        content::NotificationService::AllSources());

    const std::string buttonSelector =
        banner_type == ContributionType::MonthlyTip
        ? "[type='tip-monthly']"
        : "[type='tip']";

    // Click button to initiate sending a tip.
    rewards_service_browsertest_utils::WaitForElementThenClick(
        popup_contents,
        buttonSelector);

    // Wait for the site banner to load
    site_banner_observer.Wait();

    // Retrieve the notification source
    const auto& site_banner_source =
        static_cast<const content::Source<content::WebContents>&>(
            site_banner_observer.source());

    // Allow the site banner to update its UI. We cannot use ExecJs here,
    // because it does not resolve promises.
    (void)EvalJs(site_banner_source.ptr(),
        "new Promise(resolve => setTimeout(resolve, 0))",
        content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
        content::ISOLATED_WORLD_ID_CONTENT_END);

    return site_banner_source.ptr();
  }

  void TipPublisher(
      const std::string& publisher,
      ContributionType type,
      bool should_contribute = false,
      int32_t selection = 0) {
    // we shouldn't be adding publisher to AC list,
    // so that we can focus only on tipping part
    rewards_service_->SetPublisherMinVisitTime(8);

    // Navigate to a site in a new tab
    GURL url = https_server()->GetURL(publisher, "/index.html");
    ui_test_utils::NavigateToURLWithDisposition(
        browser(), url, WindowOpenDisposition::NEW_FOREGROUND_TAB,
        ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP);

    content::WebContents* site_banner_contents = OpenSiteBanner(type);
    ASSERT_TRUE(site_banner_contents);

    std::vector<double> tip_options =
        GetSiteBannerTipOptions(site_banner_contents);
    const double amount = tip_options.at(selection);
    const std::string amount_str = base::StringPrintf("%2.1f", amount);

    // Select the tip amount (default is 1.0 BAT)
    std::string amount_selector = base::StringPrintf(
        "div:nth-of-type(%u)>[data-test-id=amount-wrapper]",
        selection + 1);
    rewards_service_browsertest_utils::WaitForElementThenClick(
        site_banner_contents,
        amount_selector);

    // Send the tip
    rewards_service_browsertest_utils::WaitForElementThenClick(
        site_banner_contents,
        "[data-test-id='send-tip-button']");

    // Signal that direct tip was made and update wallet with new
    // balance
    if (type == ContributionType::OneTimeTip && !should_contribute) {
      WaitForPendingTipToBeSaved();
      UpdateContributionBalance(amount, should_contribute);
    }

    // Wait for thank you banner to load
    ASSERT_TRUE(WaitForLoadStop(site_banner_contents));

    const std::string confirmationText = type == ContributionType::MonthlyTip
        ? "Monthly contribution has been set!"
        : "Tip sent!";

    if (type == ContributionType::MonthlyTip) {
      WaitForRecurringTipToBeSaved();
      // Trigger contribution process
      rewards_service()->StartMonthlyContributionForTest();

      // Wait for reconciliation to complete
      if (should_contribute) {
        WaitForTipReconcileCompleted();
        const auto result = should_contribute
            ? ledger::Result::LEDGER_OK
            : ledger::Result::RECURRING_TABLE_EMPTY;
        ASSERT_EQ(tip_reconcile_status_, result);
      }

      // Signal that monthly contribution was made and update wallet
      // with new balance
      if (!should_contribute) {
        UpdateContributionBalance(amount, should_contribute);
      }
    } else if (type == ContributionType::OneTimeTip && should_contribute) {
      // Wait for reconciliation to complete
      WaitForTipReconcileCompleted();
      ASSERT_EQ(tip_reconcile_status_, ledger::Result::LEDGER_OK);
    }

    // Make sure that thank you banner shows correct publisher data
    // (domain and amount)
    {
      rewards_service_browsertest_utils::WaitForElementToContain(
          site_banner_contents,
          "body",
          confirmationText);
      rewards_service_browsertest_utils::WaitForElementToContain(
          site_banner_contents,
          "body",
          amount_str + " BAT");
      rewards_service_browsertest_utils::WaitForElementToContain(
          site_banner_contents,
          "body",
          "Share the good news:");
      rewards_service_browsertest_utils::WaitForElementToContain(
          site_banner_contents,
          "body",
          "" + GetBalance() + " BAT");
    }

    VerifyTip(amount, should_contribute, type == ContributionType::MonthlyTip);
  }

  void VerifyTip(
      const double amount,
      const bool should_contribute,
      const bool monthly,
      const bool via_code = false) {
    if (via_code && monthly) {
      return;
    }

    // Activate the Rewards settings page tab
    ActivateTabAtIndex(0);

    if (should_contribute) {
      // Make sure that balance is updated correctly
      IsBalanceCorrect();

      // Check that tip table shows the appropriate tip amount
      const std::string selector = monthly
          ? "[data-test-id='summary-monthly']"
          : "[data-test-id='summary-tips']";

      rewards_service_browsertest_utils::WaitForElementToContain(
          contents(),
          selector,
          "-" + BalanceDoubleToString(amount) + "BAT");
      return;
    }

    // Make sure that balance did not change
    IsBalanceCorrect();

    // Make sure that pending contribution box shows the correct
    // amount
    IsPendingBalanceCorrect();

    rewards_service_browsertest_utils::WaitForElementToEqual(
        contents(),
        "#tip-box-total",
        "0.0BAT0.00 USD");
  }

  void IsBalanceCorrect() {
    const std::string balance = GetBalance() + " BAT";
    rewards_service_browsertest_utils::WaitForElementToEqual(
        contents(),
        "[data-test-id='balance']",
        balance);
  }

  void IsPendingBalanceCorrect() {
    const std::string balance = GetPendingBalance() + " BAT";
    rewards_service_browsertest_utils::WaitForElementToContain(
        contents(),
        "[data-test-id='pending-contribution-box']",
        balance);
  }

  void OnWalletInitialized(brave_rewards::RewardsService* rewards_service,
                           int32_t result) {
    const auto converted_result = static_cast<ledger::Result>(result);
    ASSERT_TRUE(converted_result == ledger::Result::WALLET_CREATED ||
                converted_result == ledger::Result::NO_LEDGER_STATE ||
                converted_result == ledger::Result::LEDGER_OK);
    wallet_initialized_ = true;
    if (wait_for_wallet_initialization_loop_)
      wait_for_wallet_initialization_loop_->Quit();
  }

  void OnFetchPromotions(
      brave_rewards::RewardsService* rewards_service,
      unsigned int result,
      const std::vector<brave_rewards::Promotion>& promotions) {
    ASSERT_EQ(static_cast<ledger::Result>(result), ledger::Result::LEDGER_OK);
    promotion_initialized_ = true;
    if (wait_for_promotion_initialization_loop_)
      wait_for_promotion_initialization_loop_->Quit();
  }

  void OnPromotionFinished(
      brave_rewards::RewardsService* rewards_service,
      const uint32_t result,
      brave_rewards::Promotion promotion) {
    ASSERT_EQ(static_cast<ledger::Result>(result), ledger::Result::LEDGER_OK);
    promotion_finished_ = true;
    promotion_ = promotion;
    if (wait_for_promotion_finished_loop_) {
      wait_for_promotion_finished_loop_->Quit();
    }
  }

  void OnPublisherListNormalized(brave_rewards::RewardsService* rewards_service,
                                 const brave_rewards::ContentSiteList& list) {
    if (list.size() == 0)
      return;
    publisher_list_normalized_ = true;
    if (wait_for_publisher_list_normalized_loop_)
      wait_for_publisher_list_normalized_loop_->Quit();
  }

  void OnReconcileComplete(
      brave_rewards::RewardsService* rewards_service,
      unsigned int result,
      const std::string& contribution_id,
      const double amount,
      const int32_t type) {
    const auto converted_result = static_cast<ledger::Result>(result);
    const auto converted_type =
        static_cast<ledger::RewardsType>(type);

    if (converted_result == ledger::Result::LEDGER_OK) {
      UpdateContributionBalance(amount, true);
    }

    if (converted_type == ledger::RewardsType::AUTO_CONTRIBUTE) {
      ac_reconcile_completed_ = true;
      ac_reconcile_status_ = converted_result;
      if (wait_for_ac_completed_loop_) {
        wait_for_ac_completed_loop_->Quit();
      }

      // Multiple ac
      multiple_ac_reconcile_count_++;
      multiple_ac_reconcile_status_.push_back(converted_result);

      if (multiple_ac_reconcile_count_ == multiple_ac_reconcile_needed_) {
        multiple_ac_reconcile_completed_ = true;
        if (wait_for_multiple_ac_completed_loop_) {
          wait_for_multiple_ac_completed_loop_->Quit();
        }
      }
    }

    if (converted_type == ledger::RewardsType::ONE_TIME_TIP ||
        converted_type == ledger::RewardsType::RECURRING_TIP) {
      if (converted_result == ledger::Result::LEDGER_OK) {
        reconciled_tip_total_ += amount;
      }

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

  void OnRecurringTipSaved(
      brave_rewards::RewardsService* rewards_service,
      bool success) {
    if (!success) {
      return;
    }

    recurring_tip_saved_ = true;
    if (wait_for_recurring_tip_saved_loop_) {
      wait_for_recurring_tip_saved_loop_->Quit();
    }
  }

  void OnPendingContributionSaved(
      brave_rewards::RewardsService* rewards_service,
      int result) {
    if (result != 0) {
      return;
    }

    pending_tip_saved_ = true;
    if (wait_for_pending_tip_saved_loop_) {
      wait_for_pending_tip_saved_loop_->Quit();
    }
  }

  void OnNotificationAdded(
      brave_rewards::RewardsNotificationService* rewards_notification_service,
      const brave_rewards::RewardsNotificationService::RewardsNotification&
      notification) {
    const auto& notifications =
        rewards_notification_service->GetAllNotifications();

    for (const auto& notification : notifications) {
      switch (notification.second.type_) {
        case brave_rewards::RewardsNotificationService::
            RewardsNotificationType::REWARDS_NOTIFICATION_INSUFFICIENT_FUNDS: {
          insufficient_notification_would_have_already_shown_ = true;
          if (wait_for_insufficient_notification_loop_) {
            wait_for_insufficient_notification_loop_->Quit();
          }

          break;
        }
        default: {
          break;
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

  void TipViaCode(
      const std::string publisher_key,
      const double amount,
      const ledger::PublisherStatus status,
      const bool should_contribute = false,
      const bool recurring = false,
      const ledger::Result result = ledger::Result::LEDGER_OK) {
    tip_reconcile_completed_ = false;
    pending_tip_saved_ = false;

    auto site = std::make_unique<brave_rewards::ContentSite>();
    site->id = publisher_key;
    site->name = publisher_key;
    site->url = publisher_key;
    site->status = static_cast<int>(status);
    site->provider = "";
    site->favicon_url = "";
    rewards_service_->OnTip(publisher_key, amount, recurring, std::move(site));

    if (recurring) {
      return;
    }

    if (should_contribute) {
      // Wait for reconciliation to complete
      WaitForTipReconcileCompleted();
      ASSERT_EQ(tip_reconcile_status_, result);
      return;
    }

    // Signal to update pending contribution balance
    WaitForPendingTipToBeSaved();
    UpdateContributionBalance(amount, should_contribute);
  }

  void SetUpUpholdWallet(
      const double balance,
      const ledger::WalletStatus status = ledger::WalletStatus::VERIFIED) {
    verified_wallet_ = true;
    external_balance_ = balance;

    auto wallet = ledger::ExternalWallet::New();
    wallet->token = "token";
    wallet->address = external_wallet_address_;
    wallet->status = status;
    wallet->one_time_string = "";
    wallet->user_name = "Brave Test";
    wallet->transferred = true;
    rewards_service()->SaveExternalWallet("uphold", std::move(wallet));
  }

  MOCK_METHOD1(OnGetEnvironment, void(ledger::Environment));
  MOCK_METHOD1(OnGetDebug, void(bool));
  MOCK_METHOD1(OnGetReconcileTime, void(int32_t));
  MOCK_METHOD1(OnGetShortRetries, void(bool));

  std::unique_ptr<net::EmbeddedTestServer> https_server_;

  brave_rewards::RewardsServiceImpl* rewards_service_;

  brave_rewards::Promotion promotion_;

  std::unique_ptr<base::RunLoop> wait_for_wallet_initialization_loop_;
  bool wallet_initialized_ = false;

  std::unique_ptr<base::RunLoop> wait_for_promotion_initialization_loop_;
  bool promotion_initialized_ = false;

  std::unique_ptr<base::RunLoop> wait_for_promotion_finished_loop_;
  bool promotion_finished_ = false;

  std::unique_ptr<base::RunLoop> wait_for_publisher_list_normalized_loop_;
  bool publisher_list_normalized_ = false;

  std::unique_ptr<base::RunLoop> wait_for_ac_completed_loop_;
  bool ac_reconcile_completed_ = false;
  ledger::Result ac_reconcile_status_ = ledger::Result::LEDGER_ERROR;
  std::unique_ptr<base::RunLoop> wait_for_tip_completed_loop_;

  std::unique_ptr<base::RunLoop> wait_for_multiple_ac_completed_loop_;
  bool multiple_ac_reconcile_completed_ = false;
  int32_t multiple_ac_reconcile_count_ = 0;
  int32_t multiple_ac_reconcile_needed_ = 0;
  std::vector<ledger::Result> multiple_ac_reconcile_status_;

  bool tip_reconcile_completed_ = false;
  ledger::Result tip_reconcile_status_ = ledger::Result::LEDGER_ERROR;

  std::unique_ptr<base::RunLoop> wait_for_multiple_tip_completed_loop_;
  bool multiple_tip_reconcile_completed_ = false;
  int32_t multiple_tip_reconcile_count_ = 0;
  int32_t multiple_tip_reconcile_needed_ = 0;
  ledger::Result multiple_tip_reconcile_status_ = ledger::Result::LEDGER_ERROR;

  std::unique_ptr<base::RunLoop> wait_for_insufficient_notification_loop_;
  bool insufficient_notification_would_have_already_shown_ = false;

  std::unique_ptr<base::RunLoop> wait_for_recurring_tip_saved_loop_;
  bool recurring_tip_saved_ = false;

  std::unique_ptr<base::RunLoop> wait_for_pending_tip_saved_loop_;
  bool pending_tip_saved_ = false;

  std::unique_ptr<base::RunLoop> wait_for_attestation_loop_;

  bool last_publisher_added_ = false;
  bool alter_publisher_list_ = false;
  bool show_defaults_in_properties_ = false;
  bool request_made_ = false;
  double balance_ = 0;
  double reconciled_tip_total_ = 0;
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
  rewards_service_browsertest_utils::WaitForElementThenClick(
      contents(),
      "[data-test-id2='enableMain']");
  std::string value =
      rewards_service_browsertest_utils::WaitForElementThenGetAttribute(
        contents(),
        "[data-test-id2='enableMain']",
        "data-toggled");
  ASSERT_STREQ(value.c_str(), "false");

  // Toggle rewards back on
  rewards_service_browsertest_utils::WaitForElementThenClick(
      contents(),
      "[data-test-id2='enableMain']");
  value = rewards_service_browsertest_utils::WaitForElementThenGetAttribute(
      contents(),
      "[data-test-id2='enableMain']",
      "data-toggled");
  ASSERT_STREQ(value.c_str(), "true");
}

IN_PROC_BROWSER_TEST_F(BraveRewardsBrowserTest, ToggleAutoContribute) {
  EnableRewards();

  // once rewards has loaded, reload page to activate auto-contribute
  contents()->GetController().Reload(content::ReloadType::NORMAL, true);
  EXPECT_TRUE(WaitForLoadStop(contents()));

  // toggle auto contribute off
  rewards_service_browsertest_utils::WaitForElementThenClick(
      contents(),
      "[data-test-id2='autoContribution']");
  std::string value =
      rewards_service_browsertest_utils::WaitForElementThenGetAttribute(
        contents(),
        "[data-test-id2='autoContribution']",
        "data-toggled");
  ASSERT_STREQ(value.c_str(), "false");

  // toggle auto contribute back on
  rewards_service_browsertest_utils::WaitForElementThenClick(
      contents(),
      "[data-test-id2='autoContribution']");
  value = rewards_service_browsertest_utils::WaitForElementThenGetAttribute(
      contents(),
      "[data-test-id2='autoContribution']",
      "data-toggled");
  ASSERT_STREQ(value.c_str(), "true");
}

IN_PROC_BROWSER_TEST_F(BraveRewardsBrowserTest, ActivateSettingsModal) {
  EnableRewards();

  rewards_service_browsertest_utils::WaitForElementThenClick(
      contents(),
      "[data-test-id='settingsButton']");
  rewards_service_browsertest_utils::WaitForElementToAppear(
      contents(),
      "#modal");
}

IN_PROC_BROWSER_TEST_F(BraveRewardsBrowserTest, HandleFlagsSingleArg) {
  testing::InSequence s;
  // SetEnvironment(ledger::Environment::PRODUCTION)
  EXPECT_CALL(*this, OnGetEnvironment(ledger::Environment::PRODUCTION));
  // Staging - true and 1
  EXPECT_CALL(*this, OnGetEnvironment(ledger::Environment::STAGING)).Times(2);
  // Staging - false and random
  EXPECT_CALL(*this, OnGetEnvironment(
      ledger::Environment::PRODUCTION)).Times(2);

  rewards_service()->SetEnvironment(ledger::Environment::PRODUCTION);
  GetEnvironment();
  RunUntilIdle();

  // Staging - true
  rewards_service()->SetEnvironment(ledger::Environment::PRODUCTION);
  rewards_service()->HandleFlags("staging=true");
  GetEnvironment();
  RunUntilIdle();

  // Staging - 1
  rewards_service()->SetEnvironment(ledger::Environment::PRODUCTION);
  rewards_service()->HandleFlags("staging=1");
  GetEnvironment();
  RunUntilIdle();

  // Staging - false
  rewards_service()->SetEnvironment(ledger::Environment::STAGING);
  rewards_service()->HandleFlags("staging=false");
  GetEnvironment();
  RunUntilIdle();

  // Staging - random
  rewards_service()->SetEnvironment(ledger::Environment::STAGING);
  rewards_service()->HandleFlags("staging=werwe");
  GetEnvironment();
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

  // SetEnvironment(ledger::Environment::PRODUCTION)
  EXPECT_CALL(*this, OnGetEnvironment(ledger::Environment::PRODUCTION));
  // Development - true and 1
  EXPECT_CALL(
      *this,
      OnGetEnvironment(ledger::Environment::DEVELOPMENT)).Times(2);
  // Development - false and random
  EXPECT_CALL(
      *this,
      OnGetEnvironment(ledger::Environment::PRODUCTION)).Times(2);

  rewards_service()->SetEnvironment(ledger::Environment::PRODUCTION);
  GetEnvironment();
  RunUntilIdle();

  // Development - true
  rewards_service()->SetEnvironment(ledger::Environment::PRODUCTION);
  rewards_service()->HandleFlags("development=true");
  GetEnvironment();
  RunUntilIdle();

  // Development - 1
  rewards_service()->SetEnvironment(ledger::Environment::PRODUCTION);
  rewards_service()->HandleFlags("development=1");
  GetEnvironment();
  RunUntilIdle();

  // Development - false
  rewards_service()->SetEnvironment(ledger::Environment::PRODUCTION);
  rewards_service()->HandleFlags("development=false");
  GetEnvironment();
  RunUntilIdle();

  // Development - random
  rewards_service()->SetEnvironment(ledger::Environment::PRODUCTION);
  rewards_service()->HandleFlags("development=werwe");
  GetEnvironment();
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
  EXPECT_CALL(*this, OnGetEnvironment(ledger::Environment::STAGING));
  EXPECT_CALL(*this, OnGetDebug(true));
  EXPECT_CALL(*this, OnGetReconcileTime(10));
  EXPECT_CALL(*this, OnGetShortRetries(true));

  rewards_service()->SetEnvironment(ledger::Environment::PRODUCTION);
  rewards_service()->SetDebug(true);
  rewards_service()->SetReconcileTime(0);
  rewards_service()->SetShortRetries(false);

  rewards_service()->HandleFlags(
      "staging=true,debug=true,short-retries=true,reconcile-interval=10");

  GetReconcileTime();
  GetShortRetries();
  GetEnvironment();
  GetDebug();
  RunUntilIdle();
}

IN_PROC_BROWSER_TEST_F(BraveRewardsBrowserTest, HandleFlagsWrongInput) {
  EXPECT_CALL(*this, OnGetEnvironment(ledger::Environment::PRODUCTION));
  EXPECT_CALL(*this, OnGetDebug(false));
  EXPECT_CALL(*this, OnGetReconcileTime(0));
  EXPECT_CALL(*this, OnGetShortRetries(false));

  rewards_service()->SetEnvironment(ledger::Environment::PRODUCTION);
  rewards_service()->SetDebug(false);
  rewards_service()->SetReconcileTime(0);
  rewards_service()->SetShortRetries(false);

  rewards_service()->HandleFlags(
      "staging=,debug=,shortretries=true,reconcile-interval");

  GetReconcileTime();
  GetShortRetries();
  GetDebug();
  GetEnvironment();
  RunUntilIdle();
}

// #1 - Claim promotion via settings page
IN_PROC_BROWSER_TEST_F(BraveRewardsBrowserTest, ClaimPromotionViaSettingsPage) {
  // Enable Rewards
  EnableRewards();

  // Claim and verify promotion using settings page
  const bool use_panel = false;
  ClaimPromotion(use_panel);
}

// #2 - Claim promotion via panel
IN_PROC_BROWSER_TEST_F(BraveRewardsBrowserTest, ClaimPromotionViaPanel) {
  // Enable Rewards
  EnableRewards();

  // Claim and verify promotion using panel
  const bool use_panel = true;
  ClaimPromotion(use_panel);
}

// #3 - Panel shows correct publisher data
IN_PROC_BROWSER_TEST_F(BraveRewardsBrowserTest,
                       PanelShowsCorrectPublisherData) {
  // Enable Rewards
  EnableRewardsViaCode();

  // Navigate to a verified site in a new tab
  const std::string publisher = "duckduckgo.com";
  GURL url = https_server()->GetURL(publisher, "/index.html");
  ui_test_utils::NavigateToURLWithDisposition(
      browser(), url, WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP);

  // Open the Rewards popup
  content::WebContents* popup_contents = OpenRewardsPopup();
  ASSERT_TRUE(popup_contents);

  // Retrieve the inner text of the wallet panel and verify that it
  // looks as expected
  rewards_service_browsertest_utils::WaitForElementToContain(
      popup_contents,
      "[id='wallet-panel']",
      "Brave Verified Creator");
  rewards_service_browsertest_utils::WaitForElementToContain(
      popup_contents,
      "[id='wallet-panel']",
      publisher);

  // Retrieve the inner HTML of the wallet panel and verify that it
  // contains the expected favicon
  {
    const std::string favicon =
        "chrome://favicon/size/64@1x/https://" + publisher;
    rewards_service_browsertest_utils::WaitForElementToContainHTML(
        popup_contents,
        "#wallet-panel",
        favicon);
  }
}

// #4a - Visit verified publisher
IN_PROC_BROWSER_TEST_F(BraveRewardsBrowserTest, VisitVerifiedPublisher) {
  // Enable Rewards
  EnableRewards();

  // Visit verified publisher
  const bool verified = true;
  VisitPublisher("duckduckgo.com", verified);
}

// #4b - Visit unverified publisher
IN_PROC_BROWSER_TEST_F(BraveRewardsBrowserTest, VisitUnverifiedPublisher) {
  // Enable Rewards
  EnableRewards();

  // Visit unverified publisher
  const bool verified = false;
  VisitPublisher("brave.com", verified);
}

// #5 - Auto contribution
IN_PROC_BROWSER_TEST_F(BraveRewardsBrowserTest, AutoContribution) {
  // Enable Rewards
  EnableRewards();

  ClaimPromotionViaCode();

  // Visit verified publisher
  const bool verified = true;
  VisitPublisher("duckduckgo.com", verified);

  // Trigger contribution process
  rewards_service()->StartMonthlyContributionForTest();

  // Wait for reconciliation to complete successfully
  WaitForACReconcileCompleted();
  ASSERT_EQ(ac_reconcile_status_, ledger::Result::LEDGER_OK);

  // Make sure that balance is updated correctly
  IsBalanceCorrect();

  // Check that summary table shows the appropriate contribution
  rewards_service_browsertest_utils::WaitForElementToContain(
      contents(),
      "[color=contribute]",
      "-20.0BAT");
}

IN_PROC_BROWSER_TEST_F(BraveRewardsBrowserTest, AutoContributeWhenACOff) {
  EnableRewards();

  ClaimPromotionViaCode();

  // Visit verified publisher
  const bool verified = true;
  VisitPublisher("duckduckgo.com", verified);

  // toggle auto contribute off
  rewards_service_browsertest_utils::WaitForElementThenClick(
      contents(),
      "[data-test-id2='autoContribution']");
  std::string value =
      rewards_service_browsertest_utils::WaitForElementThenGetAttribute(
        contents(),
        "[data-test-id2='autoContribution']",
        "data-toggled");
  ASSERT_STREQ(value.c_str(), "false");

  // Trigger contribution process
  rewards_service()->StartMonthlyContributionForTest();
}

// #6 - Tip verified publisher
IN_PROC_BROWSER_TEST_F(BraveRewardsBrowserTest, TipVerifiedPublisher) {
  // Enable Rewards
  EnableRewards();

  ClaimPromotionViaCode();

  // Tip verified publisher
  TipPublisher("duckduckgo.com", ContributionType::OneTimeTip, true);
}

// #7 - Tip unverified publisher
IN_PROC_BROWSER_TEST_F(BraveRewardsBrowserTest, TipUnverifiedPublisher) {
  // Enable Rewards
  EnableRewards();

  ClaimPromotionViaCode();

  // Tip unverified publisher
  TipPublisher("brave.com", ContributionType::OneTimeTip);
}

// #8 - Recurring tip for verified publisher
IN_PROC_BROWSER_TEST_F(BraveRewardsBrowserTest,
                       RecurringTipForVerifiedPublisher) {
  // Enable Rewards
  EnableRewards();

  ClaimPromotionViaCode();

  // Tip verified publisher
  TipPublisher("duckduckgo.com", ContributionType::MonthlyTip, true);
}

// #9 - Recurring tip for unverified publisher
IN_PROC_BROWSER_TEST_F(BraveRewardsBrowserTest,
                       RecurringTipForUnverifiedPublisher) {
  // Enable Rewards
  EnableRewards();

  ClaimPromotionViaCode();

  // Tip verified publisher
  TipPublisher("brave.com", ContributionType::MonthlyTip, false);
}

// Brave tip icon is injected when visiting Twitter
IN_PROC_BROWSER_TEST_F(BraveRewardsBrowserTest, TwitterTipsInjectedOnTwitter) {
  // Enable Rewards
  EnableRewardsViaCode();

  // Navigate to Twitter in a new tab
  GURL url = https_server()->GetURL("twitter.com", "/twitter");
  ui_test_utils::NavigateToURLWithDisposition(
      browser(), url, WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP);

  // Ensure that Media tips injection is active
  rewards_service_browsertest_utils::IsMediaTipsInjected(contents(), true);
}

// Brave tip icon is not injected when visiting Twitter while Brave
// Rewards is disabled
IN_PROC_BROWSER_TEST_F(BraveRewardsBrowserTest,
                       TwitterTipsNotInjectedWhenRewardsDisabled) {
  // Navigate to Twitter in a new tab
  GURL url = https_server()->GetURL("twitter.com", "/twitter");
  ui_test_utils::NavigateToURLWithDisposition(
      browser(), url, WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP);

  // Ensure that Media tips injection is not active
  rewards_service_browsertest_utils::IsMediaTipsInjected(contents(), false);
}

// Brave tip icon is injected when visiting old Twitter
IN_PROC_BROWSER_TEST_F(BraveRewardsBrowserTest,
                       TwitterTipsInjectedOnOldTwitter) {
  // Enable Rewards
  EnableRewardsViaCode();

  // Navigate to Twitter in a new tab
  GURL url = https_server()->GetURL("twitter.com", "/oldtwitter");
  ui_test_utils::NavigateToURLWithDisposition(
      browser(), url, WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP);

  // Ensure that Media tips injection is active
  rewards_service_browsertest_utils::IsMediaTipsInjected(contents(), true);
}

// Brave tip icon is not injected when visiting old Twitter while
// Brave Rewards is disabled
IN_PROC_BROWSER_TEST_F(BraveRewardsBrowserTest,
                       TwitterTipsNotInjectedWhenRewardsDisabledOldTwitter) {
  // Navigate to Twitter in a new tab
  GURL url = https_server()->GetURL("twitter.com", "/oldtwitter");
  ui_test_utils::NavigateToURLWithDisposition(
      browser(), url, WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP);

  // Ensure that Media tips injection is not active
  rewards_service_browsertest_utils::IsMediaTipsInjected(contents(), false);
}

// Brave tip icon is not injected into non-Twitter sites
IN_PROC_BROWSER_TEST_F(BraveRewardsBrowserTest,
                       TwitterTipsNotInjectedOnNonTwitter) {
  // Enable Rewards
  EnableRewardsViaCode();

  // Navigate to a non-Twitter site in a new tab
  GURL url = https_server()->GetURL("brave.com", "/twitter");
  ui_test_utils::NavigateToURLWithDisposition(
      browser(), url, WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP);

  // Ensure that Media tips injection is not active
  rewards_service_browsertest_utils::IsMediaTipsInjected(contents(), false);
}

// Brave tip icon is injected when visiting Reddit
IN_PROC_BROWSER_TEST_F(BraveRewardsBrowserTest, RedditTipsInjectedOnReddit) {
  // Enable Rewards
  EnableRewardsViaCode();

  // Navigate to Reddit in a new tab
  GURL url = https_server()->GetURL("reddit.com", "/reddit");
  ui_test_utils::NavigateToURLWithDisposition(
      browser(), url, WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP);

  // Ensure that Media Tips injection is active
  rewards_service_browsertest_utils::IsMediaTipsInjected(contents(), true);
}

// Brave tip icon is not injected when visiting Reddit
IN_PROC_BROWSER_TEST_F(BraveRewardsBrowserTest,
                       RedditTipsNotInjectedWhenRewardsDisabled) {
  // Navigate to Reddit in a new tab
  GURL url = https_server()->GetURL("reddit.com", "/reddit");
  ui_test_utils::NavigateToURLWithDisposition(
      browser(), url, WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP);

  // Ensure that Media Tips injection is not active
  rewards_service_browsertest_utils::IsMediaTipsInjected(contents(), false);
}

// Brave tip icon is not injected when visiting Reddit
IN_PROC_BROWSER_TEST_F(BraveRewardsBrowserTest,
                       RedditTipsNotInjectedOnNonReddit) {
  // Enable Rewards
  EnableRewardsViaCode();

  // Navigate to Reddit in a new tab
  GURL url = https_server()->GetURL("brave.com", "/reddit");
  ui_test_utils::NavigateToURLWithDisposition(
      browser(), url, WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP);

  // Ensure that Media Tips injection is not active
  rewards_service_browsertest_utils::IsMediaTipsInjected(contents(), false);
}

// Brave tip icon is injected when visiting GitHub
IN_PROC_BROWSER_TEST_F(BraveRewardsBrowserTest, GitHubTipsInjectedOnGitHub) {
  // Enable Rewards
  EnableRewardsViaCode();

  // Navigate to GitHub in a new tab
  GURL url = https_server()->GetURL("github.com", "/github");
  ui_test_utils::NavigateToURLWithDisposition(
      browser(), url, WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP);

  // Ensure that Media Tips injection is active
  rewards_service_browsertest_utils::IsMediaTipsInjected(contents(), true);
}

// Brave tip icon is not injected when visiting GitHub while Brave
// Rewards is disabled
IN_PROC_BROWSER_TEST_F(BraveRewardsBrowserTest,
                       GitHubTipsNotInjectedWhenRewardsDisabled) {
  // Navigate to GitHub in a new tab
  GURL url = https_server()->GetURL("github.com", "/github");
  ui_test_utils::NavigateToURLWithDisposition(
      browser(), url, WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP);

  // Ensure that Media Tips injection is not active
  rewards_service_browsertest_utils::IsMediaTipsInjected(contents(), false);
}

// Brave tip icon is not injected when not visiting GitHub
IN_PROC_BROWSER_TEST_F(BraveRewardsBrowserTest,
                       GitHubTipsNotInjectedOnNonGitHub) {
  // Enable Rewards
  EnableRewardsViaCode();

  // Navigate to GitHub in a new tab
  GURL url = https_server()->GetURL("brave.com", "/github");
  ui_test_utils::NavigateToURLWithDisposition(
      browser(), url, WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP);

  // Ensure that Media Tips injection is not active
  rewards_service_browsertest_utils::IsMediaTipsInjected(contents(), false);
}

// Check pending contributions
IN_PROC_BROWSER_TEST_F(BraveRewardsBrowserTest,
                       PendingContributionTip) {
  const std::string publisher = "example.com";

  // Enable Rewards
  EnableRewards();

  ClaimPromotionViaCode();

  // Tip unverified publisher
  TipPublisher(publisher, ContributionType::OneTimeTip);

  // Check that link for pending is shown and open modal
  rewards_service_browsertest_utils::WaitForElementThenClick(
      contents(),
      "[data-test-id='reservedAllLink']");

  // Make sure that table is populated
  rewards_service_browsertest_utils::WaitForElementToContain(
      contents(),
      "[id='pendingContributionTable'] a",
      publisher);
}

IN_PROC_BROWSER_TEST_F(BraveRewardsBrowserTest,
    InsufficientNotificationForZeroAmountZeroPublishers) {
  AddNotificationServiceObserver();
  EnableRewardsViaCode();
  CheckInsufficientFundsForTesting();
  WaitForInsufficientFundsNotification();
  const brave_rewards::RewardsNotificationService::RewardsNotificationsMap&
      notifications = rewards_service_->GetAllNotifications();

  if (notifications.empty()) {
    SUCCEED();
    return;
  }

  bool is_showing_notification = IsShowingNotificationForType(
      RewardsNotificationType::REWARDS_NOTIFICATION_INSUFFICIENT_FUNDS);

  EXPECT_FALSE(is_showing_notification);
}

IN_PROC_BROWSER_TEST_F(BraveRewardsBrowserTest,
                       InsufficientNotificationForACNotEnoughFunds) {
  AddNotificationServiceObserver();
  EnableRewards();

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
    return;
  }

  bool is_showing_notification = IsShowingNotificationForType(
      RewardsNotificationType::REWARDS_NOTIFICATION_INSUFFICIENT_FUNDS);

  EXPECT_FALSE(is_showing_notification);
}

IN_PROC_BROWSER_TEST_F(BraveRewardsBrowserTest,
                       InsufficientNotificationForInsufficientAmount) {
  AddNotificationServiceObserver();
  EnableRewards();
  ClaimPromotionViaCode();

  TipViaCode(
      "duckduckgo.com",
      20.0,
      ledger::PublisherStatus::VERIFIED,
      false,
      true);

  TipViaCode(
      "brave.com",
      50.0,
      ledger::PublisherStatus::NOT_VERIFIED,
      false,
      true);

  CheckInsufficientFundsForTesting();
  WaitForInsufficientFundsNotification();
  const brave_rewards::RewardsNotificationService::RewardsNotificationsMap&
      notifications = rewards_service_->GetAllNotifications();

  if (notifications.empty()) {
    SUCCEED();
    return;
  }

  bool is_showing_notification = IsShowingNotificationForType(
      RewardsNotificationType::REWARDS_NOTIFICATION_INSUFFICIENT_FUNDS);

  EXPECT_FALSE(is_showing_notification);
}

IN_PROC_BROWSER_TEST_F(BraveRewardsBrowserTest,
                       InsufficientNotificationForVerifiedInsufficientAmount) {
  AddNotificationServiceObserver();
  EnableRewards();
  ClaimPromotionViaCode();

  TipViaCode(
      "duckduckgo.com",
      50.0,
      ledger::PublisherStatus::VERIFIED,
      false,
      true);

  TipViaCode(
      "brave.com",
      50.0,
      ledger::PublisherStatus::NOT_VERIFIED,
      false,
      true);

  CheckInsufficientFundsForTesting();
  WaitForInsufficientFundsNotification();
  const brave_rewards::RewardsNotificationService::RewardsNotificationsMap&
      notifications = rewards_service_->GetAllNotifications();

  if (notifications.empty()) {
    FAIL() << "Should see Insufficient Funds notification";
    return;
  }

  bool is_showing_notification = IsShowingNotificationForType(
      RewardsNotificationType::REWARDS_NOTIFICATION_INSUFFICIENT_FUNDS);

  EXPECT_TRUE(is_showing_notification);
}

// Test whether rewards is disabled in private profile.
IN_PROC_BROWSER_TEST_F(BraveRewardsBrowserTest, PrefsTestInPrivateWindow) {
  EnableRewards();
  auto* profile = browser()->profile();
  EXPECT_TRUE(profile->GetPrefs()->GetBoolean(
      brave_rewards::prefs::kBraveRewardsEnabled));

  Profile* private_profile = profile->GetOffTheRecordProfile();
  EXPECT_FALSE(private_profile->GetPrefs()->GetBoolean(
      brave_rewards::prefs::kBraveRewardsEnabled));
}

IN_PROC_BROWSER_TEST_F(BraveRewardsBrowserTest, ProcessPendingContributions) {
  AddNotificationServiceObserver();

  alter_publisher_list_ = true;

  EnableRewards();

  contents()->GetController().Reload(content::ReloadType::NORMAL, true);
  EXPECT_TRUE(WaitForLoadStop(contents()));

  // Tip unverified publisher
  TipViaCode("brave.com", 1.0, ledger::PublisherStatus::NOT_VERIFIED);
  TipViaCode("brave.com", 5.0, ledger::PublisherStatus::NOT_VERIFIED);
  TipViaCode("3zsistemi.si", 10.0, ledger::PublisherStatus::NOT_VERIFIED);
  TipViaCode("3zsistemi.si", 5.0, ledger::PublisherStatus::NOT_VERIFIED);
  TipViaCode("3zsistemi.si", 10.0, ledger::PublisherStatus::NOT_VERIFIED);
  TipViaCode("3zsistemi.si", 10.0, ledger::PublisherStatus::NOT_VERIFIED);

  ClaimPromotionViaCode();

  alter_publisher_list_ = false;
  VerifyTip(41.0, false, false, true);

  // Visit publisher
  GURL url = https_server()->GetURL("3zsistemi.si", "/index.html");
  ui_test_utils::NavigateToURLWithDisposition(
      browser(), url, WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP);

  // Refresh publisher list
  RefreshPublisherListUsingRewardsPopup();

  // Activate the Rewards settings page tab
  ActivateTabAtIndex(0);

  // Wait for new verified publisher to be processed
  WaitForMultipleTipReconcileCompleted(3);
  ASSERT_EQ(multiple_tip_reconcile_status_, ledger::Result::LEDGER_OK);
  UpdateContributionBalance(-25.0, false);  // update pending balance

  // Make sure that balance is updated correctly
  IsBalanceCorrect();

  // Check that wallet summary shows the appropriate tip amount
  rewards_service_browsertest_utils::WaitForElementToEqual(
      contents(),
      "[data-test-id=summary-tips] [color=contribute] span span",
      ExpectedTipSummaryAmountString());

  // Make sure that pending contribution box shows the correct
  // amount
  IsPendingBalanceCorrect();

  // Open the Rewards popup
  content::WebContents* popup_contents = OpenRewardsPopup();
  ASSERT_TRUE(popup_contents);

  // Check if verified notification is shown
  rewards_service_browsertest_utils::WaitForElementToContain(
      popup_contents,
      "#root",
      "3zsistemi.si");

  // Close notification
  rewards_service_browsertest_utils::WaitForElementThenClick(
      popup_contents,
      "[data-test-id=notification-close]");

  // Check if insufficient funds notification is shown
  rewards_service_browsertest_utils::WaitForElementToContain(
      popup_contents,
      "#root",
      "Insufficient Funds");
}

IN_PROC_BROWSER_TEST_F(BraveRewardsBrowserTest, RewardsPanelDefaultTipChoices) {
  show_defaults_in_properties_ = true;
  EnableRewards();

  ClaimPromotionViaCode();

  GURL url = https_server()->GetURL("3zsistemi.si", "/index.html");
  ui_test_utils::NavigateToURLWithDisposition(
      browser(), url, WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP);

  // Add a recurring tip of 10 BAT.
  TipViaCode(
      "3zsistemi.si",
      10.0,
      ledger::PublisherStatus::VERIFIED,
      false,
      true);

  content::WebContents* popup = OpenRewardsPopup();
  const auto tip_options = GetRewardsPopupTipOptions(popup);
  ASSERT_EQ(tip_options, std::vector<double>({ 0, 10, 20, 50 }));
}

IN_PROC_BROWSER_TEST_F(BraveRewardsBrowserTest, SiteBannerDefaultTipChoices) {
  show_defaults_in_properties_ = true;
  EnableRewards();

  GURL url = https_server()->GetURL("3zsistemi.si", "/index.html");
  ui_test_utils::NavigateToURLWithDisposition(
      browser(), url, WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP);

  content::WebContents* site_banner =
      OpenSiteBanner(ContributionType::OneTimeTip);
  auto tip_options = GetSiteBannerTipOptions(site_banner);
  ASSERT_EQ(tip_options, std::vector<double>({ 5, 10, 20 }));

  site_banner = OpenSiteBanner(ContributionType::MonthlyTip);
  tip_options = GetSiteBannerTipOptions(site_banner);
  ASSERT_EQ(tip_options, std::vector<double>({ 10, 20, 50 }));
}

IN_PROC_BROWSER_TEST_F(
    BraveRewardsBrowserTest,
    SiteBannerDefaultPublisherAmounts) {
  show_defaults_in_properties_ = true;
  EnableRewards();

  GURL url = https_server()->GetURL("laurenwags.github.io", "/index.html");
  ui_test_utils::NavigateToURLWithDisposition(
      browser(), url, WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP);

  content::WebContents* site_banner =
      OpenSiteBanner(ContributionType::OneTimeTip);
  const auto tip_options = GetSiteBannerTipOptions(site_banner);
  ASSERT_EQ(tip_options, std::vector<double>({ 5, 10, 20 }));
}

IN_PROC_BROWSER_TEST_F(BraveRewardsBrowserTest, NotVerifiedWallet) {
  EnableRewards();

  // Click on verify button
  rewards_service_browsertest_utils::WaitForElementThenClick(
      contents(),
      "#verify-wallet-button");

  // Click on verify button in on boarding
  rewards_service_browsertest_utils::WaitForElementThenClick(
      contents(),
      "#on-boarding-verify-button");

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
  SetUpUpholdWallet(50.0);

  // Enable Rewards
  EnableRewards();

  const double amount = 5.0;
  const bool should_contribute = true;
  TipViaCode(
      "duckduckgo.com",
      amount,
      ledger::PublisherStatus::VERIFIED,
      should_contribute);
  VerifyTip(amount, should_contribute, false, true);
}

IN_PROC_BROWSER_TEST_F(BraveRewardsBrowserTest,
                       MultipleTipsProduceMultipleFeesWithVerifiedWallet) {
  SetUpUpholdWallet(50.0);

  // Enable Rewards
  EnableRewards();

  double total_amount = 0.0;
  const double amount = 5.0;
  const double fee_percentage = 0.05;
  const double tip_fee = amount * fee_percentage;
  const bool should_contribute = true;
  TipViaCode(
      "duckduckgo.com",
      amount,
      ledger::PublisherStatus::VERIFIED,
      should_contribute);
  total_amount += amount;

  TipViaCode(
      "laurenwags.github.io",
      amount,
      ledger::PublisherStatus::VERIFIED,
      should_contribute);
  total_amount += amount;

  VerifyTip(total_amount, should_contribute, false, true);

  ledger::TransferFeeList transfer_fees =
      rewards_service()->GetTransferFeesForTesting("uphold");

  ASSERT_EQ(transfer_fees.size(), 2UL);

  for (auto const& value : transfer_fees) {
    ASSERT_EQ(value.second->amount, tip_fee);
  }
}

IN_PROC_BROWSER_TEST_F(BraveRewardsBrowserTest, TipConnectedPublisherAnon) {
  // Enable Rewards
  EnableRewards();

  ClaimPromotionViaCode();

  // Tip verified publisher
  const double amount = 5.0;
  const bool should_contribute = true;
  TipViaCode(
      "bumpsmack.com",
      amount,
      ledger::PublisherStatus::CONNECTED,
      should_contribute);
  VerifyTip(amount, should_contribute, false, true);
}

IN_PROC_BROWSER_TEST_F(
    BraveRewardsBrowserTest,
    TipConnectedPublisherAnonAndConnected) {
  SetUpUpholdWallet(50.0);

  // Enable Rewards
  EnableRewards();

  ClaimPromotionViaCode();

  // Tip verified publisher
  const double amount = 5.0;
  const bool should_contribute = true;
  TipViaCode(
      "bumpsmack.com",
      amount,
      ledger::PublisherStatus::CONNECTED,
      should_contribute);
  VerifyTip(amount, should_contribute, false, true);
}

IN_PROC_BROWSER_TEST_F(
    BraveRewardsBrowserTest,
    TipConnectedPublisherConnected) {
  SetUpUpholdWallet(50.0, ledger::WalletStatus::CONNECTED);

  // Enable Rewards
  EnableRewards();
  contents()->GetController().Reload(content::ReloadType::NORMAL, true);
  EXPECT_TRUE(WaitForLoadStop(contents()));

  // Tip connected publisher
  const double amount = 5.0;
  const bool should_contribute = false;
  TipViaCode(
      "bumpsmack.com",
      amount,
      ledger::PublisherStatus::CONNECTED,
      should_contribute,
      false,
      ledger::Result::LEDGER_ERROR);

  IsBalanceCorrect();

  // Make sure that tips table is empty
  rewards_service_browsertest_utils::WaitForElementToEqual(
      contents(),
      "#tips-table > div > div",
      "Have you tipped your favorite content creator today?");
}

IN_PROC_BROWSER_TEST_F(
    BraveRewardsBrowserTest,
    TipConnectedPublisherVerified) {
  SetUpUpholdWallet(50.0);

  // Enable Rewards
  EnableRewards();
  contents()->GetController().Reload(content::ReloadType::NORMAL, true);
  EXPECT_TRUE(WaitForLoadStop(contents()));

  // Tip connected publisher
  const double amount = 5.0;
  const bool should_contribute = false;
  TipViaCode(
      "bumpsmack.com",
      amount,
      ledger::PublisherStatus::CONNECTED,
      should_contribute,
      false,
      ledger::Result::LEDGER_ERROR);

  IsBalanceCorrect();

  // Make sure that tips table is empty
  rewards_service_browsertest_utils::WaitForElementToEqual(
      contents(),
      "#tips-table > div > div",
      "Have you tipped your favorite content creator today?");
}

// Ensure that we can make a one-time tip of a non-integral amount.
IN_PROC_BROWSER_TEST_F(BraveRewardsBrowserTest, TipNonIntegralAmount) {
  EnableRewards();

  ClaimPromotionViaCode();

  // TODO(jhoneycutt): Test that this works through the tipping UI.
  rewards_service()->OnTip("duckduckgo.com", 2.5, false);
  WaitForTipReconcileCompleted();
  ASSERT_EQ(tip_reconcile_status_, ledger::Result::LEDGER_OK);

  ASSERT_EQ(reconciled_tip_total_, 2.5);
}

// Ensure that we can make a recurring tip of a non-integral amount.
IN_PROC_BROWSER_TEST_F(BraveRewardsBrowserTest, RecurringTipNonIntegralAmount) {
  EnableRewards();

  ClaimPromotionViaCode();

  const bool verified = true;
  VisitPublisher("duckduckgo.com", verified);

  rewards_service()->OnTip("duckduckgo.com", 2.5, true);
  rewards_service()->StartMonthlyContributionForTest();
  WaitForTipReconcileCompleted();
  ASSERT_EQ(tip_reconcile_status_, ledger::Result::LEDGER_OK);

  ASSERT_EQ(reconciled_tip_total_, 2.5);
}

IN_PROC_BROWSER_TEST_F(BraveRewardsBrowserTest,
    RecurringAndPartialAutoContribution) {
  // Enable Rewards
  EnableRewards();

  ClaimPromotionViaCode();

  // Visit verified publisher
  const bool verified = true;
  VisitPublisher("duckduckgo.com", verified);

  // Set monthly recurring
  TipViaCode(
      "duckduckgo.com",
      25.0,
      ledger::PublisherStatus::VERIFIED,
      false,
      true);

  VisitPublisher("brave.com", !verified);

  // Trigger contribution process
  rewards_service()->StartMonthlyContributionForTest();

  // Wait for reconciliation to complete
  WaitForTipReconcileCompleted();
  ASSERT_EQ(tip_reconcile_status_, ledger::Result::LEDGER_OK);

  // Wait for reconciliation to complete successfully
  WaitForACReconcileCompleted();
  ASSERT_EQ(ac_reconcile_status_, ledger::Result::LEDGER_OK);

  // Make sure that balance is updated correctly
  IsBalanceCorrect();

  // Check that summary table shows the appropriate contribution
  rewards_service_browsertest_utils::WaitForElementToContain(
      contents(),
      "[color='contribute']",
      "-5.0BAT");
}

IN_PROC_BROWSER_TEST_F(BraveRewardsBrowserTest,
    MultipleRecurringOverBudgetAndPartialAutoContribution) {
  // Enable Rewards
  EnableRewards();

  TipViaCode(
      "duckduckgo.com",
      5.0,
      ledger::PublisherStatus::VERIFIED,
      false,
      true);

  TipViaCode(
      "site1.com",
      10.0,
      ledger::PublisherStatus::VERIFIED,
      false,
      true);

  TipViaCode(
      "site2.com",
      10.0,
      ledger::PublisherStatus::VERIFIED,
      false,
      true);

  TipViaCode(
      "site3.com",
      10.0,
      ledger::PublisherStatus::VERIFIED,
      false,
      true);

  ClaimPromotionViaCode();

  // Visit verified publisher
  const bool verified = true;
  VisitPublisher("duckduckgo.com", verified);

  // Trigger contribution process
  rewards_service()->StartMonthlyContributionForTest();

  // Wait for reconciliation to complete
  WaitForMultipleTipReconcileCompleted(3);
  ASSERT_EQ(tip_reconcile_status_, ledger::Result::LEDGER_OK);

  // Wait for reconciliation to complete successfully
  WaitForACReconcileCompleted();
  ASSERT_EQ(ac_reconcile_status_, ledger::Result::LEDGER_OK);

  // Make sure that balance is updated correctly
  IsBalanceCorrect();

  // Check that summary table shows the appropriate contribution

  // Check that summary table shows the appropriate contribution
  rewards_service_browsertest_utils::WaitForElementToContain(
      contents(),
      "[color='contribute']",
      "-5.0BAT");
}

IN_PROC_BROWSER_TEST_F(
  BraveRewardsBrowserTest,
  NewTabPageWidgetEnableRewards) {
  EnableRewards(true);
}

IN_PROC_BROWSER_TEST_F(BraveRewardsBrowserTest, PanelDontDoRequests) {
  // Open the Rewards popup
  content::WebContents *popup_contents = OpenRewardsPopup();
  ASSERT_TRUE(popup_contents);

  // Make sure that no request was made
  ASSERT_FALSE(request_made_);
}

IN_PROC_BROWSER_TEST_F(BraveRewardsBrowserTest, ShowMonthlyIfACOff) {
  EnableRewardsViaCode();
  rewards_service_->SetAutoContribute(false);

  GURL url = https_server()->GetURL("3zsistemi.si", "/");
  ui_test_utils::NavigateToURLWithDisposition(
      browser(), url, WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP);

  // Open the Rewards popup
  content::WebContents *popup_contents = OpenRewardsPopup();
  ASSERT_TRUE(popup_contents);

  rewards_service_browsertest_utils::WaitForElementToAppear(
      popup_contents,
      "#panel-donate-monthly");
}

IN_PROC_BROWSER_TEST_F(BraveRewardsBrowserTest, ShowACPercentInThePanel) {
  EnableRewards();

  VisitPublisher("3zsistemi.si", true);

  GURL url = https_server()->GetURL("3zsistemi.si", "/");
  ui_test_utils::NavigateToURLWithDisposition(
      browser(), url, WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP);

  // Open the Rewards popup
  content::WebContents *popup_contents = OpenRewardsPopup();
  ASSERT_TRUE(popup_contents);

  const std::string score =
      rewards_service_browsertest_utils::WaitForElementThenGetContent(
          popup_contents,
          "[data-test-id='attention-score']");
  EXPECT_NE(score.find("100%"), std::string::npos);
}

IN_PROC_BROWSER_TEST_F(
    BraveRewardsBrowserTest,
    SplitProcessorAutoContribution) {
  SetUpUpholdWallet(50.0);

  EnableRewards();

  ClaimPromotionViaCode();

  VisitPublisher("3zsistemi.si", true);

  // 30 form unblinded and 20 from uphold
  rewards_service()->SetContributionAmount(50.0);

  // Trigger contribution process
  rewards_service()->StartMonthlyContributionForTest();

  // Wait for reconciliation to complete successfully
  WaitForMultipleACReconcileCompleted(2);
  ASSERT_EQ(multiple_ac_reconcile_status_[0], ledger::Result::LEDGER_OK);
  ASSERT_EQ(multiple_ac_reconcile_status_[1], ledger::Result::LEDGER_OK);

  rewards_service_browsertest_utils::WaitForElementThenClick(
      contents(),
      "[data-test-id='showMonthlyReport']");

  rewards_service_browsertest_utils::WaitForElementToAppear(
      contents(),
      "#transactionTable");

  rewards_service_browsertest_utils::WaitForElementToContain(
      contents(),
      "#transactionTable",
      "-30.0BAT");

  rewards_service_browsertest_utils::WaitForElementToContain(
      contents(),
      "#transactionTable",
      "-20.0BAT");

  // Check that summary table shows the appropriate contribution
  rewards_service_browsertest_utils::WaitForElementToContain(
      contents(),
      "[color=contribute]",
      "-50.0BAT");
}
