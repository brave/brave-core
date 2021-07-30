/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_wallet/swap_controller_factory.h"
#include "brave/components/brave_wallet/browser/swap_controller.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "net/base/url_util.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/http_request.h"
#include "net/test/embedded_test_server/http_response.h"

// npm run test -- brave_browser_tests --filter=SwapControllerTest.*

namespace {

std::unique_ptr<net::test_server::HttpResponse> HandleRequest(
    const net::test_server::HttpRequest& request) {
  std::unique_ptr<net::test_server::BasicHttpResponse> http_response(
      new net::test_server::BasicHttpResponse());
  http_response->set_code(net::HTTP_OK);
  http_response->set_content_type("text/html");
  // https://127.0.0.1:62561/swap/v1/price?buyAmount=1000000000000000000000&buyToken=ETH&sellToken=DAI&buyTokenPercentageFee=0.000000&slippagePercentage=0.000000
  if (request.GetURL().spec().find("swap/v1/quote") != std::string::npos) {
    http_response->set_content(R"(
      {
        "price":"1916.27547998814058355",
        "guaranteedPrice":"1935.438234788021989386",
        "to":"0xdef1c0ded9bec7f1a1670819833240f027b25eff",
        "data":"0x0",
        "value":"0",
        "gas":"719000",
        "estimatedGas":"719000",
        "gasPrice":"26000000000",
        "protocolFee":"0",
        "minimumProtocolFee":"0",
        "buyTokenAddress":"0xeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee",
        "sellTokenAddress":"0x6b175474e89094c44da98b954eedeac495271d0f",
        "buyAmount":"1000000000000000000000",
        "sellAmount":"1916275479988140583549706",
        "sources":[],
        "allowanceTarget":"0xdef1c0ded9bec7f1a1670819833240f027b25eff",
        "sellTokenToEthRate":"1900.44962824532464391",
        "buyTokenToEthRate":"1"
      }
    )");
  } else if (request.GetURL().spec().find("swap/v1/price") !=
             std::string::npos) {
    http_response->set_content(R"(
      {
        "price":"1916.27547998814058355",
        "value":"0",
        "gas":"719000",
        "estimatedGas":"719000",
        "gasPrice":"26000000000",
        "protocolFee":"0",
        "minimumProtocolFee":"0",
        "buyTokenAddress":"0xeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee",
        "sellTokenAddress":"0x6b175474e89094c44da98b954eedeac495271d0f",
        "buyAmount":"1000000000000000000000",
        "sellAmount":"1916275479988140583549706",
        "sources":[],
        "allowanceTarget":"0xdef1c0ded9bec7f1a1670819833240f027b25eff",
        "sellTokenToEthRate":"1900.44962824532464391",
        "buyTokenToEthRate":"1"
      }
    )");
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

brave_wallet::mojom::SwapParamsPtr GetCannedSwapParams() {
  auto params = brave_wallet::mojom::SwapParams::New();
  params->buy_token = "ETH";
  params->sell_token = "DAI";
  params->buy_amount = "1000000000000000000000";
  return params;
}  // namespace

class SwapControllerTest : public InProcessBrowserTest {
 public:
  SwapControllerTest() : expected_success_(false) {}

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    host_resolver()->AddRule("*", "127.0.0.1");
    ResetHTTPSServer(base::BindRepeating(&HandleRequest));
  }

  ~SwapControllerTest() override {}

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
    brave_wallet::SwapController::SetBaseURLForTest(https_server_->base_url());
  }

  void OnGetPriceQuoteResponse(
      bool success,
      brave_wallet::mojom::SwapResponsePtr swap_response) {
    if (wait_for_request_) {
      wait_for_request_->Quit();
    }

    ASSERT_EQ(expected_success_, success);
    if (success) {
      // These are mainly for debugging purposes if something fails
      // with the swap response comparison it just gives you both byte arrays
      ASSERT_EQ(swap_response->price, "1916.27547998814058355");
      ASSERT_EQ(swap_response->guaranteed_price, "");
      ASSERT_EQ(swap_response->to, "");
      ASSERT_EQ(swap_response->data, "");
      ASSERT_EQ(swap_response->value, "0");
      ASSERT_EQ(swap_response->gas, "719000");
      ASSERT_EQ(swap_response->estimated_gas, "719000");
      ASSERT_EQ(swap_response->gas_price, "26000000000");
      ASSERT_EQ(swap_response->protocol_fee, "0");
      ASSERT_EQ(swap_response->minimum_protocol_fee, "0");
      ASSERT_EQ(swap_response->buy_token_address,
                "0xeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee");
      ASSERT_EQ(swap_response->sell_token_address,
                "0x6b175474e89094c44da98b954eedeac495271d0f");
      ASSERT_EQ(swap_response->buy_amount, "1000000000000000000000");
      ASSERT_EQ(swap_response->sell_amount, "1916275479988140583549706");
      ASSERT_EQ(swap_response->allowance_target,
                "0xdef1c0ded9bec7f1a1670819833240f027b25eff");
      ASSERT_EQ(swap_response->sell_token_to_eth_rate,
                "1900.44962824532464391");
      ASSERT_EQ(swap_response->buy_token_to_eth_rate, "1");
    }
    ASSERT_EQ(expected_swap_response_, *swap_response);
  }

  void OnGetTransactionPayloadResponse(
      bool success,
      brave_wallet::mojom::SwapResponsePtr swap_response) {
    if (wait_for_request_) {
      wait_for_request_->Quit();
    }

    ASSERT_EQ(expected_success_, success);
    if (success) {
      // These are mainly for debugging purposes if something fails
      // with the swap response comparison it just gives you both byte arrays
      ASSERT_EQ(swap_response->price, "1916.27547998814058355");
      ASSERT_EQ(swap_response->guaranteed_price, "1935.438234788021989386");
      ASSERT_EQ(swap_response->to,
                "0xdef1c0ded9bec7f1a1670819833240f027b25eff");
      ASSERT_EQ(swap_response->data, "0x0");
      ASSERT_EQ(swap_response->value, "0");
      ASSERT_EQ(swap_response->gas, "719000");
      ASSERT_EQ(swap_response->estimated_gas, "719000");
      ASSERT_EQ(swap_response->gas_price, "26000000000");
      ASSERT_EQ(swap_response->protocol_fee, "0");
      ASSERT_EQ(swap_response->minimum_protocol_fee, "0");
      ASSERT_EQ(swap_response->buy_token_address,
                "0xeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee");
      ASSERT_EQ(swap_response->sell_token_address,
                "0x6b175474e89094c44da98b954eedeac495271d0f");
      ASSERT_EQ(swap_response->buy_amount, "1000000000000000000000");
      ASSERT_EQ(swap_response->sell_amount, "1916275479988140583549706");
      ASSERT_EQ(swap_response->allowance_target,
                "0xdef1c0ded9bec7f1a1670819833240f027b25eff");
      ASSERT_EQ(swap_response->sell_token_to_eth_rate,
                "1900.44962824532464391");
      ASSERT_EQ(swap_response->buy_token_to_eth_rate, "1");
    }
    ASSERT_EQ(expected_swap_response_, *swap_response);
  }

  void WaitForSwapResponse(
      brave_wallet::mojom::SwapResponse expected_swap_response,
      bool expected_success) {
    if (wait_for_request_) {
      return;
    }
    expected_swap_response_ = expected_swap_response;
    expected_success_ = expected_success;
    wait_for_request_.reset(new base::RunLoop);
    wait_for_request_->Run();
  }

  content::WebContents* active_contents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  mojo::Remote<brave_wallet::mojom::SwapController> GetSwapController() {
    auto pending =
        brave_wallet::SwapControllerFactory::GetInstance()->GetForContext(
            browser()->profile());
    mojo::Remote<brave_wallet::mojom::SwapController> asset_ratio_controller;
    asset_ratio_controller.Bind(std::move(pending));
    return asset_ratio_controller;
  }

 private:
  net::EmbeddedTestServer* https_server() { return https_server_.get(); }

  bool expected_success_;
  brave_wallet::mojom::SwapResponse expected_swap_response_;

  std::unique_ptr<base::RunLoop> wait_for_request_;
  std::unique_ptr<net::EmbeddedTestServer> https_server_;
};

IN_PROC_BROWSER_TEST_F(SwapControllerTest, GetPriceQuote) {
  ResetHTTPSServer(base::BindRepeating(&HandleRequest));
  brave_wallet::mojom::SwapParams params;
  params.buy_token = "ETH";
  params.sell_token = "DAI";
  params.buy_amount = "1000000000000000000000";
  auto controller = GetSwapController();
  controller->GetPriceQuote(
      GetCannedSwapParams(),
      base::BindOnce(&SwapControllerTest::OnGetPriceQuoteResponse,
                     base::Unretained(this)));
  brave_wallet::mojom::SwapResponse expected_swap_response;
  expected_swap_response.price = "1916.27547998814058355";
  expected_swap_response.value = "0";
  expected_swap_response.gas = "719000";
  expected_swap_response.estimated_gas = "719000";
  expected_swap_response.gas_price = "26000000000";
  expected_swap_response.protocol_fee = "0";
  expected_swap_response.minimum_protocol_fee = "0";
  expected_swap_response.buy_token_address =
      "0xeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee";
  expected_swap_response.sell_token_address =
      "0x6b175474e89094c44da98b954eedeac495271d0f";
  expected_swap_response.buy_amount = "1000000000000000000000";
  expected_swap_response.sell_amount = "1916275479988140583549706";
  expected_swap_response.allowance_target =
      "0xdef1c0ded9bec7f1a1670819833240f027b25eff";
  expected_swap_response.sell_token_to_eth_rate = "1900.44962824532464391";
  expected_swap_response.buy_token_to_eth_rate = "1";

  WaitForSwapResponse(std::move(expected_swap_response), true);
}

IN_PROC_BROWSER_TEST_F(SwapControllerTest, GetPriceQuoteServerError) {
  ResetHTTPSServer(base::BindRepeating(&HandleRequestServerError));
  auto controller = GetSwapController();
  controller->GetPriceQuote(
      GetCannedSwapParams(),
      base::BindOnce(&SwapControllerTest::OnGetPriceQuoteResponse,
                     base::Unretained(this)));
  brave_wallet::mojom::SwapResponse expected_swap_response;
  WaitForSwapResponse(expected_swap_response, false);
}

IN_PROC_BROWSER_TEST_F(SwapControllerTest, GetTransactionPayload) {
  ResetHTTPSServer(base::BindRepeating(&HandleRequest));
  auto controller = GetSwapController();
  controller->GetTransactionPayload(
      GetCannedSwapParams(),
      base::BindOnce(&SwapControllerTest::OnGetTransactionPayloadResponse,
                     base::Unretained(this)));

  brave_wallet::mojom::SwapResponse expected_swap_response;
  expected_swap_response.price = "1916.27547998814058355";
  expected_swap_response.guaranteed_price = "1935.438234788021989386";
  expected_swap_response.to = "0xdef1c0ded9bec7f1a1670819833240f027b25eff";
  expected_swap_response.data = "0x0";
  expected_swap_response.value = "0";
  expected_swap_response.gas = "719000";
  expected_swap_response.estimated_gas = "719000";
  expected_swap_response.gas_price = "26000000000";
  expected_swap_response.protocol_fee = "0";
  expected_swap_response.minimum_protocol_fee = "0";
  expected_swap_response.buy_token_address =
      "0xeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee";
  expected_swap_response.sell_token_address =
      "0x6b175474e89094c44da98b954eedeac495271d0f";
  expected_swap_response.buy_amount = "1000000000000000000000";
  expected_swap_response.sell_amount = "1916275479988140583549706";
  expected_swap_response.allowance_target =
      "0xdef1c0ded9bec7f1a1670819833240f027b25eff";
  expected_swap_response.sell_token_to_eth_rate = "1900.44962824532464391";
  expected_swap_response.buy_token_to_eth_rate = "1";

  WaitForSwapResponse(std::move(expected_swap_response), true);
}

IN_PROC_BROWSER_TEST_F(SwapControllerTest, GetTransactionPayloadServerError) {
  ResetHTTPSServer(base::BindRepeating(&HandleRequestServerError));
  auto controller = GetSwapController();
  controller->GetTransactionPayload(
      GetCannedSwapParams(),
      base::BindOnce(&SwapControllerTest::OnGetTransactionPayloadResponse,
                     base::Unretained(this)));
  brave_wallet::mojom::SwapResponse expected_swap_response;
  WaitForSwapResponse(std::move(expected_swap_response), false);
}

}  // namespace
