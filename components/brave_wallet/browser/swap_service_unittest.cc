/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>
#include <utility>

#include "base/memory/ref_counted.h"
#include "base/memory/scoped_refptr.h"
#include "base/test/bind.h"
#include "base/test/mock_callback.h"
#include "base/test/task_environment.h"
#include "brave/components/brave_wallet/browser/brave_wallet_prefs.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/browser/swap_response_parser.h"
#include "brave/components/brave_wallet/browser/swap_service.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "components/grit/brave_components_strings.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "services/data_decoder/public/cpp/test_support/in_process_data_decoder.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/l10n/l10n_util.h"

namespace {

// Matcher to check equality of two mojo structs. Matcher needs copyable value
// which is not possible for some mojo types, so wrapping it with RefCounted.
template <typename T>
auto EqualsMojo(const T& value) {
  return testing::Truly(
      [value = base::MakeRefCounted<base::RefCountedData<T>>(value.Clone())](
          const T& candidate) { return mojo::Equals(candidate, value->data); });
}

auto IsTruthy(bool truthy) {
  return testing::Truly(
      [=](const auto& candidate) { return !!candidate == truthy; });
}

brave_wallet::mojom::SwapParamsPtr GetCannedSwapParams() {
  auto params = brave_wallet::mojom::SwapParams::New();
  params->buy_token = "ETH";
  params->sell_token = "DAI";
  params->buy_amount = "1000000000000000000000";
  return params;
}

brave_wallet::mojom::JupiterQuoteParamsPtr GetCannedJupiterQuoteParams() {
  auto params = brave_wallet::mojom::JupiterQuoteParams::New();
  params->output_mint = "EPjFWdd5AufqSSqeM2qN1xzybapC8G4wEGGkZwyTDt1v";
  params->input_mint = "So11111111111111111111111111111111111111112";
  params->amount = "10000";
  params->slippage_percentage = 0.5;
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

  void TestSwapConfigurationMainnet(const std::string& chain_id,
                                    const std::string& expected_swap_api_url) {
    std::string buy_token_percantage_fee = "0.00875";
    std::string fee_recipient = "0xbd9420A98a7Bd6B89765e5715e169481602D9c3d";
    std::string affiliate_address =
        "0xbd9420A98a7Bd6B89765e5715e169481602D9c3d";

    EXPECT_EQ(expected_swap_api_url, SwapService::GetBaseSwapURL(chain_id));
    EXPECT_EQ(buy_token_percantage_fee, SwapService::GetFee(chain_id));
    EXPECT_EQ(fee_recipient, SwapService::GetFeeRecipient(chain_id));
    EXPECT_EQ(affiliate_address, SwapService::GetAffiliateAddress(chain_id));
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
      "buyTokenToEthRate":"1",
      "estimatedPriceImpact": "0.7232",
      "sources": [
        {
          "name": "Uniswap_V2",
          "proportion": "1"
        }
      ]
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

  base::MockCallback<mojom::SwapService::GetPriceQuoteCallback> callback;
  EXPECT_CALL(callback, Run(EqualsMojo(expected_swap_response),
                            EqualsMojo(mojom::SwapErrorResponsePtr()), ""));

  swap_service_->GetPriceQuote(GetCannedSwapParams(), callback.Get());
  base::RunLoop().RunUntilIdle();
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
  EXPECT_CALL(callback, Run(EqualsMojo(mojom::SwapResponsePtr()),
                            EqualsMojo(ParseSwapErrorResponse(error)), ""));

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
      "buyTokenToEthRate":"1",
      "estimatedPriceImpact": "0.7232",
      "sources": [
        {
          "name": "Uniswap_V2",
          "proportion": "1"
        }
      ]
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

  base::MockCallback<mojom::SwapService::GetTransactionPayloadCallback>
      callback;
  EXPECT_CALL(callback, Run(EqualsMojo(expected_swap_response),
                            EqualsMojo(mojom::SwapErrorResponsePtr()), ""));

  swap_service_->GetTransactionPayload(GetCannedSwapParams(), callback.Get());
  base::RunLoop().RunUntilIdle();
}

TEST_F(SwapServiceUnitTest, GetTransactionPayloadError) {
  std::string error =
      R"({"code":100,"reason":"Validation Failed","validationErrors":[{"code":1000,"field":"sellAmount","reason":"should have required property 'sellAmount'"},{"code":1000,"field":"buyAmount","reason":"should have required property 'buyAmount'"},{"code":1001,"field":"","reason":"should match exactly one schema in oneOf"}]})";
  SetErrorInterceptor(error);

  base::MockCallback<mojom::SwapService::GetTransactionPayloadCallback>
      callback;
  auto expected_swap_error_response = ParseSwapErrorResponse(error);
  EXPECT_CALL(callback, Run(EqualsMojo(mojom::SwapResponsePtr()),
                            EqualsMojo(expected_swap_error_response), ""));

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

TEST_F(SwapServiceUnitTest, GetSwapConfigurationGoerli) {
  std::string swap_api_url = "https://goerli.api.0x.org/";
  std::string buy_token_percantage_fee = "0.00875";
  std::string fee_recipient = "0xa92D461a9a988A7f11ec285d39783A637Fdd6ba4";
  std::string affiliate_address;
  EXPECT_EQ(swap_api_url, SwapService::GetBaseSwapURL(mojom::kGoerliChainId));
  EXPECT_EQ(buy_token_percantage_fee,
            SwapService::GetFee(mojom::kGoerliChainId));
  EXPECT_EQ(fee_recipient, SwapService::GetFeeRecipient(mojom::kGoerliChainId));
  EXPECT_EQ(affiliate_address,
            SwapService::GetAffiliateAddress(mojom::kGoerliChainId));
}

TEST_F(SwapServiceUnitTest, GetSwapConfigurationMainnet) {
  TestSwapConfigurationMainnet(mojom::kMainnetChainId, "https://api.0x.org/");
  TestSwapConfigurationMainnet(mojom::kPolygonMainnetChainId,
                               "https://polygon.api.0x.org/");
  TestSwapConfigurationMainnet(mojom::kBinanceSmartChainMainnetChainId,
                               "https://bsc.api.0x.org/");
  TestSwapConfigurationMainnet(mojom::kAvalancheMainnetChainId,
                               "https://avalanche.api.0x.org/");
  TestSwapConfigurationMainnet(mojom::kFantomMainnetChainId,
                               "https://fantom.api.0x.org/");
  TestSwapConfigurationMainnet(mojom::kCeloMainnetChainId,
                               "https://celo.api.0x.org/");
  TestSwapConfigurationMainnet(mojom::kOptimismMainnetChainId,
                               "https://optimism.api.0x.org/");
  TestSwapConfigurationMainnet(mojom::kArbitrumMainnetChainId,
                               "https://arbitrum.api.0x.org/");
}

TEST_F(SwapServiceUnitTest, GetSwapConfigurationOtherNet) {
  std::string swap_api_url;
  std::string buy_token_percantage_fee;
  std::string fee_recipient;
  std::string affiliate_address;
  EXPECT_EQ(swap_api_url, SwapService::GetBaseSwapURL("0x3"));
  EXPECT_EQ(buy_token_percantage_fee, SwapService::GetFee("0x3"));
  EXPECT_EQ(fee_recipient, SwapService::GetFeeRecipient("0x3"));
  EXPECT_EQ(affiliate_address, SwapService::GetAffiliateAddress("0x3"));
  EXPECT_EQ(swap_api_url, SwapService::GetBaseSwapURL("0x4"));
  EXPECT_EQ(buy_token_percantage_fee, SwapService::GetFee("0x4"));
  EXPECT_EQ(fee_recipient, SwapService::GetFeeRecipient("0x4"));
  EXPECT_EQ(affiliate_address, SwapService::GetAffiliateAddress("0x4"));
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
  ASSERT_EQ(url,
            "https://quote-api.jup.ag/v1/quote?"
            "inputMint=So11111111111111111111111111111111111111112&"
            "outputMint=EPjFWdd5AufqSSqeM2qN1xzybapC8G4wEGGkZwyTDt1v&"
            "amount=10000&"
            "feeBps=85&"
            "slippage=0.500000&"
            "onlyDirectRoutes=true");

  params->output_mint = "SHDWyBxihqiCj6YekG2GUr7wqKLeLAMK1gHZck9pL6y";
  url =
      swap_service_->GetJupiterQuoteURL(params.Clone(), mojom::kSolanaMainnet);

  // OK: output mint does not have Jupiter fees
  ASSERT_EQ(url,
            "https://quote-api.jup.ag/v1/quote?"
            "inputMint=So11111111111111111111111111111111111111112&"
            "outputMint=SHDWyBxihqiCj6YekG2GUr7wqKLeLAMK1gHZck9pL6y&"
            "amount=10000&"
            "slippage=0.500000&"
            "onlyDirectRoutes=true");
}

TEST_F(SwapServiceUnitTest, GetJupiterSwapTransactionsURL) {
  auto url =
      swap_service_->GetJupiterSwapTransactionsURL(mojom::kSolanaMainnet);
  ASSERT_EQ(url, "https://quote-api.jup.ag/v1/swap");
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
          "outAmountWithSlippage": 258660,
          "swapMode": "ExactIn",
          "priceImpactPct": 0.008955716118219659,
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
          "outAmountWithSlippage": 258660,
          "swapMode": "ExactIn",
          "priceImpactPct": 0.008955716118219659,
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
          "outAmountWithSlippage": 258660,
          "swapMode": "ExactIn",
          "priceImpactPct": 0.008955716118219659,
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
          "outAmountWithSlippage": 258660,
          "swapMode": "ExactIn",
          "priceImpactPct": 0.008955716118219659,
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
          "outAmountWithSlippage": 258660,
          "swapMode": "ExactIn",
          "priceImpactPct": 0.008955716118219659,
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
          "outAmountWithSlippage": 258660,
          "swapMode": "ExactIn",
          "priceImpactPct": 0.008955716118219659,
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
          "outAmountWithSlippage": 258660,
          "swapMode": "ExactIn",
          "priceImpactPct": 0.008955716118219659,
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
          "outAmountWithSlippage": 258660,
          "swapMode": "ExactIn",
          "priceImpactPct": 0.008955716118219659,
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
          "outAmountWithSlippage": 258660,
          "swapMode": "ExactIn",
          "priceImpactPct": 0.008955716118219659,
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
      "setupTransaction": "foo",
      "swapTransaction": "bar",
      "cleanupTransaction": "baz"
    })");

  auto expected_response =
      mojom::JupiterSwapTransactions::New("foo", "bar", "baz");
  // OK: valid case
  TestGetJupiterSwapTransactions(true, std::move(expected_response), false);

  // KO: invalid output mint
  TestGetJupiterSwapTransactions(false, nullptr, true, "invalid output mint");

  // KO: invalid JSON
  SetInterceptor(R"(foo)");
  TestGetJupiterSwapTransactions(false, nullptr, true);
}

}  // namespace brave_wallet
