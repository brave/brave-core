/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/swap_service.h"

#include <map>
#include <memory>
#include <utility>
#include <vector>

#include "base/memory/ref_counted.h"
#include "base/memory/scoped_refptr.h"
#include "base/test/bind.h"
#include "base/test/mock_callback.h"
#include "base/test/task_environment.h"
#include "base/test/values_test_util.h"
#include "brave/components/brave_wallet/browser/brave_wallet_prefs.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/browser/swap_response_parser.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom-shared.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/common_utils.h"
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

}  // namespace

namespace brave_wallet {

namespace {

mojom::SwapQuoteParamsPtr GetCannedSwapQuoteParams(
    mojom::CoinType from_coin,
    const std::string& from_chain_id,
    const std::string& from_token,
    mojom::CoinType to_coin,
    const std::string& to_chain_id,
    const std::string& to_token,
    mojom::SwapProvider provider) {
  auto params = mojom::SwapQuoteParams::New();

  params->from_account_id = MakeAccountId(
      from_coin, mojom::KeyringId::kDefault, mojom::AccountKind::kDerived,
      from_coin == mojom::CoinType::ETH
          ? "0xa92D461a9a988A7f11ec285d39783A637Fdd6ba4"
          : "S5ARSDD3ddZqqqqqb2EUE2h2F1XQHBk7bErRW1WPGe4");
  params->from_chain_id = from_chain_id;
  params->from_token = from_token;
  params->from_amount = "1000000000000000000000";

  params->to_account_id = MakeAccountId(
      to_coin, mojom::KeyringId::kDefault, mojom::AccountKind::kDerived,
      to_coin == mojom::CoinType::ETH
          ? "0xa92D461a9a988A7f11ec285d39783A637Fdd6ba4"
          : "S5ARSDD3ddZqqqqqb2EUE2h2F1XQHBk7bErRW1WPGe4");
  params->to_chain_id = to_chain_id;
  params->to_token = to_token;

  params->slippage_percentage = "3";
  params->route_priority = mojom::RoutePriority::kFastest;
  params->provider = provider;
  return params;
}

mojom::SwapTransactionParamsUnionPtr GetCannedJupiterTransactionParams(
    const std::string& output_mint) {
  auto params = mojom::JupiterTransactionParams::New();

  auto quote = brave_wallet::mojom::JupiterQuote::New();
  quote->input_mint = "So11111111111111111111111111111111111111112";
  quote->in_amount = "100000000";
  quote->output_mint = output_mint;
  quote->out_amount = "10886298";
  quote->other_amount_threshold = "10885210";
  quote->swap_mode = "ExactIn";
  quote->slippage_bps = "1";
  quote->price_impact_pct = "0.008955716118219659";

  auto platform_fee = brave_wallet::mojom::JupiterPlatformFee::New();
  platform_fee->amount = "93326";
  platform_fee->fee_bps = "85";
  quote->platform_fee = std::move(platform_fee);

  auto swap_info_1 = brave_wallet::mojom::JupiterSwapInfo::New();
  swap_info_1->amm_key = "EiEAydLqSKFqRPpuwYoVxEJ6h9UZh9tsTaHgs4f8b8Z5";
  swap_info_1->label = "Lifinity V2";
  swap_info_1->input_mint = "So11111111111111111111111111111111111111112";
  swap_info_1->output_mint = "Es9vMFrzaCERmJfrF4H2FYD4KCoNkY11McCe8BenwNYB";
  swap_info_1->in_amount = "100000000";
  swap_info_1->out_amount = "10964919";
  swap_info_1->fee_amount = "20000";
  swap_info_1->fee_mint = "So11111111111111111111111111111111111111112";
  auto step_1 = brave_wallet::mojom::JupiterRouteStep::New();
  step_1->percent = "100";
  step_1->swap_info = std::move(swap_info_1);

  auto swap_info_2 = brave_wallet::mojom::JupiterSwapInfo::New();
  swap_info_2->amm_key = "UXD3M3N6Hn1JjbxugKguhJVHbYm8zHvdF5pNf7dumd5";
  swap_info_2->label = "Mercurial";
  swap_info_2->input_mint = "Es9vMFrzaCERmJfrF4H2FYD4KCoNkY11McCe8BenwNYB";
  swap_info_2->output_mint = output_mint;
  swap_info_2->in_amount = "10964919";
  swap_info_2->out_amount = "10979624";
  swap_info_2->fee_amount = "1098";
  swap_info_2->fee_mint = output_mint;
  auto step_2 = brave_wallet::mojom::JupiterRouteStep::New();
  step_2->percent = "100";
  step_2->swap_info = std::move(swap_info_2);

  quote->route_plan.push_back(step_1.Clone());
  quote->route_plan.push_back(step_2.Clone());

  params->quote = quote.Clone();
  params->user_public_key = "S5ARSDD3ddZqqqqqb2EUE2h2F1XQHBk7bErRW1WPGe4";
  params->chain_id = mojom::kSolanaMainnet;

  return mojom::SwapTransactionParamsUnion::NewJupiterTransactionParams(
      std::move(params));
}

mojom::LiFiQuotePtr GetCannedLiFiQuote() {
  auto quote = mojom::LiFiQuote::New();
  quote->routes.push_back(mojom::LiFiRoute::New());
  quote->routes[0]->id =
      "0x9a448018e09b62da15c1bd64571c21b33cb177cee5d2f07c325d6485364362a5";

  auto from_token = mojom::BlockchainToken::New();
  from_token->contract_address = "0x2791Bca1f2de4661ED88A30C99A7a9449Aa84174";
  from_token->name = "USDC.e";
  from_token->logo = "usdce.png";
  from_token->symbol = "USDCe";
  from_token->decimals = 6;
  from_token->chain_id = mojom::kPolygonMainnetChainId;
  from_token->coin = mojom::CoinType::ETH;
  quote->routes[0]->from_token = from_token.Clone();

  quote->routes[0]->from_amount = "2000000";
  quote->routes[0]->from_address = "0x552008c0f6870c2f77e5cC1d2eb9bdff03e30Ea0";

  auto to_token = mojom::BlockchainToken::New();
  to_token->contract_address = "EPjFWdd5AufqSSqeM2qN1xzybapC8G4wEGGkZwyTDt1v";
  to_token->name = "USD Coin";
  to_token->logo = "usdc.png";
  to_token->symbol = "USDC";
  to_token->decimals = 6;
  to_token->chain_id = mojom::kSolanaMainnet;
  to_token->coin = mojom::CoinType::SOL;
  quote->routes[0]->to_token = to_token.Clone();

  quote->routes[0]->to_amount = "1138627";
  quote->routes[0]->to_amount_min = "1136350";
  quote->routes[0]->to_address = "S5ARSDD3ddZqqqqqb2EUE2h2F1XQHBk7bErRW1WPGe4";

  quote->routes[0]->steps.push_back(mojom::LiFiStep::New());
  quote->routes[0]->steps[0]->id = "57d247fc-d80a-4f4a-9596-72db3061aa72";
  quote->routes[0]->steps[0]->type = mojom::LiFiStepType::kLiFi;
  quote->routes[0]->steps[0]->tool = "allbridge";
  auto tool_details = mojom::LiFiToolDetails::New();
  tool_details->key = "allbridge";
  tool_details->name = "Allbridge";
  tool_details->logo = "allbridge.png";
  quote->routes[0]->steps[0]->tool_details = tool_details.Clone();

  quote->routes[0]->steps[0]->action = mojom::LiFiAction::New();
  quote->routes[0]->steps[0]->action->from_token = from_token.Clone();
  quote->routes[0]->steps[0]->action->from_amount = "2000000";
  quote->routes[0]->steps[0]->action->to_token = to_token.Clone();
  quote->routes[0]->steps[0]->action->slippage = "0.03";
  quote->routes[0]->steps[0]->action->from_address =
      "0x552008c0f6870c2f77e5cC1d2eb9bdff03e30Ea0";
  quote->routes[0]->steps[0]->action->to_address =
      "S5ARSDD3ddZqqqqqb2EUE2h2F1XQHBk7bErRW1WPGe4";

  quote->routes[0]->steps[0]->estimate = mojom::LiFiStepEstimate::New();
  quote->routes[0]->steps[0]->estimate->tool = "allbridge";
  quote->routes[0]->steps[0]->estimate->from_amount = "2000000";
  quote->routes[0]->steps[0]->estimate->to_amount = "1138627";
  quote->routes[0]->steps[0]->estimate->to_amount_min = "1136350";
  quote->routes[0]->steps[0]->estimate->approval_address =
      "0x1231DEB6f5749EF6cE6943a275A1D3E7486F4EaE";
  quote->routes[0]->steps[0]->estimate->execution_duration = "500.298";

  auto fee_cost = mojom::LiFiFeeCost::New();
  fee_cost->name = "Allbridge's fee";
  fee_cost->description =
      "AllBridge fee and messenger fee charged by Allbridge";
  fee_cost->percentage = "0.4267";
  fee_cost->amount = "853380";
  fee_cost->included = true;
  fee_cost->token = from_token.Clone();

  quote->routes[0]->steps[0]->estimate->fee_costs.emplace(
      std::vector<mojom::LiFiFeeCostPtr>());
  quote->routes[0]->steps[0]->estimate->fee_costs->push_back(fee_cost.Clone());

  auto gas_cost = mojom::LiFiGasCost::New();
  gas_cost->type = "SEND";
  gas_cost->estimate = "185000";
  gas_cost->limit = "277500";
  gas_cost->amount = "20720000000000000";

  auto matic = mojom::BlockchainToken::New();
  matic->contract_address = "";
  matic->name = "MATIC";
  matic->logo = "matic.png";
  matic->symbol = "MATIC";
  matic->decimals = 18;
  matic->chain_id = mojom::kPolygonMainnetChainId;
  matic->coin = mojom::CoinType::ETH;
  gas_cost->token = matic.Clone();
  quote->routes[0]->steps[0]->estimate->gas_costs.push_back(gas_cost.Clone());

  quote->routes[0]->steps[0]->included_steps =
      std::vector<mojom::LiFiStepPtr>();
  quote->routes[0]->steps[0]->included_steps->push_back(mojom::LiFiStep::New());
  quote->routes[0]->steps[0]->included_steps->at(0)->id =
      "1b914bef-e1be-4895-a9b1-c57da16d9de5";
  quote->routes[0]->steps[0]->included_steps->at(0)->type =
      mojom::LiFiStepType::kCross;
  quote->routes[0]->steps[0]->included_steps->at(0)->tool = "allbridge";
  quote->routes[0]->steps[0]->included_steps->at(0)->tool_details =
      tool_details.Clone();

  quote->routes[0]->steps[0]->included_steps->at(0)->action =
      mojom::LiFiAction::New();
  quote->routes[0]->steps[0]->included_steps->at(0)->action->from_token =
      from_token.Clone();
  quote->routes[0]->steps[0]->included_steps->at(0)->action->from_amount =
      "2000000";
  quote->routes[0]->steps[0]->included_steps->at(0)->action->to_token =
      to_token.Clone();

  quote->routes[0]->steps[0]->included_steps->at(0)->action->slippage = "0.03";
  quote->routes[0]->steps[0]->included_steps->at(0)->action->from_address =
      "0x552008c0f6870c2f77e5cC1d2eb9bdff03e30Ea0";
  quote->routes[0]->steps[0]->included_steps->at(0)->action->to_address =
      "S5ARSDD3ddZqqqqqb2EUE2h2F1XQHBk7bErRW1WPGe4";

  quote->routes[0]->steps[0]->included_steps->at(0)->estimate =
      mojom::LiFiStepEstimate::New();
  quote->routes[0]->steps[0]->included_steps->at(0)->estimate->tool =
      "allbridge";
  quote->routes[0]->steps[0]->included_steps->at(0)->estimate->from_amount =
      "2000000";
  quote->routes[0]->steps[0]->included_steps->at(0)->estimate->to_amount =
      "1138627";
  quote->routes[0]->steps[0]->included_steps->at(0)->estimate->to_amount_min =
      "1136350";
  quote->routes[0]
      ->steps[0]
      ->included_steps->at(0)
      ->estimate->approval_address =
      "0x7775d63836987f444E2F14AA0fA2602204D7D3E0";
  quote->routes[0]
      ->steps[0]
      ->included_steps->at(0)
      ->estimate->execution_duration = "500.298";

  quote->routes[0]
      ->steps[0]
      ->included_steps->at(0)
      ->estimate->fee_costs.emplace(std::vector<mojom::LiFiFeeCostPtr>());
  quote->routes[0]
      ->steps[0]
      ->included_steps->at(0)
      ->estimate->fee_costs->push_back(fee_cost.Clone());

  quote->routes[0]
      ->steps[0]
      ->included_steps->at(0)
      ->estimate->gas_costs.push_back(gas_cost.Clone());

  quote->routes[0]->tags = {"CHEAPEST", "FASTEST"};
  quote->routes[0]->unique_id = "allbridge";

  return quote;
}

}  // namespace

class SwapServiceUnitTest : public testing::Test {
 public:
  SwapServiceUnitTest()
      : shared_url_loader_factory_(
            base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
                &url_loader_factory_)) {
    brave_wallet::RegisterProfilePrefs(prefs_.registry());
    swap_service_ = std::make_unique<SwapService>(shared_url_loader_factory_);
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

  void IsSwapSupported(const std::string& chain_id, bool expected_response) {
    base::MockCallback<mojom::SwapService::IsSwapSupportedCallback> callback;
    EXPECT_CALL(callback, Run(IsTruthy(expected_response)));
    swap_service_->IsSwapSupported(chain_id, callback.Get());
    task_environment_.RunUntilIdle();
  }

  void TestGetQuoteCase(const std::string& json,
                        mojom::CoinType from_coin,
                        const std::string& from_chain_id,
                        mojom::CoinType to_coin,
                        const std::string& to_chain_id,
                        const bool expected_success,
                        mojom::SwapProvider provider) {
    SetInterceptor(json);
    auto expected_error_string =
        expected_success ? ""
                         : l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR);
    base::MockCallback<mojom::SwapService::GetQuoteCallback> callback;
    EXPECT_CALL(
        callback,
        Run(IsTruthy(expected_success), IsTruthy(expected_success),
            EqualsMojo(mojom::SwapErrorUnionPtr()), expected_error_string));

    swap_service_->GetQuote(
        GetCannedSwapQuoteParams(from_coin, from_chain_id, "DAI", to_coin,
                                 to_chain_id, "ETH", provider),
        callback.Get());
    task_environment_.RunUntilIdle();
  }

  void TestGetJupiterTransaction(
      const bool expected_success,
      const std::string& expected_response,
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
    base::MockCallback<mojom::SwapService::GetTransactionCallback> callback;
    EXPECT_CALL(callback, Run(IsTruthy(expected_success),
                              EqualsMojo(mojom::SwapErrorUnionPtr()),
                              expected_error_string));

    swap_service_->GetTransaction(
        GetCannedJupiterTransactionParams(output_mint), callback.Get());
    task_environment_.RunUntilIdle();
  }

 protected:
  sync_preferences::TestingPrefServiceSyncable prefs_;
  std::unique_ptr<SwapService> swap_service_;
  base::test::TaskEnvironment task_environment_;

 private:
  network::TestURLLoaderFactory url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
  data_decoder::test::InProcessDataDecoder in_process_data_decoder_;
};

TEST_F(SwapServiceUnitTest, GetZeroExQuote) {
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

  auto expected_zero_ex_quote = mojom::ZeroExQuote::New();
  expected_zero_ex_quote->price = "1916.27547998814058355";
  expected_zero_ex_quote->value = "0";
  expected_zero_ex_quote->gas = "719000";
  expected_zero_ex_quote->estimated_gas = "719000";
  expected_zero_ex_quote->gas_price = "26000000000";
  expected_zero_ex_quote->protocol_fee = "0";
  expected_zero_ex_quote->minimum_protocol_fee = "0";
  expected_zero_ex_quote->buy_token_address =
      "0xeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee";
  expected_zero_ex_quote->sell_token_address =
      "0x6b175474e89094c44da98b954eedeac495271d0f";
  expected_zero_ex_quote->buy_amount = "1000000000000000000000";
  expected_zero_ex_quote->sell_amount = "1916275479988140583549706";
  expected_zero_ex_quote->allowance_target =
      "0xdef1c0ded9bec7f1a1670819833240f027b25eff";
  expected_zero_ex_quote->sell_token_to_eth_rate = "1900.44962824532464391";
  expected_zero_ex_quote->buy_token_to_eth_rate = "1";
  expected_zero_ex_quote->estimated_price_impact = "0.7232";

  auto source = brave_wallet::mojom::ZeroExSource::New();
  source->name = "Uniswap_V2";
  source->proportion = "1";
  expected_zero_ex_quote->sources.push_back(source.Clone());

  auto fees = brave_wallet::mojom::ZeroExFees::New();
  auto zero_ex_fee = brave_wallet::mojom::ZeroExFee::New();
  zero_ex_fee->fee_type = "volume";
  zero_ex_fee->fee_token = "0x8f3cf7ad23cd3cadbd9735aff958023239c6a063";
  zero_ex_fee->fee_amount = "148470027512868522";
  zero_ex_fee->billing_type = "on-chain";
  fees->zero_ex_fee = std::move(zero_ex_fee);
  expected_zero_ex_quote->fees = std::move(fees);

  auto expected_swap_fees = mojom::SwapFees::New();
  expected_swap_fees->fee_pct = "0";
  expected_swap_fees->discount_pct = "0";
  expected_swap_fees->effective_fee_pct = "0";
  expected_swap_fees->discount_code = mojom::SwapDiscountCode::kNone;
  expected_swap_fees->fee_param = "";

  base::MockCallback<mojom::SwapService::GetQuoteCallback> callback;
  EXPECT_CALL(callback, Run(EqualsMojo(mojom::SwapQuoteUnion::NewZeroExQuote(
                                expected_zero_ex_quote.Clone())),
                            EqualsMojo(expected_swap_fees.Clone()),
                            EqualsMojo(mojom::SwapErrorUnionPtr()), ""));

  swap_service_->GetQuote(
      GetCannedSwapQuoteParams(
          mojom::CoinType::ETH, mojom::kPolygonMainnetChainId, "DAI",
          mojom::CoinType::ETH, mojom::kPolygonMainnetChainId, "ETH",
          mojom::SwapProvider::kZeroEx),
      callback.Get());
  task_environment_.RunUntilIdle();
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

  expected_zero_ex_quote->fees->zero_ex_fee = nullptr;
  EXPECT_CALL(callback, Run(EqualsMojo(mojom::SwapQuoteUnion::NewZeroExQuote(
                                std::move(expected_zero_ex_quote))),
                            EqualsMojo(expected_swap_fees.Clone()),
                            EqualsMojo(mojom::SwapErrorUnionPtr()), ""));

  swap_service_->GetQuote(
      GetCannedSwapQuoteParams(
          mojom::CoinType::ETH, mojom::kPolygonMainnetChainId, "DAI",
          mojom::CoinType::ETH, mojom::kPolygonMainnetChainId, "ETH",
          mojom::SwapProvider::kZeroEx),
      callback.Get());
  task_environment_.RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&callback);
}

TEST_F(SwapServiceUnitTest, GetZeroExQuoteError) {
  std::string error = R"(
    {
      "code": "100",
      "reason": "Validation Failed",
      "validationErrors": [
        {
          "code": "1000",
          "field": "sellAmount",
          "reason": "should have required property 'sellAmount'"
        },
        {
          "code": "1000",
          "field": "buyAmount",
          "reason": "should have required property 'buyAmount'"
        },
        {
          "code": "1001",
          "field": "",
          "reason": "should match exactly one schema in oneOf"
        }
      ]
    })";
  SetErrorInterceptor(error);

  base::MockCallback<mojom::SwapService::GetQuoteCallback> callback;
  EXPECT_CALL(callback, Run(EqualsMojo(mojom::SwapQuoteUnionPtr()),
                            EqualsMojo(mojom::SwapFeesPtr()),
                            EqualsMojo(mojom::SwapErrorUnion::NewZeroExError(
                                zeroex::ParseErrorResponse(ParseJson(error)))),
                            ""));

  swap_service_->GetQuote(
      GetCannedSwapQuoteParams(
          mojom::CoinType::ETH, mojom::kPolygonMainnetChainId, "DAI",
          mojom::CoinType::ETH, mojom::kPolygonMainnetChainId, "ETH",
          mojom::SwapProvider::kZeroEx),
      callback.Get());
  task_environment_.RunUntilIdle();
}

TEST_F(SwapServiceUnitTest, GetZeroExQuoteUnexpectedReturn) {
  std::string unexpected_return = "Woot";
  SetInterceptor(unexpected_return);

  base::MockCallback<mojom::SwapService::GetQuoteCallback> callback;
  EXPECT_CALL(callback,
              Run(EqualsMojo(mojom::SwapQuoteUnionPtr()),
                  EqualsMojo(mojom::SwapFeesPtr()),
                  EqualsMojo(mojom::SwapErrorUnionPtr()),
                  l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR)));

  swap_service_->GetQuote(
      GetCannedSwapQuoteParams(
          mojom::CoinType::ETH, mojom::kPolygonMainnetChainId, "DAI",
          mojom::CoinType::ETH, mojom::kPolygonMainnetChainId, "ETH",
          mojom::SwapProvider::kZeroEx),
      callback.Get());
  task_environment_.RunUntilIdle();
}

TEST_F(SwapServiceUnitTest, GetZeroExTransaction) {
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

  auto expected_zero_ex_transaction = mojom::ZeroExQuote::New();
  expected_zero_ex_transaction->price = "1916.27547998814058355";
  expected_zero_ex_transaction->guaranteed_price = "1935.438234788021989386";
  expected_zero_ex_transaction->to =
      "0xdef1c0ded9bec7f1a1670819833240f027b25eff";
  expected_zero_ex_transaction->data = "0x0";
  expected_zero_ex_transaction->value = "0";
  expected_zero_ex_transaction->gas = "719000";
  expected_zero_ex_transaction->estimated_gas = "719000";
  expected_zero_ex_transaction->gas_price = "26000000000";
  expected_zero_ex_transaction->protocol_fee = "0";
  expected_zero_ex_transaction->minimum_protocol_fee = "0";
  expected_zero_ex_transaction->buy_token_address =
      "0xeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee";
  expected_zero_ex_transaction->sell_token_address =
      "0x6b175474e89094c44da98b954eedeac495271d0f";
  expected_zero_ex_transaction->buy_amount = "1000000000000000000000";
  expected_zero_ex_transaction->sell_amount = "1916275479988140583549706";
  expected_zero_ex_transaction->allowance_target =
      "0xdef1c0ded9bec7f1a1670819833240f027b25eff";
  expected_zero_ex_transaction->sell_token_to_eth_rate =
      "1900.44962824532464391";
  expected_zero_ex_transaction->buy_token_to_eth_rate = "1";
  expected_zero_ex_transaction->estimated_price_impact = "0.7232";
  auto source = brave_wallet::mojom::ZeroExSource::New();
  source->name = "Uniswap_V2";
  source->proportion = "1";
  expected_zero_ex_transaction->sources.push_back(source.Clone());

  auto fees = brave_wallet::mojom::ZeroExFees::New();
  auto zero_ex_fee = brave_wallet::mojom::ZeroExFee::New();
  zero_ex_fee->fee_type = "volume";
  zero_ex_fee->fee_token = "0x8f3cf7ad23cd3cadbd9735aff958023239c6a063";
  zero_ex_fee->fee_amount = "148470027512868522";
  zero_ex_fee->billing_type = "on-chain";
  fees->zero_ex_fee = std::move(zero_ex_fee);
  expected_zero_ex_transaction->fees = std::move(fees);

  base::MockCallback<mojom::SwapService::GetTransactionCallback> callback;
  EXPECT_CALL(callback,
              Run(EqualsMojo(mojom::SwapTransactionUnion::NewZeroExTransaction(
                      expected_zero_ex_transaction.Clone())),
                  EqualsMojo(mojom::SwapErrorUnionPtr()), ""));

  swap_service_->GetTransaction(
      mojom::SwapTransactionParamsUnion::NewZeroExTransactionParams(
          GetCannedSwapQuoteParams(
              mojom::CoinType::ETH, mojom::kPolygonMainnetChainId, "DAI",
              mojom::CoinType::ETH, mojom::kPolygonMainnetChainId, "ETH",
              mojom::SwapProvider::kZeroEx)),
      callback.Get());
  task_environment_.RunUntilIdle();
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

  expected_zero_ex_transaction->fees->zero_ex_fee = nullptr;
  EXPECT_CALL(callback,
              Run(EqualsMojo(mojom::SwapTransactionUnion::NewZeroExTransaction(
                      std::move(expected_zero_ex_transaction))),
                  EqualsMojo(mojom::SwapErrorUnionPtr()), ""));

  swap_service_->GetTransaction(
      mojom::SwapTransactionParamsUnion::NewZeroExTransactionParams(
          GetCannedSwapQuoteParams(
              mojom::CoinType::ETH, mojom::kPolygonMainnetChainId, "DAI",
              mojom::CoinType::ETH, mojom::kPolygonMainnetChainId, "ETH",
              mojom::SwapProvider::kZeroEx)),
      callback.Get());
  task_environment_.RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&callback);
}

TEST_F(SwapServiceUnitTest, GetZeroExTransactionError) {
  std::string error =
      R"({"code":"100","reason":"Validation Failed","validationErrors":[{"code":"1000","field":"sellAmount","reason":"should have required property 'sellAmount'"},{"code":"1000","field":"buyAmount","reason":"should have required property 'buyAmount'"},{"code":"1001","field":"","reason":"should match exactly one schema in oneOf"}]})";
  SetErrorInterceptor(error);

  base::MockCallback<mojom::SwapService::GetTransactionCallback> callback;
  EXPECT_CALL(callback, Run(EqualsMojo(mojom::SwapTransactionUnionPtr()),
                            EqualsMojo(mojom::SwapErrorUnion::NewZeroExError(
                                zeroex::ParseErrorResponse(ParseJson(error)))),
                            ""));

  swap_service_->GetTransaction(
      mojom::SwapTransactionParamsUnion::NewZeroExTransactionParams(
          GetCannedSwapQuoteParams(
              mojom::CoinType::ETH, mojom::kPolygonMainnetChainId, "DAI",
              mojom::CoinType::ETH, mojom::kPolygonMainnetChainId, "ETH",
              mojom::SwapProvider::kZeroEx)),
      callback.Get());
  task_environment_.RunUntilIdle();
}

TEST_F(SwapServiceUnitTest, GetZeroExTransactionUnexpectedReturn) {
  SetInterceptor("Woot");

  base::MockCallback<mojom::SwapService::GetTransactionCallback> callback;
  EXPECT_CALL(callback,
              Run(EqualsMojo(mojom::SwapTransactionUnionPtr()),
                  EqualsMojo(mojom::SwapErrorUnionPtr()),
                  l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR)));

  swap_service_->GetTransaction(
      mojom::SwapTransactionParamsUnion::NewZeroExTransactionParams(
          GetCannedSwapQuoteParams(
              mojom::CoinType::ETH, mojom::kPolygonMainnetChainId, "DAI",
              mojom::CoinType::ETH, mojom::kPolygonMainnetChainId, "ETH",
              mojom::SwapProvider::kZeroEx)),
      callback.Get());
  task_environment_.RunUntilIdle();
}

TEST_F(SwapServiceUnitTest, GetZeroExQuoteURL) {
  const std::map<std::string, std::string> non_rfqt_chain_ids = {
      {mojom::kSepoliaChainId, "sepolia.api.0x.wallet.brave.com"},
      {mojom::kBnbSmartChainMainnetChainId, "bsc.api.0x.wallet.brave.com"},
      {mojom::kAvalancheMainnetChainId, "avalanche.api.0x.wallet.brave.com"},
      {mojom::kFantomMainnetChainId, "fantom.api.0x.wallet.brave.com"},
      {mojom::kCeloMainnetChainId, "celo.api.0x.wallet.brave.com"},
      {mojom::kOptimismMainnetChainId, "optimism.api.0x.wallet.brave.com"},
      {mojom::kArbitrumMainnetChainId, "arbitrum.api.0x.wallet.brave.com"},
      {mojom::kBaseMainnetChainId, "base.api.0x.wallet.brave.com"}};

  const std::map<std::string, std::string> rfqt_chain_ids = {
      {mojom::kMainnetChainId, "api.0x.wallet.brave.com"},
      {mojom::kPolygonMainnetChainId, "polygon.api.0x.wallet.brave.com"}};

  for (const auto& [chain_id, domain] : non_rfqt_chain_ids) {
    SCOPED_TRACE(testing::Message() << "chain_id: " << chain_id);

    // OK: with fees
    auto url = swap_service_->GetZeroExQuoteURL(
        *GetCannedSwapQuoteParams(mojom::CoinType::ETH, chain_id, "DAI",
                                  mojom::CoinType::ETH, chain_id, "ETH",
                                  mojom::SwapProvider::kZeroEx),
        "0.00875");
    EXPECT_EQ(url,
              base::StringPrintf(
                  "https://%s/swap/v1/price?"
                  "takerAddress=0xa92D461a9a988A7f11ec285d39783A637Fdd6ba4&"
                  "sellAmount=1000000000000000000000&"
                  "buyToken=ETH&"
                  "sellToken=DAI&"
                  "buyTokenPercentageFee=0.00875&"
                  "feeRecipient=0xbd9420A98a7Bd6B89765e5715e169481602D9c3d&"
                  "slippagePercentage=0.030000&"
                  "affiliateAddress=0xbd9420A98a7Bd6B89765e5715e169481602D9c3d&"
                  "skipValidation=true",
                  domain.c_str()));

    // Ok: no fees
    url = swap_service_->GetZeroExQuoteURL(
        *GetCannedSwapQuoteParams(mojom::CoinType::ETH, chain_id, "DAI",
                                  mojom::CoinType::ETH, chain_id, "ETH",
                                  mojom::SwapProvider::kZeroEx),
        "");
    EXPECT_EQ(url,
              base::StringPrintf(
                  "https://%s/swap/v1/price?"
                  "takerAddress=0xa92D461a9a988A7f11ec285d39783A637Fdd6ba4&"
                  "sellAmount=1000000000000000000000&"
                  "buyToken=ETH&"
                  "sellToken=DAI&"
                  "slippagePercentage=0.030000&"
                  "affiliateAddress=0xbd9420A98a7Bd6B89765e5715e169481602D9c3d&"
                  "skipValidation=true",
                  domain.c_str()));
  }

  // If RFQ-T liquidity is available, so intentOnFilling=true is specified
  // while fetching firm quotes.
  for (const auto& [chain_id, domain] : rfqt_chain_ids) {
    SCOPED_TRACE(testing::Message() << "chain_id: " << chain_id);

    // OK: with fees
    auto url = swap_service_->GetZeroExQuoteURL(
        *GetCannedSwapQuoteParams(mojom::CoinType::ETH, chain_id, "DAI",
                                  mojom::CoinType::ETH, chain_id, "ETH",
                                  mojom::SwapProvider::kZeroEx),
        "0.00875");
    EXPECT_EQ(url,
              base::StringPrintf(
                  "https://%s/swap/v1/quote?"
                  "takerAddress=0xa92D461a9a988A7f11ec285d39783A637Fdd6ba4&"
                  "sellAmount=1000000000000000000000&"
                  "buyToken=ETH&"
                  "sellToken=DAI&"
                  "buyTokenPercentageFee=0.00875&"
                  "feeRecipient=0xbd9420A98a7Bd6B89765e5715e169481602D9c3d&"
                  "slippagePercentage=0.030000&"
                  "affiliateAddress=0xbd9420A98a7Bd6B89765e5715e169481602D9c3d&"
                  "skipValidation=true&"
                  "intentOnFilling=false",
                  domain.c_str()));

    // Ok: no fees
    url = swap_service_->GetZeroExQuoteURL(
        *GetCannedSwapQuoteParams(mojom::CoinType::ETH, chain_id, "DAI",
                                  mojom::CoinType::ETH, chain_id, "ETH",
                                  mojom::SwapProvider::kZeroEx),
        "");
    EXPECT_EQ(url,
              base::StringPrintf(
                  "https://%s/swap/v1/quote?"
                  "takerAddress=0xa92D461a9a988A7f11ec285d39783A637Fdd6ba4&"
                  "sellAmount=1000000000000000000000&"
                  "buyToken=ETH&"
                  "sellToken=DAI&"
                  "slippagePercentage=0.030000&"
                  "affiliateAddress=0xbd9420A98a7Bd6B89765e5715e169481602D9c3d&"
                  "skipValidation=true&"
                  "intentOnFilling=false",
                  domain.c_str()));
  }

  // KO: unsupported network
  EXPECT_EQ(swap_service_->GetZeroExQuoteURL(
                *GetCannedSwapQuoteParams(mojom::CoinType::ETH, "0x3", "DAI",
                                          mojom::CoinType::ETH, "0x3", "ETH",
                                          mojom::SwapProvider::kZeroEx),
                "0.00875"),
            "");
}

TEST_F(SwapServiceUnitTest, GetZeroExTransactionURL) {
  const std::map<std::string, std::string> non_rfqt_chain_ids = {
      {mojom::kSepoliaChainId, "sepolia.api.0x.wallet.brave.com"},
      {mojom::kBnbSmartChainMainnetChainId, "bsc.api.0x.wallet.brave.com"},
      {mojom::kAvalancheMainnetChainId, "avalanche.api.0x.wallet.brave.com"},
      {mojom::kFantomMainnetChainId, "fantom.api.0x.wallet.brave.com"},
      {mojom::kCeloMainnetChainId, "celo.api.0x.wallet.brave.com"},
      {mojom::kOptimismMainnetChainId, "optimism.api.0x.wallet.brave.com"},
      {mojom::kArbitrumMainnetChainId, "arbitrum.api.0x.wallet.brave.com"},
      {mojom::kBaseMainnetChainId, "base.api.0x.wallet.brave.com"}};

  const std::map<std::string, std::string> rfqt_chain_ids = {
      {mojom::kMainnetChainId, "api.0x.wallet.brave.com"},
      {mojom::kPolygonMainnetChainId, "polygon.api.0x.wallet.brave.com"}};

  for (const auto& [chain_id, domain] : non_rfqt_chain_ids) {
    SCOPED_TRACE(testing::Message() << "chain_id: " << chain_id);

    // OK: with fees
    auto url = swap_service_->GetZeroExTransactionURL(
        *GetCannedSwapQuoteParams(mojom::CoinType::ETH, chain_id, "DAI",
                                  mojom::CoinType::ETH, chain_id, "ETH",
                                  mojom::SwapProvider::kZeroEx),
        "0.00875");
    EXPECT_EQ(url,
              base::StringPrintf(
                  "https://%s/swap/v1/quote?"
                  "takerAddress=0xa92D461a9a988A7f11ec285d39783A637Fdd6ba4&"
                  "sellAmount=1000000000000000000000&"
                  "buyToken=ETH&"
                  "sellToken=DAI&"
                  "buyTokenPercentageFee=0.00875&"
                  "feeRecipient=0xbd9420A98a7Bd6B89765e5715e169481602D9c3d&"
                  "slippagePercentage=0.030000&"
                  "affiliateAddress=0xbd9420A98a7Bd6B89765e5715e169481602D9c3d",
                  domain.c_str()));

    // OK: no fees
    url = swap_service_->GetZeroExTransactionURL(
        *GetCannedSwapQuoteParams(mojom::CoinType::ETH, chain_id, "DAI",
                                  mojom::CoinType::ETH, chain_id, "ETH",
                                  mojom::SwapProvider::kZeroEx),
        "");
    EXPECT_EQ(url,
              base::StringPrintf(
                  "https://%s/swap/v1/quote?"
                  "takerAddress=0xa92D461a9a988A7f11ec285d39783A637Fdd6ba4&"
                  "sellAmount=1000000000000000000000&"
                  "buyToken=ETH&"
                  "sellToken=DAI&"
                  "slippagePercentage=0.030000&"
                  "affiliateAddress=0xbd9420A98a7Bd6B89765e5715e169481602D9c3d",
                  domain.c_str()));
  }

  // If RFQ-T liquidity is available, so intentOnFilling=true is specified
  // while fetching firm quotes.
  for (const auto& [chain_id, domain] : rfqt_chain_ids) {
    SCOPED_TRACE(testing::Message() << "chain_id: " << chain_id);

    // OK: with fees
    auto url = swap_service_->GetZeroExTransactionURL(
        *GetCannedSwapQuoteParams(mojom::CoinType::ETH, chain_id, "DAI",
                                  mojom::CoinType::ETH, chain_id, "ETH",
                                  mojom::SwapProvider::kZeroEx),
        "0.00875");
    EXPECT_EQ(url,
              base::StringPrintf(
                  "https://%s/swap/v1/quote?"
                  "takerAddress=0xa92D461a9a988A7f11ec285d39783A637Fdd6ba4&"
                  "sellAmount=1000000000000000000000&"
                  "buyToken=ETH&"
                  "sellToken=DAI&"
                  "buyTokenPercentageFee=0.00875&"
                  "feeRecipient=0xbd9420A98a7Bd6B89765e5715e169481602D9c3d&"
                  "slippagePercentage=0.030000&"
                  "affiliateAddress=0xbd9420A98a7Bd6B89765e5715e169481602D9c3d&"
                  "intentOnFilling=true",
                  domain.c_str()));

    // OK: no fees
    url = swap_service_->GetZeroExTransactionURL(
        *GetCannedSwapQuoteParams(mojom::CoinType::ETH, chain_id, "DAI",
                                  mojom::CoinType::ETH, chain_id, "ETH",
                                  mojom::SwapProvider::kZeroEx),
        "");
    EXPECT_EQ(url,
              base::StringPrintf(
                  "https://%s/swap/v1/quote?"
                  "takerAddress=0xa92D461a9a988A7f11ec285d39783A637Fdd6ba4&"
                  "sellAmount=1000000000000000000000&"
                  "buyToken=ETH&"
                  "sellToken=DAI&"
                  "slippagePercentage=0.030000&"
                  "affiliateAddress=0xbd9420A98a7Bd6B89765e5715e169481602D9c3d&"
                  "intentOnFilling=true",
                  domain.c_str()));
  }

  // KO: unsupported network
  EXPECT_EQ(swap_service_->GetZeroExTransactionURL(
                *GetCannedSwapQuoteParams(mojom::CoinType::ETH, "0x3", "DAI",
                                          mojom::CoinType::ETH, "0x3", "ETH",
                                          mojom::SwapProvider::kZeroEx),
                "0.00875"),
            "");
}

TEST_F(SwapServiceUnitTest, IsSwapSupported) {
  const std::vector<std::string> supported_chain_ids(
      {// ZeroEx
       mojom::kMainnetChainId, mojom::kSepoliaChainId,
       mojom::kPolygonMainnetChainId, mojom::kBnbSmartChainMainnetChainId,
       mojom::kAvalancheMainnetChainId, mojom::kFantomMainnetChainId,
       mojom::kCeloMainnetChainId, mojom::kOptimismMainnetChainId,
       mojom::kArbitrumMainnetChainId, mojom::kBaseMainnetChainId,

       // Jupiter
       mojom::kSolanaMainnet,

       // LiFi (in addition to ZeroEx)
       mojom::kPolygonZKEVMChainId, mojom::kGnosisChainId,
       mojom::kZkSyncEraChainId, mojom::kAuroraMainnetChainId});

  for (auto& chain_id : supported_chain_ids) {
    SCOPED_TRACE(testing::Message() << "chain_id: " << chain_id);
    IsSwapSupported(chain_id, true);
  }

  IsSwapSupported("0x4", false);
  IsSwapSupported("0x3", false);
  IsSwapSupported("", false);
  IsSwapSupported("invalid chain_id", false);
}

TEST_F(SwapServiceUnitTest, GetJupiterQuoteURL) {
  auto params = GetCannedSwapQuoteParams(
      mojom::CoinType::SOL, mojom::kSolanaMainnet, "", mojom::CoinType::SOL,
      mojom::kSolanaMainnet, "EPjFWdd5AufqSSqeM2qN1xzybapC8G4wEGGkZwyTDt1v",
      mojom::SwapProvider::kAuto);
  params->from_token = "So11111111111111111111111111111111111111112";
  params->to_token = "EPjFWdd5AufqSSqeM2qN1xzybapC8G4wEGGkZwyTDt1v";
  params->from_amount = "10000";

  // OK: with fees
  auto url = swap_service_->GetJupiterQuoteURL(*params, "85");
  EXPECT_EQ(url,
            "https://jupiter.wallet.brave.com/v6/quote?"
            "inputMint=So11111111111111111111111111111111111111112&"
            "outputMint=EPjFWdd5AufqSSqeM2qN1xzybapC8G4wEGGkZwyTDt1v&"
            "amount=10000&"
            "swapMode=ExactIn&"
            "slippageBps=300&"
            "platformFeeBps=85");

  // OK: no fees
  url = swap_service_->GetJupiterQuoteURL(*params, "");
  EXPECT_EQ(url,
            "https://jupiter.wallet.brave.com/v6/quote?"
            "inputMint=So11111111111111111111111111111111111111112&"
            "outputMint=EPjFWdd5AufqSSqeM2qN1xzybapC8G4wEGGkZwyTDt1v&"
            "amount=10000&"
            "swapMode=ExactIn&"
            "slippageBps=300");
}

TEST_F(SwapServiceUnitTest, GetJupiterTransactionURL) {
  auto url = swap_service_->GetJupiterTransactionURL(mojom::kSolanaMainnet);
  EXPECT_EQ(url, "https://jupiter.wallet.brave.com/v6/swap");
}

TEST_F(SwapServiceUnitTest, GetJupiterQuote) {
  SetInterceptor(R"(
    {
      "inputMint": "So11111111111111111111111111111111111111112",
      "inAmount": "100000000",
      "outputMint": "EPjFWdd5AufqSSqeM2qN1xzybapC8G4wEGGkZwyTDt1v",
      "outAmount": "10886298",
      "otherAmountThreshold": "10885210",
      "swapMode": "ExactIn",
      "slippageBps": 1,
      "platformFee": {
        "amount": "93326",
        "feeBps": 85
      },
      "priceImpactPct": "0.008955716118219659",
      "routePlan": [
        {
          "swapInfo": {
            "ammKey": "EiEAydLqSKFqRPpuwYoVxEJ6h9UZh9tsTaHgs4f8b8Z5",
            "label": "Lifinity V2",
            "inputMint": "So11111111111111111111111111111111111111112",
            "outputMint": "Es9vMFrzaCERmJfrF4H2FYD4KCoNkY11McCe8BenwNYB",
            "inAmount": "100000000",
            "outAmount": "10964919",
            "feeAmount": "20000",
            "feeMint": "So11111111111111111111111111111111111111112"
          },
          "percent": 100
        },
        {
          "swapInfo": {
            "ammKey": "UXD3M3N6Hn1JjbxugKguhJVHbYm8zHvdF5pNf7dumd5",
            "label": "Mercurial",
            "inputMint": "Es9vMFrzaCERmJfrF4H2FYD4KCoNkY11McCe8BenwNYB",
            "outputMint": "EPjFWdd5AufqSSqeM2qN1xzybapC8G4wEGGkZwyTDt1v",
            "inAmount": "10964919",
            "outAmount": "10979624",
            "feeAmount": "1098",
            "feeMint": "EPjFWdd5AufqSSqeM2qN1xzybapC8G4wEGGkZwyTDt1v"
          },
          "percent": 100
        }
      ]
    })");

  const auto& params = GetCannedJupiterTransactionParams(
      "EPjFWdd5AufqSSqeM2qN1xzybapC8G4wEGGkZwyTDt1v");
  auto& expected_quote = params->get_jupiter_transaction_params()->quote;

  auto expected_swap_fees = mojom::SwapFees::New();
  expected_swap_fees->fee_pct = "0";
  expected_swap_fees->discount_pct = "0";
  expected_swap_fees->effective_fee_pct = "0";
  expected_swap_fees->discount_code = mojom::SwapDiscountCode::kNone;
  expected_swap_fees->fee_param = "";

  base::MockCallback<mojom::SwapService::GetQuoteCallback> callback;
  EXPECT_CALL(callback, Run(EqualsMojo(mojom::SwapQuoteUnion::NewJupiterQuote(
                                std::move(expected_quote))),
                            EqualsMojo(expected_swap_fees.Clone()),
                            EqualsMojo(mojom::SwapErrorUnionPtr()), ""));
  swap_service_->GetQuote(
      GetCannedSwapQuoteParams(mojom::CoinType::SOL, mojom::kSolanaMainnet, "",
                               mojom::CoinType::SOL, mojom::kSolanaMainnet,
                               "EPjFWdd5AufqSSqeM2qN1xzybapC8G4wEGGkZwyTDt1v",
                               mojom::SwapProvider::kAuto),
      callback.Get());
  task_environment_.RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&callback);

  // KO: empty JSON for conversion
  TestGetQuoteCase(R"({})", mojom::CoinType::SOL, mojom::kSolanaMainnet,
                   mojom::CoinType::SOL, mojom::kSolanaMainnet, false,
                   mojom::SwapProvider::kAuto);

  // KO: invalid JSON
  TestGetQuoteCase(R"(foo)", mojom::CoinType::SOL, mojom::kSolanaMainnet,
                   mojom::CoinType::SOL, mojom::kSolanaMainnet, false,
                   mojom::SwapProvider::kAuto);
}

TEST_F(SwapServiceUnitTest, GetJupiterTransaction) {
  SetInterceptor(R"(
    {
      "swapTransaction": "bar"
    })");

  // OK: valid case
  TestGetJupiterTransaction(true, "foo", false);

  // KO: invalid output mint
  TestGetJupiterTransaction(false, "", true, "invalid output mint");

  // KO: invalid JSON
  SetInterceptor(R"(foo)");
  TestGetJupiterTransaction(false, "", true);
}

TEST_F(SwapServiceUnitTest, GetLiFiQuoteURL) {
  auto url = swap_service_->GetLiFiQuoteURL();
  EXPECT_EQ(url, "https://lifi.wallet.brave.com/v1/advanced/routes");
}

TEST_F(SwapServiceUnitTest, GetLiFiTransactionURL) {
  auto url = swap_service_->GetLiFiTransactionURL();
  EXPECT_EQ(url, "https://lifi.wallet.brave.com/v1/advanced/stepTransaction");
}

TEST_F(SwapServiceUnitTest, GetLiFiQuote) {
  SetInterceptor(R"(
    {
      "routes": [
        {
          "id": "0x9a448018e09b62da15c1bd64571c21b33cb177cee5d2f07c325d6485364362a5",
          "fromChainId": "137",
          "fromAmountUSD": "2.00",
          "fromAmount": "2000000",
          "fromToken": {
            "address": "0x2791Bca1f2de4661ED88A30C99A7a9449Aa84174",
            "chainId": "137",
            "symbol": "USDCe",
            "decimals": "6",
            "name": "USDC.e",
            "coinKey": "USDCe",
            "logoURI": "usdce.png",
            "priceUSD": "1"
          },
          "fromAddress": "0x552008c0f6870c2f77e5cC1d2eb9bdff03e30Ea0",
          "toChainId": "1151111081099710",
          "toAmountUSD": "1.14",
          "toAmount": "1138627",
          "toAmountMin": "1136350",
          "toToken": {
            "address": "EPjFWdd5AufqSSqeM2qN1xzybapC8G4wEGGkZwyTDt1v",
            "chainId": "1151111081099710",
            "symbol": "USDC",
            "decimals": "6",
            "name": "USD Coin",
            "coinKey": "USDC",
            "logoURI": "usdc.png",
            "priceUSD": "0.999501"
          },
          "toAddress": "S5ARSDD3ddZqqqqqb2EUE2h2F1XQHBk7bErRW1WPGe4",
          "gasCostUSD": "0.02",
          "containsSwitchChain": false,
          "steps": [
            {
              "type": "lifi",
              "id": "57d247fc-d80a-4f4a-9596-72db3061aa72",
              "tool": "allbridge",
              "toolDetails": {
                "key": "allbridge",
                "name": "Allbridge",
                "logoURI": "allbridge.png"
              },
              "action": {
                "fromChainId": "137",
                "fromAmount": "2000000",
                "fromAddress": "0x552008c0f6870c2f77e5cC1d2eb9bdff03e30Ea0",
                "slippage": "0.03",
                "toChainId": "1151111081099710",
                "toAddress": "S5ARSDD3ddZqqqqqb2EUE2h2F1XQHBk7bErRW1WPGe4",
                "fromToken": {
                  "address": "0x2791Bca1f2de4661ED88A30C99A7a9449Aa84174",
                  "chainId": "137",
                  "symbol": "USDCe",
                  "decimals": "6",
                  "name": "USDC.e",
                  "coinKey": "USDCe",
                  "logoURI": "usdce.png",
                  "priceUSD": "1"
                },
                "toToken": {
                  "address": "EPjFWdd5AufqSSqeM2qN1xzybapC8G4wEGGkZwyTDt1v",
                  "chainId": "1151111081099710",
                  "symbol": "USDC",
                  "decimals": "6",
                  "name": "USD Coin",
                  "coinKey": "USDC",
                  "logoURI": "usdc.png",
                  "priceUSD": "0.999501"
                }
              },
              "estimate": {
                "tool": "allbridge",
                "fromAmount": "2000000",
                "fromAmountUSD": "2.00",
                "toAmount": "1138627",
                "toAmountMin": "1136350",
                "approvalAddress": "0x1231DEB6f5749EF6cE6943a275A1D3E7486F4EaE",
                "executionDuration": "500.298",
                "feeCosts": [
                  {
                    "name": "Allbridge's fee",
                    "description": "AllBridge fee and messenger fee charged by Allbridge",
                    "token": {
                      "address": "0x2791Bca1f2de4661ED88A30C99A7a9449Aa84174",
                      "chainId": "137",
                      "symbol": "USDCe",
                      "decimals": "6",
                      "name": "USDC.e",
                      "coinKey": "USDCe",
                      "logoURI": "usdce.png",
                      "priceUSD": "1"
                    },
                    "amount": "853380",
                    "amountUSD": "0.85",
                    "percentage": "0.4267",
                    "included": true
                  }
                ],
                "gasCosts": [
                  {
                    "type": "SEND",
                    "price": "112000000000",
                    "estimate": "185000",
                    "limit": "277500",
                    "amount": "20720000000000000",
                    "amountUSD": "0.02",
                    "token": {
                      "address": "0x0000000000000000000000000000000000000000",
                      "chainId": "137",
                      "symbol": "MATIC",
                      "decimals": "18",
                      "name": "MATIC",
                      "coinKey": "MATIC",
                      "logoURI": "matic.png",
                      "priceUSD": "0.809079000000000000"
                    }
                  }
                ],
                "toAmountUSD": "1.14"
              },
              "includedSteps": [
                {
                  "id": "1b914bef-e1be-4895-a9b1-c57da16d9de5",
                  "type": "cross",
                  "action": {
                    "fromChainId": "137",
                    "fromAmount": "2000000",
                    "fromAddress": "0x552008c0f6870c2f77e5cC1d2eb9bdff03e30Ea0",
                    "slippage": "0.03",
                    "toChainId": "1151111081099710",
                    "toAddress": "S5ARSDD3ddZqqqqqb2EUE2h2F1XQHBk7bErRW1WPGe4",
                    "fromToken": {
                      "address": "0x2791Bca1f2de4661ED88A30C99A7a9449Aa84174",
                      "chainId": "137",
                      "symbol": "USDCe",
                      "decimals": "6",
                      "name": "USDC.e",
                      "coinKey": "USDCe",
                      "logoURI": "usdce.png",
                      "priceUSD": "1"
                    },
                    "toToken": {
                      "address": "EPjFWdd5AufqSSqeM2qN1xzybapC8G4wEGGkZwyTDt1v",
                      "chainId": "1151111081099710",
                      "symbol": "USDC",
                      "decimals": "6",
                      "name": "USD Coin",
                      "coinKey": "USDC",
                      "logoURI": "usdc.png",
                      "priceUSD": "0.999501"
                    }
                  },
                  "estimate": {
                    "tool": "allbridge",
                    "fromAmount": "2000000",
                    "fromAmountUSD": "2.00",
                    "toAmount": "1138627",
                    "toAmountMin": "1136350",
                    "approvalAddress": "0x7775d63836987f444E2F14AA0fA2602204D7D3E0",
                    "executionDuration": "500.298",
                    "feeCosts": [
                      {
                        "name": "Allbridge's fee",
                        "description": "AllBridge fee and messenger fee charged by Allbridge",
                        "token": {
                          "address": "0x2791Bca1f2de4661ED88A30C99A7a9449Aa84174",
                          "chainId": "137",
                          "symbol": "USDCe",
                          "decimals": "6",
                          "name": "USDC.e",
                          "coinKey": "USDCe",
                          "logoURI": "usdce.png",
                          "priceUSD": "1"
                        },
                        "amount": "853380",
                        "amountUSD": "0.85",
                        "percentage": "0.4267",
                        "included": true
                      }
                    ],
                    "gasCosts": [
                      {
                        "type": "SEND",
                        "price": "112000000000",
                        "estimate": "185000",
                        "limit": "277500",
                        "amount": "20720000000000000",
                        "amountUSD": "0.02",
                        "token": {
                          "address": "0x0000000000000000000000000000000000000000",
                          "chainId": "137",
                          "symbol": "MATIC",
                          "decimals": "18",
                          "name": "MATIC",
                          "coinKey": "MATIC",
                          "logoURI": "matic.png",
                          "priceUSD": "0.809079000000000000"
                        }
                      }
                    ]
                  },
                  "tool": "allbridge",
                  "toolDetails": {
                    "key": "allbridge",
                    "name": "Allbridge",
                    "logoURI": "allbridge.png"
                  }
                }
              ]
            }
          ],
          "tags": [
            "CHEAPEST",
            "FASTEST"
          ]
        }
      ],
      "unavailableRoutes": {
        "filteredOut": [],
        "failed": []
      }
    }
  )");

  auto expected_swap_fees = mojom::SwapFees::New();
  expected_swap_fees->fee_pct = "0";
  expected_swap_fees->discount_pct = "0";
  expected_swap_fees->effective_fee_pct = "0";
  expected_swap_fees->discount_code = mojom::SwapDiscountCode::kNone;
  expected_swap_fees->fee_param = "";

  base::MockCallback<mojom::SwapService::GetQuoteCallback> callback;
  EXPECT_CALL(
      callback,
      Run(EqualsMojo(mojom::SwapQuoteUnion::NewLifiQuote(GetCannedLiFiQuote())),
          EqualsMojo(expected_swap_fees.Clone()),
          EqualsMojo(mojom::SwapErrorUnionPtr()), ""));
  auto quote_params = GetCannedSwapQuoteParams(
      mojom::CoinType::ETH, mojom::kPolygonMainnetChainId, "DAI",
      mojom::CoinType::SOL, mojom::kSolanaMainnet,
      "EPjFWdd5AufqSSqeM2qN1xzybapC8G4wEGGkZwyTDt1v",
      mojom::SwapProvider::kAuto);
  swap_service_->GetQuote(std::move(quote_params), callback.Get());
  task_environment_.RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&callback);

  // KO: empty JSON for conversion
  TestGetQuoteCase(R"({})", mojom::CoinType::ETH, mojom::kPolygonMainnetChainId,
                   mojom::CoinType::SOL, mojom::kSolanaMainnet, false,
                   mojom::SwapProvider::kAuto);

  // KO: invalid JSON
  TestGetQuoteCase(R"(foo)", mojom::CoinType::ETH,
                   mojom::kPolygonMainnetChainId, mojom::CoinType::SOL,
                   mojom::kSolanaMainnet, false, mojom::SwapProvider::kAuto);
}

TEST_F(SwapServiceUnitTest, GetLiFiTransaction) {
  SetInterceptor(R"(
    {
      "transactionRequest": {
        "from": "0x552008c0f6870c2f77e5cC1d2eb9bdff03e30Ea0",
        "to": "0x1111111254fb6c44bac0bed2854e76f90643097d",
        "chainId": "100",
        "data": "0x...",
        "value": "0x0de0b6b3a7640000",
        "gasPrice": "0xb2d05e00",
        "gasLimit": "0x0e9cb2"
      }
    }
  )");

  auto quote = GetCannedLiFiQuote();
  auto expected_transaction = mojom::LiFiEVMTransaction::New();
  expected_transaction->from = "0x552008c0f6870c2f77e5cC1d2eb9bdff03e30Ea0";
  expected_transaction->to = "0x1111111254fb6c44bac0bed2854e76f90643097d";
  expected_transaction->chain_id = "0x64";
  expected_transaction->data = "0x...";
  expected_transaction->value = "0x0de0b6b3a7640000";
  expected_transaction->gas_price = "0xb2d05e00";
  expected_transaction->gas_limit = "0x0e9cb2";

  base::MockCallback<mojom::SwapService::GetTransactionCallback> callback;
  EXPECT_CALL(callback,
              Run(EqualsMojo(mojom::SwapTransactionUnion::NewLifiTransaction(
                      mojom::LiFiTransactionUnion::NewEvmTransaction(
                          std::move(expected_transaction)))),
                  EqualsMojo(mojom::SwapErrorUnionPtr()), ""));

  swap_service_->GetTransaction(
      mojom::SwapTransactionParamsUnion::NewLifiTransactionParams(
          std::move(quote->routes[0]->steps[0])),
      callback.Get());
  task_environment_.RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&callback);
}

TEST_F(SwapServiceUnitTest, GetLiFiQuoteError) {
  std::string error = R"(
    {
      "message": "Invalid request",
      "code": "1000"
    })";
  SetErrorInterceptor(error);

  base::MockCallback<mojom::SwapService::GetQuoteCallback> callback;

  EXPECT_CALL(
      callback,
      Run(EqualsMojo(mojom::SwapQuoteUnionPtr()),
          EqualsMojo(mojom::SwapFeesPtr()),
          EqualsMojo(mojom::SwapErrorUnion::NewLifiError(mojom::LiFiError::New(
              "Invalid request", mojom::LiFiErrorCode::kDefaultError))),
          ""));

  swap_service_->GetQuote(
      GetCannedSwapQuoteParams(mojom::CoinType::ETH,
                               mojom::kPolygonMainnetChainId, "DAI",
                               mojom::CoinType::SOL, mojom::kSolanaMainnet,
                               "EPjFWdd5AufqSSqeM2qN1xzybapC8G4wEGGkZwyTDt1v",
                               mojom::SwapProvider::kAuto),
      callback.Get());
  task_environment_.RunUntilIdle();
}

TEST_F(SwapServiceUnitTest, GetLiFiTransactionError) {
  std::string error = R"(
    {
      "message": "Invalid request",
      "code": "1000"
    })";
  SetErrorInterceptor(error);

  base::MockCallback<mojom::SwapService::GetTransactionCallback> callback;

  EXPECT_CALL(
      callback,
      Run(EqualsMojo(mojom::SwapTransactionUnionPtr()),
          EqualsMojo(mojom::SwapErrorUnion::NewLifiError(mojom::LiFiError::New(
              "Invalid request", mojom::LiFiErrorCode::kDefaultError))),
          ""));

  auto quote = GetCannedLiFiQuote();
  swap_service_->GetTransaction(
      mojom::SwapTransactionParamsUnion::NewLifiTransactionParams(
          std::move(quote->routes[0]->steps[0])),
      callback.Get());
  task_environment_.RunUntilIdle();
}

TEST_F(SwapServiceUnitTest, GetLiFiStatus) {
  SetInterceptor(R"(
    {
      "transactionId": "0x0a0e6ac13786c9a3a68f55bbb5eeb3b31a7a25e7e2aa3641c545fd3869da8016",
      "sending": {
        "txHash": "0x1263d8b3cb3cf4e3ec0f9df5947e3846e2c6b1fb2bcc3740e36514c2ddd87cbb",
        "txLink": "https://optimistic.etherscan.io/tx/0x1263d8b3cb3cf4e3ec0f9df5947e3846e2c6b1fb2bcc3740e36514c2ddd87cbb",
        "amount": "2516860",
        "token": {
          "address": "0x7F5c764cBc14f9669B88837ca1490cCa17c31607",
          "chainId": "10",
          "symbol": "USDC.e",
          "decimals": "6",
          "name": "Bridged USD Coin",
          "coinKey": "USDCe",
          "logoURI": "https://raw.githubusercontent.com/trustwallet/assets/master/blockchains/ethereum/assets/0xA0b86991c6218b36c1d19D4a2e9Eb0cE3606eB48/logo.png",
          "priceUSD": "1"
        },
        "chainId": "10",
        "gasPrice": "61761647",
        "gasUsed": "239193",
        "gasToken": {
          "address": "0x0000000000000000000000000000000000000000",
          "chainId": "10",
          "symbol": "ETH",
          "decimals": "18",
          "name": "ETH",
          "coinKey": "ETH",
          "logoURI": "https://raw.githubusercontent.com/trustwallet/assets/master/blockchains/ethereum/assets/0xC02aaA39b223FE8D0A0e5C4F27eAD9083C756Cc2/logo.png",
          "priceUSD": "3399.92"
        },
        "gasAmount": "14772953630871",
        "gasAmountUSD": "0.05",
        "amountUSD": "2.52",
        "value": "0",
        "timestamp": "1721381005"
      },
      "receiving": {
        "chainId": "10"
      },
      "lifiExplorerLink": "https://explorer.li.fi/tx/0x1263d8b3cb3cf4e3ec0f9df5947e3846e2c6b1fb2bcc3740e36514c2ddd87cbb",
      "fromAddress": "0xa92d461a9a988a7f11ec285d39783a637fdd6ba4",
      "toAddress": "0xa92d461a9a988a7f11ec285d39783a637fdd6ba4",
      "tool": "paraswap",
      "status": "PENDING",
      "substatus": "WAIT_DESTINATION_TRANSACTION",
      "substatusMessage": "The transfer is waiting for destination transaction.",
      "metadata": {
        "integrator": "brave"
      }
    }
  )");

  auto expected_response = mojom::LiFiStatus::New();
  expected_response->transaction_id =
      "0x0a0e6ac13786c9a3a68f55bbb5eeb3b31a7a25e7e2aa3641c545fd3869da8016";
  expected_response->sending = mojom::LiFiStepStatus::New();
  expected_response->sending->tx_hash =
      "0x1263d8b3cb3cf4e3ec0f9df5947e3846e2c6b1fb2bcc3740e36514c2ddd87cbb";
  expected_response->sending->tx_link =
      "https://optimistic.etherscan.io/tx/"
      "0x1263d8b3cb3cf4e3ec0f9df5947e3846e2c6b1fb2bcc3740e36514c2ddd87cbb";
  expected_response->sending->amount = "2516860";
  expected_response->sending->contract_address =
      "0x7F5c764cBc14f9669B88837ca1490cCa17c31607";
  expected_response->sending->chain_id = "0xa";

  expected_response->receiving = mojom::LiFiStepStatus::New();
  expected_response->receiving->chain_id = "0xa";

  expected_response->lifi_explorer_link =
      "https://explorer.li.fi/tx/"
      "0x1263d8b3cb3cf4e3ec0f9df5947e3846e2c6b1fb2bcc3740e36514c2ddd87cbb";
  expected_response->from_address =
      "0xa92d461a9a988a7f11ec285d39783a637fdd6ba4";
  expected_response->to_address = "0xa92d461a9a988a7f11ec285d39783a637fdd6ba4";
  expected_response->tool = "paraswap";
  expected_response->status = mojom::LiFiStatusCode::kPending;
  expected_response->substatus =
      mojom::LiFiSubstatusCode::kWaitDestinationTransaction;
  expected_response->substatus_message =
      "The transfer is waiting for destination transaction.";

  base::MockCallback<mojom::SwapService::GetLiFiStatusCallback> callback;
  EXPECT_CALL(callback, Run(EqualsMojo(expected_response),
                            EqualsMojo(mojom::LiFiErrorPtr()), ""));

  swap_service_->GetLiFiStatus(
      "0x0a0e6ac13786c9a3a68f55bbb5eeb3b31a7a25e7e2aa3641c545fd3869da8016",
      callback.Get());
  task_environment_.RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&callback);
}

}  // namespace brave_wallet
