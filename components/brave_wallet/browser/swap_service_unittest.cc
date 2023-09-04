/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <map>
#include <memory>
#include <utility>

#include "base/memory/ref_counted.h"
#include "base/memory/scoped_refptr.h"
#include "base/test/bind.h"
#include "base/test/mock_callback.h"
#include "base/test/task_environment.h"
#include "base/test/values_test_util.h"
#include "brave/components/brave_wallet/browser/brave_wallet_prefs.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/browser/swap_response_parser.h"
#include "brave/components/brave_wallet/browser/swap_service.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/test_utils.h"
#include "components/grit/brave_components_strings.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "services/data_decoder/public/cpp/test_support/in_process_data_decoder.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/l10n/l10n_util.h"

using base::test::ParseJson;

namespace {

auto IsTruthy(bool truthy) {
  return testing::Truly(
      [=](const auto& candidate) { return !!candidate == truthy; });
}

brave_wallet::mojom::SwapParamsPtr GetCannedSwapParams() {
  auto params = brave_wallet::mojom::SwapParams::New();
  params->buy_token = "ETH";
  params->sell_token = "DAI";
  params->buy_amount = "1000000000000000000000";
  params->taker_address = "0xa92D461a9a988A7f11ec285d39783A637Fdd6ba4";
  params->slippage_percentage = 0.03;
  return params;
}

brave_wallet::mojom::JupiterQuoteParamsPtr GetCannedJupiterQuoteParams() {
  auto params = brave_wallet::mojom::JupiterQuoteParams::New();
  params->output_mint = "EPjFWdd5AufqSSqeM2qN1xzybapC8G4wEGGkZwyTDt1v";
  params->input_mint = "So11111111111111111111111111111111111111112";
  params->amount = "10000";
  params->slippage_bps = 50;
  params->user_public_key = "foo";
  return params;
}

brave_wallet::mojom::JupiterSwapParamsPtr GetCannedJupiterSwapParams(
    const std::string& output_mint) {
  auto params = brave_wallet::mojom::JupiterSwapParams::New();

  auto route = brave_wallet::mojom::JupiterRoute::New();
  route->in_amount = 10000ULL;
  route->out_amount = 261273ULL;
  route->amount = 10000ULL;
  route->other_amount_threshold = 258660ULL;
  route->swap_mode = "ExactIn";
  route->price_impact_pct = 0.008955716118219659;

  auto market_info = brave_wallet::mojom::JupiterMarketInfo::New();
  market_info->id = "2yNwARmTmc3NzYMETCZQjAE5GGCPgviH6hiBsxaeikTK";
  market_info->label = "Orca";
  market_info->input_mint = "EPjFWdd5AufqSSqeM2qN1xzybapC8G4wEGGkZwyTDt1v";
  market_info->output_mint = "MNDEFzGvMt87ueuHvVU9VcTqsAP5b3fTGPsHuuPA5ey";
  market_info->not_enough_liquidity = false;
  market_info->in_amount = 10000ULL;
  market_info->out_amount = 117001203ULL;
  market_info->price_impact_pct = 1.196568750220778e-7;

  auto lp_fee = brave_wallet::mojom::JupiterFee::New();
  lp_fee->amount = 30ULL;
  lp_fee->mint = "EPjFWdd5AufqSSqeM2qN1xzybapC8G4wEGGkZwyTDt1v";
  lp_fee->pct = 0.003;

  auto platform_fee = brave_wallet::mojom::JupiterFee::New();
  platform_fee->amount = 0ULL;
  platform_fee->mint = "MNDEFzGvMt87ueuHvVU9VcTqsAP5b3fTGPsHuuPA5ey";
  platform_fee->pct = 0;

  market_info->lp_fee = lp_fee.Clone();
  market_info->platform_fee = platform_fee.Clone();
  route->market_infos.push_back(market_info.Clone());
  params->route = route.Clone();
  params->user_public_key = "foo";
  params->output_mint = output_mint;
  params->input_mint = "EPjFWdd5AufqSSqeM2qN1xzybapC8G4wEGGkZwyTDt1v";

  return params;
}

}  // namespace

namespace brave_wallet {

class SwapServiceUnitTest : public testing::Test {
 public:
  SwapServiceUnitTest()
      : shared_url_loader_factory_(
            base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
                &url_loader_factory_)) {
    brave_wallet::RegisterProfilePrefs(prefs_.registry());
    json_rpc_service_ =
        std::make_unique<JsonRpcService>(shared_url_loader_factory_, &prefs_);
    swap_service_ = std::make_unique<SwapService>(shared_url_loader_factory_,
                                                  json_rpc_service_.get());
  }

  ~SwapServiceUnitTest() override = default;

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

  bool IsSwapSupported(const std::string& chain_id) {
    bool result;
    base::RunLoop run_loop;
    swap_service_->IsSwapSupported(chain_id,
                                   base::BindLambdaForTesting([&](bool v) {
                                     result = v;
                                     run_loop.Quit();
                                   }));
    run_loop.Run();
    return result;
  }

  void TestGetJupiterQuoteCase(const std::string& json,
                               const bool expected_success) {
    SetInterceptor(json);
    auto expected_error_string =
        expected_success ? ""
                         : l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR);
    base::MockCallback<mojom::SwapService::GetJupiterQuoteCallback> callback;
    EXPECT_CALL(callback, Run(IsTruthy(expected_success),
                              EqualsMojo(mojom::JupiterErrorResponsePtr()),
                              expected_error_string));

    swap_service_->GetJupiterQuote(GetCannedJupiterQuoteParams(),
                                   callback.Get());
    base::RunLoop().RunUntilIdle();
  }

  void TestGetJupiterQuoteUint64Cases(const char* json_template) {
    // KO: negative uint64 value
    auto json = base::StringPrintf(json_template, "-10000");
    TestGetJupiterQuoteCase(json, false);

    // KO: non-integer uint64 value
    json = base::StringPrintf(json_template, "\"foo\"");
    TestGetJupiterQuoteCase(json, false);

    // KO: uint64 value overflow (UINT64_MAX + 1)
    json = base::StringPrintf(json_template, "18446744073709551616");
    TestGetJupiterQuoteCase(json, false);

    // OK: max uint64 amount value
    json = base::StringPrintf(json_template, "18446744073709551615");
    TestGetJupiterQuoteCase(json, true);
  }

  void TestGetJupiterSwapTransactions(
      const bool expected_success,
      brave_wallet::mojom::JupiterSwapTransactionsPtr expected_response,
      const bool has_error,
      const std::string& output_mint =
          "EPjFWdd5AufqSSqeM2qN1xzybapC8G4wEGGkZwyTDt1v"  // USDC
  ) {
    auto expected_error_string =
        expected_success
            ? testing::AnyOf(std::string(), std::string())
            : testing::AnyOf(
                  l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR),
                  l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    base::MockCallback<mojom::SwapService::GetJupiterSwapTransactionsCallback>
        callback;
    EXPECT_CALL(callback, Run(IsTruthy(expected_success),
                              EqualsMojo(mojom::JupiterErrorResponsePtr()),
                              expected_error_string));

    swap_service_->GetJupiterSwapTransactions(
        GetCannedJupiterSwapParams(output_mint), callback.Get());
    base::RunLoop().RunUntilIdle();
  }

 protected:
  sync_preferences::TestingPrefServiceSyncable prefs_;
  std::unique_ptr<JsonRpcService> json_rpc_service_;
  std::unique_ptr<SwapService> swap_service_;

 private:
  base::test::TaskEnvironment task_environment_;
  network::TestURLLoaderFactory url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
  data_decoder::test::InProcessDataDecoder in_process_data_decoder_;
};

TEST_F(SwapServiceUnitTest, GetPriceQuote) {
  // Case 1: non-null zeroExFee
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
      "allowanceTarget":"0xdef1c0ded9bec7f1a1670819833240f027b25eff",
      "sellTokenToEthRate":"1900.44962824532464391",
      "buyTokenToEthRate":"1",
      "estimatedPriceImpact": "0.7232",
      "sources": [
        {
          "name": "Uniswap_V2",
          "proportion": "1"
        }
      ],
      "fees": {
        "zeroExFee" : {
          "feeType" : "volume",
          "feeToken" : "0x8f3cf7ad23cd3cadbd9735aff958023239c6a063",
          "feeAmount" : "148470027512868522",
          "billingType" : "on-chain"
        }
      }
    })");

  auto expected_swap_response = mojom::SwapResponse::New();
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
  expected_swap_response->estimated_price_impact = "0.7232";

  auto source = brave_wallet::mojom::ZeroExSource::New();
  source->name = "Uniswap_V2";
  source->proportion = "1";
  expected_swap_response->sources.push_back(source.Clone());

  auto fees = brave_wallet::mojom::ZeroExFees::New();
  auto zero_ex_fee = brave_wallet::mojom::ZeroExFee::New();
  zero_ex_fee->fee_type = "volume";
  zero_ex_fee->fee_token = "0x8f3cf7ad23cd3cadbd9735aff958023239c6a063";
  zero_ex_fee->fee_amount = "148470027512868522";
  zero_ex_fee->billing_type = "on-chain";
  fees->zero_ex_fee = std::move(zero_ex_fee);
  expected_swap_response->fees = std::move(fees);

  base::MockCallback<mojom::SwapService::GetPriceQuoteCallback> callback;
  EXPECT_CALL(callback, Run(EqualsMojo(expected_swap_response),
                            EqualsMojo(mojom::SwapErrorResponsePtr()), ""));

  swap_service_->GetPriceQuote(GetCannedSwapParams(), callback.Get());
  base::RunLoop().RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&callback);

  // Case 2: null zeroExFee
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
      "allowanceTarget":"0xdef1c0ded9bec7f1a1670819833240f027b25eff",
      "sellTokenToEthRate":"1900.44962824532464391",
      "buyTokenToEthRate":"1",
      "estimatedPriceImpact": "0.7232",
      "sources": [
        {
          "name": "Uniswap_V2",
          "proportion": "1"
        }
      ],
      "fees": {
        "zeroExFee": null
      }
    })");

  expected_swap_response->fees->zero_ex_fee = nullptr;
  EXPECT_CALL(callback, Run(EqualsMojo(expected_swap_response),
                            EqualsMojo(mojom::SwapErrorResponsePtr()), ""));

  swap_service_->GetPriceQuote(GetCannedSwapParams(), callback.Get());
  base::RunLoop().RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&callback);
}

TEST_F(SwapServiceUnitTest, GetPriceQuoteError) {
  std::string error = R"(
    {
      "code": 100,
      "reason": "Validation Failed",
      "validationErrors": [
        {
          "code": 1000,
          "field": "sellAmount",
          "reason": "should have required property 'sellAmount'"
        },
        {
          "code": 1000,
          "field": "buyAmount",
          "reason": "should have required property 'buyAmount'"
        },
        {
          "code": 1001,
          "field": "",
          "reason": "should match exactly one schema in oneOf"
        }
      ]
    })";
  SetErrorInterceptor(error);

  base::MockCallback<mojom::SwapService::GetPriceQuoteCallback> callback;
  EXPECT_CALL(callback,
              Run(EqualsMojo(mojom::SwapResponsePtr()),
                  EqualsMojo(ParseSwapErrorResponse(ParseJson(error))), ""));

  swap_service_->GetPriceQuote(brave_wallet::mojom::SwapParams::New(),
                               callback.Get());
  base::RunLoop().RunUntilIdle();
}

TEST_F(SwapServiceUnitTest, GetPriceQuoteUnexpectedReturn) {
  std::string unexpected_return = "Woot";
  SetInterceptor(unexpected_return);

  base::MockCallback<mojom::SwapService::GetPriceQuoteCallback> callback;
  EXPECT_CALL(callback,
              Run(EqualsMojo(mojom::SwapResponsePtr()),
                  EqualsMojo(mojom::SwapErrorResponsePtr()),
                  l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR)));

  swap_service_->GetPriceQuote(GetCannedSwapParams(), callback.Get());
  base::RunLoop().RunUntilIdle();
}

TEST_F(SwapServiceUnitTest, GetTransactionPayload) {
  // Case 1: non-null zeroExFee
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
      "allowanceTarget":"0xdef1c0ded9bec7f1a1670819833240f027b25eff",
      "sellTokenToEthRate":"1900.44962824532464391",
      "buyTokenToEthRate":"1",
      "estimatedPriceImpact": "0.7232",
      "sources": [
        {
          "name": "Uniswap_V2",
          "proportion": "1"
        }
      ],
      "fees": {
        "zeroExFee": {
          "feeType": "volume",
          "feeToken": "0x8f3cf7ad23cd3cadbd9735aff958023239c6a063",
          "feeAmount": "148470027512868522",
          "billingType": "on-chain"
        }
      }
    })");

  auto expected_swap_response = mojom::SwapResponse::New();
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
  expected_swap_response->estimated_price_impact = "0.7232";
  auto source = brave_wallet::mojom::ZeroExSource::New();
  source->name = "Uniswap_V2";
  source->proportion = "1";
  expected_swap_response->sources.push_back(source.Clone());

  auto fees = brave_wallet::mojom::ZeroExFees::New();
  auto zero_ex_fee = brave_wallet::mojom::ZeroExFee::New();
  zero_ex_fee->fee_type = "volume";
  zero_ex_fee->fee_token = "0x8f3cf7ad23cd3cadbd9735aff958023239c6a063";
  zero_ex_fee->fee_amount = "148470027512868522";
  zero_ex_fee->billing_type = "on-chain";
  fees->zero_ex_fee = std::move(zero_ex_fee);
  expected_swap_response->fees = std::move(fees);

  base::MockCallback<mojom::SwapService::GetTransactionPayloadCallback>
      callback;
  EXPECT_CALL(callback, Run(EqualsMojo(expected_swap_response),
                            EqualsMojo(mojom::SwapErrorResponsePtr()), ""));

  swap_service_->GetTransactionPayload(GetCannedSwapParams(), callback.Get());
  base::RunLoop().RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&callback);

  // Case 2: null zeroExFee
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
      "allowanceTarget":"0xdef1c0ded9bec7f1a1670819833240f027b25eff",
      "sellTokenToEthRate":"1900.44962824532464391",
      "buyTokenToEthRate":"1",
      "estimatedPriceImpact": "0.7232",
      "sources": [
        {
          "name": "Uniswap_V2",
          "proportion": "1"
        }
      ],
      "fees": {
        "zeroExFee": null
      }
    })");

  expected_swap_response->fees->zero_ex_fee = nullptr;
  EXPECT_CALL(callback, Run(EqualsMojo(expected_swap_response),
                            EqualsMojo(mojom::SwapErrorResponsePtr()), ""));

  swap_service_->GetTransactionPayload(GetCannedSwapParams(), callback.Get());
  base::RunLoop().RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&callback);
}

TEST_F(SwapServiceUnitTest, GetTransactionPayloadError) {
  std::string error =
      R"({"code":100,"reason":"Validation Failed","validationErrors":[{"code":1000,"field":"sellAmount","reason":"should have required property 'sellAmount'"},{"code":1000,"field":"buyAmount","reason":"should have required property 'buyAmount'"},{"code":1001,"field":"","reason":"should match exactly one schema in oneOf"}]})";
  SetErrorInterceptor(error);

  base::MockCallback<mojom::SwapService::GetTransactionPayloadCallback>
      callback;
  EXPECT_CALL(callback,
              Run(EqualsMojo(mojom::SwapResponsePtr()),
                  EqualsMojo(ParseSwapErrorResponse(ParseJson(error))), ""));

  swap_service_->GetTransactionPayload(mojom::SwapParams::New(),
                                       callback.Get());
  base::RunLoop().RunUntilIdle();
}

TEST_F(SwapServiceUnitTest, GetTransactionPayloadUnexpectedReturn) {
  SetInterceptor("Woot");

  base::MockCallback<mojom::SwapService::GetTransactionPayloadCallback>
      callback;
  EXPECT_CALL(callback,
              Run(EqualsMojo(mojom::SwapResponsePtr()),
                  EqualsMojo(mojom::SwapErrorResponsePtr()),
                  l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR)));

  swap_service_->GetTransactionPayload(GetCannedSwapParams(), callback.Get());
  base::RunLoop().RunUntilIdle();
}

TEST_F(SwapServiceUnitTest, GetPriceQuoteURL) {
  const std::map<std::string, std::string> non_rfqt_chain_ids = {
      {mojom::kGoerliChainId, "goerli.api.0x.org"},
      {mojom::kBinanceSmartChainMainnetChainId, "bsc.api.0x.org"},
      {mojom::kAvalancheMainnetChainId, "avalanche.api.0x.org"},
      {mojom::kFantomMainnetChainId, "fantom.api.0x.org"},
      {mojom::kCeloMainnetChainId, "celo.api.0x.org"},
      {mojom::kOptimismMainnetChainId, "optimism.api.0x.org"},
      {mojom::kArbitrumMainnetChainId, "arbitrum.api.0x.org"},
      {mojom::kBaseMainnetChainId, "base.api.0x.org"}};

  const std::map<std::string, std::string> rfqt_chain_ids = {
      {mojom::kMainnetChainId, "api.0x.org"},
      {mojom::kPolygonMainnetChainId, "polygon.api.0x.org"}};

  auto params = GetCannedSwapParams();

  for (const auto& [chain_id, domain] : non_rfqt_chain_ids) {
    SCOPED_TRACE(testing::Message() << "chain_id: " << chain_id);
    auto url = swap_service_->GetPriceQuoteURL(params.Clone(), chain_id);

    EXPECT_EQ(url,
              base::StringPrintf(
                  "https://%s/swap/v1/price?"
                  "takerAddress=0xa92D461a9a988A7f11ec285d39783A637Fdd6ba4&"
                  "buyAmount=1000000000000000000000&"
                  "buyToken=ETH&"
                  "sellToken=DAI&"
                  "buyTokenPercentageFee=0.00875&"
                  "slippagePercentage=0.030000&"
                  "feeRecipient=0xbd9420A98a7Bd6B89765e5715e169481602D9c3d&"
                  "affiliateAddress=0xbd9420A98a7Bd6B89765e5715e169481602D9c3d&"
                  "skipValidation=true",
                  domain.c_str()));
  }

  // If RFQ-T liquidity is available, so intentOnFilling=true is specified
  // while fetching firm quotes.
  for (const auto& [chain_id, domain] : rfqt_chain_ids) {
    SCOPED_TRACE(testing::Message() << "chain_id: " << chain_id);
    auto url = swap_service_->GetPriceQuoteURL(params.Clone(), chain_id);

    EXPECT_EQ(url,
              base::StringPrintf(
                  "https://%s/swap/v1/quote?"
                  "takerAddress=0xa92D461a9a988A7f11ec285d39783A637Fdd6ba4&"
                  "buyAmount=1000000000000000000000&"
                  "buyToken=ETH&"
                  "sellToken=DAI&"
                  "buyTokenPercentageFee=0.00875&"
                  "slippagePercentage=0.030000&"
                  "feeRecipient=0xbd9420A98a7Bd6B89765e5715e169481602D9c3d&"
                  "affiliateAddress=0xbd9420A98a7Bd6B89765e5715e169481602D9c3d&"
                  "skipValidation=true&"
                  "intentOnFilling=false",
                  domain.c_str()));
  }

  // KO: unsupported network
  EXPECT_EQ(swap_service_->GetPriceQuoteURL(params.Clone(), "0x3"), "");
}

TEST_F(SwapServiceUnitTest, GetTransactionPayloadURL) {
  const std::map<std::string, std::string> non_rfqt_chain_ids = {
      {mojom::kGoerliChainId, "goerli.api.0x.org"},
      {mojom::kBinanceSmartChainMainnetChainId, "bsc.api.0x.org"},
      {mojom::kAvalancheMainnetChainId, "avalanche.api.0x.org"},
      {mojom::kFantomMainnetChainId, "fantom.api.0x.org"},
      {mojom::kCeloMainnetChainId, "celo.api.0x.org"},
      {mojom::kOptimismMainnetChainId, "optimism.api.0x.org"},
      {mojom::kArbitrumMainnetChainId, "arbitrum.api.0x.org"},
      {mojom::kBaseMainnetChainId, "base.api.0x.org"}};

  const std::map<std::string, std::string> rfqt_chain_ids = {
      {mojom::kMainnetChainId, "api.0x.org"},
      {mojom::kPolygonMainnetChainId, "polygon.api.0x.org"}};

  auto params = GetCannedSwapParams();

  for (const auto& [chain_id, domain] : non_rfqt_chain_ids) {
    SCOPED_TRACE(testing::Message() << "chain_id: " << chain_id);
    auto url =
        swap_service_->GetTransactionPayloadURL(params.Clone(), chain_id);

    EXPECT_EQ(url,
              base::StringPrintf(
                  "https://%s/swap/v1/quote?"
                  "takerAddress=0xa92D461a9a988A7f11ec285d39783A637Fdd6ba4&"
                  "buyAmount=1000000000000000000000&"
                  "buyToken=ETH&"
                  "sellToken=DAI&"
                  "buyTokenPercentageFee=0.00875&"
                  "slippagePercentage=0.030000&"
                  "feeRecipient=0xbd9420A98a7Bd6B89765e5715e169481602D9c3d&"
                  "affiliateAddress=0xbd9420A98a7Bd6B89765e5715e169481602D9c3d",
                  domain.c_str()));
  }

  // If RFQ-T liquidity is available, so intentOnFilling=true is specified
  // while fetching firm quotes.
  for (const auto& [chain_id, domain] : rfqt_chain_ids) {
    SCOPED_TRACE(testing::Message() << "chain_id: " << chain_id);
    auto url =
        swap_service_->GetTransactionPayloadURL(params.Clone(), chain_id);

    EXPECT_EQ(url,
              base::StringPrintf(
                  "https://%s/swap/v1/quote?"
                  "takerAddress=0xa92D461a9a988A7f11ec285d39783A637Fdd6ba4&"
                  "buyAmount=1000000000000000000000&"
                  "buyToken=ETH&"
                  "sellToken=DAI&"
                  "buyTokenPercentageFee=0.00875&"
                  "slippagePercentage=0.030000&"
                  "feeRecipient=0xbd9420A98a7Bd6B89765e5715e169481602D9c3d&"
                  "affiliateAddress=0xbd9420A98a7Bd6B89765e5715e169481602D9c3d&"
                  "intentOnFilling=true",
                  domain.c_str()));
  }

  // KO: unsupported network
  EXPECT_EQ(swap_service_->GetTransactionPayloadURL(params.Clone(), "0x3"), "");
}

TEST_F(SwapServiceUnitTest, IsSwapSupported) {
  const std::vector<std::string> supported_chain_ids({
      mojom::kMainnetChainId,
      mojom::kGoerliChainId,
      mojom::kPolygonMainnetChainId,
      mojom::kPolygonMainnetChainId,
      mojom::kBinanceSmartChainMainnetChainId,
      mojom::kAvalancheMainnetChainId,
      mojom::kFantomMainnetChainId,
      mojom::kCeloMainnetChainId,
      mojom::kOptimismMainnetChainId,
      mojom::kArbitrumMainnetChainId,
      mojom::kBaseMainnetChainId,
  });

  for (auto& chain_id : supported_chain_ids) {
    EXPECT_TRUE(IsSwapSupported(chain_id));
  }

  EXPECT_FALSE(IsSwapSupported("0x4"));
  EXPECT_FALSE(IsSwapSupported("0x3"));
  EXPECT_FALSE(IsSwapSupported(""));
  EXPECT_FALSE(IsSwapSupported("invalid chain_id"));
}
TEST_F(SwapServiceUnitTest, GetJupiterQuoteURL) {
  auto params = GetCannedJupiterQuoteParams();
  auto url =
      swap_service_->GetJupiterQuoteURL(params.Clone(), mojom::kSolanaMainnet);

  // OK: output mint has Jupiter fees
  EXPECT_EQ(url,
            "https://quote-api.jup.ag/v4/quote?"
            "inputMint=So11111111111111111111111111111111111111112&"
            "outputMint=EPjFWdd5AufqSSqeM2qN1xzybapC8G4wEGGkZwyTDt1v&"
            "amount=10000&"
            "swapMode=ExactIn&"
            "slippageBps=50&"
            "feeBps=85");

  params->output_mint = "SHDWyBxihqiCj6YekG2GUr7wqKLeLAMK1gHZck9pL6y";
  url =
      swap_service_->GetJupiterQuoteURL(params.Clone(), mojom::kSolanaMainnet);

  // OK: output mint does not have Jupiter fees
  EXPECT_EQ(url,
            "https://quote-api.jup.ag/v4/quote?"
            "inputMint=So11111111111111111111111111111111111111112&"
            "outputMint=SHDWyBxihqiCj6YekG2GUr7wqKLeLAMK1gHZck9pL6y&"
            "amount=10000&"
            "swapMode=ExactIn&"
            "slippageBps=50");
}

TEST_F(SwapServiceUnitTest, GetJupiterSwapTransactionsURL) {
  auto url =
      swap_service_->GetJupiterSwapTransactionsURL(mojom::kSolanaMainnet);
  EXPECT_EQ(url, "https://quote-api.jup.ag/v4/swap");
}

TEST_F(SwapServiceUnitTest, GetJupiterQuote) {
  SetInterceptor(R"(
    {
      "data": [
        {
          "inAmount": 10000,
          "outAmount": 261273,
          "amount": 10000,
          "otherAmountThreshold": 258660,
          "swapMode": "ExactIn",
          "priceImpactPct": 0.008955716118219659,
          "slippageBps": 50,
          "marketInfos": [
            {
              "id": "2yNwARmTmc3NzYMETCZQjAE5GGCPgviH6hiBsxaeikTK",
              "label": "Orca",
              "inputMint": "EPjFWdd5AufqSSqeM2qN1xzybapC8G4wEGGkZwyTDt1v",
              "outputMint": "MNDEFzGvMt87ueuHvVU9VcTqsAP5b3fTGPsHuuPA5ey",
              "notEnoughLiquidity": false,
              "inAmount": 10000,
              "outAmount": 117001203,
              "priceImpactPct": 1.196568750220778e-7,
              "lpFee": {
                "amount": 30,
                "mint": "EPjFWdd5AufqSSqeM2qN1xzybapC8G4wEGGkZwyTDt1v",
                "pct": 0.003
              },
              "platformFee": {
                "amount": 0,
                "mint": "MNDEFzGvMt87ueuHvVU9VcTqsAP5b3fTGPsHuuPA5ey",
                "pct": 0
              }
            }
          ]
        }
      ],
      "timeTaken": 0.044471802000089156
    })");
  base::RunLoop run_loop;

  auto expected_response = mojom::JupiterQuote::New();
  auto& expected_route =
      expected_response->routes.emplace_back(mojom::JupiterRoute::New());
  expected_route->in_amount = 10000ULL;
  expected_route->out_amount = 261273ULL;
  expected_route->amount = 10000ULL;
  expected_route->other_amount_threshold = 258660ULL;
  expected_route->swap_mode = "ExactIn";
  expected_route->price_impact_pct = 0.008955716118219659;
  expected_route->slippage_bps = 50;
  auto& expected_market_info = expected_route->market_infos.emplace_back(
      mojom::JupiterMarketInfo::New());
  expected_market_info->id = "2yNwARmTmc3NzYMETCZQjAE5GGCPgviH6hiBsxaeikTK";
  expected_market_info->label = "Orca";
  expected_market_info->input_mint =
      "EPjFWdd5AufqSSqeM2qN1xzybapC8G4wEGGkZwyTDt1v";
  expected_market_info->output_mint =
      "MNDEFzGvMt87ueuHvVU9VcTqsAP5b3fTGPsHuuPA5ey";
  expected_market_info->not_enough_liquidity = false;
  expected_market_info->in_amount = 10000ULL;
  expected_market_info->out_amount = 117001203ULL;
  expected_market_info->price_impact_pct = 1.196568750220778e-7;
  expected_market_info->lp_fee = mojom::JupiterFee::New();
  expected_market_info->lp_fee->amount = 30ULL;
  expected_market_info->lp_fee->mint =
      "EPjFWdd5AufqSSqeM2qN1xzybapC8G4wEGGkZwyTDt1v";
  expected_market_info->lp_fee->pct = 0.003;
  expected_market_info->platform_fee = mojom::JupiterFee::New();
  expected_market_info->platform_fee->amount = 0ULL;
  expected_market_info->platform_fee->mint =
      "MNDEFzGvMt87ueuHvVU9VcTqsAP5b3fTGPsHuuPA5ey";
  expected_market_info->platform_fee->pct = 0;

  base::MockCallback<mojom::SwapService::GetJupiterQuoteCallback> callback;
  EXPECT_CALL(callback, Run(EqualsMojo(expected_response),
                            EqualsMojo(mojom::JupiterErrorResponsePtr()), ""));
  swap_service_->GetJupiterQuote(GetCannedJupiterQuoteParams(), callback.Get());
  base::RunLoop().RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&callback);

  // KO: empty JSON for conversion
  TestGetJupiterQuoteCase(R"({})", false);

  // KO: invalid JSON
  TestGetJupiterQuoteCase(R"(foo)", false);

  // Test inAmount uint64 value
  TestGetJupiterQuoteUint64Cases(R"(
    {
      "data": [
        {
          "inAmount": %s,
          "outAmount": 261273,
          "amount": 10000,
          "otherAmountThreshold": 258660,
          "swapMode": "ExactIn",
          "priceImpactPct": 0.008955716118219659,
          "slippageBps": 50,
          "marketInfos": []
        }
      ],
      "timeTaken": 0.044471802000089156
    })");

  // Test outAmount uint64 value
  TestGetJupiterQuoteUint64Cases(R"(
    {
      "data": [
        {
          "inAmount": 10000,
          "outAmount": %s,
          "amount": 10000,
          "otherAmountThreshold": 258660,
          "swapMode": "ExactIn",
          "priceImpactPct": 0.008955716118219659,
          "slippageBps": 50,
          "marketInfos": []
        }
      ],
      "timeTaken": 0.044471802000089156
    })");

  // Test amount uint64 value
  TestGetJupiterQuoteUint64Cases(R"(
    {
      "data": [
        {
          "inAmount": 10000,
          "outAmount": 261273,
          "amount": %s,
          "otherAmountThreshold": 258660,
          "swapMode": "ExactIn",
          "priceImpactPct": 0.008955716118219659,
          "slippageBps": 50,
          "marketInfos": []
        }
      ],
      "timeTaken": 0.044471802000089156
    })");

  // Test otherAmountThreshold uint64 value
  TestGetJupiterQuoteUint64Cases(R"(
    {
      "data": [
        {
          "inAmount": 10000,
          "outAmount": 261273,
          "amount": 10000,
          "otherAmountThreshold": %s,
          "swapMode": "ExactIn",
          "priceImpactPct": 0.008955716118219659,
          "slippageBps": 50,
          "marketInfos": []
        }
      ],
      "timeTaken": 0.044471802000089156
    })");

  // Test marketInfos->inAmount uint64 value
  TestGetJupiterQuoteUint64Cases(R"(
    {
      "data": [
        {
          "inAmount": 10000,
          "outAmount": 261273,
          "amount": 10000,
          "otherAmountThreshold": 258660,
          "swapMode": "ExactIn",
          "priceImpactPct": 0.008955716118219659,
          "slippageBps": 50,
          "marketInfos": [
            {
              "id": "2yNwARmTmc3NzYMETCZQjAE5GGCPgviH6hiBsxaeikTK",
              "label": "Orca",
              "inputMint": "EPjFWdd5AufqSSqeM2qN1xzybapC8G4wEGGkZwyTDt1v",
              "outputMint": "MNDEFzGvMt87ueuHvVU9VcTqsAP5b3fTGPsHuuPA5ey",
              "notEnoughLiquidity": false,
              "inAmount": %s,
              "outAmount": 117001203,
              "priceImpactPct": 1.196568750220778e-7,
              "lpFee": {
                "amount": 30,
                "mint": "EPjFWdd5AufqSSqeM2qN1xzybapC8G4wEGGkZwyTDt1v",
                "pct": 0.003
              },
              "platformFee": {
                "amount": 0,
                "mint": "MNDEFzGvMt87ueuHvVU9VcTqsAP5b3fTGPsHuuPA5ey",
                "pct": 0
              }
            }
          ]
        }
      ],
      "timeTaken": 0.044471802000089156
    })");

  // Test marketInfos->outAmount uint64 value
  TestGetJupiterQuoteUint64Cases(R"(
    {
      "data": [
        {
          "inAmount": 10000,
          "outAmount": 261273,
          "amount": 10000,
          "otherAmountThreshold": 258660,
          "swapMode": "ExactIn",
          "priceImpactPct": 0.008955716118219659,
          "slippageBps": 50,
          "marketInfos": [
            {
              "id": "2yNwARmTmc3NzYMETCZQjAE5GGCPgviH6hiBsxaeikTK",
              "label": "Orca",
              "inputMint": "EPjFWdd5AufqSSqeM2qN1xzybapC8G4wEGGkZwyTDt1v",
              "outputMint": "MNDEFzGvMt87ueuHvVU9VcTqsAP5b3fTGPsHuuPA5ey",
              "notEnoughLiquidity": false,
              "inAmount": 10000,
              "outAmount": %s,
              "priceImpactPct": 1.196568750220778e-7,
              "lpFee": {
                "amount": 30,
                "mint": "EPjFWdd5AufqSSqeM2qN1xzybapC8G4wEGGkZwyTDt1v",
                "pct": 0.003
              },
              "platformFee": {
                "amount": 0,
                "mint": "MNDEFzGvMt87ueuHvVU9VcTqsAP5b3fTGPsHuuPA5ey",
                "pct": 0
              }
            }
          ]
        }
      ],
      "timeTaken": 0.044471802000089156
    })");

  // Test lpFee->amount uint64 value
  TestGetJupiterQuoteUint64Cases(R"(
    {
      "data": [
        {
          "inAmount": 10000,
          "outAmount": 261273,
          "amount": 10000,
          "otherAmountThreshold": 258660,
          "swapMode": "ExactIn",
          "priceImpactPct": 0.008955716118219659,
          "slippageBps": 50,
          "marketInfos": [
            {
              "id": "2yNwARmTmc3NzYMETCZQjAE5GGCPgviH6hiBsxaeikTK",
              "label": "Orca",
              "inputMint": "EPjFWdd5AufqSSqeM2qN1xzybapC8G4wEGGkZwyTDt1v",
              "outputMint": "MNDEFzGvMt87ueuHvVU9VcTqsAP5b3fTGPsHuuPA5ey",
              "notEnoughLiquidity": false,
              "inAmount": 10000,
              "outAmount": 10000,
              "priceImpactPct": 1.196568750220778e-7,
              "lpFee": {
                "amount": %s,
                "mint": "EPjFWdd5AufqSSqeM2qN1xzybapC8G4wEGGkZwyTDt1v",
                "pct": 0.003
              },
              "platformFee": {
                "amount": 0,
                "mint": "MNDEFzGvMt87ueuHvVU9VcTqsAP5b3fTGPsHuuPA5ey",
                "pct": 0
              }
            }
          ]
        }
      ],
      "timeTaken": 0.044471802000089156
    })");

  // Test platformFee->amount  uint64 value
  TestGetJupiterQuoteUint64Cases(R"(
    {
      "data": [
        {
          "inAmount": 10000,
          "outAmount": 261273,
          "amount": 10000,
          "otherAmountThreshold": 258660,
          "swapMode": "ExactIn",
          "priceImpactPct": 0.008955716118219659,
          "slippageBps": 50,
          "marketInfos": [
            {
              "id": "2yNwARmTmc3NzYMETCZQjAE5GGCPgviH6hiBsxaeikTK",
              "label": "Orca",
              "inputMint": "EPjFWdd5AufqSSqeM2qN1xzybapC8G4wEGGkZwyTDt1v",
              "outputMint": "MNDEFzGvMt87ueuHvVU9VcTqsAP5b3fTGPsHuuPA5ey",
              "notEnoughLiquidity": false,
              "inAmount": 10000,
              "outAmount": 10000,
              "priceImpactPct": 1.196568750220778e-7,
              "lpFee": {
                "amount": 30,
                "mint": "EPjFWdd5AufqSSqeM2qN1xzybapC8G4wEGGkZwyTDt1v",
                "pct": 0.003
              },
              "platformFee": {
                "amount": %s,
                "mint": "MNDEFzGvMt87ueuHvVU9VcTqsAP5b3fTGPsHuuPA5ey",
                "pct": 0
              }
            }
          ]
        }
      ],
      "timeTaken": 0.044471802000089156
    })");
}

TEST_F(SwapServiceUnitTest, GetJupiterSwapTransactions) {
  SetInterceptor(R"(
    {
      "swapTransaction": "bar"
    })");

  auto expected_response = mojom::JupiterSwapTransactions::New("foo");
  // OK: valid case
  TestGetJupiterSwapTransactions(true, std::move(expected_response), false);

  // KO: invalid output mint
  TestGetJupiterSwapTransactions(false, nullptr, true, "invalid output mint");

  // KO: invalid JSON
  SetInterceptor(R"(foo)");
  TestGetJupiterSwapTransactions(false, nullptr, true);
}

TEST_F(SwapServiceUnitTest, GetBraveFee) {
  auto params = mojom::BraveSwapFeeParams::New();
  auto expected_response = mojom::BraveSwapFeeResponse::New();

  base::RunLoop run_loop;

  // Case 1: 0x swap on Ethereum
  //         1000 DAI -> ETH
  params->chain_id = mojom::kMainnetChainId;
  params->input_token = "0x6b175474e89094c44da98b954eedeac495271d0f";
  params->output_token = "0xeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee";
  params->taker = "0xa92D461a9a988A7f11ec285d39783A637Fdd6ba4";

  expected_response->fee_param = "0.00875";
  expected_response->protocol_fee_pct = "0.15";
  expected_response->brave_fee_pct = "0.875";
  expected_response->discount_on_brave_fee_pct = "0";
  expected_response->effective_fee_pct = "0.875";
  expected_response->discount_code = mojom::DiscountCode::kNone;
  expected_response->has_brave_fee = true;

  base::MockCallback<mojom::SwapService::GetBraveFeeCallback> callback;
  EXPECT_CALL(callback, Run(EqualsMojo(expected_response), ""));
  swap_service_->GetBraveFee(params->Clone(), callback.Get());
  base::RunLoop().RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&callback);

  // Case 2: Jupiter swap on Solana (no fees)
  //         1000 SOL -> RAY
  params->chain_id = mojom::kSolanaMainnet;
  params->input_token = "So11111111111111111111111111111111111111112";
  params->output_token = "4k3Dyjzvzp8eMZWUXbBCjEvwSkkk59S5iCNLY3QrkX6Rxxx";
  params->taker = "LUKAzPV8dDbVykTVT14pCGKzFfNcgZgRbAXB8AGdKx3";

  expected_response->fee_param = "85";
  expected_response->protocol_fee_pct = "0";
  expected_response->brave_fee_pct = "0.85";
  expected_response->discount_on_brave_fee_pct = "100";
  expected_response->effective_fee_pct = "0";
  expected_response->discount_code =
      mojom::DiscountCode::kUnknownJupiterOutputMint;
  expected_response->has_brave_fee = false;

  EXPECT_CALL(callback, Run(EqualsMojo(expected_response), ""));
  swap_service_->GetBraveFee(params->Clone(), callback.Get());
  base::RunLoop().RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&callback);

  // Case 3: Jupiter swap on Solana (fees)
  //         1000 SOL -> USDC
  params->chain_id = mojom::kSolanaMainnet;
  params->input_token = "So11111111111111111111111111111111111111112";
  params->output_token = "EPjFWdd5AufqSSqeM2qN1xzybapC8G4wEGGkZwyTDt1v";
  params->taker = "LUKAzPV8dDbVykTVT14pCGKzFfNcgZgRbAXB8AGdKx3";

  expected_response->fee_param = "85";
  expected_response->protocol_fee_pct = "0";
  expected_response->brave_fee_pct = "0.85";
  expected_response->discount_on_brave_fee_pct = "0";
  expected_response->effective_fee_pct = "0.85";
  expected_response->discount_code = mojom::DiscountCode::kNone;
  expected_response->has_brave_fee = true;

  EXPECT_CALL(callback, Run(EqualsMojo(expected_response), ""));
  swap_service_->GetBraveFee(params->Clone(), callback.Get());
  base::RunLoop().RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&callback);

  // Case 4: Unsupported network
  //         1000 DAI -> ETH
  params->chain_id = mojom::kAuroraMainnetChainId;
  EXPECT_CALL(callback,
              Run(EqualsMojo(mojom::BraveSwapFeeResponsePtr()),
                  l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR)));
  swap_service_->GetBraveFee(params->Clone(), callback.Get());
  base::RunLoop().RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&callback);
}

}  // namespace brave_wallet
