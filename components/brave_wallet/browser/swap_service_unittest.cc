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

mojom::SquidQuotePtr GetCannedSquidQuote() {
  auto quote = mojom::SquidQuote::New();

  auto eth = mojom::BlockchainToken::New();
  eth->contract_address = "";
  eth->name = "Ethereum";
  eth->logo =
      "https://raw.githubusercontent.com/0xsquid/assets/main/images/tokens/"
      "eth.svg";
  eth->symbol = "ETH";
  eth->decimals = 18;
  eth->coingecko_id = "ethereum";
  eth->chain_id = mojom::kArbitrumMainnetChainId;
  eth->coin = mojom::CoinType::ETH;

  auto weth = mojom::BlockchainToken::New();
  weth->contract_address = "0x82af49447d8a07e3bd95bd0d56f35241523fbab1";
  weth->name = "Wrapped ETH";
  weth->logo =
      "https://raw.githubusercontent.com/0xsquid/assets/main/images/tokens/"
      "weth.svg";
  weth->symbol = "WETH";
  weth->decimals = 18;
  weth->coingecko_id = "weth";
  weth->chain_id = mojom::kArbitrumMainnetChainId;
  weth->coin = mojom::CoinType::ETH;

  auto usdc = mojom::BlockchainToken::New();
  usdc->contract_address = "0xaf88d065e77c8cC2239327C5EDb3A432268e5831";
  usdc->name = "USD Coin";
  usdc->logo =
      "https://raw.githubusercontent.com/0xsquid/assets/main/images/tokens/"
      "usdc.svg";
  usdc->symbol = "USDC";
  usdc->decimals = 6;
  usdc->coingecko_id = "usd-coin";
  usdc->chain_id = mojom::kArbitrumMainnetChainId;
  usdc->coin = mojom::CoinType::ETH;

  quote->actions.push_back(mojom::SquidAction::New());
  quote->actions[0]->type = mojom::SquidActionType::kWrap;
  quote->actions[0]->description = "Wrap ETH to WETH";
  quote->actions[0]->provider = "Native Wrapper";
  quote->actions[0]->logo_uri =
      "https://raw.githubusercontent.com/0xsquid/assets/main/images/providers/"
      "weth.svg";
  quote->actions[0]->from_amount = "10000000000000000";
  quote->actions[0]->from_token = eth.Clone();
  quote->actions[0]->to_amount = "10000000000000000";
  quote->actions[0]->to_amount_min = "10000000000000000";
  quote->actions[0]->to_token = weth.Clone();

  quote->actions.push_back(mojom::SquidAction::New());
  quote->actions[1]->type = mojom::SquidActionType::kSwap;
  quote->actions[1]->description = "Swap from WETH to USDC";
  quote->actions[1]->provider = "Uniswap V3";
  quote->actions[1]->logo_uri =
      "https://raw.githubusercontent.com/0xsquid/assets/main/images/providers/"
      "uniswap.svg";
  quote->actions[1]->from_amount = "10000000000000000";
  quote->actions[1]->from_token = weth.Clone();
  quote->actions[1]->to_amount = "25826875";
  quote->actions[1]->to_amount_min = "25749394";
  quote->actions[1]->to_token = usdc.Clone();

  quote->aggregate_price_impact = "0.0";
  quote->aggregate_slippage = "0.9600000000000001";
  quote->estimated_route_duration = "20";
  quote->exchange_rate = "2582.6875";
  quote->is_boost_supported = false;
  quote->from_amount = "10000000000000000";
  quote->from_token = eth.Clone();
  quote->to_amount = "25826875";
  quote->to_amount_min = "25749394";
  quote->to_token = usdc.Clone();

  quote->fee_costs.push_back(mojom::SquidFeeCost::New());
  quote->fee_costs[0]->amount = "311053437062551";
  quote->fee_costs[0]->description = "Gas receiver fee";
  quote->fee_costs[0]->name = "Gas receiver fee";
  quote->fee_costs[0]->token = eth.Clone();

  quote->gas_costs.push_back(mojom::SquidGasCost::New());
  quote->gas_costs[0]->amount = "1452012968376000";
  quote->gas_costs[0]->gas_limit = "953658";
  quote->gas_costs[0]->token = eth.Clone();

  quote->allowance_target = "0xce16F69375520ab01377ce7B88f5BA8C48F8D666";

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
      "blockNumber": "20114676",
      "buyAmount": "100032748",
      "buyToken": "0xdac17f958d2ee523a2206206994597c13d831ec7",
      "fees": {
        "integratorFee": null,
        "zeroExFee": {
          "amount": "0",
          "token": "0xdeadbeef",
          "type": "volume"
        },
        "gasFee": null
      },
      "gas": "288095",
      "gasPrice": "7062490000",
      "issues": {
        "allowance": {
          "actual": "0",
          "spender": "0x0000000000001ff3684f28c67538d4d072c22734"
        },
        "balance": {
          "token": "0xa0b86991c6218b36c1d19d4a2e9eb0ce3606eb48",
          "actual": "0",
          "expected": "100000000"
        },
        "simulationIncomplete": false,
        "invalidSourcesPassed": []
      },
      "liquidityAvailable": true,
      "minBuyAmount": "99032421",
      "route": {
        "fills": [
          {
            "from": "0xa0b86991c6218b36c1d19d4a2e9eb0ce3606eb48",
            "to": "0xdac17f958d2ee523a2206206994597c13d831ec7",
            "source": "SolidlyV3",
            "proportionBps": "10000"
          }
        ],
        "tokens": [
          {
            "address": "0xa0b86991c6218b36c1d19d4a2e9eb0ce3606eb48",
            "symbol": "USDC"
          },
          {
            "address": "0xdac17f958d2ee523a2206206994597c13d831ec7",
            "symbol": "USDT"
          }
        ]
      },
      "sellAmount": "100000000",
      "sellToken": "0xa0b86991c6218b36c1d19d4a2e9eb0ce3606eb48",
      "tokenMetadata": {
        "buyToken": {
          "buyTaxBps": "0",
          "sellTaxBps": "0"
        },
        "sellToken": {
          "buyTaxBps": "0",
          "sellTaxBps": "0"
        }
      },
      "totalNetworkFee": "2034668056550000",
      "zid": "0x111111111111111111111111"
    }
  )");

  auto expected_zero_ex_quote = mojom::ZeroExQuote::New();
  expected_zero_ex_quote->buy_amount = "100032748";
  expected_zero_ex_quote->buy_token =
      "0xdac17f958d2ee523a2206206994597c13d831ec7";
  expected_zero_ex_quote->sell_amount = "100000000";
  expected_zero_ex_quote->sell_token =
      "0xa0b86991c6218b36c1d19d4a2e9eb0ce3606eb48";

  auto zero_ex_fee = mojom::ZeroExFee::New();
  zero_ex_fee->type = "volume";
  zero_ex_fee->token = "0xdeadbeef";
  zero_ex_fee->amount = "0";
  expected_zero_ex_quote->fees = mojom::ZeroExFees::New();
  expected_zero_ex_quote->fees->zero_ex_fee = std::move(zero_ex_fee);

  expected_zero_ex_quote->gas = "288095";
  expected_zero_ex_quote->gas_price = "7062490000";
  expected_zero_ex_quote->liquidity_available = true;
  expected_zero_ex_quote->min_buy_amount = "99032421";
  expected_zero_ex_quote->total_network_fee = "2034668056550000";

  auto fill = mojom::ZeroExRouteFill::New();
  fill->from = "0xa0b86991c6218b36c1d19d4a2e9eb0ce3606eb48";
  fill->to = "0xdac17f958d2ee523a2206206994597c13d831ec7";
  fill->source = "SolidlyV3";
  fill->proportion_bps = "10000";
  expected_zero_ex_quote->route = mojom::ZeroExRoute::New();
  expected_zero_ex_quote->route->fills.push_back(fill.Clone());

  expected_zero_ex_quote->allowance_target =
      "0x0000000000001fF3684f28c67538d4D072C22734";

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
      "blockNumber": "20114676",
      "buyAmount": "100032748",
      "buyToken": "0xdac17f958d2ee523a2206206994597c13d831ec7",
      "fees": {
        "integratorFee": null,
        "zeroExFee": null,
        "gasFee": null
      },
      "gas": "288095",
      "gasPrice": "7062490000",
      "issues": {
        "allowance": {
          "actual": "0",
          "spender": "0x0000000000001ff3684f28c67538d4d072c22734"
        },
        "balance": {
          "token": "0xa0b86991c6218b36c1d19d4a2e9eb0ce3606eb48",
          "actual": "0",
          "expected": "100000000"
        },
        "simulationIncomplete": false,
        "invalidSourcesPassed": []
      },
      "liquidityAvailable": true,
      "minBuyAmount": "99032421",
      "route": {
        "fills": [
          {
            "from": "0xa0b86991c6218b36c1d19d4a2e9eb0ce3606eb48",
            "to": "0xdac17f958d2ee523a2206206994597c13d831ec7",
            "source": "SolidlyV3",
            "proportionBps": "10000"
          }
        ],
        "tokens": [
          {
            "address": "0xa0b86991c6218b36c1d19d4a2e9eb0ce3606eb48",
            "symbol": "USDC"
          },
          {
            "address": "0xdac17f958d2ee523a2206206994597c13d831ec7",
            "symbol": "USDT"
          }
        ]
      },
      "sellAmount": "100000000",
      "sellToken": "0xa0b86991c6218b36c1d19d4a2e9eb0ce3606eb48",
      "tokenMetadata": {
        "buyToken": {
          "buyTaxBps": "0",
          "sellTaxBps": "0"
        },
        "sellToken": {
          "buyTaxBps": "0",
          "sellTaxBps": "0"
        }
      },
      "totalNetworkFee": "2034668056550000",
      "zid": "0x111111111111111111111111"
    }
  )");

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
  // Case 1: validation error
  std::string error = R"(
    {
      "name": "INPUT_INVALID",
      "message": "Validation Failed"
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
  testing::Mock::VerifyAndClearExpectations(&callback);

  // Case 2: insufficient liquidity
  SetInterceptor(R"(
    {
      "liquidityAvailable": false,
      "zid": "0x111111111111111111111111"
    }
  )");
  auto error_response = mojom::ZeroExError::New();
  error_response->name = "INSUFFICIENT_LIQUIDITY";
  error_response->message =
      l10n_util::GetStringUTF8(IDS_BRAVE_WALLET_SWAP_INSUFFICIENT_LIQUIDITY);
  error_response->is_insufficient_liquidity = true;

  EXPECT_CALL(callback, Run(EqualsMojo(mojom::SwapQuoteUnionPtr()),
                            EqualsMojo(mojom::SwapFeesPtr()),
                            EqualsMojo(mojom::SwapErrorUnion::NewZeroExError(
                                std::move(error_response))),
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
  SetInterceptor(R"(
    {
      "transaction": {
        "to": "0x7f6cee965959295cc64d0e6c00d99d6532d8e86b",
        "data": "0xdeadbeef",
        "gas": "288079",
        "gasPrice": "4837860000",
        "value": "0"
      }
    }
  )");

  auto expected_zero_ex_transaction = mojom::ZeroExTransaction::New();
  expected_zero_ex_transaction->to =
      "0x7f6cee965959295cc64d0e6c00d99d6532d8e86b";
  expected_zero_ex_transaction->data = "0xdeadbeef";
  expected_zero_ex_transaction->gas = "288079";
  expected_zero_ex_transaction->gas_price = "4837860000";
  expected_zero_ex_transaction->value = "0";

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
}

TEST_F(SwapServiceUnitTest, GetZeroExTransactionError) {
  std::string error =
      R"({"name":"INPUT_INVALID","message":"Validation Failed"})";
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
  const std::map<std::string, std::string> chain_ids = {
      {mojom::kMainnetChainId, "1"},
      {mojom::kArbitrumMainnetChainId, "42161"},
      {mojom::kAvalancheMainnetChainId, "43114"},
      {mojom::kBaseMainnetChainId, "8453"},
      {mojom::kBlastMainnetChainId, "238"},
      {mojom::kBnbSmartChainMainnetChainId, "56"},
      {mojom::kLineaChainId, "59144"},
      {mojom::kOptimismMainnetChainId, "10"},
      {mojom::kPolygonMainnetChainId, "137"},
      {mojom::kScrollChainId, "534352"}};

  for (const auto& [chain_id, encoded_chain_id] : chain_ids) {
    SCOPED_TRACE(testing::Message() << "chain_id: " << chain_id);

    // OK: with fees
    auto url = swap_service_->GetZeroExQuoteURL(
        *GetCannedSwapQuoteParams(mojom::CoinType::ETH, chain_id, "DAI",
                                  mojom::CoinType::ETH, chain_id, "ETH",
                                  mojom::SwapProvider::kZeroEx),
        "85");
    EXPECT_EQ(url,
              base::StringPrintf(
                  "https://api.0x.wallet.brave.com/swap/allowance-holder/price?"
                  "chainId=%s&"
                  "taker=0xa92D461a9a988A7f11ec285d39783A637Fdd6ba4&"
                  "sellAmount=1000000000000000000000&"
                  "buyToken=ETH&"
                  "sellToken=DAI&"
                  "swapFeeBps=85&"
                  "swapFeeRecipient=0xbd9420A98a7Bd6B89765e5715e169481602D9c3d&"
                  "swapFeeToken=ETH&"
                  "slippageBps=300",
                  encoded_chain_id.c_str()));

    // Ok: no fees
    url = swap_service_->GetZeroExQuoteURL(
        *GetCannedSwapQuoteParams(mojom::CoinType::ETH, chain_id, "DAI",
                                  mojom::CoinType::ETH, chain_id, "ETH",
                                  mojom::SwapProvider::kZeroEx),
        "");
    EXPECT_EQ(url,
              base::StringPrintf(
                  "https://api.0x.wallet.brave.com/swap/allowance-holder/price?"
                  "chainId=%s&"
                  "taker=0xa92D461a9a988A7f11ec285d39783A637Fdd6ba4&"
                  "sellAmount=1000000000000000000000&"
                  "buyToken=ETH&"
                  "sellToken=DAI&"
                  "slippageBps=300",
                  encoded_chain_id.c_str()));
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
  const std::map<std::string, std::string> chain_ids = {
      {mojom::kMainnetChainId, "1"},
      {mojom::kArbitrumMainnetChainId, "42161"},
      {mojom::kAvalancheMainnetChainId, "43114"},
      {mojom::kBaseMainnetChainId, "8453"},
      {mojom::kBlastMainnetChainId, "238"},
      {mojom::kBnbSmartChainMainnetChainId, "56"},
      {mojom::kLineaChainId, "59144"},
      {mojom::kOptimismMainnetChainId, "10"},
      {mojom::kPolygonMainnetChainId, "137"},
      {mojom::kScrollChainId, "534352"}};

  for (const auto& [chain_id, encoded_chain_id] : chain_ids) {
    SCOPED_TRACE(testing::Message() << "chain_id: " << chain_id);

    // OK: with fees
    auto url = swap_service_->GetZeroExTransactionURL(
        *GetCannedSwapQuoteParams(mojom::CoinType::ETH, chain_id, "DAI",
                                  mojom::CoinType::ETH, chain_id, "ETH",
                                  mojom::SwapProvider::kZeroEx),
        "85");
    EXPECT_EQ(url,
              base::StringPrintf(
                  "https://api.0x.wallet.brave.com/swap/allowance-holder/quote?"
                  "chainId=%s&"
                  "taker=0xa92D461a9a988A7f11ec285d39783A637Fdd6ba4&"
                  "sellAmount=1000000000000000000000&"
                  "buyToken=ETH&"
                  "sellToken=DAI&"
                  "swapFeeBps=85&"
                  "swapFeeRecipient=0xbd9420A98a7Bd6B89765e5715e169481602D9c3d&"
                  "swapFeeToken=ETH&"
                  "slippageBps=300",
                  encoded_chain_id.c_str()));

    // OK: no fees
    url = swap_service_->GetZeroExTransactionURL(
        *GetCannedSwapQuoteParams(mojom::CoinType::ETH, chain_id, "DAI",
                                  mojom::CoinType::ETH, chain_id, "ETH",
                                  mojom::SwapProvider::kZeroEx),
        "");
    EXPECT_EQ(url,
              base::StringPrintf(
                  "https://api.0x.wallet.brave.com/swap/allowance-holder/quote?"
                  "chainId=%s&"
                  "taker=0xa92D461a9a988A7f11ec285d39783A637Fdd6ba4&"
                  "sellAmount=1000000000000000000000&"
                  "buyToken=ETH&"
                  "sellToken=DAI&"
                  "slippageBps=300",
                  encoded_chain_id.c_str()));
  }

  // KO: unsupported network
  EXPECT_EQ(swap_service_->GetZeroExTransactionURL(
                *GetCannedSwapQuoteParams(mojom::CoinType::ETH, "0x3", "DAI",
                                          mojom::CoinType::ETH, "0x3", "ETH",
                                          mojom::SwapProvider::kZeroEx),
                "85"),
            "");
}

TEST_F(SwapServiceUnitTest, IsSwapSupported) {
  const std::vector<std::string> supported_chain_ids(
      {// ZeroEx
       mojom::kMainnetChainId, mojom::kArbitrumMainnetChainId,
       mojom::kAvalancheMainnetChainId, mojom::kBaseMainnetChainId,
       mojom::kBlastMainnetChainId, mojom::kBnbSmartChainMainnetChainId,
       mojom::kLineaChainId, mojom::kOptimismMainnetChainId,
       mojom::kPolygonMainnetChainId, mojom::kScrollChainId,

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
  auto expected_transaction = mojom::LiFiEvmTransaction::New();
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

TEST_F(SwapServiceUnitTest, GetSquidURL) {
  auto url = swap_service_->GetSquidURL();
  EXPECT_EQ(url, "https://squid.wallet.brave.com/v2/route");
}

TEST_F(SwapServiceUnitTest, GetSquidQuote) {
  SetInterceptor(R"(
    {
      "route": {
        "estimate": {
          "actions": [
            {
              "type": "wrap",
              "chainType": "evm",
              "fromChain": "42161",
              "toChain": "42161",
              "fromToken": {
                "type": "evm",
                "chainId": "42161",
                "address": "0xEeeeeEeeeEeEeeEeEeEeeEEEeeeeEeeeeeeeEEeE",
                "name": "Ethereum",
                "symbol": "ETH",
                "decimals": "18",
                "logoURI": "https://raw.githubusercontent.com/0xsquid/assets/main/images/tokens/eth.svg",
                "coingeckoId": "ethereum",
                "usdPrice": "2581.298038575404"
              },
              "toToken": {
                "type": "evm",
                "chainId": "42161",
                "address": "0x82af49447d8a07e3bd95bd0d56f35241523fbab1",
                "name": "Wrapped ETH",
                "symbol": "WETH",
                "decimals": "18",
                "logoURI": "https://raw.githubusercontent.com/0xsquid/assets/main/images/tokens/weth.svg",
                "coingeckoId": "weth",
                "axelarNetworkSymbol": "WETH",
                "subGraphIds": [
                  "arbitrum-weth-wei"
                ],
                "subGraphOnly": false,
                "usdPrice": "2583.905483882831"
              },
              "fromAmount": "10000000000000000",
              "toAmount": "10000000000000000",
              "toAmountMin": "10000000000000000",
              "exchangeRate": "1.0",
              "priceImpact": "0.00",
              "stage": "0",
              "provider": "Native Wrapper",
              "logoURI": "https://raw.githubusercontent.com/0xsquid/assets/main/images/providers/weth.svg",
              "description": "Wrap ETH to WETH"
            },
            {
              "type": "swap",
              "chainType": "evm",
              "fromChain": "42161",
              "toChain": "42161",
              "fromToken": {
                "type": "evm",
                "chainId": "42161",
                "address": "0x82af49447d8a07e3bd95bd0d56f35241523fbab1",
                "name": "Wrapped ETH",
                "symbol": "WETH",
                "decimals": "18",
                "logoURI": "https://raw.githubusercontent.com/0xsquid/assets/main/images/tokens/weth.svg",
                "coingeckoId": "weth",
                "axelarNetworkSymbol": "WETH",
                "subGraphIds": [
                  "arbitrum-weth-wei"
                ],
                "subGraphOnly": false,
                "usdPrice": "2583.905483882831"
              },
              "toToken": {
                "type": "evm",
                "chainId": "42161",
                "address": "0xaf88d065e77c8cC2239327C5EDb3A432268e5831",
                "name": "USD Coin",
                "symbol": "USDC",
                "decimals": "6",
                "logoURI": "https://raw.githubusercontent.com/0xsquid/assets/main/images/tokens/usdc.svg",
                "coingeckoId": "usd-coin",
                "subGraphIds": [
                  "uusdc",
                  "cctp-uusdc-arbitrum-to-noble"
                ],
                "subGraphOnly": false,
                "usdPrice": "0.999301671003392"
              },
              "fromAmount": "10000000000000000",
              "toAmount": "25826875",
              "toAmountMin": "25749394",
              "exchangeRate": "2582.6875",
              "priceImpact": "-0.0000627062536913",
              "stage": "0",
              "provider": "Uniswap V3",
              "logoURI": "https://raw.githubusercontent.com/0xsquid/assets/main/images/providers/uniswap.svg",
              "description": "Swap from WETH to USDC"
            }
          ],
          "fromAmount": "10000000000000000",
          "toAmount": "25826875",
          "toAmountMin": "25749394",
          "sendAmount": "10000000000000000",
          "exchangeRate": "2582.6875",
          "aggregatePriceImpact": "0.0",
          "fromAmountUSD": "25.81",
          "toAmountUSD": "25.70",
          "toAmountMinUSD": "25.62",
          "aggregateSlippage": "0.9600000000000001",
          "index": "0",
          "fromToken": {
            "type": "evm",
            "chainId": "42161",
            "address": "0xEeeeeEeeeEeEeeEeEeEeeEEEeeeeEeeeeeeeEEeE",
            "name": "Ethereum",
            "symbol": "ETH",
            "decimals": "18",
            "logoURI": "https://raw.githubusercontent.com/0xsquid/assets/main/images/tokens/eth.svg",
            "coingeckoId": "ethereum",
            "usdPrice": "2581.298038575404"
          },
          "toToken": {
            "type": "evm",
            "chainId": "42161",
            "address": "0xaf88d065e77c8cC2239327C5EDb3A432268e5831",
            "name": "USD Coin",
            "symbol": "USDC",
            "decimals": "6",
            "logoURI": "https://raw.githubusercontent.com/0xsquid/assets/main/images/tokens/usdc.svg",
            "coingeckoId": "usd-coin",
            "subGraphIds": [
              "uusdc",
              "cctp-uusdc-arbitrum-to-noble"
            ],
            "subGraphOnly": false,
            "usdPrice": "0.999301671003392"
          },
          "isBoostSupported": false,
          "feeCosts": [
            {
              "amount": "311053437062551",
              "amountUsd": "0.80",
              "description": "Gas receiver fee",
              "gasLimit": "696400",
              "gasMultiplier": "1.1550000000000002",
              "name": "Gas receiver fee",
              "token": {
                "type": "evm",
                "chainId": "42161",
                "address": "0xEeeeeEeeeEeEeeEeEeEeeEEEeeeeEeeeeeeeEEeE",
                "name": "Ethereum",
                "symbol": "ETH",
                "decimals": "18",
                "logoURI": "https://raw.githubusercontent.com/0xsquid/assets/main/images/tokens/eth.svg",
                "coingeckoId": "ethereum",
                "usdPrice": 2581.298038575404
              }
            }
          ],
          "gasCosts": [
            {
              "type": "executeCall",
              "token": {
                "type": "evm",
                "chainId": "42161",
                "address": "0xEeeeeEeeeEeEeeEeEeEeeEEEeeeeEeeeeeeeEEeE",
                "name": "Ethereum",
                "symbol": "ETH",
                "decimals": "18",
                "logoURI": "https://raw.githubusercontent.com/0xsquid/assets/main/images/tokens/eth.svg",
                "coingeckoId": "ethereum",
                "usdPrice": 2581.298038575404
              },
              "amount": "1452012968376000",
              "gasLimit": "953658",
              "amountUsd": "3.75"
            }
          ],
          "estimatedRouteDuration": "20"
        },
        "transactionRequest": {
          "routeType": "CALL_BRIDGE_CALL",
          "target": "0xce16F69375520ab01377ce7B88f5BA8C48F8D666",
          "data": "0xdeadbeef",
          "value": "1000368231439378717",
          "gasLimit": "995464",
          "lastBaseFeePerGas": "10000000",
          "maxFeePerGas": "1520000000",
          "maxPriorityFeePerGas": "1500000000",
          "gasPrice": "10000000",
          "requestId": "c8f8eb102224d0de969ce595612ef1ab"
        }
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
  EXPECT_CALL(callback, Run(EqualsMojo(mojom::SwapQuoteUnion::NewSquidQuote(
                                GetCannedSquidQuote())),
                            EqualsMojo(expected_swap_fees.Clone()),
                            EqualsMojo(mojom::SwapErrorUnionPtr()), ""));
  auto quote_params = GetCannedSwapQuoteParams(
      mojom::CoinType::ETH, mojom::kPolygonMainnetChainId, "ETH",
      mojom::CoinType::ETH, mojom::kArbitrumMainnetChainId, "USDC",
      mojom::SwapProvider::kSquid);
  swap_service_->GetQuote(std::move(quote_params), callback.Get());
  task_environment_.RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&callback);
}

TEST_F(SwapServiceUnitTest, GetSquidTransaction) {
  SetInterceptor(R"(
    {
      "route": {
        "estimate": {
          "actions": [],
          "fromAmount": "10000000000000000",
          "toAmount": "49836297930093733",
          "toAmountMin": "49686789036303451",
          "sendAmount": "10000000000000000",
          "exchangeRate": "4.9836297930093733",
          "aggregatePriceImpact": "0.0",
          "fromAmountUSD": "25.81",
          "toAmountUSD": "25.70",
          "toAmountMinUSD": "25.62",
          "aggregateSlippage": "0.9600000000000001",
          "index": "0",
          "fromToken": {
            "type": "evm",
            "chainId": "42161",
            "address": "0xEeeeeEeeeEeEeeEeEeEeeEEEeeeeEeeeeeeeEEeE",
            "name": "Ethereum",
            "symbol": "ETH",
            "decimals": "18",
            "logoURI": "https://raw.githubusercontent.com/0xsquid/assets/main/images/tokens/eth.svg",
            "coingeckoId": "ethereum",
            "usdPrice": "2581.298038575404"
          },
          "toToken": {
            "type": "evm",
            "chainId": "56",
            "address": "0xEeeeeEeeeEeEeeEeEeEeeEEEeeeeEeeeeeeeEEeE",
            "name": "BNB",
            "symbol": "BNB",
            "decimals": "18",
            "logoURI": "https://raw.githubusercontent.com/0xsquid/assets/main/images/tokens/bnb.svg",
            "coingeckoId": "binancecoin",
            "axelarNetworkSymbol": "WBNB",
            "subGraphIds": [
              "wbnb-wei"
            ],
            "subGraphOnly": false,
            "usdPrice": "515.821925157587"
          },
          "isBoostSupported": true,
          "feeCosts": [],
          "gasCosts": [],
          "estimatedRouteDuration": "20"
        },
        "transactionRequest": {
          "routeType": "CALL_BRIDGE_CALL",
          "target": "0xce16F69375520ab01377ce7B88f5BA8C48F8D666",
          "data": "0xdeadbeef",
          "value": "1000368231439378717",
          "gasLimit": "995464",
          "lastBaseFeePerGas": "10000000",
          "maxFeePerGas": "1520000000",
          "maxPriorityFeePerGas": "1500000000",
          "gasPrice": "10000000",
          "requestId": "c8f8eb102224d0de969ce595612ef1ab"
        }
      }
    }
  )");

  auto expected_transaction = mojom::SquidEvmTransaction::New();
  expected_transaction->data = "0xdeadbeef";
  expected_transaction->target = "0xce16F69375520ab01377ce7B88f5BA8C48F8D666";
  expected_transaction->value = "1000368231439378717";
  expected_transaction->gas_limit = "995464";
  expected_transaction->last_base_fee_per_gas = "10000000";
  expected_transaction->max_fee_per_gas = "1520000000";
  expected_transaction->max_priority_fee_per_gas = "1500000000";
  expected_transaction->gas_price = "10000000";
  expected_transaction->chain_id = mojom::kArbitrumMainnetChainId;

  auto quote_params = GetCannedSwapQuoteParams(
      mojom::CoinType::ETH, mojom::kArbitrumMainnetChainId, "ETH",
      mojom::CoinType::ETH, mojom::kArbitrumMainnetChainId, "USDC",
      mojom::SwapProvider::kSquid);

  base::MockCallback<mojom::SwapService::GetTransactionCallback> callback;
  EXPECT_CALL(callback,
              Run(EqualsMojo(mojom::SwapTransactionUnion::NewSquidTransaction(
                      mojom::SquidTransactionUnion::NewEvmTransaction(
                          std::move(expected_transaction)))),
                  EqualsMojo(mojom::SwapErrorUnionPtr()), ""));

  swap_service_->GetTransaction(
      mojom::SwapTransactionParamsUnion::NewSquidTransactionParams(
          std::move(quote_params)),
      callback.Get());
  task_environment_.RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&callback);
}

TEST_F(SwapServiceUnitTest, GetSquidQuoteError) {
  std::string error = R"(
    {
      "message": "onChainQuoting must be a `boolean` type.",
      "statusCode": "400",
      "type": "SCHEMA_VALIDATION_ERROR"
    }
  )";
  SetErrorInterceptor(error);

  base::MockCallback<mojom::SwapService::GetQuoteCallback> callback;

  EXPECT_CALL(
      callback,
      Run(EqualsMojo(mojom::SwapQuoteUnionPtr()),
          EqualsMojo(mojom::SwapFeesPtr()),
          EqualsMojo(
              mojom::SwapErrorUnion::NewSquidError(mojom::SquidError::New(
                  "onChainQuoting must be a `boolean` type.",
                  mojom::SquidErrorType::kSchemaValidationError, false))),
          ""));

  swap_service_->GetQuote(
      GetCannedSwapQuoteParams(
          mojom::CoinType::ETH, mojom::kArbitrumMainnetChainId, "ETH",
          mojom::CoinType::ETH, mojom::kArbitrumMainnetChainId, "USDC",
          mojom::SwapProvider::kSquid),
      callback.Get());
  task_environment_.RunUntilIdle();
}

TEST_F(SwapServiceUnitTest, GetSquidTransactionError) {
  std::string error = R"(
    {
      "message": "onChainQuoting must be a `boolean` type.",
      "statusCode": "400",
      "type": "SCHEMA_VALIDATION_ERROR"
    }
  )";
  SetErrorInterceptor(error);

  auto quote_params = GetCannedSwapQuoteParams(
      mojom::CoinType::ETH, mojom::kArbitrumMainnetChainId, "ETH",
      mojom::CoinType::ETH, mojom::kArbitrumMainnetChainId, "USDC",
      mojom::SwapProvider::kSquid);

  base::MockCallback<mojom::SwapService::GetTransactionCallback> callback;

  EXPECT_CALL(
      callback,
      Run(EqualsMojo(mojom::SwapTransactionUnionPtr()),
          EqualsMojo(
              mojom::SwapErrorUnion::NewSquidError(mojom::SquidError::New(
                  "onChainQuoting must be a `boolean` type.",
                  mojom::SquidErrorType::kSchemaValidationError, false))),
          ""));

  swap_service_->GetTransaction(
      mojom::SwapTransactionParamsUnion::NewSquidTransactionParams(
          std::move(quote_params)),
      callback.Get());
  task_environment_.RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&callback);
}

}  // namespace brave_wallet
