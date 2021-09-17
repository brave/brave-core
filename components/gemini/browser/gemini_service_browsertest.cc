/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/path_service.h"
#include "brave/browser/gemini/gemini_service_factory.h"
#include "brave/common/brave_paths.h"
#include "brave/components/gemini/browser/gemini_service.h"
#include "brave/components/gemini/browser/pref_names.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/country_codes/country_codes.h"
#include "components/prefs/pref_service.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "net/dns/mock_host_resolver.h"
#include "net/base/url_util.h"
#include "net/test/embedded_test_server/http_request.h"
#include "net/test/embedded_test_server/http_response.h"

// npm run test -- brave_browser_tests --filter=GeminiAPIBrowserTest.*

namespace {

std::unique_ptr<net::test_server::HttpResponse> HandleRequest(
    const net::test_server::HttpRequest& request) {
  std::unique_ptr<net::test_server::BasicHttpResponse> http_response(
      new net::test_server::BasicHttpResponse());
  http_response->set_code(net::HTTP_OK);
  http_response->set_content_type("text/html");
  std::string request_path = request.GetURL().path();
  if (request_path == auth_path_access_token) {
    http_response->set_content(R"({
        "access_token": "83f2bf51-a2c4-4c2e-b7c4-46cef6a8dba5",
        "refresh_token": "fb5587ee-d9cf-4cb5-a586-4aed72cc9bea",
        "scope": "Trader",
        "token_type": "Bearer",
        "expires_in": 60000
    })");
  } else if (request_path == std::string(api_path_get_quote) + "/buy/btcusd") {
    http_response->set_content(R"({
      "quoteId": 1328,
      "maxAgeMs": 60000,
      "pair": "BTCUSD",
      "price": "6445.07",
      "priceCurrency": "USD",
      "side": "buy",
      "quantity": "0.01505181",
      "quantityCurrency": "BTC",
      "fee": "2.9900309233",
      "feeCurrency": "USD",
      "depositFee": "0",
      "depositFeeCurrency": "USD",
      "totalSpend": "100",
      "totalSpendCurrency": "USD"
    })");
  } else if (request_path == std::string(api_path_get_quote) + "/sell/batusd") {
    http_response->set_content(R"({
      "quoteId": 1328,
      "maxAgeMs": 60000,
      "pair": "BATUSD",
      "price": "0.25635",
      "priceCurrency": "USD",
      "side": "sell",
      "quantity": "20.00",
      "quantityCurrency": "BAT",
      "fee": "0.99",
      "feeCurrency": "USD",
      "depositFee": "0",
      "depositFeeCurrency": "BAT",
      "totalSpend": "20",
      "totalSpendCurrency": "BAT"
    })");
  } else if (request_path == api_path_account_balances) {
    http_response->set_content(R"(
      [
        {
            "type": "exchange",
            "currency": "BTC",
            "amount": "1154.62034001",
            "available": "1129.10517279",
            "availableForWithdrawal": "1129.10517279"
        },
        {
            "type": "exchange",
            "currency": "USD",
            "amount": "18722.79",
            "available": "14481.62",
            "availableForWithdrawal": "14481.62"
        },
        {
            "type": "exchange",
            "currency": "ETH",
            "amount": "20124.50369697",
            "available": "20124.50369697",
            "availableForWithdrawal": "20124.50369697"
        }
      ]
    )");
  } else if (request_path == std::string(
      api_path_account_addresses) + "/BTC") {
    http_response->set_content(R"(
      [
        {
          "address" : "n2saq73aDTu42bRgEHd8gd4to1gCzHxrdj",
          "timestamp" : 1424285102000,
          "label" : "my bitcoin address"
        }
      ]
    )");
  }
  return std::move(http_response);
}

std::unique_ptr<net::test_server::HttpResponse> HandleRequestUnauthorized(
    const net::test_server::HttpRequest& request) {
  std::unique_ptr<net::test_server::BasicHttpResponse> http_response(
      new net::test_server::BasicHttpResponse());
  http_response->set_content_type("text/html");
  http_response->set_code(net::HTTP_UNAUTHORIZED);
  return std::move(http_response);
}

std::unique_ptr<net::test_server::HttpResponse> HandleRequestServerError(
    const net::test_server::HttpRequest& request) {
  std::unique_ptr<net::test_server::BasicHttpResponse> http_response(
      new net::test_server::BasicHttpResponse());
  http_response->set_content_type("text/html");
  http_response->set_code(net::HTTP_INTERNAL_SERVER_ERROR);
  return std::move(http_response);
}

const char kGeminiAPIExistsScript[] =
    "window.domAutomationController.send(!!chrome.gemini)";

}  // namespace

class GeminiAPIBrowserTest : public InProcessBrowserTest {
 public:
  GeminiAPIBrowserTest() :
      expected_success_(false) {
  }

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    host_resolver()->AddRule("*", "127.0.0.1");

    ResetHTTPSServer(base::BindRepeating(&HandleRequest));

    brave::RegisterPathProvider();
    base::FilePath test_data_dir;
    base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir);
  }

  ~GeminiAPIBrowserTest() override {
  }

  content::WebContents* contents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  void ResetHTTPSServer(
      const net::EmbeddedTestServer::HandleRequestCallback& callback) {
    https_server_.reset(new net::EmbeddedTestServer(
        net::test_server::EmbeddedTestServer::TYPE_HTTPS));
    https_server_->SetSSLConfig(net::EmbeddedTestServer::CERT_OK);
    https_server_->RegisterRequestHandler(callback);
    ASSERT_TRUE(https_server_->Start());
    GeminiService* service = GetGeminiService();
    std::string host = https_server_->base_url().host() + ":" +
        std::to_string(https_server_->port());
    service->SetOAuthHostForTest(host);
    service->SetApiHostForTest(host);
  }

  void OnGetAccessToken(bool unauthorized, bool check_set_prefs) {
    if (check_set_prefs) {
      ASSERT_FALSE(browser()->profile()->GetPrefs()->GetString(
          kGeminiAccessToken).empty());
      ASSERT_FALSE(browser()->profile()->GetPrefs()->GetString(
          kGeminiRefreshToken).empty());
    }
    if (wait_for_request_) {
      wait_for_request_->Quit();
    }
    ASSERT_EQ(expected_success_, unauthorized);
  }

  void WaitForGetAccessToken(bool expected_success) {
    if (wait_for_request_) {
      return;
    }
    expected_success_ = expected_success;
    wait_for_request_.reset(new base::RunLoop);
    wait_for_request_->Run();
  }

  void WaitForRefreshAccessToken(bool expected_success) {
    if (wait_for_request_) {
      return;
    }
    expected_success_ = expected_success;
    wait_for_request_.reset(new base::RunLoop);
    wait_for_request_->Run();
  }

  void OnGetOrderQuote(const std::string& quote_id,
      const std::string& quantity, const std::string& fee,
      const std::string& price, const std::string& total_price,
      const std::string& error) {
    if (wait_for_request_) {
      wait_for_request_->Quit();
    }
    ASSERT_EQ(expected_quote_id_, quote_id);
    ASSERT_EQ(expected_quantity_, quantity);
    ASSERT_EQ(expected_total_fee_, fee);
    ASSERT_EQ(expected_quote_price_, price);
    ASSERT_EQ(expected_total_price_, total_price);
  }

  void WaitForGetOrderQuote(const std::string& expected_quote_id,
      const std::string& expected_quantity,
      const std::string& expected_total_fee,
      const std::string& expected_quote_price,
      const std::string& expected_total_price) {
    if (wait_for_request_) {
      return;
    }

    expected_quote_id_ = expected_quote_id;
    expected_quote_price_ = expected_quote_price;
    expected_total_fee_ = expected_total_fee;
    expected_quantity_ = expected_quantity;
    expected_total_price_ = expected_total_price;

    wait_for_request_.reset(new base::RunLoop);
    wait_for_request_->Run();
  }

  void OnGetAccountBalances(
      const GeminiAccountBalances& balances,
      bool success) {
    if (wait_for_request_) {
      wait_for_request_->Quit();
    }
    ASSERT_EQ(expected_balances_, balances);
    ASSERT_EQ(expected_success_, success);
  }

  void WaitForGetAccountBalances(
      const GeminiAccountBalances& expected_balances,
      bool expected_success) {
    if (wait_for_request_) {
      return;
    }
    expected_balances_ = expected_balances;
    expected_success_ = expected_success;
    wait_for_request_.reset(new base::RunLoop);
    wait_for_request_->Run();
  }

  void OnGetDepositInfo(const std::string& address) {
    if (wait_for_request_) {
      wait_for_request_->Quit();
    }
    ASSERT_EQ(expected_address_, address);
  }

  void WaitForGetDepositInfo(const std::string& expected_address) {
    if (wait_for_request_) {
      return;
    }
    expected_address_ = expected_address;
    wait_for_request_.reset(new base::RunLoop);
    wait_for_request_->Run();
  }

  content::WebContents* active_contents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  bool NavigateToNewTabUntilLoadStop() {
    EXPECT_TRUE(
        ui_test_utils::NavigateToURL(browser(), GURL("chrome://newtab")));
    return WaitForLoadStop(active_contents());
  }

  bool NavigateToVersionTabUntilLoadStop() {
    EXPECT_TRUE(
        ui_test_utils::NavigateToURL(browser(), GURL("chrome://version")));
    return WaitForLoadStop(active_contents());
  }

  GeminiService* GetGeminiService() {
    GeminiService* service = GeminiServiceFactory::GetInstance()
        ->GetForProfile(Profile::FromBrowserContext(browser()->profile()));
    EXPECT_TRUE(service);
    return service;
  }

 private:
  net::EmbeddedTestServer* https_server() { return https_server_.get(); }

  bool expected_success_;
  std::string expected_quote_id_;
  std::string expected_quote_price_;
  std::string expected_total_price_;
  std::string expected_total_fee_;
  std::string expected_quantity_;
  std::string expected_address_;
  GeminiAccountBalances expected_balances_;

  std::unique_ptr<base::RunLoop> wait_for_request_;
  std::unique_ptr<net::EmbeddedTestServer> https_server_;
};


IN_PROC_BROWSER_TEST_F(GeminiAPIBrowserTest, GetOAuthClientURL) {
  EXPECT_TRUE(NavigateToNewTabUntilLoadStop());
  auto* service = GetGeminiService();
  service->SetClientIdForTest("fake-client-id");

  GURL client_url(service->GetOAuthClientUrl());
  GURL expected_url(
    "https://exchange.gemini.com/auth?"
    "response_type=code&"
    "client_id=fake-client-id&"
    "redirect_uri=com.brave.gemini%3A%2F%2Fauthorization&"
    "scope=addresses%3Aread%2Cbalances%3Aread%2Corders%3Acreate&"
    "code_challenge=da0KASk6XZX4ksgvIGAa87iwNSVvmWdys2GYh3kjBZw&"
    "code_challenge_method=S256&"
    "state=placeholder");
  client_url = net::AppendOrReplaceQueryParameter(client_url, "state",
      "fake-state");
  client_url = net::AppendOrReplaceQueryParameter(client_url, "code_challenge",
      "fake-challenge");
  expected_url = net::AppendOrReplaceQueryParameter(expected_url,
      "state", "fake-state");
  expected_url = net::AppendOrReplaceQueryParameter(expected_url,
      "code_challenge", "fake-challenge");
  ASSERT_EQ(expected_url, client_url);
}

#if !defined(OS_WIN)
IN_PROC_BROWSER_TEST_F(GeminiAPIBrowserTest, GetAccessToken) {
  ResetHTTPSServer(base::BindRepeating(&HandleRequest));
  EXPECT_TRUE(NavigateToNewTabUntilLoadStop());
  auto* service = GetGeminiService();
  service->SetAuthToken("abc123");
  ASSERT_TRUE(service->GetAccessToken(
      base::BindOnce(
          &GeminiAPIBrowserTest::OnGetAccessToken,
          base::Unretained(this), true)));
  WaitForGetAccessToken(true);
}
#endif

IN_PROC_BROWSER_TEST_F(GeminiAPIBrowserTest, GetAccessTokenUnauthorized) {
  ResetHTTPSServer(base::BindRepeating(&HandleRequestUnauthorized));
  EXPECT_TRUE(NavigateToNewTabUntilLoadStop());
  auto* service = GetGeminiService();
  service->SetAuthToken("abc123");
  ASSERT_TRUE(service->GetAccessToken(
      base::BindOnce(
          &GeminiAPIBrowserTest::OnGetAccessToken,
          base::Unretained(this), false)));
  WaitForGetAccessToken(false);
}

IN_PROC_BROWSER_TEST_F(GeminiAPIBrowserTest, GetAccessTokenServerError) {
  ResetHTTPSServer(base::BindRepeating(&HandleRequestServerError));
  EXPECT_TRUE(NavigateToNewTabUntilLoadStop());
  auto* service = GetGeminiService();
  service->SetAuthToken("abc123");
  ASSERT_TRUE(service->GetAccessToken(
      base::BindOnce(
          &GeminiAPIBrowserTest::OnGetAccessToken,
          base::Unretained(this), false)));
  WaitForGetAccessToken(false);
}

#if !defined(OS_WIN)
IN_PROC_BROWSER_TEST_F(GeminiAPIBrowserTest, RefreshAccessToken) {
  ResetHTTPSServer(base::BindRepeating(&HandleRequest));
  EXPECT_TRUE(NavigateToNewTabUntilLoadStop());
  auto* service = GetGeminiService();
  service->SetAuthToken("abc123");
  ASSERT_TRUE(service->RefreshAccessToken(
      base::BindOnce(
          &GeminiAPIBrowserTest::OnGetAccessToken,
          base::Unretained(this), true)));
  WaitForRefreshAccessToken(true);
}
#endif

IN_PROC_BROWSER_TEST_F(GeminiAPIBrowserTest, RefreshTokenUnauthorized) {
  ResetHTTPSServer(base::BindRepeating(&HandleRequestUnauthorized));
  EXPECT_TRUE(NavigateToNewTabUntilLoadStop());
  auto* service = GetGeminiService();
  service->SetAuthToken("abc123");
  ASSERT_TRUE(service->RefreshAccessToken(
      base::BindOnce(
          &GeminiAPIBrowserTest::OnGetAccessToken,
          base::Unretained(this), false)));
  WaitForRefreshAccessToken(false);
}

IN_PROC_BROWSER_TEST_F(GeminiAPIBrowserTest, RefreshTokenServerError) {
  ResetHTTPSServer(base::BindRepeating(&HandleRequestServerError));
  EXPECT_TRUE(NavigateToNewTabUntilLoadStop());
  auto* service = GetGeminiService();
  service->SetAuthToken("abc123");
  ASSERT_TRUE(service->RefreshAccessToken(
      base::BindOnce(
          &GeminiAPIBrowserTest::OnGetAccessToken,
          base::Unretained(this), false)));
  WaitForRefreshAccessToken(false);
}

IN_PROC_BROWSER_TEST_F(GeminiAPIBrowserTest, GetOrderQuoteBuy) {
  ResetHTTPSServer(base::BindRepeating(&HandleRequest));
  EXPECT_TRUE(NavigateToNewTabUntilLoadStop());
  auto* service = GetGeminiService();
  ASSERT_TRUE(service->GetOrderQuote("buy", "btcusd", "100",
      base::BindOnce(
          &GeminiAPIBrowserTest::OnGetOrderQuote,
          base::Unretained(this))));
  WaitForGetOrderQuote("1328", "0.01505181", "2.9900309233", "6445.07", "100");
}

IN_PROC_BROWSER_TEST_F(GeminiAPIBrowserTest, GetOrderQuoteSell) {
  ResetHTTPSServer(base::BindRepeating(&HandleRequest));
  EXPECT_TRUE(NavigateToNewTabUntilLoadStop());
  auto* service = GetGeminiService();
  ASSERT_TRUE(service->GetOrderQuote("sell", "batusd", "20",
      base::BindOnce(
          &GeminiAPIBrowserTest::OnGetOrderQuote,
          base::Unretained(this))));
  WaitForGetOrderQuote("1328", "20.00", "0.99", "0.25635", "4.137000");
}

IN_PROC_BROWSER_TEST_F(GeminiAPIBrowserTest, GetOrderQuoteUnauthorized) {
  ResetHTTPSServer(base::BindRepeating(&HandleRequestUnauthorized));
  EXPECT_TRUE(NavigateToNewTabUntilLoadStop());
  auto* service = GetGeminiService();
  ASSERT_TRUE(service->GetOrderQuote("buy", "btcusd", "10",
      base::BindOnce(
          &GeminiAPIBrowserTest::OnGetOrderQuote,
          base::Unretained(this))));
  WaitForGetOrderQuote("", "", "", "", "");
}

IN_PROC_BROWSER_TEST_F(GeminiAPIBrowserTest, GetOrderQuoteServerError) {
  ResetHTTPSServer(base::BindRepeating(&HandleRequestServerError));
  EXPECT_TRUE(NavigateToNewTabUntilLoadStop());
  auto* service = GetGeminiService();
  ASSERT_TRUE(service->GetOrderQuote("buy", "btcusd", "10",
      base::BindOnce(
          &GeminiAPIBrowserTest::OnGetOrderQuote,
          base::Unretained(this))));
  WaitForGetOrderQuote("", "", "", "", "");
}

IN_PROC_BROWSER_TEST_F(GeminiAPIBrowserTest, GetAccountBalances) {
  ResetHTTPSServer(base::BindRepeating(&HandleRequest));
  EXPECT_TRUE(NavigateToNewTabUntilLoadStop());
  auto* service = GetGeminiService();
  ASSERT_TRUE(service->GetAccountBalances(
      base::BindOnce(
          &GeminiAPIBrowserTest::OnGetAccountBalances,
          base::Unretained(this))));
  WaitForGetAccountBalances(
      GeminiAccountBalances {
          {
            {"BTC", "1129.10517279"},
            {"USD", "14481.62"},
            {"ETH", "20124.50369697"}
          }
      }, false);
}

IN_PROC_BROWSER_TEST_F(GeminiAPIBrowserTest, GetAccountBalancesUnauthorized) {
  ResetHTTPSServer(base::BindRepeating(&HandleRequestUnauthorized));
  EXPECT_TRUE(NavigateToNewTabUntilLoadStop());
  auto* service = GetGeminiService();
  ASSERT_TRUE(service->GetAccountBalances(
      base::BindOnce(
          &GeminiAPIBrowserTest::OnGetAccountBalances,
          base::Unretained(this))));
  WaitForGetAccountBalances(GeminiAccountBalances(), true);
}

IN_PROC_BROWSER_TEST_F(GeminiAPIBrowserTest, GetAccountBalancesServerError) {
  ResetHTTPSServer(base::BindRepeating(&HandleRequestServerError));
  EXPECT_TRUE(NavigateToNewTabUntilLoadStop());
  auto* service = GetGeminiService();
  ASSERT_TRUE(service->GetAccountBalances(
      base::BindOnce(
          &GeminiAPIBrowserTest::OnGetAccountBalances,
          base::Unretained(this))));
  WaitForGetAccountBalances(GeminiAccountBalances(), false);
}

IN_PROC_BROWSER_TEST_F(GeminiAPIBrowserTest, GetDepositInfo) {
  ResetHTTPSServer(base::BindRepeating(&HandleRequest));
  EXPECT_TRUE(NavigateToNewTabUntilLoadStop());
  auto* service = GetGeminiService();
  ASSERT_TRUE(service->GetDepositInfo("BTC",
      base::BindOnce(
          &GeminiAPIBrowserTest::OnGetDepositInfo,
          base::Unretained(this))));
  std::string address = "n2saq73aDTu42bRgEHd8gd4to1gCzHxrdj";
  WaitForGetDepositInfo(address);
}

IN_PROC_BROWSER_TEST_F(GeminiAPIBrowserTest, GetDepositInfoUnauthorized) {
  ResetHTTPSServer(base::BindRepeating(&HandleRequestUnauthorized));
  EXPECT_TRUE(NavigateToNewTabUntilLoadStop());
  auto* service = GetGeminiService();
  ASSERT_TRUE(service->GetDepositInfo("BTC",
      base::BindOnce(
          &GeminiAPIBrowserTest::OnGetDepositInfo,
          base::Unretained(this))));
  WaitForGetDepositInfo("");
}

IN_PROC_BROWSER_TEST_F(GeminiAPIBrowserTest, GetDepositInfoServerError) {
  ResetHTTPSServer(base::BindRepeating(&HandleRequestServerError));
  EXPECT_TRUE(NavigateToNewTabUntilLoadStop());
  auto* service = GetGeminiService();
  ASSERT_TRUE(service->GetDepositInfo("BTC",
      base::BindOnce(
          &GeminiAPIBrowserTest::OnGetDepositInfo,
          base::Unretained(this))));
  WaitForGetDepositInfo("");
}

IN_PROC_BROWSER_TEST_F(GeminiAPIBrowserTest, NewTabHasGeminiAPIAccess) {
  EXPECT_TRUE(NavigateToNewTabUntilLoadStop());
  bool result = false;
  EXPECT_TRUE(
      ExecuteScriptAndExtractBool(contents(), kGeminiAPIExistsScript,
        &result));
  ASSERT_TRUE(result);
}

IN_PROC_BROWSER_TEST_F(GeminiAPIBrowserTest,
    OtherChromeTabHasGeminiAPIAccess) {
  EXPECT_TRUE(NavigateToVersionTabUntilLoadStop());
  bool result = true;
  EXPECT_TRUE(
      ExecuteScriptAndExtractBool(contents(), kGeminiAPIExistsScript,
        &result));
  ASSERT_FALSE(result);
}
