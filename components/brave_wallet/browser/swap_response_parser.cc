/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/swap_response_parser.h"

#include <optional>
#include <utility>
#include <vector>

#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/json_rpc_requests_helper.h"
#include "brave/components/brave_wallet/browser/json_rpc_response_parser.h"
#include "brave/components/brave_wallet/browser/swap_responses.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom-shared.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "brave/components/brave_wallet/common/string_utils.h"
#include "brave/components/json/json_helper.h"
#include "url/gurl.h"

namespace brave_wallet {

namespace {
std::optional<std::string> ParseNullableString(const base::Value& value) {
  if (value.is_none()) {
    return std::nullopt;
  }

  if (value.is_string()) {
    return value.GetString();
  }

  if (value.is_int()) {
    return base::NumberToString(value.GetInt());
  }

  if (value.is_double()) {
    return base::NumberToString(value.GetDouble());
  }

  return std::nullopt;
}

std::optional<std::vector<uint8_t>> ParseStringAsBytes(
    const base::Value& value) {
  auto str = ParseNullableString(value);
  if (!str) {
    return std::nullopt;
  }
  if (str->empty()) {
    return std::nullopt;
  }
  return std::vector<uint8_t>(str->begin(), str->end());
}

}  // namespace

namespace zeroex {
namespace {

mojom::ZeroExFeePtr ParseZeroExFee(const base::Value& value) {
  if (value.is_none()) {
    return nullptr;
  }

  if (!value.is_dict()) {
    return nullptr;
  }

  auto zero_ex_fee_value =
      swap_responses::ZeroExFee::FromValue(value.GetDict());
  if (!zero_ex_fee_value) {
    return nullptr;
  }

  auto zero_ex_fee = mojom::ZeroExFee::New();
  zero_ex_fee->token = zero_ex_fee_value->token;
  zero_ex_fee->amount = zero_ex_fee_value->amount;
  zero_ex_fee->type = zero_ex_fee_value->type;

  return zero_ex_fee;
}

mojom::ZeroExRoutePtr ParseRoute(const swap_responses::ZeroExRoute& value) {
  auto route = mojom::ZeroExRoute::New();
  for (const auto& fill_value : value.fills) {
    auto fill = mojom::ZeroExRouteFill::New();
    fill->from = fill_value.from;
    fill->to = fill_value.to;
    fill->source = fill_value.source;
    fill->proportion_bps = fill_value.proportion_bps;
    route->fills.push_back(std::move(fill));
  }

  return route;
}

mojom::ZeroExQuotePtr ParseQuote(
    const swap_responses::ZeroExQuoteResponse& value) {
  auto quote = mojom::ZeroExQuote::New();

  if (value.buy_amount.has_value()) {
    quote->buy_amount = value.buy_amount.value();
  } else {
    return nullptr;
  }

  if (value.buy_token.has_value()) {
    quote->buy_token = value.buy_token.value();
  } else {
    return nullptr;
  }

  if (value.gas.has_value()) {
    quote->gas = value.gas.value();
  } else {
    return nullptr;
  }

  if (value.gas_price.has_value()) {
    quote->gas_price = value.gas_price.value();
  } else {
    return nullptr;
  }

  quote->liquidity_available = value.liquidity_available;

  if (value.min_buy_amount.has_value()) {
    quote->min_buy_amount = value.min_buy_amount.value();
  } else {
    return nullptr;
  }

  if (value.sell_amount.has_value()) {
    quote->sell_amount = value.sell_amount.value();
  } else {
    return nullptr;
  }

  if (value.sell_token.has_value()) {
    quote->sell_token = value.sell_token.value();
  } else {
    return nullptr;
  }

  if (value.total_network_fee.has_value()) {
    quote->total_network_fee = value.total_network_fee.value();
  } else {
    return nullptr;
  }

  if (value.route.has_value()) {
    quote->route = ParseRoute(value.route.value());
  } else {
    return nullptr;
  }

  if (value.fees.has_value()) {
    auto fees = mojom::ZeroExFees::New();
    if (auto zero_ex_fee = ParseZeroExFee(value.fees.value().zero_ex_fee);
        zero_ex_fee) {
      fees->zero_ex_fee = std::move(zero_ex_fee);
    }
    quote->fees = std::move(fees);
  } else {
    return nullptr;
  }

  return quote;
}

}  // namespace

mojom::ZeroExQuotePtr ParseQuoteResponse(const base::Value& json_value,
                                         const std::string& chain_id) {
  // {
  //   "blockNumber": "20114692",
  //   "buyAmount": "100037537",
  //   "buyToken": "0xdac17f958d2ee523a2206206994597c13d831ec7",
  //   "fees": {
  //     "integratorFee": null,
  //     "zeroExFee": null,
  //     "gasFee": null
  //   },
  //   "issues": {
  //     "allowance": {
  //       "actual": "0",
  //       "spender": "0x0000000000001ff3684f28c67538d4d072c22734"
  //     },
  //     "balance": {
  //       "token": "0xa0b86991c6218b36c1d19d4a2e9eb0ce3606eb48",
  //       "actual": "0",
  //       "expected": "100000000"
  //     },
  //     "simulationIncomplete": false,
  //     "invalidSourcesPassed": []
  //   },
  //   "liquidityAvailable": true,
  //   "minBuyAmount": "99037162",
  //   "route": {
  //     "fills": [
  //       {
  //         "from": "0xa0b86991c6218b36c1d19d4a2e9eb0ce3606eb48",
  //         "to": "0xdac17f958d2ee523a2206206994597c13d831ec7",
  //         "source": "SolidlyV3",
  //         "proportionBps": "10000"
  //       }
  //     ],
  //     "tokens": [
  //       {
  //         "address": "0xa0b86991c6218b36c1d19d4a2e9eb0ce3606eb48",
  //         "symbol": "USDC"
  //       },
  //       {
  //         "address": "0xdac17f958d2ee523a2206206994597c13d831ec7",
  //         "symbol": "USDT"
  //       }
  //     ]
  //   },
  //   "sellAmount": "100000000",
  //   "sellToken": "0xa0b86991c6218b36c1d19d4a2e9eb0ce3606eb48",
  //   "tokenMetadata": {
  //     "buyToken": {
  //       "buyTaxBps": "0",
  //       "sellTaxBps": "0"
  //     },
  //     "sellToken": {
  //       "buyTaxBps": "0",
  //       "sellTaxBps": "0"
  //     }
  //   },
  //   "totalNetworkFee": "1393685870940000",
  //   "transaction": {
  //     "to": "0x7f6cee965959295cc64d0e6c00d99d6532d8e86b",
  //     "data":
  //     "0x1fff991f00000000000000000000000070a9f34f9b34c64957b9c401a97bfed35b95049e000000000000000000000000dac17f958d2ee523a2206206994597c13d831ec70000000000000000000000000000000000000000000000000000000005e72fea00000000000000000000000000000000000000000000000000000000000000a000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000002000000000000000000000000000000000000000000000000000000000000004000000000000000000000000000000000000000000000000000000000000001c00000000000000000000000000000000000000000000000000000000000000144c1fb425e0000000000000000000000007f6cee965959295cc64d0e6c00d99d6532d8e86b000000000000000000000000a0b86991c6218b36c1d19d4a2e9eb0ce3606eb480000000000000000000000000000000000000000000000000000000005f5e1000000000000000000000000000000000000006e898131631616b1779bad70bc17000000000000000000000000000000000000000000000000000000006670d06c00000000000000000000000000000000000000000000000000000000000000c00000000000000000000000000000000000000000000000000000000000000041ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000016438c9c147000000000000000000000000a0b86991c6218b36c1d19d4a2e9eb0ce3606eb4800000000000000000000000000000000000000000000000000000000000027100000000000000000000000006146be494fee4c73540cb1c5f87536abf1452500000000000000000000000000000000000000000000000000000000000000004400000000000000000000000000000000000000000000000000000000000000a00000000000000000000000000000000000000000000000000000000000000084c31b8d7a0000000000000000000000007f6cee965959295cc64d0e6c00d99d6532d8e86b00000000000000000000000000000000000000000000000000000000000000010000000000000000000000000000000000000000000000000000000005f5e10000000000000000000000000000000000000000000000000000000001000276a40000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
  //     "gas": "288079",
  //     "gasPrice": "4837860000",
  //     "value": "0"
  //   },
  //   "zid": "0x111111111111111111111111"
  // }

  auto swap_response_value =
      swap_responses::ZeroExQuoteResponse::FromValue(json_value);
  if (!swap_response_value) {
    return nullptr;
  }

  auto swap_response = mojom::ZeroExQuote::New();

  if (!swap_response_value->liquidity_available) {
    swap_response->liquidity_available = false;
    return swap_response;
  }

  swap_response = ParseQuote(swap_response_value.value());
  if (!swap_response) {
    return nullptr;
  }

  swap_response->allowance_target =
      GetZeroExAllowanceHolderAddress(chain_id).value_or("");

  return swap_response;
}

mojom::ZeroExTransactionPtr ParseTransactionResponse(
    const base::Value& json_value) {
  auto swap_response_value =
      swap_responses::ZeroExTransactionResponse::FromValue(json_value);
  if (!swap_response_value) {
    return nullptr;
  }

  auto transaction = mojom::ZeroExTransaction::New();
  transaction->to = swap_response_value->transaction.to;
  transaction->data = swap_response_value->transaction.data;
  transaction->gas = swap_response_value->transaction.gas;
  transaction->gas_price = swap_response_value->transaction.gas_price;
  transaction->value = swap_response_value->transaction.value;

  return transaction;
}

mojom::ZeroExErrorPtr ParseErrorResponse(const base::Value& json_value) {
  // {
  //    "code": "SWAP_VALIDATION_FAILED",
  //    "message": "Validation Failed"
  // }
  auto swap_error_response_value =
      swap_responses::ZeroExErrorResponse::FromValue(json_value);
  if (!swap_error_response_value) {
    return nullptr;
  }

  auto result = mojom::ZeroExError::New();
  result->name = swap_error_response_value->name;
  result->message = swap_error_response_value->message;

  return result;
}

}  // namespace zeroex

namespace jupiter {
constexpr char kNoRoutesMessage[] =
    "No routes found for the input and output mints";
mojom::JupiterQuotePtr ParseQuoteResponse(const base::Value& json_value) {
  // {
  //   "inputMint": "So11111111111111111111111111111111111111112",
  //   "inAmount": "1000000",
  //   "outputMint": "DezXAZ8z7PnrnRJjz3wXBoRgixCa6xjnB7YaB1pPB263",
  //   "outAmount": "781469842",
  //   "otherAmountThreshold": "781391696",
  //   "swapMode": "ExactIn",
  //   "slippageBps": "1",
  //   "platformFee": null,
  //   "priceImpactPct": "0",
  //   "routePlan": [
  //     {
  //       "swapInfo": {
  //         "ammKey": "HCk6LA93xPVsF8g4v6gjkiCd88tLXwZq4eJwiYNHR8da",
  //         "label": "Raydium",
  //         "inputMint": "So11111111111111111111111111111111111111112",
  //         "outputMint": "HhJpBhRRn4g56VsyLuT8DL5Bv31HkXqsrahTTUCZeZg4",
  //         "inAmount": "997500",
  //         "outAmount": "4052482154",
  //         "feeAmount": "2500",
  //         "feeMint": "So11111111111111111111111111111111111111112"
  //       },
  //       "percent": "100"
  //     },
  //     {
  //       "swapInfo": {
  //         "ammKey": "HqrRmb2MbEiTrJS5KXhDzUoKbSLbBXJvhNBGEyDNo9Tr",
  //         "label": "Meteora",
  //         "inputMint": "HhJpBhRRn4g56VsyLuT8DL5Bv31HkXqsrahTTUCZeZg4",
  //         "outputMint": "dipQRV1bWwJbZ3A2wHohXiTZC77CzFGigbFEcvsyMrS",
  //         "inAmount": "4052482154",
  //         "outAmount": "834185227",
  //         "feeAmount": "10131205",
  //         "feeMint": "dipQRV1bWwJbZ3A2wHohXiTZC77CzFGigbFEcvsyMrS"
  //       },
  //       "percent": "100"
  //     },
  //     {
  //       "swapInfo": {
  //         "ammKey": "6shkv2VNBPWVABvShgcGmrv98Z1vR3EcdwND6XUwGoFq",
  //         "label": "Meteora",
  //         "inputMint": "dipQRV1bWwJbZ3A2wHohXiTZC77CzFGigbFEcvsyMrS",
  //         "outputMint": "DezXAZ8z7PnrnRJjz3wXBoRgixCa6xjnB7YaB1pPB263",
  //         "inAmount": "834185227",
  //         "outAmount": "781469842",
  //         "feeAmount": "2085463",
  //         "feeMint": "DezXAZ8z7PnrnRJjz3wXBoRgixCa6xjnB7YaB1pPB263"
  //       },
  //       "percent": "100"
  //     }
  //   ]
  // }
  auto quote_value =
      swap_responses::JupiterQuoteResponse::FromValue(json_value);
  if (!quote_value) {
    return nullptr;
  }

  auto swap_quote = mojom::JupiterQuote::New();
  swap_quote->input_mint = quote_value->input_mint;
  swap_quote->in_amount = quote_value->in_amount;
  swap_quote->output_mint = quote_value->output_mint;
  swap_quote->out_amount = quote_value->out_amount;
  swap_quote->other_amount_threshold = quote_value->other_amount_threshold;
  swap_quote->swap_mode = quote_value->swap_mode;
  swap_quote->slippage_bps = quote_value->slippage_bps;
  swap_quote->price_impact_pct = quote_value->price_impact_pct;

  if (!quote_value->platform_fee.is_none()) {
    if (!quote_value->platform_fee.is_dict()) {
      return nullptr;
    }

    auto platform_fee_value = swap_responses::JupiterPlatformFee::FromValue(
        quote_value->platform_fee.GetDict());

    swap_quote->platform_fee = mojom::JupiterPlatformFee::New();
    swap_quote->platform_fee->amount = platform_fee_value->amount;
    swap_quote->platform_fee->fee_bps = platform_fee_value->fee_bps;
  }

  for (const auto& step_value : quote_value->route_plan) {
    auto step = mojom::JupiterRouteStep::New();
    step->percent = step_value.percent;

    auto swap_info = mojom::JupiterSwapInfo::New();
    swap_info->amm_key = step_value.swap_info.amm_key;
    swap_info->label = step_value.swap_info.label;
    swap_info->input_mint = step_value.swap_info.input_mint;
    swap_info->output_mint = step_value.swap_info.output_mint;
    swap_info->in_amount = step_value.swap_info.in_amount;
    swap_info->out_amount = step_value.swap_info.out_amount;
    swap_info->fee_amount = step_value.swap_info.fee_amount;
    swap_info->fee_mint = step_value.swap_info.fee_mint;
    step->swap_info = std::move(swap_info);

    swap_quote->route_plan.push_back(std::move(step));
  }

  return swap_quote;
}

std::optional<std::string> ParseTransactionResponse(
    const base::Value& json_value) {
  auto value = swap_responses::JupiterSwapTransactions::FromValue(json_value);
  if (!value) {
    return std::nullopt;
  }

  return value->swap_transaction;
}

mojom::JupiterErrorPtr ParseErrorResponse(const base::Value& json_value) {
  auto jupiter_error_response_value =
      swap_responses::JupiterErrorResponse::FromValue(json_value);
  if (!jupiter_error_response_value) {
    return nullptr;
  }

  auto result = mojom::JupiterError::New();
  result->status_code = jupiter_error_response_value->status_code;
  result->error = jupiter_error_response_value->error;
  result->message = jupiter_error_response_value->message;

  result->is_insufficient_liquidity =
      result->message.contains(kNoRoutesMessage);

  return result;
}
}  // namespace jupiter

namespace gate3 {

namespace {

std::optional<mojom::SwapProvider> ParseProvider(
    const swap_responses::Gate3Provider& value) {
  switch (value) {
    case swap_responses::Gate3Provider::kAuto:
      return mojom::SwapProvider::kAuto;
    case swap_responses::Gate3Provider::kNearIntents:
      return mojom::SwapProvider::kNearIntents;
    case swap_responses::Gate3Provider::kZeroEx:
      return mojom::SwapProvider::kZeroEx;
    case swap_responses::Gate3Provider::kJupiter:
      return mojom::SwapProvider::kJupiter;
    case swap_responses::Gate3Provider::kLifi:
      return mojom::SwapProvider::kLiFi;
    case swap_responses::Gate3Provider::kSquid:
      return mojom::SwapProvider::kSquid;
    default:
      return std::nullopt;
  }
}

mojom::Gate3SwapErrorKind ParseErrorKind(
    const swap_responses::Gate3ErrorKind& value) {
  switch (value) {
    case swap_responses::Gate3ErrorKind::kInsufficientLiquidity:
      return mojom::Gate3SwapErrorKind::kInsufficientLiquidity;
    case swap_responses::Gate3ErrorKind::kAmountTooLow:
      return mojom::Gate3SwapErrorKind::kAmountTooLow;
    case swap_responses::Gate3ErrorKind::kUnsupportedNetwork:
      return mojom::Gate3SwapErrorKind::kUnsupportedNetwork;
    case swap_responses::Gate3ErrorKind::kUnsupportedTokens:
      return mojom::Gate3SwapErrorKind::kUnsupportedTokens;
    case swap_responses::Gate3ErrorKind::kInvalidRequest:
      return mojom::Gate3SwapErrorKind::kInvalidRequest;
    case swap_responses::Gate3ErrorKind::kUnknown:
    case swap_responses::Gate3ErrorKind::kNone:
    default:
      return mojom::Gate3SwapErrorKind::kUnknown;
  }
}

std::optional<mojom::CoinType> ParseCoinType(const std::string& value) {
  const std::string upper_value = base::ToUpperASCII(value);
  if (upper_value == "ETH") {
    return mojom::CoinType::ETH;
  }
  if (upper_value == "SOL") {
    return mojom::CoinType::SOL;
  }
  if (upper_value == "BTC") {
    return mojom::CoinType::BTC;
  }
  if (upper_value == "FIL") {
    return mojom::CoinType::FIL;
  }
  if (upper_value == "ZEC") {
    return mojom::CoinType::ZEC;
  }
  if (upper_value == "ADA") {
    return mojom::CoinType::ADA;
  }
  return std::nullopt;
}

mojom::ChainIdPtr ParseChainId(const swap_responses::Gate3ChainSpec& value) {
  auto coin_type = ParseCoinType(value.coin);
  if (!coin_type) {
    return nullptr;
  }

  auto result = mojom::ChainId::New();
  result->coin = *coin_type;
  result->chain_id = value.chain_id;
  return result;
}

mojom::Gate3SwapToolPtr ParseTool(const swap_responses::Gate3SwapTool& value) {
  auto result = mojom::Gate3SwapTool::New();
  result->name = value.name;
  result->logo = ParseNullableString(value.logo).value_or("");
  return result;
}

mojom::Gate3SwapStepTokenPtr ParseStepToken(
    const swap_responses::Gate3SwapStepToken& value) {
  auto result = mojom::Gate3SwapStepToken::New();

  auto coin = ParseCoinType(value.coin);
  if (!coin) {
    return nullptr;
  }
  result->coin = *coin;

  result->chain_id = value.chain_id;
  result->contract_address =
      ParseNullableString(value.contract_address).value_or("");
  result->symbol = value.symbol;

  result->logo = ParseNullableString(value.logo).value_or("");
  return result;
}

mojom::Gate3SwapRouteStepPtr ParseRouteStep(
    const swap_responses::Gate3SwapRouteStep& value) {
  auto result = mojom::Gate3SwapRouteStep::New();
  result->source_token = ParseStepToken(value.source_token);
  if (!result->source_token) {
    return nullptr;
  }
  result->source_amount = value.source_amount;
  result->destination_token = ParseStepToken(value.destination_token);
  if (!result->destination_token) {
    return nullptr;
  }
  result->destination_amount = value.destination_amount;
  result->tool = ParseTool(value.tool);
  result->percent = value.percent;
  return result;
}

mojom::Gate3SwapTransactionParamsUnionPtr ParseTransactionParams(
    const base::Value& value) {
  if (value.is_none()) {
    return nullptr;
  }

  if (!value.is_dict()) {
    return nullptr;
  }

  const auto& dict = value.GetDict();

  // Check for EVM transaction params
  if (const auto* evm_value = dict.Find("evm");
      evm_value && !evm_value->is_none()) {
    auto evm_params =
        swap_responses::Gate3SwapEvmTransactionParams::FromValue(*evm_value);
    if (!evm_params) {
      return nullptr;
    }
    auto chain = ParseChainId(evm_params->chain);
    if (!chain) {
      return nullptr;
    }
    auto evm_result = mojom::Gate3SwapEvmTransactionParams::New();
    evm_result->chain = std::move(chain);
    evm_result->from = evm_params->from;
    evm_result->to = evm_params->to;
    evm_result->value = evm_params->value;
    evm_result->data = evm_params->data;
    evm_result->gas_limit = evm_params->gas_limit;
    evm_result->gas_price = ParseNullableString(evm_params->gas_price);
    return mojom::Gate3SwapTransactionParamsUnion::NewEvmTransactionParams(
        std::move(evm_result));
  }

  // Check for Solana transaction params
  if (const auto* solana_value = dict.Find("solana");
      solana_value && !solana_value->is_none()) {
    auto solana_params =
        swap_responses::Gate3SwapSolanaTransactionParams::FromValue(
            *solana_value);
    if (!solana_params) {
      return nullptr;
    }
    auto chain = ParseChainId(solana_params->chain);
    if (!chain) {
      return nullptr;
    }
    auto solana_result = mojom::Gate3SwapSolanaTransactionParams::New();
    solana_result->chain = std::move(chain);
    solana_result->from = solana_params->from;
    solana_result->to = solana_params->to;
    solana_result->lamports = solana_params->lamports;

    solana_result->spl_token_mint =
        ParseNullableString(solana_params->spl_token_mint);
    solana_result->spl_token_amount =
        ParseNullableString(solana_params->spl_token_amount);
    solana_result->decimals = ParseNullableString(solana_params->decimals);

    solana_result->versioned_transaction =
        ParseNullableString(solana_params->versioned_transaction);

    solana_result->compute_unit_limit =
        ParseNullableString(solana_params->compute_unit_limit);
    solana_result->compute_unit_price =
        ParseNullableString(solana_params->compute_unit_price);

    return mojom::Gate3SwapTransactionParamsUnion::NewSolanaTransactionParams(
        std::move(solana_result));
  }

  // Check for Bitcoin transaction params
  if (const auto* btc_value = dict.Find("bitcoin");
      btc_value && !btc_value->is_none()) {
    auto btc_params =
        swap_responses::Gate3SwapBitcoinTransactionParams::FromValue(
            *btc_value);
    if (!btc_params) {
      return nullptr;
    }
    auto chain = ParseChainId(btc_params->chain);
    if (!chain) {
      return nullptr;
    }
    auto btc_result = mojom::Gate3SwapBitcoinTransactionParams::New();
    btc_result->chain = std::move(chain);
    btc_result->to = btc_params->to;
    btc_result->value = btc_params->value;
    btc_result->refund_to = btc_params->refund_to;
    return mojom::Gate3SwapTransactionParamsUnion::NewBitcoinTransactionParams(
        std::move(btc_result));
  }

  // Check for Cardano transaction params
  if (const auto* cardano_value = dict.Find("cardano");
      cardano_value && !cardano_value->is_none()) {
    auto cardano_params =
        swap_responses::Gate3SwapCardanoTransactionParams::FromValue(
            *cardano_value);
    if (!cardano_params) {
      return nullptr;
    }
    auto chain = ParseChainId(cardano_params->chain);
    if (!chain) {
      return nullptr;
    }
    auto cardano_result = mojom::Gate3SwapCardanoTransactionParams::New();
    cardano_result->chain = std::move(chain);
    cardano_result->to = cardano_params->to;
    cardano_result->value = cardano_params->value;
    cardano_result->refund_to = cardano_params->refund_to;
    return mojom::Gate3SwapTransactionParamsUnion::NewCardanoTransactionParams(
        std::move(cardano_result));
  }

  // Check for ZCash transaction params
  if (const auto* zcash_value = dict.Find("zcash");
      zcash_value && !zcash_value->is_none()) {
    auto zcash_params =
        swap_responses::Gate3SwapZCashTransactionParams::FromValue(
            *zcash_value);
    if (!zcash_params) {
      return nullptr;
    }
    auto chain = ParseChainId(zcash_params->chain);
    if (!chain) {
      return nullptr;
    }
    auto zcash_result = mojom::Gate3SwapZCashTransactionParams::New();
    zcash_result->chain = std::move(chain);
    zcash_result->to = zcash_params->to;
    zcash_result->value = zcash_params->value;
    zcash_result->refund_to = zcash_params->refund_to;
    return mojom::Gate3SwapTransactionParamsUnion::NewZcashTransactionParams(
        std::move(zcash_result));
  }

  return nullptr;
}

mojom::Gate3SwapNetworkFeePtr ParseNetworkFee(
    const swap_responses::Gate3SwapNetworkFee& value) {
  auto result = mojom::Gate3SwapNetworkFee::New();
  result->amount = value.amount;

  // Convert string decimals to int32
  int32_t decimals = 0;
  if (base::StringToInt(value.decimals, &decimals)) {
    result->decimals = decimals;
  } else {
    return nullptr;
  }

  result->symbol = value.symbol;
  return result;
}

mojom::Gate3SwapRoutePtr ParseRoute(
    const swap_responses::Gate3SwapRoute& value) {
  auto provider = ParseProvider(value.provider);
  if (!provider) {
    return nullptr;
  }

  auto result = mojom::Gate3SwapRoute::New();
  result->id = value.id;
  result->provider = *provider;

  for (const auto& step_value : value.steps) {
    auto step = ParseRouteStep(step_value);
    if (!step) {
      return nullptr;
    }
    result->steps.push_back(std::move(step));
  }

  result->source_amount = value.source_amount;
  result->destination_amount = value.destination_amount;
  result->destination_amount_min = value.destination_amount_min;
  result->estimated_time = ParseNullableString(value.estimated_time);
  result->price_impact = ParseNullableString(value.price_impact);

  if (!value.network_fee.is_none()) {
    auto network_fee_value =
        swap_responses::Gate3SwapNetworkFee::FromValue(value.network_fee);
    if (network_fee_value) {
      result->network_fee = ParseNetworkFee(*network_fee_value);
    }
  }

  result->gasless = value.gasless;
  result->deposit_address = ParseNullableString(value.deposit_address);
  result->deposit_memo = ParseStringAsBytes(value.deposit_memo);
  result->expires_at = ParseNullableString(value.expires_at);
  result->slippage_percentage = value.slippage_percentage;

  if (!value.transaction_params.is_none()) {
    auto transaction_params = ParseTransactionParams(value.transaction_params);

    if (!transaction_params) {
      return nullptr;
    }

    result->transaction_params = std::move(transaction_params);
  }

  result->requires_token_allowance = value.requires_token_allowance;
  result->requires_firm_route = value.requires_firm_route;

  return result;
}

}  // namespace

mojom::Gate3SwapQuotePtr ParseQuoteResponse(const base::Value& json_value) {
  auto value = swap_responses::Gate3Quote::FromValue(json_value);
  if (!value) {
    return nullptr;
  }

  if (value->routes.empty()) {
    return nullptr;
  }

  auto result = mojom::Gate3SwapQuote::New();

  for (const auto& route_value : value->routes) {
    auto route = ParseRoute(route_value);
    if (!route) {
      return nullptr;
    }
    result->routes.push_back(std::move(route));
  }

  return result;
}

mojom::Gate3SwapErrorPtr ParseErrorResponse(const base::Value& json_value) {
  auto value = swap_responses::Gate3Error::FromValue(json_value);
  if (!value) {
    return nullptr;
  }

  auto result = mojom::Gate3SwapError::New();
  result->message = value->message;
  result->kind = ParseErrorKind(value->kind);
  return result;
}

mojom::Gate3SwapStatusPtr ParseStatusResponse(const base::Value& json_value) {
  auto value = swap_responses::Gate3StatusResponse::FromValue(json_value);
  if (!value) {
    return nullptr;
  }

  auto result = mojom::Gate3SwapStatus::New();

  // Map the status code
  switch (value->status) {
    case swap_responses::Gate3StatusCode::kPending:
      result->status = mojom::Gate3SwapStatusCode::kPending;
      break;
    case swap_responses::Gate3StatusCode::kProcessing:
      result->status = mojom::Gate3SwapStatusCode::kProcessing;
      break;
    case swap_responses::Gate3StatusCode::kSuccess:
      result->status = mojom::Gate3SwapStatusCode::kSuccess;
      break;
    case swap_responses::Gate3StatusCode::kFailed:
      result->status = mojom::Gate3SwapStatusCode::kFailed;
      break;
    case swap_responses::Gate3StatusCode::kRefunded:
      result->status = mojom::Gate3SwapStatusCode::kRefunded;
      break;
    case swap_responses::Gate3StatusCode::kNone:
    default:
      // Invalid/unknown status, default to pending
      result->status = mojom::Gate3SwapStatusCode::kPending;
      break;
  }

  result->internal_status = value->internal_status;
  GURL explorer_gurl(value->explorer_url);
  if (explorer_gurl.is_valid() && explorer_gurl.SchemeIs("https")) {
    result->explorer_url = value->explorer_url;
  }

  return result;
}

}  // namespace gate3

}  // namespace brave_wallet
