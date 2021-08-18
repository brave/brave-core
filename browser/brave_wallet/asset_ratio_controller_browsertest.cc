/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_wallet/asset_ratio_controller_factory.h"
#include "brave/components/brave_wallet/browser/asset_ratio_controller.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
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

// npm run test -- brave_browser_tests --filter=AssetRatioControllerTest.*

using brave_wallet::mojom::AssetTimePrice;

namespace {

std::unique_ptr<net::test_server::HttpResponse> HandleRequest(
    const net::test_server::HttpRequest& request) {
  std::unique_ptr<net::test_server::BasicHttpResponse> http_response(
      new net::test_server::BasicHttpResponse());
  http_response->set_code(net::HTTP_OK);
  http_response->set_content_type("text/html");
  if (request.GetURL().spec().find("/v2/history") != std::string::npos) {
    http_response->set_content(R"({
      "payload": {
        "prices":[[1622733088498,0.8201346624954003],[1622737203757,0.8096978545029869]],
        "market_caps":[[1622733088498,1223507820.383275],[1622737203757,1210972881.4928021]],
        "total_volumes":[[1622733088498,163426828.00299588],[1622737203757,157618689.0971025]]
      }
    })");
  } else {
    http_response->set_content(
        R"({
         "payload":{
           "basic-attention-token":{
             "btc":0.00001732,
             "btc_timeframe_change":8.021672460190562,
             "usd":0.55393,
             "usd_timeframe_change":9.523443444373276
           },
           "bat":{
             "btc":0.00001732,
             "btc_timeframe_change":8.021672460190562,
             "usd":0.55393,
             "usd_timeframe_change":9.523443444373276
           },
           "link":{
             "btc":0.00261901,
             "btc_timeframe_change":0.5871625385632929,
             "usd":83.77,
             "usd_timeframe_change":1.7646208048244043
           }
         },
         "lastUpdated":"2021-07-16T19:11:28.907Z"
       })");
  }
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

}  // namespace

class AssetRatioControllerTest : public InProcessBrowserTest {
 public:
  AssetRatioControllerTest() : expected_success_(false) {}

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    host_resolver()->AddRule("*", "127.0.0.1");
    ResetHTTPSServer(base::BindRepeating(&HandleRequest));
  }

  ~AssetRatioControllerTest() override {}

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
    brave_wallet::AssetRatioController::SetBaseURLForTest(
        https_server_->base_url());
  }

  void OnGetPrice(bool success,
                  std::vector<brave_wallet::mojom::AssetPricePtr> prices) {
    if (wait_for_request_) {
      wait_for_request_->Quit();
    }
    ASSERT_EQ(expected_prices_response_, prices);
    ASSERT_EQ(expected_success_, success);
  }

  void OnGetPriceHistory(bool success,
                         std::vector<brave_wallet::mojom::AssetTimePricePtr>
                             price_history_response) {
    if (wait_for_request_) {
      wait_for_request_->Quit();
    }
    ASSERT_EQ(expected_price_history_response_, price_history_response);
    ASSERT_EQ(expected_success_, success);
  }

  void WaitForPriceResponse(
      std::vector<brave_wallet::mojom::AssetPricePtr> expected_prices_response,
      bool expected_success) {
    if (wait_for_request_) {
      return;
    }
    expected_prices_response_ = std::move(expected_prices_response);
    expected_success_ = expected_success;
    wait_for_request_.reset(new base::RunLoop);
    wait_for_request_->Run();
  }

  void WaitForPriceHistoryResponse(
      std::vector<brave_wallet::mojom::AssetTimePricePtr>
          expected_price_history_response,
      bool expected_success) {
    if (wait_for_request_) {
      return;
    }
    expected_price_history_response_ =
        std::move(expected_price_history_response);
    expected_success_ = expected_success;
    wait_for_request_.reset(new base::RunLoop);
    wait_for_request_->Run();
  }

  content::WebContents* active_contents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  mojo::Remote<brave_wallet::mojom::AssetRatioController>
  GetAssetRatioController() {
    auto pending =
        brave_wallet::AssetRatioControllerFactory::GetInstance()->GetForContext(
            browser()->profile());
    mojo::Remote<brave_wallet::mojom::AssetRatioController>
        asset_ratio_controller;
    asset_ratio_controller.Bind(std::move(pending));
    return asset_ratio_controller;
  }

 private:
  net::EmbeddedTestServer* https_server() { return https_server_.get(); }

  bool expected_success_;
  std::vector<brave_wallet::mojom::AssetPricePtr> expected_prices_response_;
  std::vector<brave_wallet::mojom::AssetTimePricePtr>
      expected_price_history_response_;

  std::unique_ptr<base::RunLoop> wait_for_request_;
  std::unique_ptr<net::EmbeddedTestServer> https_server_;
};

IN_PROC_BROWSER_TEST_F(AssetRatioControllerTest, GetPrice) {
  std::vector<brave_wallet::mojom::AssetPricePtr> expected_prices_response;
  ResetHTTPSServer(base::BindRepeating(&HandleRequest));
  auto controller = GetAssetRatioController();
  controller->GetPrice({"bat", "link"}, {"btc", "usd"},
                       brave_wallet::mojom::AssetPriceTimeframe::OneDay,
                       base::BindOnce(&AssetRatioControllerTest::OnGetPrice,
                                      base::Unretained(this)));

  auto asset_price = brave_wallet::mojom::AssetPrice::New();
  asset_price->from_asset = "bat";
  asset_price->to_asset = "btc";
  asset_price->price = "0.00001732";
  asset_price->asset_timeframe_change = "8.021672460190562";
  expected_prices_response.push_back(std::move(asset_price));

  asset_price = brave_wallet::mojom::AssetPrice::New();
  asset_price->from_asset = "bat";
  asset_price->to_asset = "usd";
  asset_price->price = "0.55393";
  asset_price->asset_timeframe_change = "9.523443444373276";
  expected_prices_response.push_back(std::move(asset_price));

  asset_price = brave_wallet::mojom::AssetPrice::New();
  asset_price->from_asset = "link";
  asset_price->to_asset = "btc";
  asset_price->price = "0.00261901";
  asset_price->asset_timeframe_change = "0.5871625385632929";
  expected_prices_response.push_back(std::move(asset_price));

  asset_price = brave_wallet::mojom::AssetPrice::New();
  asset_price->from_asset = "link";
  asset_price->to_asset = "usd";
  asset_price->price = "83.77";
  asset_price->asset_timeframe_change = "1.7646208048244043";
  expected_prices_response.push_back(std::move(asset_price));

  WaitForPriceResponse(std::move(expected_prices_response), true);
}

IN_PROC_BROWSER_TEST_F(AssetRatioControllerTest, GetPriceServerError) {
  std::vector<brave_wallet::mojom::AssetPricePtr> expected_prices_response;
  ResetHTTPSServer(base::BindRepeating(&HandleRequestServerError));
  auto controller = GetAssetRatioController();
  controller->GetPrice({"bat", "link"}, {"btc", "usd"},
                       brave_wallet::mojom::AssetPriceTimeframe::OneDay,
                       base::BindOnce(&AssetRatioControllerTest::OnGetPrice,
                                      base::Unretained(this)));
  WaitForPriceResponse(std::move(expected_prices_response), false);
}

IN_PROC_BROWSER_TEST_F(AssetRatioControllerTest, GetPriceHistory) {
  ResetHTTPSServer(base::BindRepeating(&HandleRequest));
  auto controller = GetAssetRatioController();
  controller->GetPriceHistory(
      "bat", brave_wallet::mojom::AssetPriceTimeframe::OneDay,
      base::BindOnce(&AssetRatioControllerTest::OnGetPriceHistory,
                     base::Unretained(this)));

  std::vector<brave_wallet::mojom::AssetTimePricePtr>
      expected_price_history_response;

  auto asset_time_price = brave_wallet::mojom::AssetTimePrice::New();
  asset_time_price->date = base::TimeDelta::FromMilliseconds(1622733088498);
  asset_time_price->price = "0.8201346624954003";
  expected_price_history_response.push_back(std::move(asset_time_price));

  asset_time_price = brave_wallet::mojom::AssetTimePrice::New();
  asset_time_price->date = base::TimeDelta::FromMilliseconds(1622737203757);
  asset_time_price->price = "0.8096978545029869";
  expected_price_history_response.push_back(std::move(asset_time_price));

  WaitForPriceHistoryResponse(std::move(expected_price_history_response), true);
}

IN_PROC_BROWSER_TEST_F(AssetRatioControllerTest, GetPriceHistoryServerError) {
  ResetHTTPSServer(base::BindRepeating(&HandleRequestServerError));
  auto controller = GetAssetRatioController();
  controller->GetPriceHistory(
      "bat", brave_wallet::mojom::AssetPriceTimeframe::OneDay,
      base::BindOnce(&AssetRatioControllerTest::OnGetPriceHistory,
                     base::Unretained(this)));
  std::vector<brave_wallet::mojom::AssetTimePricePtr>
      expected_price_history_response;
  WaitForPriceHistoryResponse(std::move(expected_price_history_response),
                              false);
}
