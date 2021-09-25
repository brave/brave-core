/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>
#include <utility>

#include "base/test/bind.h"
#include "brave/components/brave_wallet/browser/swap_controller.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "content/public/test/browser_task_environment.h"
#include "content/public/test/test_browser_context.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

brave_wallet::mojom::SwapParamsPtr GetCannedSwapParams() {
  auto params = brave_wallet::mojom::SwapParams::New();
  params->buy_token = "ETH";
  params->sell_token = "DAI";
  params->buy_amount = "1000000000000000000000";
  return params;
}

void OnRequestResponse(
    bool* callback_run,
    bool expected_success,
    brave_wallet::mojom::SwapResponsePtr expected_swap_response,
    const std::string& expected_error,
    bool success,
    brave_wallet::mojom::SwapResponsePtr swap_response,
    const std::string& error) {
  EXPECT_EQ(expected_success, success);
  EXPECT_EQ(expected_swap_response, swap_response);
  EXPECT_EQ(expected_error, error);
  *callback_run = true;
}

}  // namespace

namespace brave_wallet {

class SwapControllerUnitTest : public testing::Test {
 public:
  SwapControllerUnitTest()
      : browser_context_(new content::TestBrowserContext()),
        shared_url_loader_factory_(
            base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
                &url_loader_factory_)) {
    swap_controller_.reset(new SwapController(shared_url_loader_factory_));
  }

  ~SwapControllerUnitTest() override = default;

  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory() {
    return shared_url_loader_factory_;
  }

  void SetInterceptor(const std::string& content) {
    url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
        [&, content](const network::ResourceRequest& request) {
          url_loader_factory_.ClearResponses();
          url_loader_factory_.AddResponse(request.url.spec(), content);
        }));
  }

  void SetErrorInterceptor(const std::string& content) {
    url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
        [&](const network::ResourceRequest& request) {
          url_loader_factory_.ClearResponses();
          url_loader_factory_.AddResponse(request.url.spec(), content,
                                          net::HTTP_REQUEST_TIMEOUT);
        }));
  }

 protected:
  std::unique_ptr<SwapController> swap_controller_;

 private:
  content::BrowserTaskEnvironment browser_task_environment_;
  std::unique_ptr<content::TestBrowserContext> browser_context_;
  network::TestURLLoaderFactory url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
};

TEST_F(SwapControllerUnitTest, GetPriceQuote) {
  SetInterceptor(R"(
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
    })");

  auto expected_swap_response = brave_wallet::mojom::SwapResponse::New();
  expected_swap_response->price = "1916.27547998814058355";
  expected_swap_response->value = "0";
  expected_swap_response->gas = "719000";
  expected_swap_response->estimated_gas = "719000";
  expected_swap_response->gas_price = "26000000000";
  expected_swap_response->protocol_fee = "0";
  expected_swap_response->minimum_protocol_fee = "0";
  expected_swap_response->buy_token_address =
      "0xeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee";
  expected_swap_response->sell_token_address =
      "0x6b175474e89094c44da98b954eedeac495271d0f";
  expected_swap_response->buy_amount = "1000000000000000000000";
  expected_swap_response->sell_amount = "1916275479988140583549706";
  expected_swap_response->allowance_target =
      "0xdef1c0ded9bec7f1a1670819833240f027b25eff";
  expected_swap_response->sell_token_to_eth_rate = "1900.44962824532464391";
  expected_swap_response->buy_token_to_eth_rate = "1";

  bool callback_run = false;
  swap_controller_->GetPriceQuote(
      GetCannedSwapParams(),
      base::BindOnce(&OnRequestResponse, &callback_run, true,
                     std::move(expected_swap_response), ""));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_run);
}

TEST_F(SwapControllerUnitTest, GetPriceQuoteError) {
  std::string error =
      R"({"code":100,"reason":"Validation Failed","validationErrors":[{"field":"sellAmount","code":1000,"reason":"should have required property 'sellAmount'"},{"field":"buyAmount","code":1000,"reason":"should have required property 'buyAmount'"},{"field":"","code":1001,"reason":"should match exactly one schema in oneOf"}]})";
  SetErrorInterceptor(error);
  bool callback_run = false;
  swap_controller_->GetPriceQuote(
      brave_wallet::mojom::SwapParams::New(),
      base::BindOnce(&OnRequestResponse, &callback_run, false, nullptr, error));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_run);
}

TEST_F(SwapControllerUnitTest, GetPriceQuoteUnexpectedReturn) {
  std::string error = "Could not parse response body: Woot";
  std::string unexpected_return = "Woot";
  SetInterceptor(unexpected_return);
  bool callback_run = false;
  swap_controller_->GetPriceQuote(
      GetCannedSwapParams(),
      base::BindOnce(&OnRequestResponse, &callback_run, false, nullptr, error));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_run);
}

TEST_F(SwapControllerUnitTest, GetTransactionPayload) {
  SetInterceptor(R"(
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
    })");

  auto expected_swap_response = brave_wallet::mojom::SwapResponse::New();
  expected_swap_response->price = "1916.27547998814058355";
  expected_swap_response->guaranteed_price = "1935.438234788021989386";
  expected_swap_response->to = "0xdef1c0ded9bec7f1a1670819833240f027b25eff";
  expected_swap_response->data = "0x0";
  expected_swap_response->value = "0";
  expected_swap_response->gas = "719000";
  expected_swap_response->estimated_gas = "719000";
  expected_swap_response->gas_price = "26000000000";
  expected_swap_response->protocol_fee = "0";
  expected_swap_response->minimum_protocol_fee = "0";
  expected_swap_response->buy_token_address =
      "0xeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee";
  expected_swap_response->sell_token_address =
      "0x6b175474e89094c44da98b954eedeac495271d0f";
  expected_swap_response->buy_amount = "1000000000000000000000";
  expected_swap_response->sell_amount = "1916275479988140583549706";
  expected_swap_response->allowance_target =
      "0xdef1c0ded9bec7f1a1670819833240f027b25eff";
  expected_swap_response->sell_token_to_eth_rate = "1900.44962824532464391";
  expected_swap_response->buy_token_to_eth_rate = "1";

  bool callback_run = false;
  swap_controller_->GetTransactionPayload(
      GetCannedSwapParams(),
      base::BindOnce(&OnRequestResponse, &callback_run, true,
                     std::move(expected_swap_response), ""));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_run);
}

TEST_F(SwapControllerUnitTest, GetTransactionPayloadError) {
  std::string error = R"(
    {
      "code": 100,
      "reason": "Validation Failed",
      "validationErrors": [{
        "code": 1000,
        "field": "sellAmount",
        "reason": "should have required property 'sellAmount'"
      }, {
        "code": 1000,
        "field": "buyAmount",
        "reason": "should have required property 'buyAmount'"
      }, {
        "code": 1001,
        "field": "",
        "reason": "should match exactly one schema in oneOf"
      }]
    })";

  SetErrorInterceptor(error);
  bool callback_run = false;
  swap_controller_->GetTransactionPayload(
      brave_wallet::mojom::SwapParams::New(),
      base::BindOnce(&OnRequestResponse, &callback_run, false, nullptr, error));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_run);
}

TEST_F(SwapControllerUnitTest, GetTransactionPayloadUnexpectedReturn) {
  std::string error = "Could not parse response body: Woot";
  std::string unexpected_return = "Woot";
  SetInterceptor(unexpected_return);
  bool callback_run = false;
  swap_controller_->GetTransactionPayload(
      GetCannedSwapParams(),
      base::BindOnce(&OnRequestResponse, &callback_run, false, nullptr, error));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_run);
}

}  // namespace brave_wallet
