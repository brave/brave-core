/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/json/json_reader.h"
#include "base/path_service.h"
#include "brave/browser/crypto_dot_com/crypto_dot_com_service_factory.h"
#include "brave/common/brave_paths.h"
#include "brave/components/crypto_dot_com/browser/crypto_dot_com_service.h"
#include "brave/components/crypto_dot_com/common/constants.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "net/base/url_util.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/http_request.h"
#include "net/test/embedded_test_server/http_response.h"

// npm run test -- brave_browser_tests --filter=CryptoDotComAPIBrowserTest.*

namespace {

std::unique_ptr<net::test_server::HttpResponse> HandleRequest(
    const net::test_server::HttpRequest& request) {
  std::unique_ptr<net::test_server::BasicHttpResponse> http_response(
      new net::test_server::BasicHttpResponse());
  http_response->set_code(net::HTTP_OK);
  http_response->set_content_type("text/html");

  std::string request_path = request.GetURL().path();
  std::string test_ticker_info_path =
      std::string(get_ticker_info_path) + "?instrument_name=BTC_USDT";
  std::string test_chart_data_path =
      std::string(get_chart_data_path) +
      "?instrument_name=BTC_USDT&timeframe=4h&depth=42";

  if (request_path == test_ticker_info_path) {
    http_response->set_content(R"({
        "code": 0,
        "method": "public/get-ticker",
        "result": {
            "instrument_name": "BTC_USDT",
            "data": {
                "i": "BTC_USDT",
                "b": 11760.03,
                "k": 11762.97,
                "a": 11759.2,
                "t": 1598254503038,
                "v": 786.863035,
                "h": 11773.98,
                "l": 11520.55,
                "c": 148.95
            }
        }
    })");
  } else if (request_path == test_chart_data_path) {
    http_response->set_content(R"({
        "code": 0,
        "method": "public/get-candlestick",
        "result": {
            "instrument_name": "BTC_USDT",
            "depth": 1,
            "interval": "1D",
            "data": [
                {
                    "t": 1598227200000,
                    "o": 11646.9,
                    "h": 11792.51,
                    "l": 11594.55,
                    "c": 11787.25,
                    "v": 228.290252
                },
                {
                    "t": 16982337200000,
                    "o": 12646.9,
                    "h": 13882.51,
                    "l": 14734.55,
                    "c": 15787.25,
                    "v": 268.290252
                }
            ]
        }
    })");
  } else if (request_path == get_pairs_path) {
    http_response->set_content(R"({
        "code": 0,
        "method": "public/get-instruments",
        "result": {
            "instruments": [
                {
                    "instrument_name": "NEO_BTC",
                    "quote_currency": "BTC",
                    "base_currency": "NEO",
                    "price_decimals": 6,
                    "quantity_decimals": 3
                },
                {
                    "instrument_name": "ETH_BTC",
                    "quote_currency": "BTC",
                    "base_currency": "ETH",
                    "price_decimals": 6,
                    "quantity_decimals": 3
                }
            ]
        }
    })");
  } else if (request_path == get_gainers_losers_path) {
    http_response->set_content(R"({
        "code": 0,
        "result": {
            "gainers": [
                {
                    "currency": "BTC",
                    "currency_name": "Bitcoin",
                    "instrument_name": "BTC_USDT",
                    "image_url": "",
                    "last_price": "10000.00",
                    "percent_change": "50.11"
                },
                {
                    "currency": "XRP",
                    "currency_name": "XRP",
                    "instrument_name": "XRP_USDT",
                    "image_url": "",
                    "last_price": "0.10",
                    "percent_change": "-20.12"
                }
            ]
        }
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

const char kCryptoDotComAPIExistsScript[] =
    "window.domAutomationController.send(!!chrome.cryptoDotCom)";

}  // namespace

class CryptoDotComAPIBrowserTest : public InProcessBrowserTest {
 public:
  CryptoDotComAPIBrowserTest() {}

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    host_resolver()->AddRule("*", "127.0.0.1");

    ResetHTTPSServer(base::BindRepeating(&HandleRequest));

    brave::RegisterPathProvider();
    base::FilePath test_data_dir;
    base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir);
  }

  ~CryptoDotComAPIBrowserTest() override {
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
  }

  void OnTickerInfo(const CryptoDotComTickerInfo& info) {
    if (wait_for_request_) {
      wait_for_request_->Quit();
    }
    ASSERT_EQ(expected_ticker_info_, info);
  }

  void WaitForGetTickerInfo(const CryptoDotComTickerInfo& info) {
    if (wait_for_request_) {
      return;
    }

    expected_ticker_info_ = info;

    wait_for_request_.reset(new base::RunLoop);
    wait_for_request_->Run();
  }

  void OnChartData(const CryptoDotComChartData& data) {
    if (wait_for_request_) {
      wait_for_request_->Quit();
    }
    ASSERT_EQ(expected_chart_data_, data);
  }

  void WaitForGetChartData(const CryptoDotComChartData& data) {
    if (wait_for_request_) {
      return;
    }

    expected_chart_data_ = data;

    wait_for_request_.reset(new base::RunLoop);
    wait_for_request_->Run();
  }

  void OnSupportedPairs(const CryptoDotComSupportedPairs& pairs) {
    if (wait_for_request_) {
      wait_for_request_->Quit();
    }
    ASSERT_EQ(expected_pairs_, pairs);
  }

  void WaitForGetSupportedPairs(const CryptoDotComSupportedPairs& pairs) {
    if (wait_for_request_) {
      return;
    }

    expected_pairs_ = pairs;

    wait_for_request_.reset(new base::RunLoop);
    wait_for_request_->Run();
  }

  void OnAssetRankings(const CryptoDotComAssetRankings& rankings) {
    if (wait_for_request_) {
      wait_for_request_->Quit();
    }
    ASSERT_EQ(expected_rankings_, rankings);
  }

  void WaitForGetAssetRankings(const CryptoDotComAssetRankings& rankings) {
    if (wait_for_request_) {
      return;
    }

    expected_rankings_ = rankings;

    wait_for_request_.reset(new base::RunLoop);
    wait_for_request_->Run();
  }

  void OnGetValue(base::Value value) {
    if (wait_for_request_) {
      wait_for_request_->Quit();
    }

    EXPECT_EQ(expected_value_, value);
  }

  void WaitForValueResponse(base::Value expected_value) {
    if (wait_for_request_) {
      return;
    }

    expected_value_ = std::move(expected_value);

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

  CryptoDotComService* GetCryptoDotComService() {
    CryptoDotComService* service = CryptoDotComServiceFactory::GetInstance()
        ->GetForProfile(Profile::FromBrowserContext(browser()->profile()));
    EXPECT_TRUE(service);
    return service;
  }

  CryptoDotComTickerInfo GetEmptyTickerInfo() {
    CryptoDotComTickerInfo empty_info;
    return empty_info;
  }

  CryptoDotComChartData GetEmptyChartData() {
    CryptoDotComChartData empty_data;
    const std::map<std::string, double> empty_data_point = {
        {"t", 0}, {"o", 0}, {"h", 0}, {"l", 0}, {"c", 0}, {"v", 0}};
    empty_data.push_back(empty_data_point);
    return empty_data;
  }

  CryptoDotComSupportedPairs GetEmptyPairs() {
    CryptoDotComSupportedPairs empty_pairs;
    const std::map<std::string, std::string> empty_pair = {{"pair", ""},
                                                           {"quote", ""},
                                                           {"base", ""},
                                                           {"price", ""},
                                                           {"quantity", ""}};
    empty_pairs.push_back(empty_pair);
    return empty_pairs;
  }

  CryptoDotComAssetRankings GetEmptyRankings() {
    CryptoDotComAssetRankings empty_rankings;
    std::vector<std::map<std::string, std::string>> gainers;
    std::vector<std::map<std::string, std::string>> losers;
    empty_rankings.insert({"gainers", gainers});
    empty_rankings.insert({"losers", losers});
    return empty_rankings;
  }

 private:
  net::EmbeddedTestServer* https_server() { return https_server_.get(); }

  CryptoDotComTickerInfo expected_ticker_info_;
  CryptoDotComChartData expected_chart_data_;
  CryptoDotComSupportedPairs expected_pairs_;
  CryptoDotComAssetRankings expected_rankings_;
  base::Value expected_value_;

  std::unique_ptr<base::RunLoop> wait_for_request_;
  std::unique_ptr<net::EmbeddedTestServer> https_server_;
};


IN_PROC_BROWSER_TEST_F(CryptoDotComAPIBrowserTest, GetTickerInfo) {
  ResetHTTPSServer(base::BindRepeating(&HandleRequest));
  EXPECT_TRUE(NavigateToNewTabUntilLoadStop());
  auto* service = GetCryptoDotComService();
  ASSERT_TRUE(service->GetTickerInfo(
      "BTC_USDT",
      base::BindOnce(
          &CryptoDotComAPIBrowserTest::OnTickerInfo,
          base::Unretained(this))));
  WaitForGetTickerInfo(GetEmptyTickerInfo());
}

IN_PROC_BROWSER_TEST_F(CryptoDotComAPIBrowserTest, GetTickerInfoUnauthorized) {
  ResetHTTPSServer(base::BindRepeating(&HandleRequestUnauthorized));
  EXPECT_TRUE(NavigateToNewTabUntilLoadStop());
  auto* service = GetCryptoDotComService();
  ASSERT_TRUE(service->GetTickerInfo(
      "BTC_USDT",
      base::BindOnce(
          &CryptoDotComAPIBrowserTest::OnTickerInfo,
          base::Unretained(this))));
  WaitForGetTickerInfo(GetEmptyTickerInfo());
}

IN_PROC_BROWSER_TEST_F(CryptoDotComAPIBrowserTest, GetTickerInfoServerError) {
  ResetHTTPSServer(base::BindRepeating(&HandleRequestServerError));
  EXPECT_TRUE(NavigateToNewTabUntilLoadStop());
  auto* service = GetCryptoDotComService();
  ASSERT_TRUE(service->GetTickerInfo(
      "BTC_USDT",
      base::BindOnce(
          &CryptoDotComAPIBrowserTest::OnTickerInfo,
          base::Unretained(this))));
  WaitForGetTickerInfo(GetEmptyTickerInfo());
}

IN_PROC_BROWSER_TEST_F(CryptoDotComAPIBrowserTest, GetChartData) {
  ResetHTTPSServer(base::BindRepeating(&HandleRequest));
  EXPECT_TRUE(NavigateToNewTabUntilLoadStop());
  auto* service = GetCryptoDotComService();
  ASSERT_TRUE(service->GetChartData(
      "BTC_USDT",
      base::BindOnce(
          &CryptoDotComAPIBrowserTest::OnChartData,
          base::Unretained(this))));
  WaitForGetChartData(GetEmptyChartData());
}

IN_PROC_BROWSER_TEST_F(CryptoDotComAPIBrowserTest, GetChartDataUnauthorized) {
  ResetHTTPSServer(base::BindRepeating(&HandleRequestUnauthorized));
  EXPECT_TRUE(NavigateToNewTabUntilLoadStop());
  auto* service = GetCryptoDotComService();
  ASSERT_TRUE(service->GetChartData(
      "BTC_USDT",
      base::BindOnce(
          &CryptoDotComAPIBrowserTest::OnChartData,
          base::Unretained(this))));
  WaitForGetChartData(GetEmptyChartData());
}

IN_PROC_BROWSER_TEST_F(CryptoDotComAPIBrowserTest, GetChartDataServerError) {
  ResetHTTPSServer(base::BindRepeating(&HandleRequestServerError));
  EXPECT_TRUE(NavigateToNewTabUntilLoadStop());
  auto* service = GetCryptoDotComService();
  ASSERT_TRUE(service->GetChartData(
      "BTC_USDT",
      base::BindOnce(
          &CryptoDotComAPIBrowserTest::OnChartData,
          base::Unretained(this))));
  WaitForGetChartData(GetEmptyChartData());
}

IN_PROC_BROWSER_TEST_F(CryptoDotComAPIBrowserTest, GetSupportedPairs) {
  ResetHTTPSServer(base::BindRepeating(&HandleRequest));
  EXPECT_TRUE(NavigateToNewTabUntilLoadStop());
  auto* service = GetCryptoDotComService();
  ASSERT_TRUE(service->GetSupportedPairs(
      base::BindOnce(
          &CryptoDotComAPIBrowserTest::OnSupportedPairs,
          base::Unretained(this))));
  WaitForGetSupportedPairs(GetEmptyPairs());
}

IN_PROC_BROWSER_TEST_F(CryptoDotComAPIBrowserTest,
    GetSupportedPairsUnauthorized) {
  ResetHTTPSServer(base::BindRepeating(&HandleRequestUnauthorized));
  EXPECT_TRUE(NavigateToNewTabUntilLoadStop());
  auto* service = GetCryptoDotComService();
  ASSERT_TRUE(service->GetSupportedPairs(
      base::BindOnce(
          &CryptoDotComAPIBrowserTest::OnSupportedPairs,
          base::Unretained(this))));
  WaitForGetSupportedPairs(GetEmptyPairs());
}

IN_PROC_BROWSER_TEST_F(CryptoDotComAPIBrowserTest,
    GetSupportedPairsServerError) {
  ResetHTTPSServer(base::BindRepeating(&HandleRequestServerError));
  EXPECT_TRUE(NavigateToNewTabUntilLoadStop());
  auto* service = GetCryptoDotComService();
  ASSERT_TRUE(service->GetSupportedPairs(
      base::BindOnce(
          &CryptoDotComAPIBrowserTest::OnSupportedPairs,
          base::Unretained(this))));
  WaitForGetSupportedPairs(GetEmptyPairs());
}

IN_PROC_BROWSER_TEST_F(CryptoDotComAPIBrowserTest, GetAssetRankings) {
  ResetHTTPSServer(base::BindRepeating(&HandleRequest));
  EXPECT_TRUE(NavigateToNewTabUntilLoadStop());
  auto* service = GetCryptoDotComService();
  ASSERT_TRUE(service->GetAssetRankings(
      base::BindOnce(
          &CryptoDotComAPIBrowserTest::OnAssetRankings,
          base::Unretained(this))));
  WaitForGetAssetRankings(GetEmptyRankings());
}

IN_PROC_BROWSER_TEST_F(CryptoDotComAPIBrowserTest,
    GetAssetRankingsUnauthorized) {
  ResetHTTPSServer(base::BindRepeating(&HandleRequestUnauthorized));
  EXPECT_TRUE(NavigateToNewTabUntilLoadStop());
  auto* service = GetCryptoDotComService();
  ASSERT_TRUE(service->GetAssetRankings(
      base::BindOnce(
          &CryptoDotComAPIBrowserTest::OnAssetRankings,
          base::Unretained(this))));
  WaitForGetAssetRankings(GetEmptyRankings());
}

IN_PROC_BROWSER_TEST_F(CryptoDotComAPIBrowserTest,
    GetAssetRankingsServerError) {
  ResetHTTPSServer(base::BindRepeating(&HandleRequestServerError));
  EXPECT_TRUE(NavigateToNewTabUntilLoadStop());
  auto* service = GetCryptoDotComService();
  ASSERT_TRUE(service->GetAssetRankings(
      base::BindOnce(
          &CryptoDotComAPIBrowserTest::OnAssetRankings,
          base::Unretained(this))));
  WaitForGetAssetRankings(GetEmptyRankings());
}

IN_PROC_BROWSER_TEST_F(CryptoDotComAPIBrowserTest,
                       GetAccountBalanceServerError) {
  ResetHTTPSServer(base::BindRepeating(&HandleRequestServerError));
  EXPECT_TRUE(NavigateToNewTabUntilLoadStop());
  auto* service = GetCryptoDotComService();
  ASSERT_TRUE(service->GetAccountBalances(base::BindOnce(
      &CryptoDotComAPIBrowserTest::OnGetValue, base::Unretained(this))));
  auto expected_response = base::JSONReader::Read(kEmptyAccountBalances);
  WaitForValueResponse(std::move(*expected_response));
}

IN_PROC_BROWSER_TEST_F(CryptoDotComAPIBrowserTest, GetNewsEventsServerError) {
  ResetHTTPSServer(base::BindRepeating(&HandleRequestServerError));
  EXPECT_TRUE(NavigateToNewTabUntilLoadStop());
  auto* service = GetCryptoDotComService();
  ASSERT_TRUE(service->GetNewsEvents(base::BindOnce(
      &CryptoDotComAPIBrowserTest::OnGetValue, base::Unretained(this))));
  auto expected_response = base::JSONReader::Read(kEmptyNewsEvents);
  WaitForValueResponse(expected_response->FindListKey("events")->Clone());
}

IN_PROC_BROWSER_TEST_F(CryptoDotComAPIBrowserTest,
                       GetDepositAddressServerError) {
  ResetHTTPSServer(base::BindRepeating(&HandleRequestServerError));
  EXPECT_TRUE(NavigateToNewTabUntilLoadStop());
  auto* service = GetCryptoDotComService();
  ASSERT_TRUE(service->GetDepositAddress(
      "BAT", base::BindOnce(&CryptoDotComAPIBrowserTest::OnGetValue,
                            base::Unretained(this))));
  auto expected_response = base::JSONReader::Read(kEmptyDepositAddress);
  WaitForValueResponse(std::move(*expected_response));
}

IN_PROC_BROWSER_TEST_F(CryptoDotComAPIBrowserTest,
    NewTabHasCryptoDotComAPIAccess) {
  EXPECT_TRUE(NavigateToNewTabUntilLoadStop());
  bool result = false;
  EXPECT_TRUE(
      ExecuteScriptAndExtractBool(contents(), kCryptoDotComAPIExistsScript,
        &result));
  ASSERT_TRUE(result);
}

IN_PROC_BROWSER_TEST_F(CryptoDotComAPIBrowserTest,
    OtherChromeTabHasCryptoDotComAPIAccess) {
  EXPECT_TRUE(NavigateToVersionTabUntilLoadStop());
  bool result = true;
  EXPECT_TRUE(
      ExecuteScriptAndExtractBool(contents(), kCryptoDotComAPIExistsScript,
        &result));
  ASSERT_FALSE(result);
}
