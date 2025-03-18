/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <memory>
#include <optional>
#include <string>
#include <utility>

#include "base/base64.h"
#include "base/containers/span.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/json/json_writer.h"
#include "base/memory/weak_ptr.h"
#include "base/numerics/byte_conversions.h"
#include "base/path_service.h"
#include "base/test/bind.h"
#include "base/test/values_test_util.h"
#include "brave/browser/brave_rewards/rewards_service_factory.h"
#include "brave/browser/ui/brave_rewards/rewards_panel_coordinator.h"
#include "brave/components/brave_rewards/content/rewards_service_impl.h"
#include "brave/components/brave_rewards/core/engine/publisher/protos/channel_response.pb.h"
#include "brave/components/brave_rewards/core/pref_names.h"
#include "brave/components/constants/brave_paths.h"
#include "brave/components/constants/webui_url_constants.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/os_crypt/sync/os_crypt.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"

namespace brave_rewards {

// See the "EnableRewards" test for hints on writing new Rewards page tests.
class RewardsPageBrowserTest : public InProcessBrowserTest {
 protected:
  RewardsPageBrowserTest() = default;
  ~RewardsPageBrowserTest() override = default;

  void SetUpOnMainThread() override {
    test_data_dir_ = base::PathService::CheckedGet(brave::DIR_TEST_DATA)
                         .AppendASCII("brave_rewards")
                         .AppendASCII("rewards_page");

    GetRewardsService().ForTestingSetTestResponseCallback(base::BindRepeating(
        &RewardsPageBrowserTest::HandleRequest, base::Unretained(this)));
  }

  PrefService& GetPrefs() {
    auto* prefs = browser()->profile()->GetPrefs();
    CHECK(prefs);
    return *prefs;
  }

  std::optional<std::string> EncryptPrefString(const std::string& value) {
    std::string encrypted;
    if (!OSCrypt::EncryptString(value, &encrypted)) {
      return {};
    }
    return base::Base64Encode(encrypted);
  }

  RewardsServiceImpl& GetRewardsService() {
    auto* rewards_service = static_cast<RewardsServiceImpl*>(
        RewardsServiceFactory::GetForProfile(browser()->profile()));
    CHECK(rewards_service);
    return *rewards_service;
  }

  static std::string BuildPublisherChannelResponse(
      const std::string& channel_id) {
    publishers_pb::ChannelResponseList message;
    auto* channel = message.add_channel_responses();
    channel->set_channel_identifier(channel_id);
    auto* uphold_wallet = channel->add_wallets()->mutable_uphold_wallet();
    uphold_wallet->set_wallet_state(publishers_pb::UPHOLD_ACCOUNT_KYC);
    uphold_wallet->set_address("address1");
    std::string out;
    message.SerializeToString(&out);

    // Add padded header.
    uint32_t length = out.length();
    out.insert(0, 4, ' ');
    base::as_writable_byte_span(out).first<4u>().copy_from(
        base::numerics::U32ToBigEndian(length));
    return out;
  }

  void LoadScript(const std::string& filename) {
    std::string script;
    auto full_path = test_data_dir_.AppendASCII(filename);
    {
      base::ScopedAllowBlockingForTesting allow_blocking;
      ASSERT_TRUE(base::ReadFileToString(full_path, &script));
    }
    ASSERT_TRUE(page_contents_);
    ASSERT_TRUE(content::ExecJs(page_contents_.get(), script));
  }

  void GivenRewardsIsEnabled() {
    auto& prefs = GetPrefs();

    prefs.SetBoolean(prefs::kEnabled, true);
    prefs.SetString(prefs::kDeclaredGeo, "US");
    prefs.SetString(prefs::kWalletBrave, R"(
        {"payment_id":"2b6e71a6-f3c7-5999-9235-11605a60ec93",
         "recovery_seed":"QgcQHdg6fo53/bGKVwZlL1UkLiql8X7U68jaWgz6FWQ="})");

    auto params = base::test::ParseJsonDict(R"(
        {
          "ac": {
            "choice": 1.0,
            "choices": [1.0, 2.0, 3.0, 5.0, 7.0, 10.0, 20.0]
          },
          "payout_status": {
            "bitflyer": "",
            "gemini": "",
            "solana": "",
            "uphold": "",
            "zebpay": ""
          },
          "rate": 0.25,
          "tip": {
            "choices": [1.25, 5.0, 10.5],
            "monthly_choices": [1.25, 5.0, 10.5]
          },
          "tos_version": 1,
          "vbat_deadline": "13343184000000000",
          "vbat_expired": true,
          "wallet_provider_regions": {
            "bitflyer": {
              "allow": ["JP"],
              "block": []
            },
            "gemini": {
              "allow": ["US", "SG", "GB", "CA"],
              "block": []
            },
            "solana": {
              "allow": [],
              "block": ["KP", "ES"]
            },
            "uphold": {
              "allow": ["US", "SG", "GB", "CA"],
              "block": []
            },
            "zebpay": {
              "allow": ["IN"],
              "block": []
            }
          }
        })");

    prefs.SetDict(prefs::kParameters, std::move(params));
  }

  void GivenUserIsConnected() {
    base::Value::Dict wallet;
    wallet.Set("token", "token");
    wallet.Set("address", "abe5f454-fedd-4ea9-9203-470ae7315bb3");
    wallet.Set("status", static_cast<int>(mojom::WalletStatus::kConnected));
    wallet.Set("user_name", "Brave Test");

    std::string json;
    base::JSONWriter::Write(wallet, &json);
    auto encrypted = EncryptPrefString(json);
    ASSERT_TRUE(encrypted);

    auto& prefs = GetPrefs();
    prefs.SetString(prefs::kExternalWalletType, "uphold");
    prefs.SetString(prefs::kWalletUphold, *encrypted);
  }

  void StartRewardsEngine() {
    base::RunLoop run_loop;
    GetRewardsService().StartProcessForTesting(
        base::BindLambdaForTesting([&]() { run_loop.Quit(); }));
    run_loop.Run();
  }

  void RunTests() {
    ASSERT_TRUE(page_contents_);
    ASSERT_TRUE(content::ExecJs(page_contents_.get(), "testing.runTests()"));
  }

  void NavigateToRewardsPage(const std::string& relative_url) {
    GURL url = GURL(kRewardsPageURL).Resolve(relative_url);
    ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
    auto* web_contents = browser()->tab_strip_model()->GetActiveWebContents();
    ASSERT_TRUE(web_contents);
    page_contents_ = web_contents->GetWeakPtr();
    LoadScript("test_setup.js");
  }

  void OpenRewardsPanel() {
    content::CreateAndLoadWebContentsObserver popup_observer;

    auto* coordinator = RewardsPanelCoordinator::FromBrowser(browser());
    ASSERT_TRUE(coordinator);
    ASSERT_TRUE(coordinator->OpenRewardsPanel());

    for (page_contents_.reset(); !page_contents_;) {
      content::WebContents* web_contents = popup_observer.Wait();
      GURL url = web_contents->GetLastCommittedURL();
      if (url.host() == kRewardsPageTopHost) {
        page_contents_ = web_contents->GetWeakPtr();
      }
    }

    LoadScript("test_setup.js");
  }

  using RequestHandlerResult = std::optional<std::pair<int, std::string>>;
  using RequestHandler =
      base::RepeatingCallback<RequestHandlerResult(const GURL&,
                                                   const std::string&)>;

  void SetRequestHandler(RequestHandler handler) {
    request_handler_ = std::move(handler);
  }

  static RequestHandlerResult HandleEnableRewardsRequest(
      const GURL& url,
      const std::string& method) {
    if (url.path_piece() == "/v4/wallets" && method == "POST") {
      return std::pair{201, R"({
            "paymentId": "33fe956b-ed15-515b-bccd-b6cc63a80e0e"
          })"};
    }
    return std::nullopt;
  }

  template <typename F>
  void WaitForFinishNavigation(F pred) {
    if (pred(page_contents_->GetLastCommittedURL())) {
      return;
    }
    base::RunLoop run_loop;
    content::DidFinishNavigationObserver url_observer(
        page_contents_.get(),
        base::BindLambdaForTesting(
            [&run_loop, &pred](content::NavigationHandle* navigation_handle) {
              if (pred(navigation_handle->GetURL())) {
                run_loop.Quit();
              }
            }));
    run_loop.Run();
  }

  void SavePublisherInfo(const std::string& id) {
    auto publisher = mojom::PublisherInfo::New();
    publisher->id = id;
    publisher->name = id;
    publisher->url = id;
    publisher->status = mojom::PublisherStatus::UPHOLD_VERIFIED;
    base::RunLoop run_loop;
    GetRewardsService().SavePublisherInfo(
        0, std::move(publisher),
        base::BindLambdaForTesting([&](mojom::Result) { run_loop.Quit(); }));
    run_loop.Run();
  }

 private:
  std::string GetMethodString(int32_t method) {
    return RewardsServiceImpl::UrlMethodToRequestType(
        static_cast<mojom::UrlMethod>(method));
  }

  void HandleRequest(const std::string& url,
                     int32_t method,
                     int* response_status_code,
                     std::string* response,
                     base::flat_map<std::string, std::string>* headers) {
    if (request_handler_) {
      std::string method_string = GetMethodString(method);
      auto result = request_handler_.Run(GURL(url), method_string);
      if (!result) {
        LOG(ERROR) << "Request <" << method_string << " " << url
                   << "> not handled";
        result = {404, ""};
      }
      *response_status_code = std::get<0>(*result);
      *response = std::get<1>(*result);
    } else {
      LOG(ERROR) << "Rewards page request handler not available";
      *response_status_code = 404;
      *response = "";
    }
  }

  base::FilePath test_data_dir_;
  base::WeakPtr<content::WebContents> page_contents_;
  RequestHandler request_handler_;
};

IN_PROC_BROWSER_TEST_F(RewardsPageBrowserTest, EnableRewards) {
  // Writing a new test?
  //
  // First, set up the Rewards profile state. This may involve setting prefs and
  // and starting the Rewards engine, and possibly calling methods on the
  // Rewards service.
  //
  // If necessary, set up a handler for Rewards engine API requests.
  SetRequestHandler(base::BindRepeating(HandleEnableRewardsRequest));

  // Next, navigate to the Rewards page (or open the Rewards panel). This will
  // also load the test setup script into the JS global object.
  NavigateToRewardsPage("/");

  // Load test scripts. Generally, there should be a test script that matches
  // the name of this test. Test scripts can perform actions on the page and
  // check for various success indicators. Test scripts can also provide mock
  // network responses that will be used by the Rewards engine.
  LoadScript("enable_rewards_test.js");

  // Run the tests in the page.
  RunTests();

  // Finally, perform any desired assertions on browser/profile state.
  ASSERT_FALSE(GetPrefs().GetString(prefs::kWalletBrave).empty());
  ASSERT_EQ(GetPrefs().GetString(prefs::kDeclaredGeo), "US");
}

IN_PROC_BROWSER_TEST_F(RewardsPageBrowserTest, EnableRewardsFromPanel) {
  SetRequestHandler(base::BindRepeating(HandleEnableRewardsRequest));
  OpenRewardsPanel();
  LoadScript("enable_rewards_test.js");
  RunTests();
  ASSERT_FALSE(GetPrefs().GetString(prefs::kWalletBrave).empty());
}

IN_PROC_BROWSER_TEST_F(RewardsPageBrowserTest, ResetRewards) {
  GivenRewardsIsEnabled();
  StartRewardsEngine();
  NavigateToRewardsPage("/reset");
  LoadScript("reset_rewards_test.js");
  RunTests();
  ASSERT_TRUE(GetPrefs().GetString(prefs::kWalletBrave).empty());
}

IN_PROC_BROWSER_TEST_F(RewardsPageBrowserTest, ConnectAccount) {
  GivenRewardsIsEnabled();
  StartRewardsEngine();

  NavigateToRewardsPage("/");
  LoadScript("connect_account_test.js");
  RunTests();

  // The rewards page should redirect the user to the external wallet provider's
  // login page. Wait for the redirection to occur and pull out the "state"
  // querystring parameter.
  std::string state;
  WaitForFinishNavigation([&state](const GURL& url) {
    std::string url_spec = url.spec();
    if (url_spec.find("/authorize/") == std::string::npos) {
      return false;
    }
    if (auto pos = url_spec.find("&state="); pos != std::string::npos) {
      state = url_spec.substr(pos);
    }
    return true;
  });

  SetRequestHandler(base::BindLambdaForTesting(
      [](const GURL& url, const std::string& method) -> RequestHandlerResult {
        if (url.path_piece() == "/oauth2/token" && method == "POST") {
          return std::pair{200, R"({ "access_token": "abc123" })"};
        }
        if (url.path_piece() == "/v0/me" && method == "GET") {
          return std::pair{200, R"({
                "firstName": "Test",
                "id": "abc123",
                "identityCountry": "US",
                "currencies": ["BAT"]
              })"};
        }
        if (url.path_piece() == "/v0/me/capabilities" && method == "GET") {
          return std::pair{200, R"([
                { "key": "sends", "enabled": true, "requirements": [] },
                { "key": "receives", "enabled": true, "requirements": [] }
              ])"};
        }
        if (url.path_piece() == "/v0/me/cards" && method == "POST") {
          return std::pair{200, R"({ "id": "abc123" })"};
        }
        if (url.path_piece() == "/v0/me/cards/abc123" && method == "PATCH") {
          return std::pair{200, ""};
        }
        std::string claim_path =
            "/v3/wallet/uphold/2b6e71a6-f3c7-5999-9235-11605a60ec93/claim";
        if (url.path_piece() == claim_path && method == "POST") {
          return std::pair{200, R"({ "geoCountry": "US" })"};
        }
        return std::nullopt;
      }));

  NavigateToRewardsPage("/uphold/authorization/?code=123456&state=" + state);
  LoadScript("connect_account_auth_test.js");
  RunTests();
}

IN_PROC_BROWSER_TEST_F(RewardsPageBrowserTest, SendContribution) {
  SetRequestHandler(base::BindLambdaForTesting(
      [](const GURL& url, const std::string& method) -> RequestHandlerResult {
        std::string card_path =
            "/v0/me/cards/abe5f454-fedd-4ea9-9203-470ae7315bb3";
        if (url.path_piece() == card_path && method == "GET") {
          return std::pair{200, R"({ "available": "30.0" })"};
        }
        if (url.path_piece() == "/publishers/prefixes/a379" &&
            method == "GET") {
          return std::pair{200, BuildPublisherChannelResponse("example.com")};
        }
        std::string transactions_path =
            "/v0/me/cards/abe5f454-fedd-4ea9-9203-470ae7315bb3/transactions";
        if (url.path_piece() == transactions_path && method == "POST") {
          return std::pair{200, R"({
              "id": "ba1ba438-49a8-4618-8c0b-099b69afc722"
            })"};
        }
        std::string commit_path =
            "/v0/me/cards/abe5f454-fedd-4ea9-9203-470ae7315bb3/transactions/"
            "ba1ba438-49a8-4618-8c0b-099b69afc722/commit";
        if (url.path_piece() == commit_path && method == "POST") {
          return std::pair{200, R"({ "status": "completed" })"};
        }
        return std::nullopt;
      }));

  GivenRewardsIsEnabled();
  GivenUserIsConnected();
  StartRewardsEngine();
  SavePublisherInfo("example.com");
  NavigateToRewardsPage("/?creator=example.com");
  LoadScript("send_contribution_test.js");
  RunTests();
}

}  // namespace brave_rewards
