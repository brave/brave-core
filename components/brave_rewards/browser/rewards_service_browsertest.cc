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
#include "brave/components/brave_rewards/browser/rewards_service_factory.h"
#include "brave/components/brave_rewards/browser/rewards_service_impl.h"
#include "brave/components/brave_rewards/browser/rewards_service_observer.h"
#include "brave/components/brave_rewards/browser/rewards_notification_service_impl.h"  // NOLINT
#include "brave/components/brave_rewards/browser/rewards_notification_service_observer.h"  // NOLINT
#include "brave/components/brave_rewards/common/pref_names.h"
#include "brave/components/brave_ads/browser/ads_service_factory.h"
#include "brave/components/brave_ads/browser/ads_service_impl.h"
#include "brave/components/brave_ads/common/pref_names.h"
#include "brave/components/brave_ads/browser/locale_helper_mock.h"
#include "brave/components/brave_ads/browser/notification_helper_mock.h"
#include "brave/browser/ui/views/brave_actions/brave_actions_container.h"
#include "brave/browser/ui/views/location_bar/brave_location_bar_view.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/common/chrome_constants.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/testing_profile.h"
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

using ::testing::NiceMock;
using ::testing::Return;

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

    MaybeMockLocaleHelper();

    MockNotificationHelper();
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
    rewards_service_->test_response_callback_ =
        base::BindRepeating(&BraveRewardsBrowserTest::GetTestResponse,
                            base::Unretained(this));

    ads_service_ = static_cast<brave_ads::AdsServiceImpl*>(
        brave_ads::AdsServiceFactory::GetForProfile(browser_profile));
    ASSERT_NE(nullptr, ads_service_);

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

  bool SetUpUserDataDirectory() override {
    MaybeMockUserProfilePreferencesForBraveAdsUpgradePath();

    return true;
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

  bool IsAdsEnabled() {
    return ads_service_->IsEnabled();
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

  std::vector<double> GetSiteBannerTipOptions(
      content::WebContents* site_banner) {
    WaitForSelector(site_banner, "[data-test-id=amount-wrapper] div span");
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

  void WaitForSelector(content::WebContents* contents,
      const std::string& selector) const {
    auto script = content::JsReplace(
        "new Promise((resolve) => {"
        "  let count = 20;"
        "  let interval = setInterval(function() {"
        "    if (count == 0) {"
        "      clearInterval(interval);"
        "      resolve(false);"
        "    } else {"
        "      count -= 1;"
        "    }"
        "    const element = document.querySelector($1);"
        "    if (element) {"
        "      clearInterval(interval);"
        "      resolve(true);"
        "    }"
        "  }, 500);"
        "});",
        selector);
    auto js_result = EvalJs(
        contents,
        script,
        content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
        content::ISOLATED_WORLD_ID_CONTENT_END);
    ASSERT_TRUE(js_result.ExtractBool());
  }

  void DragAndDrop(
      content::WebContents* contents,
      const std::string& drag_selector,
      const std::string& drop_selector) {
    const std::string js_code = base::StringPrintf(
        R"(
					var triggerDragAndDrop = function (selectorDrag, selectorDrop) {

					  // function for triggering mouse events
					  var fireMouseEvent = function (type, elem, centerX, centerY) {
					    var evt = document.createEvent('MouseEvents');
					    evt.initMouseEvent(type, true, true, window, 1, 1, 1, centerX,
					                       centerY, false, false, false, false, 0, elem);
					    elem.dispatchEvent(evt);
					  };

					  // fetch target elements
					  var elemDrag = document.querySelector(selectorDrag);
					  var elemDrop = document.querySelector(selectorDrop);
					  if (!elemDrag || !elemDrop) return false;

					  // calculate positions
					  var pos = elemDrag.getBoundingClientRect();
					  var center1X = Math.floor((pos.left + pos.right) / 2);
					  var center1Y = Math.floor((pos.top + pos.bottom) / 2);
					  pos = elemDrop.getBoundingClientRect();
					  var center2X = Math.floor((pos.left + pos.right) / 2);
					  var center2Y = Math.floor((pos.top + pos.bottom) / 2);

					  // mouse over dragged element and mousedown
					  fireMouseEvent('mousemove', elemDrag, center1X, center1Y);
					  fireMouseEvent('mouseenter', elemDrag, center1X, center1Y);
					  fireMouseEvent('mouseover', elemDrag, center1X, center1Y);
					  fireMouseEvent('mousedown', elemDrag, center1X, center1Y);

					  // start dragging process over to drop target
					  fireMouseEvent('dragstart', elemDrag, center1X, center1Y);
					  fireMouseEvent('drag', elemDrag, center1X, center1Y);
					  fireMouseEvent('mousemove', elemDrag, center1X, center1Y);
					  fireMouseEvent('drag', elemDrag, center2X, center2Y);
					  fireMouseEvent('mousemove', elemDrop, center2X, center2Y);

					  // trigger dragging process on top of drop target
					  fireMouseEvent('mouseenter', elemDrop, center2X, center2Y);
					  fireMouseEvent('dragenter', elemDrop, center2X, center2Y);
					  fireMouseEvent('mouseover', elemDrop, center2X, center2Y);
					  fireMouseEvent('dragover', elemDrop, center2X, center2Y);

					  // release dragged element on top of drop target
					  fireMouseEvent('drop', elemDrop, center2X, center2Y);
					  fireMouseEvent('dragend', elemDrag, center2X, center2Y);
					  fireMouseEvent('mouseup', elemDrag, center2X, center2Y);

					  return true;
					};

					triggerDragAndDrop(
						'%s',
						'%s')
        )",
        drag_selector.c_str(),
        drop_selector.c_str());
    content::EvalJsResult jsResult = EvalJs(
        contents,
        js_code,
        content::EXECUTE_SCRIPT_NO_RESOLVE_PROMISES,
        content::ISOLATED_WORLD_ID_CONTENT_END);
    ASSERT_TRUE(jsResult.ExtractBool());
  }

  void WaitForBraveAdsHaveArrivedNotification() {
    if (brave_ads_have_arrived_notification_was_already_shown_) {
      return;
    }

    brave_ads_have_arrived_notification_run_loop_ =
        std::make_unique<base::RunLoop>();
    brave_ads_have_arrived_notification_run_loop_->Run();
  }

  void AddNotificationServiceObserver() {
    rewards_service_->GetNotificationService()->AddObserver(this);
  }

  void MaybeMockLocaleHelper() {
    const std::map<std::string, std::string> locale_for_tests = {
      {"BraveAdsLocaleIsSupported", "en_US"},
      {"BraveAdsLocaleIsNotSupported", "en_XX"},
      {"BraveAdsLocaleIsNewlySupported", "ja_JP"},
      {"BraveAdsLocaleIsNewlySupportedForLatestSchemaVersion", newly_supported_locale_},  // NOLINT
      {"BraveAdsLocaleIsNotNewlySupported", "en_XX"},
      {"PRE_AutoEnableAdsForSupportedLocales", "en_US"},
      {"AutoEnableAdsForSupportedLocales", "en_US"},
      {"PRE_DoNotAutoEnableAdsForUnsupportedLocales", "en_XX"},
      {"DoNotAutoEnableAdsForUnsupportedLocales", "en_XX"},
      {"PRE_ShowBraveAdsHaveArrivedNotificationForNewLocale", "en_XX"},
      {"ShowBraveAdsHaveArrivedNotificationForNewLocale", "en_US"},
      {"PRE_DoNotShowBraveAdsHaveArrivedNotificationForUnsupportedLocale", "en_XX"},  // NOLINT
      {"DoNotShowBraveAdsHaveArrivedNotificationForUnsupportedLocale", "en_XX"}
    };

    const ::testing::TestInfo* const test_info =
        ::testing::UnitTest::GetInstance()->current_test_info();
    ASSERT_NE(nullptr, test_info);

    const auto it = locale_for_tests.find(test_info->name());
    if (it == locale_for_tests.end()) {
      MaybeMockLocaleHelperForBraveAdsUpgradePath();
      return;
    }

    MockLocaleHelper(it->second);
  }

  void MaybeMockLocaleHelperForBraveAdsUpgradePath() {
    std::vector<std::string> parameters;
    if (!GetUpgradePathParams(&parameters)) {
      return;
    }

    const ::testing::TestInfo* const test_info =
        ::testing::UnitTest::GetInstance()->current_test_info();
    ASSERT_NE(nullptr, test_info);
    const std::string test_name = test_info->name();

    const std::string newly_supported_locale_parameter = parameters.at(2);
    ASSERT_TRUE(!newly_supported_locale_parameter.empty());

    std::string locale;
    if (test_name.find("PRE_UpgradePath") == 0) {
      if (newly_supported_locale_parameter == "ForNewlySupportedLocale") {
        locale = newly_supported_locale_;
      } else {
        locale = "en_US";
      }
    } else {
      const std::string supported_locale_parameter = parameters.at(1);
      ASSERT_TRUE(!supported_locale_parameter.empty());

      if (newly_supported_locale_parameter == "ForNewlySupportedLocale") {
        locale = newly_supported_locale_;
      } else if (supported_locale_parameter == "ForSupportedLocale") {
        locale = "en_US";
      } else {
        locale = "en_XX";
      }
    }

    MockLocaleHelper(locale);
  }

  void MockLocaleHelper(
      const std::string& locale) {
    locale_helper_mock_ =
        std::make_unique<NiceMock<brave_ads::LocaleHelperMock>>();

    brave_ads::LocaleHelper::GetInstance()->set_for_testing(
        locale_helper_mock_.get());

    ON_CALL(*locale_helper_mock_, GetLocale())
        .WillByDefault(Return(locale));
  }

  void MockNotificationHelper() {
    notification_helper_mock_ =
        std::make_unique<NiceMock<brave_ads::NotificationHelperMock>>();

    brave_ads::NotificationHelper::GetInstance()->set_for_testing(
        notification_helper_mock_.get());

    // TODO(https://openradar.appspot.com/27768556): We must mock
    // NotificationHelper::ShouldShowNotifications to return false as a
    // workaround to UNUserNotificationCenter throwing an exception during tests
    ON_CALL(*notification_helper_mock_, ShouldShowNotifications())
        .WillByDefault(Return(false));
  }

  void MaybeMockUserProfilePreferencesForBraveAdsUpgradePath() {
    std::vector<std::string> parameters;
    if (!GetUpgradePathParams(&parameters)) {
      return;
    }

    const std::string preferences_parameter = parameters.at(0);
    ASSERT_TRUE(!preferences_parameter.empty());

    MockUserProfilePreferences(preferences_parameter);
  }

  bool GetUpgradePathParams(
      std::vector<std::string>* parameters) {
    EXPECT_NE(nullptr, parameters);

    const ::testing::TestInfo* const test_info =
        ::testing::UnitTest::GetInstance()->current_test_info();
    EXPECT_NE(nullptr, test_info);

    const std::string test_suite_name = test_info->test_suite_name();
    if (test_suite_name != "BraveRewardsBrowserTest/BraveAdsBrowserTest") {
      return false;
    }

    const std::string test_name = test_info->name();
    const auto test_name_components = base::SplitString(test_name, "/",
        base::KEEP_WHITESPACE, base::SPLIT_WANT_ALL);
    EXPECT_EQ(2UL, test_name_components.size());

    // test_name_components:
    //   0 = Name
    //   1 = Parameters

    const std::string name = test_name_components.at(0);
    if (name != "UpgradePath" && name != "PRE_UpgradePath") {
      return false;
    }

    // parameters:
    //   0 = Preferences
    //   1 = Supported locale
    //   2 = Newly supported locale
    //   3 = Rewards enabled
    //   4 = Ads enabled
    //   5 = Should show notification

    *parameters = base::SplitString(test_name_components.at(1), "_",
        base::KEEP_WHITESPACE, base::SPLIT_WANT_ALL);
    EXPECT_EQ(6UL, parameters->size());

    return true;
  }

  base::FilePath GetUserDataPath() const {
    base::FilePath path;
    base::PathService::Get(chrome::DIR_USER_DATA, &path);
    path = path.AppendASCII(TestingProfile::kTestUserProfileDir);
    return path;
  }

  base::FilePath GetTestDataPath() const {
    // TODO(tmancey): We should be able to use |GetTestDataDir| however the path
    // was invalid during setup, therefore investigate further
    base::FilePath path;
    base::PathService::Get(base::DIR_SOURCE_ROOT, &path);
    path = path.Append(FILE_PATH_LITERAL("brave"));
    path = path.Append(FILE_PATH_LITERAL("test"));
    path = path.Append(FILE_PATH_LITERAL("data"));
    return path;
  }

  void MockUserProfilePreferences(
      const std::string& preference) const {
    auto user_data_path = GetUserDataPath();
    ASSERT_TRUE(base::CreateDirectory(user_data_path));

    const auto preferences_path =
        user_data_path.Append(chrome::kPreferencesFilename);

    // TODO(tmancey): We should be able to use |GetTestDataDir| however the path
    // was invalid during setup, therefore investigate further
    auto test_data_path = GetTestDataPath();
    test_data_path = test_data_path.AppendASCII("rewards-data");
    test_data_path = test_data_path.AppendASCII("migration");
    test_data_path = test_data_path.AppendASCII(preference);
    ASSERT_TRUE(base::PathExists(test_data_path));

    ASSERT_TRUE(base::CopyFile(test_data_path, preferences_path));
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
    WaitForSelector(popup_contents, "[data-test-id='rewards-panel']");

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

    ads_service_->SetEnabled(
        wallet_created && ads_service_->IsSupportedLocale());
    ASSERT_TRUE(wallet_created);
    ASSERT_TRUE(IsRewardsEnabled());
  }

  brave_rewards::RewardsServiceImpl* rewards_service() {
    return rewards_service_;
  }

  void ClaimPromotion(bool use_panel) {
    // Wait for promotion to initialize
    WaitForPromotionInitialization();

    // Use the appropriate WebContents
    content::WebContents *contents =
        use_panel ? OpenRewardsPopup() : BraveRewardsBrowserTest::contents();
    ASSERT_TRUE(contents);

    // Claim promotion via settings page or panel, as instructed
    if (use_panel) {
      ASSERT_TRUE(ExecJs(contents,
                         "document.getElementsByTagName('button')[0].click();",
                         content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
                         content::ISOLATED_WORLD_ID_CONTENT_END));
    } else {
      WaitForSelector(contents, "[data-test-id='claimGrant']");
      ASSERT_TRUE(ExecJs(
          contents,
          "document.querySelector(\"[data-test-id='claimGrant']\").click();",
          content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
          content::ISOLATED_WORLD_ID_CONTENT_END));
    }

    // Wait for CAPTCHA
    WaitForSelector(contents, "[data-test-id='captcha']");

    DragAndDrop(
        contents,
        "[data-test-id=\"captcha-triangle\"]",
        "[data-test-id=\"captcha-drop\"]");

    WaitForPromotionFinished();

    // Ensure that promotion looks as expected
    const std::string promotion_id = "6820f6a4-c6ef-481d-879c-d2c30c8928c3";
    EXPECT_STREQ(std::to_string(promotion_.amount).c_str(), "30.000000");
    EXPECT_STREQ(promotion_.promotion_id.c_str(), promotion_id.c_str());
    EXPECT_EQ(promotion_.type, 0u);
    EXPECT_EQ(promotion_.expires_at, 1740816427ull);
    balance_ += 30;

    // Check that promotion notification shows the appropriate amount
    const std::string selector =
        use_panel ? "[id='root']" : "[data-test-id='newTokenGrant']";

    WaitForSelector(contents, selector);

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

    // Dismiss the promotion notification
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
    ActivateTabAtIndex(0);

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

  std::string ElementInnerText(const std::string& selector,
                               int delay_ms = 0) const {
    auto script = content::JsReplace(
        "const delay = t => new Promise(resolve => setTimeout(resolve, t));"
        "delay($1).then(() => document.querySelector($2).innerText);",
        delay_ms,
        selector);

    auto js_result = EvalJs(
        contents(),
        script,
        content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
        content::ISOLATED_WORLD_ID_CONTENT_END);

    return js_result.ExtractString();
  }

  std::string RewardsPageBalance() const {
    return ElementInnerText("[data-test-id='balance']", 1000);
  }

  std::string RewardsPagePendingContributions() const {
    std::string pending_text = ElementInnerText(
        "[data-test-id='pending-contribution-box']",
        500);

    // The pending text is of the form "You’ve designated 1.0 BAT for..."
    size_t start = pending_text.find("designated ");
    assert(start != std::string::npos);
    start += 11;

    size_t end = pending_text.find(" for");
    assert(start != std::string::npos);

    return pending_text.substr(start, end - start);
  }

  std::string RewardsPageTipSummaryAmount() const {
    std::string amount = ElementInnerText(
        "[data-test-id=summary-tips] [color=donation] span span");
    return amount + " BAT";
  }

  std::string ExpectedPendingBalanceString() const {
    return GetPendingBalance() + " BAT";
  }

  std::string ExpectedBalanceString() const {
    return GetBalance() + " BAT";
  }

  std::string ExpectedTipSummaryAmountString() const {
    // The tip summary page formats 2.4999 as 2.4, so we do the same here.
    double truncated_amount = floor(reconciled_tip_total_ * 10) / 10;
    return BalanceDoubleToString(-truncated_amount) + " BAT";
  }

  void ActivateTabAtIndex(int index) const {
    browser()->tab_strip_model()->ActivateTabAt(
        index,
        TabStripModel::UserGestureDetails(TabStripModel::GestureType::kOther));
  }

  void RefreshPublisherListUsingRewardsPopup() const {
    content::EvalJsResult js_result = EvalJs(
        OpenRewardsPopup(),
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
      ASSERT_TRUE(js_result.ExtractBool());
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
        ui_test_utils::BROWSER_TEST_WAIT_FOR_NAVIGATION);

    content::WebContents* site_banner_contents = OpenSiteBanner(type);
    ASSERT_TRUE(site_banner_contents);

    std::vector<double> tip_options =
        GetSiteBannerTipOptions(site_banner_contents);
    const double amount = tip_options.at(selection);
    const std::string amount_str = base::StringPrintf("%2.1f", amount);

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
        "  document.querySelector(\"[data-test-id='send-tip-button']\")"
        "  .click());",
        content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
        content::ISOLATED_WORLD_ID_CONTENT_END));

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
      content::EvalJsResult js_result = EvalJs(
          site_banner_contents,
          "const delay = t => new Promise(resolve => setTimeout(resolve, t));"
          "delay(1000).then(() => "
          "  document.documentElement.innerText);",
          content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
          content::ISOLATED_WORLD_ID_CONTENT_END);
      EXPECT_NE(js_result.ExtractString().find(
          confirmationText), std::string::npos);
      EXPECT_NE(js_result.ExtractString().find(
           "" + amount_str + " BAT"), std::string::npos);
      EXPECT_NE(js_result.ExtractString().find(
          "Share the good news:"), std::string::npos);
      EXPECT_NE(js_result.ExtractString().find(
           "" + GetBalance() + " BAT"), std::string::npos);
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
      ASSERT_EQ(RewardsPageBalance(), ExpectedBalanceString());

      // Check that tip table shows the appropriate tip amount
      const std::string selector = monthly
          ? "[data-test-id='summary-donation']"
          : "[data-test-id='summary-tips']";

      std::string page_amount = ElementInnerText(selector);
      ASSERT_NE(page_amount.find("-" + BalanceDoubleToString(amount) + "BAT"),
          std::string::npos);
    } else {
      // Make sure that balance did not change
      ASSERT_EQ(RewardsPageBalance(), ExpectedBalanceString());

      // Make sure that pending contribution box shows the correct
      // amount
      ASSERT_EQ(RewardsPagePendingContributions(),
          ExpectedPendingBalanceString());

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
      const std::string& viewing_id,
      const double amount,
      const int32_t type) {
    UpdateContributionBalance(amount, true);

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
      reconciled_tip_total_ += amount;

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

        case brave_rewards::RewardsNotificationService::
            RewardsNotificationType::REWARDS_NOTIFICATION_ADS_ONBOARDING: {
          brave_ads_have_arrived_notification_was_already_shown_ = true;

          if (brave_ads_have_arrived_notification_run_loop_) {
            brave_ads_have_arrived_notification_run_loop_->Quit();
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
      const bool recurring = false) {
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
      ASSERT_EQ(tip_reconcile_status_, ledger::Result::LEDGER_OK);
      return;
    }

    // Signal to update pending contribution balance
    WaitForPendingTipToBeSaved();
    UpdateContributionBalance(amount, should_contribute);
  }

  MOCK_METHOD1(OnGetEnvironment, void(ledger::Environment));
  MOCK_METHOD1(OnGetDebug, void(bool));
  MOCK_METHOD1(OnGetReconcileTime, void(int32_t));
  MOCK_METHOD1(OnGetShortRetries, void(bool));

  std::unique_ptr<net::EmbeddedTestServer> https_server_;

  brave_rewards::RewardsServiceImpl* rewards_service_;

  brave_ads::AdsServiceImpl* ads_service_;

  std::unique_ptr<brave_ads::LocaleHelperMock> locale_helper_mock_;
  const std::string newly_supported_locale_ = "cs_CZ";

  std::unique_ptr<brave_ads::NotificationHelperMock> notification_helper_mock_;

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
  bool tip_reconcile_completed_ = false;
  ledger::Result tip_reconcile_status_ = ledger::Result::LEDGER_ERROR;
  std::unique_ptr<base::RunLoop> wait_for_multiple_tip_completed_loop_;
  bool multiple_tip_reconcile_completed_ = false;
  int32_t multiple_tip_reconcile_count_ = 0;
  int32_t multiple_tip_reconcile_needed_ = 0;
  ledger::Result multiple_tip_reconcile_status_ = ledger::Result::LEDGER_ERROR;

  std::unique_ptr<base::RunLoop> wait_for_insufficient_notification_loop_;
  bool insufficient_notification_would_have_already_shown_ = false;

  std::unique_ptr<base::RunLoop> brave_ads_have_arrived_notification_run_loop_;
  bool brave_ads_have_arrived_notification_was_already_shown_ = false;

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
  bool first_url_ac_called_ = false;
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

  // Claim promotion using panel
  const bool use_panel = true;
  ClaimPromotion(use_panel);

  // Visit verified publisher
  const bool verified = true;
  VisitPublisher("duckduckgo.com", verified);

  // Trigger contribution process
  rewards_service()->StartMonthlyContributionForTest();

  // Wait for reconciliation to complete successfully
  WaitForACReconcileCompleted();
  ASSERT_EQ(ac_reconcile_status_, ledger::Result::LEDGER_OK);

  // Make sure that balance is updated correctly
  ASSERT_EQ(RewardsPageBalance(), ExpectedBalanceString());

  // Check that summary table shows the appropriate contribution
  ASSERT_NE(ElementInnerText("[color=contribute]").find("-20.0BAT"),
      std::string::npos);
}

IN_PROC_BROWSER_TEST_F(BraveRewardsBrowserTest, AutoContributeWhenACOff) {
  EnableRewards();

  // Claim promotion using panel
  const bool use_panel = true;
  ClaimPromotion(use_panel);

  // Visit verified publisher
  const bool verified = true;
  VisitPublisher("duckduckgo.com", verified);

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

  // Trigger contribution process
  rewards_service()->StartMonthlyContributionForTest();
  ASSERT_FALSE(first_url_ac_called_);
}

// #6 - Tip verified publisher
IN_PROC_BROWSER_TEST_F(BraveRewardsBrowserTest, TipVerifiedPublisher) {
  // Enable Rewards
  EnableRewards();

  // Claim promotion using settings page
  const bool use_panel = true;
  ClaimPromotion(use_panel);

  // Tip verified publisher
  TipPublisher("duckduckgo.com", ContributionType::OneTimeTip, true);
}

// #7 - Tip unverified publisher
IN_PROC_BROWSER_TEST_F(BraveRewardsBrowserTest, TipUnverifiedPublisher) {
  // Enable Rewards
  EnableRewards();

  // Claim promotion using settings page
  const bool use_panel = true;
  ClaimPromotion(use_panel);

  // Tip unverified publisher
  TipPublisher("brave.com", ContributionType::OneTimeTip);
}

// #8 - Recurring tip for verified publisher
IN_PROC_BROWSER_TEST_F(BraveRewardsBrowserTest,
                       RecurringTipForVerifiedPublisher) {
  // Enable Rewards
  EnableRewards();

  // Claim promotion using settings page
  const bool use_panel = true;
  ClaimPromotion(use_panel);

  // Tip verified publisher
  TipPublisher("duckduckgo.com", ContributionType::MonthlyTip, true);
}

// #9 - Recurring tip for unverified publisher
IN_PROC_BROWSER_TEST_F(BraveRewardsBrowserTest,
                       RecurringTipForUnverifiedPublisher) {
  // Enable Rewards
  EnableRewards();

  // Claim promotion using settings page
  const bool use_panel = true;
  ClaimPromotion(use_panel);

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
  EnableRewardsViaCode();

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
  EnableRewardsViaCode();

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
  EnableRewardsViaCode();

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
  EnableRewardsViaCode();

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
  EnableRewardsViaCode();

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
  EnableRewardsViaCode();

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

  // Enable Rewards
  EnableRewards();

  // Claim promotion using settings page
  const bool use_panel = true;
  ClaimPromotion(use_panel);

  // Tip unverified publisher
  TipPublisher(publisher, ContributionType::OneTimeTip);

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
  // Claim promotion using panel
  const bool use_panel = true;
  ClaimPromotion(use_panel);

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
  // Claim promotion using panel
  const bool use_panel = true;
  ClaimPromotion(use_panel);

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

  // Claim promotion using panel
  ClaimPromotion(true);

  alter_publisher_list_ = false;
  VerifyTip(41.0, false, false, true);

  // Visit publisher
  GURL url = https_server()->GetURL("3zsistemi.si", "/index.html");
  ui_test_utils::NavigateToURLWithDisposition(
      browser(), url, WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_NAVIGATION);

  // Refresh publisher list
  RefreshPublisherListUsingRewardsPopup();

  // Activate the Rewards settings page tab
  ActivateTabAtIndex(0);

  // Wait for new verified publisher to be processed
  WaitForMultipleTipReconcileCompleted(3);
  ASSERT_EQ(multiple_tip_reconcile_status_, ledger::Result::LEDGER_OK);
  UpdateContributionBalance(-25.0, false);  // update pending balance

  // Make sure that balance is updated correctly
  ASSERT_EQ(RewardsPageBalance(), ExpectedBalanceString());

  // Check that wallet summary shows the appropriate tip amount
  ASSERT_EQ(RewardsPageTipSummaryAmount(), ExpectedTipSummaryAmountString());

  // Make sure that pending contribution box shows the correct
  // amount
  ASSERT_EQ(RewardsPagePendingContributions(), ExpectedPendingBalanceString());

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
}

IN_PROC_BROWSER_TEST_F(BraveRewardsBrowserTest, RewardsPanelDefaultTipChoices) {
  show_defaults_in_properties_ = true;
  EnableRewards();

  bool use_panel = true;
  ClaimPromotion(use_panel);

  GURL url = https_server()->GetURL("3zsistemi.si", "/index.html");
  ui_test_utils::NavigateToURLWithDisposition(
      browser(), url, WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_NAVIGATION);

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
      ui_test_utils::BROWSER_TEST_WAIT_FOR_NAVIGATION);

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
      ui_test_utils::BROWSER_TEST_WAIT_FOR_NAVIGATION);

  content::WebContents* site_banner =
      OpenSiteBanner(ContributionType::OneTimeTip);
  const auto tip_options = GetSiteBannerTipOptions(site_banner);
  ASSERT_EQ(tip_options, std::vector<double>({ 5, 10, 20 }));
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
  verified_wallet_ = true;
  external_balance_ = 50.0;

  auto wallet = ledger::ExternalWallet::New();
  wallet->token = "token";
  wallet->address = external_wallet_address_;
  wallet->status = ledger::WalletStatus::VERIFIED;
  wallet->one_time_string = "";
  wallet->user_name = "Brave Test";
  wallet->transferred = true;
  rewards_service()->SaveExternalWallet("uphold", std::move(wallet));

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

  // Stop observing the Rewards service
  rewards_service()->RemoveObserver(this);
}

IN_PROC_BROWSER_TEST_F(BraveRewardsBrowserTest,
                       MultipleTipsProduceMultipleFeesWithVerifiedWallet) {
  verified_wallet_ = true;
  external_balance_ = 50.0;

  auto wallet = ledger::ExternalWallet::New();
  wallet->token = "token";
  wallet->address = external_wallet_address_;
  wallet->status = ledger::WalletStatus::VERIFIED;
  wallet->one_time_string = "";
  wallet->user_name = "Brave Test";
  wallet->transferred = true;
  rewards_service()->SaveExternalWallet("uphold", std::move(wallet));

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

  // Stop observing the Rewards service
  rewards_service()->RemoveObserver(this);
}

IN_PROC_BROWSER_TEST_F(BraveRewardsBrowserTest, TipConnectedPublisherAnon) {
  // Enable Rewards
  EnableRewards();

  // Claim promotion using settings page
  const bool use_panel = true;
  ClaimPromotion(use_panel);

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
  verified_wallet_ = true;
  external_balance_ = 50.0;

  auto wallet = ledger::ExternalWallet::New();
  wallet->token = "token";
  wallet->address = external_wallet_address_;
  wallet->status = ledger::WalletStatus::CONNECTED;
  wallet->one_time_string = "";
  wallet->user_name = "Brave Test";
  wallet->transferred = true;
  rewards_service()->SaveExternalWallet("uphold", std::move(wallet));

  // Enable Rewards
  EnableRewards();

  // Claim promotion using settings page
  const bool use_panel = true;
  ClaimPromotion(use_panel);

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
  verified_wallet_ = true;
  external_balance_ = 50.0;

  auto wallet = ledger::ExternalWallet::New();
  wallet->token = "token";
  wallet->address = external_wallet_address_;
  wallet->status = ledger::WalletStatus::CONNECTED;
  wallet->one_time_string = "";
  wallet->user_name = "Brave Test";
  wallet->transferred = true;
  rewards_service()->SaveExternalWallet("uphold", std::move(wallet));

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
      should_contribute);
  VerifyTip(amount, should_contribute, false, true);
}

IN_PROC_BROWSER_TEST_F(
    BraveRewardsBrowserTest,
    TipConnectedPublisherVerified) {
  verified_wallet_ = true;
  external_balance_ = 50.0;

  auto wallet = ledger::ExternalWallet::New();
  wallet->token = "token";
  wallet->address = external_wallet_address_;
  wallet->status = ledger::WalletStatus::VERIFIED;
  wallet->one_time_string = "";
  wallet->user_name = "Brave Test";
  wallet->transferred = true;
  rewards_service()->SaveExternalWallet("uphold", std::move(wallet));

  // Enable Rewards
  EnableRewards();
  contents()->GetController().Reload(content::ReloadType::NORMAL, true);
  EXPECT_TRUE(WaitForLoadStop(contents()));

  // Tip verified publisher
  const double amount = 5.0;
  const bool should_contribute = false;
  TipViaCode(
      "bumpsmack.com",
      amount,
      ledger::PublisherStatus::CONNECTED,
      should_contribute);
  VerifyTip(amount, should_contribute, false, true);
}

// Ensure that we can make a one-time tip of a non-integral amount.
IN_PROC_BROWSER_TEST_F(BraveRewardsBrowserTest, TipNonIntegralAmount) {
  EnableRewards();

  const bool use_panel = true;
  ClaimPromotion(use_panel);

  // TODO(jhoneycutt): Test that this works through the tipping UI.
  rewards_service()->OnTip("duckduckgo.com", 2.5, false);
  WaitForTipReconcileCompleted();
  ASSERT_EQ(tip_reconcile_status_, ledger::Result::LEDGER_OK);

  ASSERT_EQ(reconciled_tip_total_, 2.5);
}

// Ensure that we can make a recurring tip of a non-integral amount.
IN_PROC_BROWSER_TEST_F(BraveRewardsBrowserTest, RecurringTipNonIntegralAmount) {
  EnableRewards();

  const bool use_panel = true;
  ClaimPromotion(use_panel);

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

  // Claim promotion using panel (30 BAT)
  const bool use_panel = true;
  ClaimPromotion(use_panel);

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
  {
    const std::string result = RewardsPageBalance();
    EXPECT_NE(result.find(ExpectedBalanceString()), std::string::npos);
  }

  // Check that summary table shows the appropriate contribution
  {
    const std::string result = ElementInnerText("[color='contribute']");
    EXPECT_NE(result.find("-5.0BAT"), std::string::npos);
  }
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

  // Claim promotion using panel (30 BAT)
  const bool use_panel = true;
  ClaimPromotion(use_panel);

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
  {
    const std::string result = RewardsPageBalance();
    EXPECT_NE(result.find(ExpectedBalanceString()), std::string::npos);
  }

  // Check that summary table shows the appropriate contribution
  {
    const std::string result = ElementInnerText("[color='contribute']");
    EXPECT_NE(result.find("-5.0BAT"), std::string::npos);
  }
}

IN_PROC_BROWSER_TEST_F(BraveRewardsBrowserTest,
    BraveAdsLocaleIsSupported) {
  EXPECT_TRUE(ads_service_->IsSupportedLocale());
}

IN_PROC_BROWSER_TEST_F(BraveRewardsBrowserTest,
    BraveAdsLocaleIsNotSupported) {
  EXPECT_FALSE(ads_service_->IsSupportedLocale());
}

IN_PROC_BROWSER_TEST_F(BraveRewardsBrowserTest,
    BraveAdsLocaleIsNewlySupported) {
  GetPrefs()->SetInteger(
      brave_ads::prefs::kSupportedRegionsLastSchemaVersion, 3);

  GetPrefs()->SetInteger(brave_ads::prefs::kSupportedRegionsSchemaVersion,
      brave_ads::prefs::kSupportedRegionsSchemaVersionNumber);

  EXPECT_TRUE(ads_service_->IsNewlySupportedLocale());
}

IN_PROC_BROWSER_TEST_F(BraveRewardsBrowserTest,
    BraveAdsLocaleIsNewlySupportedForLatestSchemaVersion) {
  // IMPORTANT: When adding new schema versions |newly_supported_locale_| must
  // be updated in |BraveRewardsBrowserTest| to reflect a locale from the latest
  // schema version in "bat-native-ads/src/bat/ads/internal/static_values.h"

  GetPrefs()->SetInteger(brave_ads::prefs::kSupportedRegionsLastSchemaVersion,
      brave_ads::prefs::kSupportedRegionsSchemaVersionNumber);

  GetPrefs()->SetInteger(brave_ads::prefs::kSupportedRegionsSchemaVersion,
      brave_ads::prefs::kSupportedRegionsSchemaVersionNumber);

  EXPECT_TRUE(ads_service_->IsNewlySupportedLocale());
}

IN_PROC_BROWSER_TEST_F(BraveRewardsBrowserTest,
    BraveAdsLocaleIsNotNewlySupported) {
  GetPrefs()->SetInteger(
      brave_ads::prefs::kSupportedRegionsLastSchemaVersion, 2);

  GetPrefs()->SetInteger(brave_ads::prefs::kSupportedRegionsSchemaVersion,
      brave_ads::prefs::kSupportedRegionsSchemaVersionNumber);

  EXPECT_FALSE(ads_service_->IsNewlySupportedLocale());
}

IN_PROC_BROWSER_TEST_F(BraveRewardsBrowserTest,
    PRE_AutoEnableAdsForSupportedLocales) {
  EnableRewardsViaCode();

  EXPECT_TRUE(IsAdsEnabled());
}

IN_PROC_BROWSER_TEST_F(BraveRewardsBrowserTest,
    AutoEnableAdsForSupportedLocales) {
  EXPECT_TRUE(IsAdsEnabled());
}

IN_PROC_BROWSER_TEST_F(BraveRewardsBrowserTest,
    PRE_DoNotAutoEnableAdsForUnsupportedLocales) {
  EnableRewardsViaCode();

  EXPECT_FALSE(IsAdsEnabled());
}

IN_PROC_BROWSER_TEST_F(BraveRewardsBrowserTest,
    DoNotAutoEnableAdsForUnsupportedLocales) {
  EXPECT_FALSE(IsAdsEnabled());
}

IN_PROC_BROWSER_TEST_F(BraveRewardsBrowserTest,
    PRE_ShowBraveAdsHaveArrivedNotificationForNewLocale) {
  EnableRewardsViaCode();

  EXPECT_FALSE(IsAdsEnabled());
}

IN_PROC_BROWSER_TEST_F(BraveRewardsBrowserTest,
    ShowBraveAdsHaveArrivedNotificationForNewLocale) {
  AddNotificationServiceObserver();

  WaitForBraveAdsHaveArrivedNotification();

  EXPECT_FALSE(IsAdsEnabled());
}

IN_PROC_BROWSER_TEST_F(BraveRewardsBrowserTest,
    PRE_DoNotShowBraveAdsHaveArrivedNotificationForUnsupportedLocale) {
  EnableRewardsViaCode();

  EXPECT_FALSE(IsAdsEnabled());
}

IN_PROC_BROWSER_TEST_F(BraveRewardsBrowserTest,
    DoNotShowBraveAdsHaveArrivedNotificationForUnsupportedLocale) {
  bool is_showing_notification = IsShowingNotificationForType(
      RewardsNotificationType::REWARDS_NOTIFICATION_ADS_ONBOARDING);

  EXPECT_FALSE(is_showing_notification);
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
      ui_test_utils::BROWSER_TEST_WAIT_FOR_NAVIGATION);

  // Open the Rewards popup
  content::WebContents *popup_contents = OpenRewardsPopup();
  ASSERT_TRUE(popup_contents);

  WaitForSelector(popup_contents, "#panel-donate-monthly");
}

struct BraveAdsUpgradePathParamInfo {
  // |preferences| should be set to the name of the preferences filename located
  // at "src/brave/test/data/rewards-data/migration"
  std::string preferences;

  // |supported_locale| should be set to |true| if the locale should be set to a
  // supported locale; otherwise, should be set to |false|
  bool supported_locale;

  // |newly_supported_locale| should be set to |true| if the locale should be
  // set to a newly supported locale; otherwise, should be set to |false|
  bool newly_supported_locale;

  // |rewards_enabled| should be set to |true| if Brave rewards should be
  // enabled after upgrade; otherwise, should be set to |false|
  bool rewards_enabled;

  // |ads_enabled| should be set to |true| if Brave ads should be enabled after
  // upgrade; otherwise, should be set to |false|
  bool ads_enabled;

  // |should_show_onboarding| should be set to |true| if Brave ads onboarding
  // should be shown after upgrade; otherwise, should be set to |false|
  bool should_show_onboarding;
};

class BraveAdsBrowserTest
    : public BraveRewardsBrowserTest,
      public ::testing::WithParamInterface<BraveAdsUpgradePathParamInfo> {};

const BraveAdsUpgradePathParamInfo kTests[] = {
  // Test Suite with expected outcomes for upgrade paths instantiated using
  // Value-Parameterized Tests

  // Upgrade from 0.62 to current version
  {
    "PreferencesForVersion062WithRewardsDisabled",
    false, /* supported_locale */
    false, /* newly_supported_locale */
    false, /* rewards_enabled */
    false, /* ads_enabled */
    false  /* should_show_onboarding */
  },
  {
    "PreferencesForVersion062WithRewardsEnabled",
    false, /* supported_locale */
    false, /* newly_supported_locale */
    true,  /* rewards_enabled */
    false, /* ads_enabled */
    false  /* should_show_onboarding */
  },
  {
    "PreferencesForVersion062WithRewardsDisabled",
    true,  /* supported_locale */
    false, /* newly_supported_locale */
    false, /* rewards_enabled */
    false, /* ads_enabled */
    false  /* should_show_onboarding */
  },
  {
    "PreferencesForVersion062WithRewardsEnabled",
    true,  /* supported_locale */
    false, /* newly_supported_locale */
    true,  /* rewards_enabled */
    false, /* ads_enabled */
    true   /* should_show_onboarding */
  },
  {
    "PreferencesForVersion062WithRewardsDisabled",
    true,  /* supported_locale */
    true,  /* newly_supported_locale */
    false, /* rewards_enabled */
    false, /* ads_enabled */
    false  /* should_show_onboarding */
  },
  {
    "PreferencesForVersion062WithRewardsEnabled",
    true,  /* supported_locale */
    true,  /* newly_supported_locale */
    true,  /* rewards_enabled */
    false, /* ads_enabled */
    true   /* should_show_onboarding */
  },

  // Upgrade from 0.63 to current version (Initial release of Brave ads)
  {
    "PreferencesForVersion063WithRewardsAndAdsDisabled",
    false, /* supported_locale */
    false, /* newly_supported_locale */
    false, /* rewards_enabled */
    false, /* ads_enabled */
    false  /* should_show_onboarding */
  },
  {
    "PreferencesForVersion063WithRewardsEnabledAndAdsDisabled",
    false, /* supported_locale */
    false, /* newly_supported_locale */
    true,  /* rewards_enabled */
    false, /* ads_enabled */
    false  /* should_show_onboarding */
  },
  {
    "PreferencesForVersion063WithRewardsAndAdsEnabled",
    false, /* supported_locale */
    false, /* newly_supported_locale */
    true,  /* rewards_enabled */
    false, /* ads_enabled */
    false  /* should_show_onboarding */
  },
  {
    "PreferencesForVersion063WithRewardsAndAdsDisabled",
    true,  /* supported_locale */
    false, /* newly_supported_locale */
    false, /* rewards_enabled */
    false, /* ads_enabled */
    false  /* should_show_onboarding */
  },
  {
    "PreferencesForVersion063WithRewardsEnabledAndAdsDisabled",
    true,  /* supported_locale */
    false, /* newly_supported_locale */
    true,  /* rewards_enabled */
    false, /* ads_enabled */
    true   /* should_show_onboarding */
  },
  // TODO(tmancey): The following test failed due to the ads_enabled flag being
  // incorrectly set to false
  // {
  //   "PreferencesForVersion063WithRewardsAndAdsEnabled",
  //   true,  /* supported_locale */
  //   false, /* newly_supported_locale */
  //   true,  /* rewards_enabled */
  //   true,  /* ads_enabled */
  //   false  /* should_show_onboarding */
  // },
  {
    "PreferencesForVersion063WithRewardsAndAdsDisabled",
    true,  /* supported_locale */
    true,  /* newly_supported_locale */
    false, /* rewards_enabled */
    false, /* ads_enabled */
    false  /* should_show_onboarding */
  },
  {
    "PreferencesForVersion063WithRewardsEnabledAndAdsDisabled",
    true,  /* supported_locale */
    true,  /* newly_supported_locale */
    true,  /* rewards_enabled */
    false, /* ads_enabled */
    true   /* should_show_onboarding */
  },
  {
    "PreferencesForVersion063WithRewardsAndAdsEnabled",
    true,  /* supported_locale */
    true,  /* newly_supported_locale */
    true,  /* rewards_enabled */
    false, /* ads_enabled */
    true   /* should_show_onboarding */
  },

  // Upgrade from 0.67 to current version
  {
    "PreferencesForVersion067WithRewardsAndAdsDisabled",
    false, /* supported_locale */
    false, /* newly_supported_locale */
    false, /* rewards_enabled */
    false, /* ads_enabled */
    false  /* should_show_onboarding */
  },
  {
    "PreferencesForVersion067WithRewardsEnabledAndAdsDisabled",
    false, /* supported_locale */
    false, /* newly_supported_locale */
    true,  /* rewards_enabled */
    false, /* ads_enabled */
    false  /* should_show_onboarding */
  },
  {
    "PreferencesForVersion067WithRewardsAndAdsEnabled",
    false, /* supported_locale */
    false, /* newly_supported_locale */
    true,  /* rewards_enabled */
    false, /* ads_enabled */
    false  /* should_show_onboarding */
  },
  {
    "PreferencesForVersion067WithRewardsAndAdsDisabled",
    true,  /* supported_locale */
    false, /* newly_supported_locale */
    false, /* rewards_enabled */
    false, /* ads_enabled */
    false  /* should_show_onboarding */
  },
  {
    "PreferencesForVersion067WithRewardsEnabledAndAdsDisabled",
    true,  /* supported_locale */
    false, /* newly_supported_locale */
    true,  /* rewards_enabled */
    false, /* ads_enabled */
    true   /* should_show_onboarding */
  },
  {
    "PreferencesForVersion067WithRewardsAndAdsEnabled",
    true,  /* supported_locale */
    false, /* newly_supported_locale */
    true,  /* rewards_enabled */
    true,  /* ads_enabled */
    false  /* should_show_onboarding */
  },
  {
    "PreferencesForVersion067WithRewardsAndAdsDisabled",
    true,  /* supported_locale */
    true,  /* newly_supported_locale */
    false, /* rewards_enabled */
    false, /* ads_enabled */
    false  /* should_show_onboarding */
  },
  {
    "PreferencesForVersion067WithRewardsEnabledAndAdsDisabled",
    true,  /* supported_locale */
    true,  /* newly_supported_locale */
    true,  /* rewards_enabled */
    false, /* ads_enabled */
    true   /* should_show_onboarding */
  },
  {
    "PreferencesForVersion067WithRewardsAndAdsEnabled",
    true,  /* supported_locale */
    true,  /* newly_supported_locale */
    true,  /* rewards_enabled */
    false, /* ads_enabled */
    true   /* should_show_onboarding */
  },

  // Upgrade from 0.68 to current version
  {
    "PreferencesForVersion068WithRewardsAndAdsDisabled",
    false, /* supported_locale */
    false, /* newly_supported_locale */
    false, /* rewards_enabled */
    false, /* ads_enabled */
    false  /* should_show_onboarding */
  },
  {
    "PreferencesForVersion068WithRewardsEnabledAndAdsDisabled",
    false, /* supported_locale */
    false, /* newly_supported_locale */
    true,  /* rewards_enabled */
    false, /* ads_enabled */
    false  /* should_show_onboarding */
  },
  {
    "PreferencesForVersion068WithRewardsAndAdsEnabled",
    false, /* supported_locale */
    false, /* newly_supported_locale */
    true,  /* rewards_enabled */
    false, /* ads_enabled */
    false  /* should_show_onboarding */
  },
  {
    "PreferencesForVersion068WithRewardsAndAdsDisabled",
    true,  /* supported_locale */
    false, /* newly_supported_locale */
    false, /* rewards_enabled */
    false, /* ads_enabled */
    false  /* should_show_onboarding */
  },
  {
    "PreferencesForVersion068WithRewardsEnabledAndAdsDisabled",
    true,  /* supported_locale */
    false, /* newly_supported_locale */
    true,  /* rewards_enabled */
    false, /* ads_enabled */
    true   /* should_show_onboarding */
  },
  {
    "PreferencesForVersion068WithRewardsAndAdsEnabled",
    true,  /* supported_locale */
    false, /* newly_supported_locale */
    true,  /* rewards_enabled */
    true,  /* ads_enabled */
    false  /* should_show_onboarding */
  },
  {
    "PreferencesForVersion068WithRewardsAndAdsDisabled",
    true,  /* supported_locale */
    true,  /* newly_supported_locale */
    false, /* rewards_enabled */
    false, /* ads_enabled */
    false  /* should_show_onboarding */
  },
  {
    "PreferencesForVersion068WithRewardsEnabledAndAdsDisabled",
    true,  /* supported_locale */
    true,  /* newly_supported_locale */
    true,  /* rewards_enabled */
    false, /* ads_enabled */
    true   /* should_show_onboarding */
  },
  {
    "PreferencesForVersion068WithRewardsAndAdsEnabled",
    true,  /* supported_locale */
    true,  /* newly_supported_locale */
    true,  /* rewards_enabled */
    false, /* ads_enabled */
    true   /* should_show_onboarding */
  },

  // Upgrade from 0.69 to current version
  {
    "PreferencesForVersion069WithRewardsAndAdsDisabled",
    false, /* supported_locale */
    false, /* newly_supported_locale */
    false, /* rewards_enabled */
    false, /* ads_enabled */
    false  /* should_show_onboarding */
  },
  {
    "PreferencesForVersion069WithRewardsEnabledAndAdsDisabled",
    false, /* supported_locale */
    false, /* newly_supported_locale */
    true,  /* rewards_enabled */
    false, /* ads_enabled */
    false  /* should_show_onboarding */
  },
  {
    "PreferencesForVersion069WithRewardsAndAdsEnabled",
    false, /* supported_locale */
    false, /* newly_supported_locale */
    true,  /* rewards_enabled */
    false, /* ads_enabled */
    false  /* should_show_onboarding */
  },
  {
    "PreferencesForVersion069WithRewardsAndAdsDisabled",
    true,  /* supported_locale */
    false, /* newly_supported_locale */
    false, /* rewards_enabled */
    false, /* ads_enabled */
    false  /* should_show_onboarding */
  },
  {
    "PreferencesForVersion069WithRewardsEnabledAndAdsDisabled",
    true,  /* supported_locale */
    false, /* newly_supported_locale */
    true,  /* rewards_enabled */
    false, /* ads_enabled */
    true   /* should_show_onboarding */
  },
  {
    "PreferencesForVersion069WithRewardsAndAdsEnabled",
    true,  /* supported_locale */
    false, /* newly_supported_locale */
    true,  /* rewards_enabled */
    true,  /* ads_enabled */
    false  /* should_show_onboarding */
  },
  {
    "PreferencesForVersion069WithRewardsAndAdsDisabled",
    true,  /* supported_locale */
    true,  /* newly_supported_locale */
    false, /* rewards_enabled */
    false, /* ads_enabled */
    false  /* should_show_onboarding */
  },
  {
    "PreferencesForVersion069WithRewardsEnabledAndAdsDisabled",
    true,  /* supported_locale */
    true,  /* newly_supported_locale */
    true,  /* rewards_enabled */
    false, /* ads_enabled */
    true   /* should_show_onboarding */
  },
  {
    "PreferencesForVersion069WithRewardsAndAdsEnabled",
    true,  /* supported_locale */
    true,  /* newly_supported_locale */
    true,  /* rewards_enabled */
    false, /* ads_enabled */
    true   /* should_show_onboarding */
  },

  // Upgrade from 0.70 to current version
  {
    "PreferencesForVersion070WithRewardsAndAdsDisabled",
    false, /* supported_locale */
    false, /* newly_supported_locale */
    false, /* rewards_enabled */
    false, /* ads_enabled */
    false  /* should_show_onboarding */
  },
  {
    "PreferencesForVersion070WithRewardsEnabledAndAdsDisabled",
    false, /* supported_locale */
    false, /* newly_supported_locale */
    true,  /* rewards_enabled */
    false, /* ads_enabled */
    false  /* should_show_onboarding */
  },
  {
    "PreferencesForVersion070WithRewardsAndAdsEnabled",
    false, /* supported_locale */
    false, /* newly_supported_locale */
    true,  /* rewards_enabled */
    false, /* ads_enabled */
    false  /* should_show_onboarding */
  },
  {
    "PreferencesForVersion070WithRewardsAndAdsDisabled",
    true,  /* supported_locale */
    false, /* newly_supported_locale */
    false, /* rewards_enabled */
    false, /* ads_enabled */
    false  /* should_show_onboarding */
  },
  {
    "PreferencesForVersion070WithRewardsEnabledAndAdsDisabled",
    true,  /* supported_locale */
    false, /* newly_supported_locale */
    true,  /* rewards_enabled */
    false, /* ads_enabled */
    true   /* should_show_onboarding */
  },
  {
    "PreferencesForVersion070WithRewardsAndAdsEnabled",
    true,  /* supported_locale */
    false, /* newly_supported_locale */
    true,  /* rewards_enabled */
    true,  /* ads_enabled */
    false  /* should_show_onboarding */
  },
  {
    "PreferencesForVersion070WithRewardsAndAdsDisabled",
    true,  /* supported_locale */
    true,  /* newly_supported_locale */
    false, /* rewards_enabled */
    false, /* ads_enabled */
    false  /* should_show_onboarding */
  },
  {
    "PreferencesForVersion070WithRewardsEnabledAndAdsDisabled",
    true,  /* supported_locale */
    true,  /* newly_supported_locale */
    true,  /* rewards_enabled */
    false, /* ads_enabled */
    true   /* should_show_onboarding */
  },
  {
    "PreferencesForVersion070WithRewardsAndAdsEnabled",
    true,  /* supported_locale */
    true,  /* newly_supported_locale */
    true,  /* rewards_enabled */
    false, /* ads_enabled */
    true   /* should_show_onboarding */
  },

  // Upgrade from 0.71 to current version
  {
    "PreferencesForVersion071WithRewardsAndAdsDisabled",
    false, /* supported_locale */
    false, /* newly_supported_locale */
    false, /* rewards_enabled */
    false, /* ads_enabled */
    false  /* should_show_onboarding */
  },
  {
    "PreferencesForVersion071WithRewardsEnabledAndAdsDisabled",
    false, /* supported_locale */
    false, /* newly_supported_locale */
    true,  /* rewards_enabled */
    false, /* ads_enabled */
    false  /* should_show_onboarding */
  },
  {
    "PreferencesForVersion071WithRewardsAndAdsEnabled",
    false, /* supported_locale */
    false, /* newly_supported_locale */
    true,  /* rewards_enabled */
    false, /* ads_enabled */
    false  /* should_show_onboarding */
  },
  {
    "PreferencesForVersion071WithRewardsAndAdsDisabled",
    true,  /* supported_locale */
    false, /* newly_supported_locale */
    false, /* rewards_enabled */
    false, /* ads_enabled */
    false  /* should_show_onboarding */
  },
  {
    "PreferencesForVersion071WithRewardsEnabledAndAdsDisabled",
    true,  /* supported_locale */
    false, /* newly_supported_locale */
    true,  /* rewards_enabled */
    false, /* ads_enabled */
    true   /* should_show_onboarding */
  },
  {
    "PreferencesForVersion071WithRewardsAndAdsEnabled",
    true,  /* supported_locale */
    false, /* newly_supported_locale */
    true,  /* rewards_enabled */
    true,  /* ads_enabled */
    false  /* should_show_onboarding */
  },
  {
    "PreferencesForVersion071WithRewardsAndAdsDisabled",
    true,  /* supported_locale */
    true,  /* newly_supported_locale */
    false, /* rewards_enabled */
    false, /* ads_enabled */
    false  /* should_show_onboarding */
  },
  {
    "PreferencesForVersion071WithRewardsEnabledAndAdsDisabled",
    true,  /* supported_locale */
    true,  /* newly_supported_locale */
    true,  /* rewards_enabled */
    false, /* ads_enabled */
    true   /* should_show_onboarding */
  },
  {
    "PreferencesForVersion071WithRewardsAndAdsEnabled",
    true,  /* supported_locale */
    true,  /* newly_supported_locale */
    true,  /* rewards_enabled */
    false, /* ads_enabled */
    true   /* should_show_onboarding */
  },

  // Upgrade from 0.72 to current version
  {
    "PreferencesForVersion072WithRewardsAndAdsDisabled",
    false, /* supported_locale */
    false, /* newly_supported_locale */
    false, /* rewards_enabled */
    false, /* ads_enabled */
    false  /* should_show_onboarding */
  },
  {
    "PreferencesForVersion072WithRewardsEnabledAndAdsDisabled",
    false, /* supported_locale */
    false, /* newly_supported_locale */
    true,  /* rewards_enabled */
    false, /* ads_enabled */
    false  /* should_show_onboarding */
  },
  {
    "PreferencesForVersion072WithRewardsAndAdsEnabled",
    false, /* supported_locale */
    false, /* newly_supported_locale */
    true,  /* rewards_enabled */
    false, /* ads_enabled */
    false  /* should_show_onboarding */
  },
  {
    "PreferencesForVersion072WithRewardsAndAdsDisabled",
    true,  /* supported_locale */
    false, /* newly_supported_locale */
    false, /* rewards_enabled */
    false, /* ads_enabled */
    false  /* should_show_onboarding */
  },
  {
    "PreferencesForVersion072WithRewardsEnabledAndAdsDisabled",
    true,  /* supported_locale */
    false, /* newly_supported_locale */
    true,  /* rewards_enabled */
    false, /* ads_enabled */
    true   /* should_show_onboarding */
  },
  {
    "PreferencesForVersion072WithRewardsAndAdsEnabled",
    true,  /* supported_locale */
    false, /* newly_supported_locale */
    true,  /* rewards_enabled */
    true,  /* ads_enabled */
    false  /* should_show_onboarding */
  },
  {
    "PreferencesForVersion072WithRewardsAndAdsDisabled",
    true,  /* supported_locale */
    true,  /* newly_supported_locale */
    false, /* rewards_enabled */
    false, /* ads_enabled */
    false  /* should_show_onboarding */
  },
  {
    "PreferencesForVersion072WithRewardsEnabledAndAdsDisabled",
    true,  /* supported_locale */
    true,  /* newly_supported_locale */
    true,  /* rewards_enabled */
    false, /* ads_enabled */
    true   /* should_show_onboarding */
  },
  {
    "PreferencesForVersion072WithRewardsAndAdsEnabled",
    true,  /* supported_locale */
    true,  /* newly_supported_locale */
    true,  /* rewards_enabled */
    false, /* ads_enabled */
    true   /* should_show_onboarding */
  },

  // Upgrade from 1.2 to current version
  {
    "PreferencesForVersion12WithRewardsAndAdsDisabled",
    false, /* supported_locale */
    false, /* newly_supported_locale */
    false, /* rewards_enabled */
    false, /* ads_enabled */
    false  /* should_show_onboarding */
  },
  {
    "PreferencesForVersion12WithRewardsEnabledAndAdsDisabled",
    false, /* supported_locale */
    false, /* newly_supported_locale */
    true,  /* rewards_enabled */
    false, /* ads_enabled */
    false  /* should_show_onboarding */
  },
  {
    "PreferencesForVersion12WithRewardsAndAdsEnabled",
    false, /* supported_locale */
    false, /* newly_supported_locale */
    true,  /* rewards_enabled */
    false, /* ads_enabled */
    false  /* should_show_onboarding */
  },
  {
    "PreferencesForVersion12WithRewardsAndAdsDisabled",
    true,  /* supported_locale */
    false, /* newly_supported_locale */
    false, /* rewards_enabled */
    false, /* ads_enabled */
    false  /* should_show_onboarding */
  },
  {
    "PreferencesForVersion12WithRewardsEnabledAndAdsDisabled",
    true,  /* supported_locale */
    false, /* newly_supported_locale */
    true,  /* rewards_enabled */
    false, /* ads_enabled */
    false  /* should_show_onboarding */
  },
  {
    "PreferencesForVersion12WithRewardsAndAdsEnabled",
    true,  /* supported_locale */
    false, /* newly_supported_locale */
    true,  /* rewards_enabled */
    true,  /* ads_enabled */
    false  /* should_show_onboarding */
  },
  {
    "PreferencesForVersion12WithRewardsAndAdsDisabled",
    true,  /* supported_locale */
    true,  /* newly_supported_locale */
    false, /* rewards_enabled */
    false, /* ads_enabled */
    false  /* should_show_onboarding */
  },
  {
    "PreferencesForVersion12WithRewardsEnabledAndAdsDisabled",
    true,  /* supported_locale */
    true,  /* newly_supported_locale */
    true,  /* rewards_enabled */
    false, /* ads_enabled */
    true   /* should_show_onboarding */
  },
  {
    "PreferencesForVersion12WithRewardsAndAdsEnabled",
    true,  /* supported_locale */
    true,  /* newly_supported_locale */
    true,  /* rewards_enabled */
    false, /* ads_enabled */
    true   /* should_show_onboarding */
  }
};

IN_PROC_BROWSER_TEST_P(BraveAdsBrowserTest, PRE_UpgradePath) {
  // Handled in |MaybeMockLocaleHelperForBraveAdsUpgradePath|
}

IN_PROC_BROWSER_TEST_P(BraveAdsBrowserTest, UpgradePath) {
  BraveAdsUpgradePathParamInfo param(GetParam());

  EXPECT_EQ(IsRewardsEnabled(), param.rewards_enabled);

  EXPECT_EQ(IsAdsEnabled(), param.ads_enabled);

  bool is_showing_notification = IsShowingNotificationForType(
      RewardsNotificationType::REWARDS_NOTIFICATION_ADS_ONBOARDING);
  EXPECT_EQ(is_showing_notification, param.should_show_onboarding);
}

// Generate the test case name from the metadata included in
// |BraveAdsUpgradePathParamInfo|
static std::string GetTestCaseName(
    ::testing::TestParamInfo<BraveAdsUpgradePathParamInfo> param_info) {
  const char* preferences = param_info.param.preferences.c_str();

  const char* supported_locale = param_info.param.supported_locale ?
      "ForSupportedLocale" : "ForUnsupportedLocale";

  const char* newly_supported_locale = param_info.param.newly_supported_locale ?
      "ForNewlySupportedLocale" : "ForUnsupportedLocale";

  const char* rewards_enabled = param_info.param.rewards_enabled ?
      "RewardsShouldBeEnabled" : "RewardsShouldBeDisabled";

  const char* ads_enabled = param_info.param.ads_enabled ?
      "AdsShouldBeEnabled" : "AdsShouldBeDisabled";

  const char* should_show_onboarding = param_info.param.should_show_onboarding ?
      "ShouldShowOnboarding" : "ShouldNotShowOnboarding";

  // NOTE: You should not remove, change the format or reorder the following
  // parameters as they are parsed in |GetUpgradePathParams|
  return base::StringPrintf("%s_%s_%s_%s_%s_%s", preferences, supported_locale,
      newly_supported_locale, rewards_enabled, ads_enabled,
          should_show_onboarding);
}

INSTANTIATE_TEST_SUITE_P(BraveRewardsBrowserTest,
    BraveAdsBrowserTest, ::testing::ValuesIn(kTests), GetTestCaseName);
