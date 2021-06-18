/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/path_service.h"
#include "brave/browser/binance/binance_service_factory.h"
#include "brave/common/brave_paths.h"
#include "brave/common/pref_names.h"
#include "brave/components/binance/browser/binance_service.h"
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

// npm run test -- brave_browser_tests --filter=BinanceAPIBrowserTest.*

namespace {

std::unique_ptr<net::test_server::HttpResponse> HandleRequest(
    const net::test_server::HttpRequest& request) {
  std::unique_ptr<net::test_server::BasicHttpResponse> http_response(
      new net::test_server::BasicHttpResponse());
  http_response->set_code(net::HTTP_OK);
  http_response->set_content_type("text/html");
  std::string request_path = request.GetURL().path();
  if (request_path == oauth_path_access_token) {
    http_response->set_content(R"({
        "access_token": "83f2bf51-a2c4-4c2e-b7c4-46cef6a8dba5",
        "refresh_token": "fb5587ee-d9cf-4cb5-a586-4aed72cc9bea",
        "scope": "read",
        "token_type": "bearer",
        "expires_in": 30714
    })");
  } else if (request_path == oauth_path_convert_quote) {
    http_response->set_content(R"({
      "code": "000000",
      "message": null,
      "data": {
        "quoteId": "b5481fb7f8314bb2baf55aa6d4fcf068",
        "quotePrice": "1094.01086957",
        "tradeFee": "8",
        "railFee": "0",
        "totalFee": "8",
        "totalAmount": "100649",
        "showPrice": "1094.01086957"
      }
    })");
  } else if (request_path == oauth_path_account_balances) {
    http_response->set_content(R"({
      "code": "000000",
      "message": null,
      "data": [{
        "asset": "BAT",
        "free": "1000.00000",
        "locked": "0.00000000",
        "freeze": "0.00000000",
        "withdrawing": "0.00000000",
        "btcValuation": "0.021100",
        "fiatValuation": "20000.00000"
      }]
    })");
  } else if (request_path == oauth_path_deposit_info) {
    http_response->set_content(R"({
      "code": "000000",
      "message": null,
      "data": {
        "coin": "BTC",
        "address": "112tfsHDk6Yk8PbNnTVkv7yPox4aWYYDtW",
        "tag": "",
        "url": "https://btc.com/112tfsHDk6Yk8PbNnTVkv7yPox4aWYYDtW",
        "time": 1566366289000
      },
      "success": true
    })");
  } else if (request_path == oauth_path_convert_confirm) {
    http_response->set_content(R"({
      "code": "000000",
      "message": null,
      "data": {
        "quoteId": "b5481fb7f8314bb2baf55aa6d4fcf068",
        "status": "FAIL",
        "orderId": "ab0ab6cfd62240d79e10347fc5000bc4",
        "fromAsset": "BNB",
        "toAsset": "TRX",
        "sourceAmount": 100,
        "obtainAmount": 100649,
        "tradeFee": 8,
        "price": 1094.01086957,
        "feeType": 1,
        "feeRate": 0.08000000,
        "fixFee": 13.00000000
      },
      "success": true
    })");
  } else if (request_path == oauth_path_convert_assets) {
    http_response->set_content(R"({
      "code":"000000",
      "message":null,
      "data":[{
        "assetCode":"BTC",
        "assetName":"Bitcoin",
        "logoUrl":"https://bin.bnbstatic.com/images/20191211/fake.png",
        "size":"6",
        "order":0,
        "freeAsset":"0.00508311",
        "subSelector":[{
          "assetCode":"BNB",
          "assetName":"BNB",
          "logoUrl":"https://bin.bnbstatic.com/images/fake.png",
          "size":"2",
          "order":1,
          "perTimeMinLimit":"0.00200000",
          "perTimeMaxLimit":"1.00000000",
          "dailyMaxLimit":"10.00000000",
          "hadDailyLimit":"0",
          "needMarket":true,
          "feeType":1,
          "feeRate":"0.00050000",
          "fixFee":"1.00000000",
          "feeCoin":"BTC",
          "forexRate":"1.00000000",
          "expireTime":30
        }]
      }],
      "success":true
    })");
  } else if (request_path == oauth_path_revoke_token) {
    http_response->set_content(R"({
      "code": "000000",
      "message": null,
      "data": true,
      "success": true
    })");
  } else if (request_path == gateway_path_networks) {
    http_response->set_content(R"({
      "code": "000000",
      "message": null,
      "data": [
        {
          "coin": "BAT",
          "networkList": [
            {
              "coin": "BAT",
              "network": "ETH",
              "isDefault": true
            },
            {
              "coin": "BAT",
              "network": "BNB",
              "isDefault": false
            }
          ]
        },
        {
          "coin": "GAS",
          "networkList": [
            {
              "coin": "GAS",
              "network": "BTC",
              "isDefault": false
            },
            {
              "coin": "GAS",
              "network": "NEO",
              "isDefault": true
            }
          ]
        }
      ]
      })");
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

const char kBinanceAPIExistsScript[] =
    "window.domAutomationController.send(!!chrome.binance)";

}  // namespace

class BinanceAPIBrowserTest : public InProcessBrowserTest {
 public:
  BinanceAPIBrowserTest() :
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

  ~BinanceAPIBrowserTest() override {
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
    BinanceService* service = GetBinanceService();
    std::string host = https_server_->base_url().host() + ":" +
        std::to_string(https_server_->port());
    service->SetOAuthHostForTest(host);
    service->SetGatewayHostForTest(host);
  }

  void OnGetAccessToken(bool unauthorized, bool check_set_prefs) {
    if (check_set_prefs) {
      ASSERT_FALSE(browser()->profile()->GetPrefs()->GetString(
          kBinanceAccessToken).empty());
      ASSERT_FALSE(browser()->profile()->GetPrefs()->GetString(
          kBinanceRefreshToken).empty());
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

  void OnGetConvertQuote(const std::string& quote_id,
      const std::string& quote_price, const std::string& total_fee,
      const std::string& total_amount) {
    if (wait_for_request_) {
      wait_for_request_->Quit();
    }
    ASSERT_EQ(expected_quote_id_, quote_id);
    ASSERT_EQ(expected_quote_price_, quote_price);
    ASSERT_EQ(expected_total_fee_, total_fee);
    ASSERT_EQ(expected_total_amount_, total_amount);
  }

  void WaitForGetConvertQuote(const std::string& expected_quote_id,
      const std::string& expected_quote_price,
      const std::string& expected_total_fee,
      const std::string& expected_total_amount) {
    if (wait_for_request_) {
      return;
    }

    expected_quote_id_ = expected_quote_id;
    expected_quote_price_ = expected_quote_price;
    expected_total_fee_ = expected_total_fee;
    expected_total_amount_ = expected_total_amount;

    wait_for_request_.reset(new base::RunLoop);
    wait_for_request_->Run();
  }

  void OnGetAccountBalances(const BinanceAccountBalances& balances,
      bool success) {
    if (wait_for_request_) {
      wait_for_request_->Quit();
    }
    ASSERT_EQ(expected_balances_, balances);
    ASSERT_EQ(expected_success_, success);
  }

  void WaitForGetAccountBalances(
      const BinanceAccountBalances& expected_balances,
      bool expected_success) {
    if (wait_for_request_) {
      return;
    }
    expected_balances_ = expected_balances;
    expected_success_ = expected_success;
    wait_for_request_.reset(new base::RunLoop);
    wait_for_request_->Run();
  }

  void OnGetDepositInfo(const std::string& address, const std::string& tag,
      bool success) {
    if (wait_for_request_) {
      wait_for_request_->Quit();
    }
    ASSERT_EQ(expected_address_, address);
    ASSERT_EQ(expected_tag_, tag);
    ASSERT_EQ(expected_success_, success);
  }

  void WaitForGetDepositInfo(
      const std::string& expected_address, const std::string& expected_tag,
      bool expected_success) {
    if (wait_for_request_) {
      return;
    }
    expected_address_ = expected_address;
    expected_tag_ = expected_tag;
    expected_success_ = expected_success;
    wait_for_request_.reset(new base::RunLoop);
    wait_for_request_->Run();
  }

  void OnConfirmConvert(bool success, const std::string& error_message) {
    if (wait_for_request_) {
      wait_for_request_->Quit();
    }
    ASSERT_EQ(expected_success_, success);
    ASSERT_EQ(expected_error_message_, error_message);
  }

  void WaitForConfirmConvert(
      bool expected_success, const std::string& expected_error_message) {
    if (wait_for_request_) {
      return;
    }
    expected_success_ = expected_success;
    expected_error_message_ = expected_error_message;
    wait_for_request_.reset(new base::RunLoop);
    wait_for_request_->Run();
  }

  void OnGetConvertAssets(
      const BinanceConvertAsserts& assets) {
    if (wait_for_request_) {
      wait_for_request_->Quit();
    }
    ASSERT_EQ(expected_assets_with_sub_, assets);
  }

  void WaitForGetConvertAssets(
      const BinanceConvertAsserts& expected_assets) {
    if (wait_for_request_) {
      return;
    }
    expected_assets_with_sub_ = expected_assets;
    wait_for_request_.reset(new base::RunLoop);
    wait_for_request_->Run();
  }

  void OnRevokeToken(bool success) {
    if (wait_for_request_) {
      wait_for_request_->Quit();
    }
    ASSERT_EQ(expected_success_, success);
    if (success) {
      ASSERT_TRUE(browser()->profile()->GetPrefs()->GetString(
          kBinanceAccessToken).empty());
      ASSERT_TRUE(browser()->profile()->GetPrefs()->GetString(
          kBinanceRefreshToken).empty());
    }
  }

  void WaitForRevokeToken(bool success) {
    if (wait_for_request_) {
      return;
    }
    expected_success_ = success;
    wait_for_request_.reset(new base::RunLoop);
    wait_for_request_->Run();
  }

  void OnGetCoinNetworks(const BinanceCoinNetworks& networks) {
    if (wait_for_request_) {
      wait_for_request_->Quit();
    }
    ASSERT_EQ(expected_networks_, networks);
  }

  void WaitForGetCoinNetworks(
      const BinanceCoinNetworks& expected_networks) {
    if (wait_for_request_) {
      return;
    }
    expected_networks_ = expected_networks;
    wait_for_request_.reset(new base::RunLoop);
    wait_for_request_->Run();
  }

  content::WebContents* active_contents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  bool NavigateToNewTabUntilLoadStop() {
    ui_test_utils::NavigateToURL(browser(), GURL("chrome://newtab"));
    return WaitForLoadStop(active_contents());
  }

  bool NavigateToVersionTabUntilLoadStop() {
    ui_test_utils::NavigateToURL(browser(), GURL("chrome://version"));
    return WaitForLoadStop(active_contents());
  }

  BinanceService* GetBinanceService() {
    BinanceService* service = BinanceServiceFactory::GetInstance()
        ->GetForProfile(Profile::FromBrowserContext(browser()->profile()));
    EXPECT_TRUE(service);
    return service;
  }

 private:
  net::EmbeddedTestServer* https_server() { return https_server_.get(); }

  bool expected_success_;
  std::string expected_quote_id_;
  std::string expected_quote_price_;
  std::string expected_total_fee_;
  std::string expected_total_amount_;
  std::string expected_address_;
  std::string expected_tag_;
  std::string expected_error_message_;
  std::vector<std::string> expected_assets_;
  BinanceAccountBalances expected_balances_;
  BinanceCoinNetworks expected_networks_;
  BinanceConvertAsserts expected_assets_with_sub_;

  std::unique_ptr<base::RunLoop> wait_for_request_;
  std::unique_ptr<net::EmbeddedTestServer> https_server_;
};

IN_PROC_BROWSER_TEST_F(BinanceAPIBrowserTest, GetOAuthClientURL) {
  EXPECT_TRUE(NavigateToNewTabUntilLoadStop());
  auto* service = GetBinanceService();
  service->SetClientIdForTest("ultra-fake-id");

  GURL client_url(service->GetOAuthClientUrl());
  GURL expected_url(
    "https://accounts.binance.com/en/oauth/authorize?"
    "response_type=code&"
    "client_id=ultra-fake-id&"
    "redirect_uri=com.brave.binance%3A%2F%2Fauthorization&"
    "scope=user%3Aemail%2Cuser%3Aaddress%2Casset%3Abalance%2Casset%3Aocbs&"
    "code_challenge=da0KASk6XZX4ksgvIGAa87iwNSVvmWdys2GYh3kjBZw&"
    "code_challenge_method=S256&"
    "ref=39346846");
  // Replace the code_challenge since it is always different
  client_url = net::AppendOrReplaceQueryParameter(client_url, "code_challenge",
      "ultra-fake-id");
  expected_url = net::AppendOrReplaceQueryParameter(expected_url,
      "code_challenge", "ultra-fake-id");
  ASSERT_EQ(expected_url, client_url);
}

// Test disabled due to failure when run from a Powershell context
// TODO(ryanml): Fix test when running on Windows Powershell and remove guard
#if !defined(OS_WIN)
IN_PROC_BROWSER_TEST_F(BinanceAPIBrowserTest, GetAccessToken) {
  ResetHTTPSServer(base::BindRepeating(&HandleRequest));
  EXPECT_TRUE(NavigateToNewTabUntilLoadStop());
  auto* service = GetBinanceService();
  service->SetAuthToken("abc123");
  ASSERT_TRUE(service->GetAccessToken(
      base::BindOnce(
          &BinanceAPIBrowserTest::OnGetAccessToken,
          base::Unretained(this), true)));
  WaitForGetAccessToken(true);
}
#endif

IN_PROC_BROWSER_TEST_F(BinanceAPIBrowserTest, GetAccessTokenUnauthorized) {
  ResetHTTPSServer(base::BindRepeating(&HandleRequestUnauthorized));
  EXPECT_TRUE(NavigateToNewTabUntilLoadStop());
  auto* service = GetBinanceService();
  service->SetAuthToken("abc123");
  ASSERT_TRUE(service->GetAccessToken(
      base::BindOnce(
          &BinanceAPIBrowserTest::OnGetAccessToken,
          base::Unretained(this), false)));
  WaitForGetAccessToken(false);
}

IN_PROC_BROWSER_TEST_F(BinanceAPIBrowserTest, GetAccessTokenServerError) {
  ResetHTTPSServer(base::BindRepeating(&HandleRequestServerError));
  EXPECT_TRUE(NavigateToNewTabUntilLoadStop());
  auto* service = GetBinanceService();
  service->SetAuthToken("abc123");
  ASSERT_TRUE(service->GetAccessToken(
      base::BindOnce(
          &BinanceAPIBrowserTest::OnGetAccessToken,
          base::Unretained(this), false)));
  WaitForGetAccessToken(false);
}

IN_PROC_BROWSER_TEST_F(BinanceAPIBrowserTest, GetConvertQuote) {
  ResetHTTPSServer(base::BindRepeating(&HandleRequest));
  EXPECT_TRUE(NavigateToNewTabUntilLoadStop());
  auto* service = GetBinanceService();
  ASSERT_TRUE(service->GetConvertQuote("BTC", "ETH", "1",
      base::BindOnce(
          &BinanceAPIBrowserTest::OnGetConvertQuote,
          base::Unretained(this))));
  WaitForGetConvertQuote("b5481fb7f8314bb2baf55aa6d4fcf068",
      "1094.01086957", "8", "100649");
}

IN_PROC_BROWSER_TEST_F(BinanceAPIBrowserTest, GetConvertQuoteUnauthorized) {
  ResetHTTPSServer(base::BindRepeating(&HandleRequestUnauthorized));
  EXPECT_TRUE(NavigateToNewTabUntilLoadStop());
  auto* service = GetBinanceService();
  ASSERT_TRUE(service->GetConvertQuote("BTC", "ETH", "1",
      base::BindOnce(
          &BinanceAPIBrowserTest::OnGetConvertQuote,
          base::Unretained(this))));
  WaitForGetConvertQuote("", "", "", "");
}

IN_PROC_BROWSER_TEST_F(BinanceAPIBrowserTest, GetConvertQuoteServerError) {
  ResetHTTPSServer(base::BindRepeating(&HandleRequestServerError));
  EXPECT_TRUE(NavigateToNewTabUntilLoadStop());
  auto* service = GetBinanceService();
  ASSERT_TRUE(service->GetConvertQuote("BTC", "ETH", "1",
      base::BindOnce(
          &BinanceAPIBrowserTest::OnGetConvertQuote,
          base::Unretained(this))));
  WaitForGetConvertQuote("", "", "", "");
}

IN_PROC_BROWSER_TEST_F(BinanceAPIBrowserTest, GetAccountBalances) {
  ResetHTTPSServer(base::BindRepeating(&HandleRequest));
  EXPECT_TRUE(NavigateToNewTabUntilLoadStop());
  auto* service = GetBinanceService();
  ASSERT_TRUE(service->GetAccountBalances(
      base::BindOnce(
          &BinanceAPIBrowserTest::OnGetAccountBalances,
          base::Unretained(this))));
  WaitForGetAccountBalances(
      BinanceAccountBalances {
          {"BAT", {"1000.00000", "0.021100", "20000.00000"}}
      }, true);
}

IN_PROC_BROWSER_TEST_F(BinanceAPIBrowserTest, GetAccountBalancesUnauthorized) {
  ResetHTTPSServer(base::BindRepeating(&HandleRequestUnauthorized));
  EXPECT_TRUE(NavigateToNewTabUntilLoadStop());
  auto* service = GetBinanceService();
  ASSERT_TRUE(service->GetAccountBalances(
      base::BindOnce(
          &BinanceAPIBrowserTest::OnGetAccountBalances,
          base::Unretained(this))));
  WaitForGetAccountBalances(BinanceAccountBalances(), false);
}

IN_PROC_BROWSER_TEST_F(BinanceAPIBrowserTest, GetAccountBalancesServerError) {
  ResetHTTPSServer(base::BindRepeating(&HandleRequestServerError));
  EXPECT_TRUE(NavigateToNewTabUntilLoadStop());
  auto* service = GetBinanceService();
  ASSERT_TRUE(service->GetAccountBalances(
      base::BindOnce(
          &BinanceAPIBrowserTest::OnGetAccountBalances,
          base::Unretained(this))));
  WaitForGetAccountBalances(BinanceAccountBalances(), false);
}

IN_PROC_BROWSER_TEST_F(BinanceAPIBrowserTest, GetDepositInfo) {
  ResetHTTPSServer(base::BindRepeating(&HandleRequest));
  EXPECT_TRUE(NavigateToNewTabUntilLoadStop());
  auto* service = GetBinanceService();
  ASSERT_TRUE(service->GetDepositInfo("BTC", "BTC",
      base::BindOnce(
          &BinanceAPIBrowserTest::OnGetDepositInfo,
          base::Unretained(this))));
  std::string address = "112tfsHDk6Yk8PbNnTVkv7yPox4aWYYDtW";
  std::string tag = "";
  WaitForGetDepositInfo(address, tag, true);
}

IN_PROC_BROWSER_TEST_F(BinanceAPIBrowserTest, GetDepositInfoUnauthorized) {
  ResetHTTPSServer(base::BindRepeating(&HandleRequestUnauthorized));
  EXPECT_TRUE(NavigateToNewTabUntilLoadStop());
  auto* service = GetBinanceService();
  ASSERT_TRUE(service->GetDepositInfo("BTC", "BTC",
      base::BindOnce(
          &BinanceAPIBrowserTest::OnGetDepositInfo,
          base::Unretained(this))));
  WaitForGetDepositInfo("", "", false);
}

IN_PROC_BROWSER_TEST_F(BinanceAPIBrowserTest, GetDepositInfoServerError) {
  ResetHTTPSServer(base::BindRepeating(&HandleRequestServerError));
  EXPECT_TRUE(NavigateToNewTabUntilLoadStop());
  auto* service = GetBinanceService();
  ASSERT_TRUE(service->GetDepositInfo("BTC", "BTC",
      base::BindOnce(
          &BinanceAPIBrowserTest::OnGetDepositInfo,
          base::Unretained(this))));
  WaitForGetDepositInfo("", "", false);
}

IN_PROC_BROWSER_TEST_F(BinanceAPIBrowserTest, ConfirmConvert) {
  ResetHTTPSServer(base::BindRepeating(&HandleRequest));
  EXPECT_TRUE(NavigateToNewTabUntilLoadStop());
  auto* service = GetBinanceService();
  ASSERT_TRUE(service->ConfirmConvert("b5481fb7f8314bb2baf55aa6d4fcf068",
      base::BindOnce(
          &BinanceAPIBrowserTest::OnConfirmConvert,
          base::Unretained(this))));
  WaitForConfirmConvert(true, "");
}

IN_PROC_BROWSER_TEST_F(BinanceAPIBrowserTest, ConfirmConvertUnauthorized) {
  ResetHTTPSServer(base::BindRepeating(&HandleRequestUnauthorized));
  EXPECT_TRUE(NavigateToNewTabUntilLoadStop());
  auto* service = GetBinanceService();
  ASSERT_TRUE(service->ConfirmConvert("b5481fb7f8314bb2baf55aa6d4fcf068",
      base::BindOnce(
          &BinanceAPIBrowserTest::OnConfirmConvert,
          base::Unretained(this))));
  WaitForConfirmConvert(false, "");
}

IN_PROC_BROWSER_TEST_F(BinanceAPIBrowserTest, ConfirmConvertServerError) {
  ResetHTTPSServer(base::BindRepeating(&HandleRequestServerError));
  EXPECT_TRUE(NavigateToNewTabUntilLoadStop());
  auto* service = GetBinanceService();
  ASSERT_TRUE(service->ConfirmConvert("b5481fb7f8314bb2baf55aa6d4fcf068",
      base::BindOnce(
          &BinanceAPIBrowserTest::OnConfirmConvert,
          base::Unretained(this))));
  WaitForConfirmConvert(false, "");
}

IN_PROC_BROWSER_TEST_F(BinanceAPIBrowserTest, GetConvertAssets) {
  ResetHTTPSServer(base::BindRepeating(&HandleRequest));
  EXPECT_TRUE(NavigateToNewTabUntilLoadStop());
  auto* service = GetBinanceService();
  ASSERT_TRUE(service->GetConvertAssets(
      base::BindOnce(
          &BinanceAPIBrowserTest::OnGetConvertAssets,
          base::Unretained(this))));
  std::map<std::string, std::string> inner_sub {
    { "asset", "BNB", },
    { "minAmount", "0.00200000" }
  };
  std::vector<std::map<std::string, std::string>> sub {inner_sub};
  BinanceConvertAsserts assets {{"BTC", sub}};
  WaitForGetConvertAssets(assets);
}

IN_PROC_BROWSER_TEST_F(BinanceAPIBrowserTest, GetConvertAssetsUnauthorized) {
  ResetHTTPSServer(base::BindRepeating(&HandleRequestUnauthorized));
  EXPECT_TRUE(NavigateToNewTabUntilLoadStop());
  auto* service = GetBinanceService();
  ASSERT_TRUE(service->GetConvertAssets(
      base::BindOnce(
          &BinanceAPIBrowserTest::OnGetConvertAssets,
          base::Unretained(this))));
  WaitForGetConvertAssets(BinanceConvertAsserts());
}

IN_PROC_BROWSER_TEST_F(BinanceAPIBrowserTest, GetConvertAssetsServerError) {
  ResetHTTPSServer(base::BindRepeating(&HandleRequestServerError));
  EXPECT_TRUE(NavigateToNewTabUntilLoadStop());
  auto* service = GetBinanceService();
  ASSERT_TRUE(service->GetConvertAssets(
      base::BindOnce(
          &BinanceAPIBrowserTest::OnGetConvertAssets,
          base::Unretained(this))));
  WaitForGetConvertAssets(BinanceConvertAsserts());
}

// Test disabled due to failure when run from a Powershell context
// TODO(ryanml): Fix test when running on Windows Powershell and remove guard
#if !defined(OS_WIN)
IN_PROC_BROWSER_TEST_F(BinanceAPIBrowserTest, RevokeToken) {
  ResetHTTPSServer(base::BindRepeating(&HandleRequest));
  EXPECT_TRUE(NavigateToNewTabUntilLoadStop());
  auto* service = GetBinanceService();
  service->SetAuthToken("abc123");
  ASSERT_TRUE(service->GetAccessToken(
      base::BindOnce(
          &BinanceAPIBrowserTest::OnGetAccessToken,
          base::Unretained(this), true)));

  ASSERT_TRUE(service->RevokeToken(
      base::BindOnce(
          &BinanceAPIBrowserTest::OnRevokeToken,
          base::Unretained(this))));
  WaitForRevokeToken(true);
}
#endif

IN_PROC_BROWSER_TEST_F(BinanceAPIBrowserTest, RevokeTokenUnauthorized) {
  ResetHTTPSServer(base::BindRepeating(&HandleRequestUnauthorized));
  EXPECT_TRUE(NavigateToNewTabUntilLoadStop());
  auto* service = GetBinanceService();
  ASSERT_TRUE(service->RevokeToken(
      base::BindOnce(
          &BinanceAPIBrowserTest::OnRevokeToken,
          base::Unretained(this))));
  WaitForRevokeToken(false);
}

IN_PROC_BROWSER_TEST_F(BinanceAPIBrowserTest, RevokeTokenServerError) {
  ResetHTTPSServer(base::BindRepeating(&HandleRequestServerError));
  EXPECT_TRUE(NavigateToNewTabUntilLoadStop());
  auto* service = GetBinanceService();
  ASSERT_TRUE(service->RevokeToken(
      base::BindOnce(
          &BinanceAPIBrowserTest::OnRevokeToken,
          base::Unretained(this))));
  WaitForRevokeToken(false);
}

IN_PROC_BROWSER_TEST_F(BinanceAPIBrowserTest, GetBinanceTLD) {
  ResetHTTPSServer(base::BindRepeating(&HandleRequestServerError));
  EXPECT_TRUE(NavigateToNewTabUntilLoadStop());
  const std::string usCode = "US";
  const std::string canadaCode = "CA";
  const int32_t us_id = country_codes::CountryCharsToCountryID(
    usCode.at(0), usCode.at(1));
  const int32_t canada_id = country_codes::CountryCharsToCountryID(
    canadaCode.at(0), canadaCode.at(1));

  auto* service = GetBinanceService();
  browser()->profile()->GetPrefs()->SetInteger(
      country_codes::kCountryIDAtInstall, us_id);
  ASSERT_EQ(service->GetBinanceTLD(), "us");

  browser()->profile()->GetPrefs()->SetInteger(
      country_codes::kCountryIDAtInstall, canada_id);
  ASSERT_EQ(service->GetBinanceTLD(), "com");
}

IN_PROC_BROWSER_TEST_F(BinanceAPIBrowserTest, NewTabHasBinanceAPIAccess) {
  EXPECT_TRUE(NavigateToNewTabUntilLoadStop());
  bool result = false;
  EXPECT_TRUE(
      ExecuteScriptAndExtractBool(contents(), kBinanceAPIExistsScript,
        &result));
  ASSERT_TRUE(result);
}

IN_PROC_BROWSER_TEST_F(BinanceAPIBrowserTest,
    OtherChromeTabHasBinanceAPIAccess) {
  EXPECT_TRUE(NavigateToVersionTabUntilLoadStop());
  bool result = true;
  EXPECT_TRUE(
      ExecuteScriptAndExtractBool(contents(), kBinanceAPIExistsScript,
        &result));
  ASSERT_FALSE(result);
}

IN_PROC_BROWSER_TEST_F(BinanceAPIBrowserTest, GetCoinNetworks) {
  ResetHTTPSServer(base::BindRepeating(&HandleRequest));
  EXPECT_TRUE(NavigateToNewTabUntilLoadStop());
  auto* service = GetBinanceService();
  ASSERT_TRUE(service->GetCoinNetworks(
      base::BindOnce(
          &BinanceAPIBrowserTest::OnGetCoinNetworks,
          base::Unretained(this))));
  WaitForGetCoinNetworks(
      BinanceCoinNetworks {
          {"BAT", "ETH"},
          {"GAS", "NEO"}
      });
}

IN_PROC_BROWSER_TEST_F(BinanceAPIBrowserTest, GetCoinNetworksUnauthorized) {
  ResetHTTPSServer(base::BindRepeating(&HandleRequestUnauthorized));
  EXPECT_TRUE(NavigateToNewTabUntilLoadStop());
  auto* service = GetBinanceService();
  ASSERT_TRUE(service->GetCoinNetworks(
      base::BindOnce(
          &BinanceAPIBrowserTest::OnGetCoinNetworks,
          base::Unretained(this))));
  WaitForGetCoinNetworks(BinanceCoinNetworks());
}

IN_PROC_BROWSER_TEST_F(BinanceAPIBrowserTest, GetCoinNetworksServerError) {
  ResetHTTPSServer(base::BindRepeating(&HandleRequestServerError));
  EXPECT_TRUE(NavigateToNewTabUntilLoadStop());
  auto* service = GetBinanceService();
  ASSERT_TRUE(service->GetCoinNetworks(
      base::BindOnce(
          &BinanceAPIBrowserTest::OnGetCoinNetworks,
          base::Unretained(this))));
  WaitForGetCoinNetworks(BinanceCoinNetworks());
}
