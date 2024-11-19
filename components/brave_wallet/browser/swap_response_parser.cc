/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/swap_response_parser.h"

#include <optional>
#include <utility>
#include <vector>

#include "base/containers/contains.h"
#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/json_rpc_requests_helper.h"
#include "brave/components/brave_wallet/browser/json_rpc_response_parser.h"
#include "brave/components/brave_wallet/browser/swap_responses.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom-shared.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "brave/components/brave_wallet/common/string_utils.h"
#include "brave/components/json/json_helper.h"

namespace brave_wallet {

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
      base::Contains(result->message, kNoRoutesMessage);

  return result;
}
}  // namespace jupiter

namespace lifi {

namespace {

std::optional<std::string> ChainIdToHex(const std::string& value) {
  // LiFi uses the following two chain ID strings interchangeably for Solana
  // Ref: https://docs.li.fi/li.fi-api/solana/request-examples
  if (value == "SOL" || value == kLiFiSolanaMainnetChainID) {
    return mojom::kSolanaMainnet;
  }

  std::optional<uint256_t> out = Base10ValueToUint256(value);
  if (!out) {
    return std::nullopt;
  }

  return Uint256ValueToHex(*out);
}

mojom::LiFiStatusCode ParseStatusCode(const swap_responses::LiFiStatus& value) {
  switch (value) {
    case swap_responses::LiFiStatus::kNotFound:
      return mojom::LiFiStatusCode::kNotFound;

    case swap_responses::LiFiStatus::kInvalid:
      return mojom::LiFiStatusCode::kInvalid;

    case swap_responses::LiFiStatus::kPending:
      return mojom::LiFiStatusCode::kPending;

    case swap_responses::LiFiStatus::kDone:
      return mojom::LiFiStatusCode::kDone;

    case swap_responses::LiFiStatus::kFailed:
      return mojom::LiFiStatusCode::kFailed;

    default:
      return mojom::LiFiStatusCode::kInvalid;
  }
}

mojom::LiFiSubstatusCode ParseSubstatusCode(
    const swap_responses::LiFiSubstatus& value) {
  switch (value) {
    case swap_responses::LiFiSubstatus::kWaitSourceConfirmations:
      return mojom::LiFiSubstatusCode::kWaitSourceConfirmations;

    case swap_responses::LiFiSubstatus::kWaitDestinationTransaction:
      return mojom::LiFiSubstatusCode::kWaitDestinationTransaction;

    case swap_responses::LiFiSubstatus::kBridgeNotAvailable:
      return mojom::LiFiSubstatusCode::kBridgeNotAvailable;

    case swap_responses::LiFiSubstatus::kChainNotAvailable:
      return mojom::LiFiSubstatusCode::kChainNotAvailable;

    case swap_responses::LiFiSubstatus::kRefundInProgress:
      return mojom::LiFiSubstatusCode::kRefundInProgress;

    case swap_responses::LiFiSubstatus::kUnknownError:
      return mojom::LiFiSubstatusCode::kUnknownError;

    case swap_responses::LiFiSubstatus::kCompleted:
      return mojom::LiFiSubstatusCode::kCompleted;

    case swap_responses::LiFiSubstatus::kPartial:
      return mojom::LiFiSubstatusCode::kPartial;

    case swap_responses::LiFiSubstatus::kRefunded:
      return mojom::LiFiSubstatusCode::kRefunded;

    case swap_responses::LiFiSubstatus::kNotProcessableRefundNeeded:
      return mojom::LiFiSubstatusCode::kNotProcessableRefundNeeded;

    case swap_responses::LiFiSubstatus::kOutOfGas:
      return mojom::LiFiSubstatusCode::kOutOfGas;

    case swap_responses::LiFiSubstatus::kSlippageExceeded:
      return mojom::LiFiSubstatusCode::kSlippageExceeded;

    case swap_responses::LiFiSubstatus::kInsufficientAllowance:
      return mojom::LiFiSubstatusCode::kInsufficientAllowance;

    case swap_responses::LiFiSubstatus::kInsufficientBalance:
      return mojom::LiFiSubstatusCode::kInsufficientBalance;

    case swap_responses::LiFiSubstatus::kExpired:
      return mojom::LiFiSubstatusCode::kExpired;

    default:
      return mojom::LiFiSubstatusCode::kUnknownError;
  }
}

mojom::LiFiStepStatusPtr ParseStepStatus(
    const swap_responses::LiFiStepStatus& value) {
  auto result = mojom::LiFiStepStatus::New();

  if (auto chain_id = ChainIdToHex(value.chain_id)) {
    result->chain_id = chain_id.value();
  } else {
    return nullptr;
  }

  result->tx_hash = value.tx_hash;
  result->tx_link = value.tx_link;
  result->amount = value.amount;

  if (value.token) {
    result->contract_address = value.token->address;
  }

  return result;
}

mojom::BlockchainTokenPtr ParseToken(const swap_responses::LiFiToken& value) {
  auto result = mojom::BlockchainToken::New();
  result->name = value.name;
  result->symbol = value.symbol;
  result->logo = value.logo_uri.value_or("");
  result->contract_address =
      (base::ToLowerASCII(value.address) ==
           kLiFiNativeEVMAssetContractAddress ||
       value.address == kLiFiNativeSVMAssetContractAddress)
          ? ""
          : value.address;

  if (!base::StringToInt(value.decimals, &result->decimals)) {
    return nullptr;
  }

  auto chain_id = ChainIdToHex(value.chain_id);
  if (!chain_id) {
    return nullptr;
  }
  result->chain_id = chain_id.value();

  // LiFi does not return the coin type, so we infer it from the chain ID.
  if (result->chain_id == mojom::kSolanaMainnet) {
    result->coin = mojom::CoinType::SOL;
  } else {
    result->coin = mojom::CoinType::ETH;
  }

  return result;
}

std::optional<mojom::LiFiStepType> ParseStepType(const std::string& value) {
  if (value == "swap") {
    return mojom::LiFiStepType::kSwap;
  }

  if (value == "cross") {
    return mojom::LiFiStepType::kCross;
  }

  if (value == "lifi") {
    return mojom::LiFiStepType::kLiFi;
  }

  return std::nullopt;
}

mojom::LiFiActionPtr ParseAction(const swap_responses::LiFiAction& value) {
  auto result = mojom::LiFiAction::New();
  result->from_amount = value.from_amount;
  result->from_token = ParseToken(value.from_token);
  result->from_address = value.from_address;

  result->to_token = ParseToken(value.to_token);
  result->to_address = value.to_address;

  result->slippage = value.slippage;
  result->destination_call_data = value.destination_call_data;
  return result;
}

mojom::LiFiStepEstimatePtr ParseEstimate(
    const swap_responses::LiFiEstimate& value) {
  auto result = mojom::LiFiStepEstimate::New();
  result->tool = value.tool;
  result->from_amount = value.from_amount;
  result->to_amount = value.to_amount;
  result->to_amount_min = value.to_amount_min;
  result->approval_address = value.approval_address;

  if (value.fee_costs) {
    result->fee_costs.emplace(std::vector<mojom::LiFiFeeCostPtr>());
    for (const auto& fee_cost_value : *value.fee_costs) {
      auto fee_cost = mojom::LiFiFeeCost::New();
      fee_cost->name = fee_cost_value.name;
      fee_cost->description = fee_cost_value.description;
      fee_cost->percentage = fee_cost_value.percentage;
      fee_cost->token = ParseToken(fee_cost_value.token);
      fee_cost->amount = fee_cost_value.amount;
      fee_cost->included = fee_cost_value.included;
      result->fee_costs->push_back(std::move(fee_cost));
    }
  }

  for (const auto& gas_cost_value : value.gas_costs) {
    auto gas_cost = mojom::LiFiGasCost::New();
    gas_cost->type = gas_cost_value.type;
    gas_cost->estimate = gas_cost_value.estimate;
    gas_cost->limit = gas_cost_value.limit;
    gas_cost->amount = gas_cost_value.amount;
    gas_cost->token = ParseToken(gas_cost_value.token);
    result->gas_costs.push_back(std::move(gas_cost));
  }

  result->execution_duration = value.execution_duration;

  return result;
}

mojom::LiFiStepPtr ParseStep(const swap_responses::LiFiStep& value) {
  auto result = mojom::LiFiStep::New();
  result->id = value.id;

  auto step_type = ParseStepType(value.type);
  if (!step_type) {
    return nullptr;
  }
  result->type = std::move(*step_type);

  result->tool = value.tool;

  auto tool_details = mojom::LiFiToolDetails::New();
  tool_details->key = value.tool_details.key;
  tool_details->name = value.tool_details.name;
  tool_details->logo = value.tool_details.logo_uri;
  result->tool_details = std::move(tool_details);

  result->action = ParseAction(value.action);
  result->estimate = ParseEstimate(value.estimate);

  result->integrator = value.integrator;

  if (!value.included_steps) {
    return result;
  }

  std::vector<mojom::LiFiStepPtr> included_steps = {};
  for (const auto& included_step_value : *value.included_steps) {
    auto included_step = ParseStep(included_step_value);
    if (!included_step) {
      return nullptr;
    }

    included_steps.push_back(std::move(included_step));
  }
  result->included_steps = std::move(included_steps);

  return result;
}

}  // namespace

mojom::LiFiQuotePtr ParseQuoteResponse(const base::Value& json_value) {
  auto value = swap_responses::LiFiQuoteResponse::FromValue(json_value);
  if (!value) {
    return nullptr;
  }

  auto result = mojom::LiFiQuote::New();

  for (const auto& route_value : value->routes) {
    auto route = mojom::LiFiRoute::New();
    route->id = route_value.id;

    auto from_token = ParseToken(route_value.from_token);
    if (!from_token) {
      return nullptr;
    }
    route->from_token = std::move(from_token);
    route->from_amount = route_value.from_amount;
    route->from_address = route_value.from_address;

    auto to_token = ParseToken(route_value.to_token);
    if (!to_token) {
      return nullptr;
    }
    route->to_token = std::move(to_token);

    route->to_amount = route_value.to_amount;
    route->to_amount_min = route_value.to_amount_min;
    route->to_address = route_value.to_address;

    for (const auto& step_value : route_value.steps) {
      auto step = ParseStep(step_value);
      if (!step) {
        return nullptr;
      }

      route->steps.push_back(std::move(step));
    }

    std::vector<std::string> tools;
    for (const auto& step : route->steps) {
      tools.push_back(step->tool);
    }
    route->unique_id = base::JoinString(tools, "-");

    route->tags = route_value.tags;
    result->routes.push_back(std::move(route));
  }

  return result;
}

mojom::LiFiTransactionUnionPtr ParseTransactionResponse(
    const base::Value& json_value) {
  auto value = swap_responses::LiFiTransactionResponse::FromValue(json_value);
  if (!value) {
    return nullptr;
  }

  // SOL -> any transfers
  if (!value->transaction_request.data.empty() &&
      !value->transaction_request.from && !value->transaction_request.to &&
      !value->transaction_request.value &&
      !value->transaction_request.gas_price &&
      !value->transaction_request.gas_limit &&
      !value->transaction_request.chain_id.has_value()) {
    return mojom::LiFiTransactionUnion::NewSolanaTransaction(
        value->transaction_request.data);
  }

  // EVM -> any transfers
  if (value->transaction_request.data.empty() ||

      !value->transaction_request.from ||
      value->transaction_request.from->empty() ||

      !value->transaction_request.to ||
      value->transaction_request.to->empty() ||

      !value->transaction_request.value ||
      value->transaction_request.value->empty() ||

      !value->transaction_request.gas_price ||
      value->transaction_request.gas_price->empty() ||

      !value->transaction_request.gas_limit ||
      value->transaction_request.gas_limit->empty() ||

      !value->transaction_request.chain_id ||
      !value->transaction_request.chain_id.has_value()) {
    return nullptr;
  }

  auto evm_transaction = mojom::LiFiEvmTransaction::New();
  evm_transaction->data = value->transaction_request.data;
  evm_transaction->from = value->transaction_request.from.value();
  evm_transaction->to = value->transaction_request.to.value();
  evm_transaction->value = value->transaction_request.value.value();
  evm_transaction->gas_price = value->transaction_request.gas_price.value();
  evm_transaction->gas_limit = value->transaction_request.gas_limit.value();

  if (value->transaction_request.chain_id.has_value()) {
    auto chain_id = ChainIdToHex(value->transaction_request.chain_id.value());
    if (!chain_id) {
      return nullptr;
    }
    evm_transaction->chain_id = chain_id.value();
  }

  return mojom::LiFiTransactionUnion::NewEvmTransaction(
      std::move(evm_transaction));
}

mojom::LiFiErrorCode ParseLiFiErrorCode(
    const std::optional<std::string>& value) {
  if (!value) {
    return mojom::LiFiErrorCode::kSuccess;
  }

  if (value == "1000") {
    return mojom::LiFiErrorCode::kDefaultError;
  }
  if (value == "1001") {
    return mojom::LiFiErrorCode::kFailedToBuildTransactionError;
  }
  if (value == "1002") {
    return mojom::LiFiErrorCode::kNoQuoteError;
  }
  if (value == "1003") {
    return mojom::LiFiErrorCode::kNotFoundError;
  }
  if (value == "1004") {
    return mojom::LiFiErrorCode::kNotProcessableError;
  }
  if (value == "1005") {
    return mojom::LiFiErrorCode::kRateLimitError;
  }
  if (value == "1006") {
    return mojom::LiFiErrorCode::kServerError;
  }
  if (value == "1007") {
    return mojom::LiFiErrorCode::kSlippageError;
  }
  if (value == "1008") {
    return mojom::LiFiErrorCode::kThirdPartyError;
  }
  if (value == "1009") {
    return mojom::LiFiErrorCode::kTimeoutError;
  }
  if (value == "1010") {
    return mojom::LiFiErrorCode::kUnauthorizedError;
  }
  if (value == "1011") {
    return mojom::LiFiErrorCode::kValidationError;
  }

  return mojom::LiFiErrorCode::kDefaultError;
}

mojom::LifiToolErrorCode ParseLiFiToolErrorCode(const std::string& value) {
  // No route was found for this action.
  if (value == "NO_POSSIBLE_ROUTE") {
    return mojom::LifiToolErrorCode::kNoPossibleRoute;
  }

  // The tool's liquidity is insufficient.
  if (value == "INSUFFICIENT_LIQUIDITY") {
    return mojom::LifiToolErrorCode::kInsufficientLiquidity;
  }

  // The third-party tool timed out.
  if (value == "TOOL_TIMEOUT") {
    return mojom::LifiToolErrorCode::kToolTimeout;
  }

  // An unknown error occurred.
  if (value == "UNKNOWN_ERROR") {
    return mojom::LifiToolErrorCode::kUnknownError;
  }

  // There was a problem getting on-chain data. Please try again later.
  if (value == "RPC_ERROR") {
    return mojom::LifiToolErrorCode::kRpcError;
  }

  // The initial amount is too low to transfer using this tool.
  if (value == "AMOUNT_TOO_LOW") {
    return mojom::LifiToolErrorCode::kAmountTooLow;
  }

  // The initial amount is too high to transfer using this tool.
  if (value == "AMOUNT_TOO_HIGH") {
    return mojom::LifiToolErrorCode::kAmountTooHigh;
  }

  // The fees are higher than the initial amount -- this would result in
  // negative resulting token.
  if (value == "FEES_HIGHER_THAN_AMOUNT" || value == "FEES_HGHER_THAN_AMOUNT") {
    return mojom::LifiToolErrorCode::kFeesHigherThanAmount;
  }

  // This tool does not support different recipient addresses.
  if (value == "DIFFERENT_RECIPIENT_NOT_SUPPORTED") {
    return mojom::LifiToolErrorCode::kDifferentRecipientNotSupported;
  }

  // The third-party tool returned an error.
  if (value == "TOOL_SPECIFIC_ERROR") {
    return mojom::LifiToolErrorCode::kToolSpecificError;
  }

  // The tool cannot guarantee that the minimum amount will be met.
  if (value == "CANNOT_GUARANTEE_MIN_AMOUNT") {
    return mojom::LifiToolErrorCode::kCannotGuaranteeMinAmount;
  }

  return mojom::LifiToolErrorCode::kUnknownError;
}

mojom::LiFiErrorPtr ParseErrorResponse(const base::Value& json_value) {
  auto value = swap_responses::LiFiErrorResponse::FromValue(json_value);
  if (!value) {
    return nullptr;
  }

  auto result = mojom::LiFiError::New();
  result->message = value->message;
  result->code = ParseLiFiErrorCode(value->code);

  return result;
}

mojom::LiFiStatusPtr ParseStatusResponse(const base::Value& json_value) {
  auto value = swap_responses::LiFiStatusResponse::FromValue(json_value);
  if (!value) {
    return nullptr;
  }

  auto result = mojom::LiFiStatus::New();
  result->transaction_id = value->transaction_id;

  if (auto sending = ParseStepStatus(value->sending)) {
    result->sending = std::move(sending);
  } else {
    return nullptr;
  }

  if (auto receiving = ParseStepStatus(value->receiving)) {
    result->receiving = std::move(receiving);
  } else {
    return nullptr;
  }

  result->lifi_explorer_link = value->lifi_explorer_link;
  result->from_address = value->from_address;
  result->to_address = value->to_address;
  result->tool = value->tool;

  result->status = ParseStatusCode(value->status);
  result->substatus = ParseSubstatusCode(value->substatus);
  result->substatus_message = value->substatus_message;

  return result;
}

}  // namespace lifi

namespace squid {

namespace {
constexpr char kNoTokenData[] = "Unable to fetch token data";

std::optional<std::string> ChainIdToHex(const std::string& value) {
  std::optional<uint256_t> out = Base10ValueToUint256(value);
  if (!out) {
    return std::nullopt;
  }

  return Uint256ValueToHex(*out);
}

mojom::SquidErrorType ParseErrorType(const std::string& value) {
  if (value == "BAD_REQUEST") {
    return mojom::SquidErrorType::kBadRequest;
  }

  if (value == "SCHEMA_VALIDATION_ERROR") {
    return mojom::SquidErrorType::kSchemaValidationError;
  }

  return mojom::SquidErrorType::kUnknownError;
}

mojom::SquidActionType ParseActionType(const std::string& value) {
  if (value == "wrap") {
    return mojom::SquidActionType::kWrap;
  }

  if (value == "unwrap") {
    return mojom::SquidActionType::kUnwrap;
  }

  if (value == "swap") {
    return mojom::SquidActionType::kSwap;
  }

  if (value == "bridge") {
    return mojom::SquidActionType::kBridge;
  }

  return mojom::SquidActionType::kUnknown;
}

mojom::BlockchainTokenPtr ParseToken(const swap_responses::SquidToken& value) {
  auto result = mojom::BlockchainToken::New();
  result->name = value.name;
  result->symbol = value.symbol;
  result->logo = value.logo_uri.value_or("");
  result->contract_address =
      base::ToLowerASCII(value.address) == kNativeEVMAssetContractAddress
          ? ""
          : value.address;

  if (!base::StringToInt(value.decimals, &result->decimals)) {
    return nullptr;
  }

  auto chain_id = ChainIdToHex(value.chain_id);
  if (!chain_id) {
    return nullptr;
  }
  result->chain_id = chain_id.value();

  if (value.type == "evm") {
    result->coin = mojom::CoinType::ETH;
  } else {
    return nullptr;
  }

  result->coingecko_id = value.coingecko_id.value_or("");

  return result;
}

mojom::SquidGasCostPtr ParseGasCost(const swap_responses::SquidGasCost& value) {
  auto result = mojom::SquidGasCost::New();
  result->amount = value.amount;
  result->gas_limit = value.gas_limit;
  result->token = ParseToken(value.token);
  return result;
}

mojom::SquidFeeCostPtr ParseFeeCost(const swap_responses::SquidFeeCost& value) {
  auto result = mojom::SquidFeeCost::New();
  result->amount = value.amount;
  result->description = value.description;
  result->name = value.name;
  result->token = ParseToken(value.token);
  return result;
}

mojom::SquidActionPtr ParseAction(const swap_responses::SquidAction& value) {
  auto result = mojom::SquidAction::New();
  result->type = ParseActionType(value.type);
  result->description = value.description;
  result->provider = value.provider;
  result->logo_uri = value.logo_uri.value_or("");
  result->from_amount = value.from_amount;
  result->from_token = ParseToken(value.from_token);
  result->to_amount = value.to_amount;
  result->to_amount_min = value.to_amount_min;
  result->to_token = ParseToken(value.to_token);
  return result;
}

}  // namespace

mojom::SquidQuotePtr ParseQuoteResponse(const base::Value& json_value) {
  auto value = swap_responses::SquidQuoteResponse::FromValue(json_value);
  if (!value) {
    return nullptr;
  }

  auto result = mojom::SquidQuote::New();
  for (const auto& action_value : value->route.estimate.actions) {
    auto action = ParseAction(action_value);
    if (!action) {
      return nullptr;
    }

    result->actions.push_back(std::move(action));
  }

  result->aggregate_price_impact = value->route.estimate.aggregate_price_impact;
  result->aggregate_slippage = value->route.estimate.aggregate_slippage;
  result->estimated_route_duration =
      value->route.estimate.estimated_route_duration;
  result->exchange_rate = value->route.estimate.exchange_rate;

  for (const auto& gas_cost_value : value->route.estimate.gas_costs) {
    auto gas_cost = ParseGasCost(gas_cost_value);
    if (!gas_cost) {
      return nullptr;
    }

    result->gas_costs.push_back(std::move(gas_cost));
  }

  for (const auto& fee_cost_value : value->route.estimate.fee_costs) {
    auto fee_cost = ParseFeeCost(fee_cost_value);
    if (!fee_cost) {
      return nullptr;
    }

    result->fee_costs.push_back(std::move(fee_cost));
  }

  result->is_boost_supported = value->route.estimate.is_boost_supported;
  result->from_amount = value->route.estimate.from_amount;
  result->from_token = ParseToken(value->route.estimate.from_token);
  result->to_amount = value->route.estimate.to_amount;
  result->to_amount_min = value->route.estimate.to_amount_min;
  result->to_token = ParseToken(value->route.estimate.to_token);

  // We pass quoteOnly=false to the Squid API, so the response will always
  // contain a transactionRequest field.
  //
  // This is a workaround to avoid having to make an additional request to the
  // Squid API to get Squid router contract address.
  if (!value->route.transaction_request) {
    return nullptr;
  }

  result->allowance_target = value->route.transaction_request->target;

  return result;
}

mojom::SquidTransactionUnionPtr ParseTransactionResponse(
    const base::Value& json_value) {
  auto value = swap_responses::SquidQuoteResponse::FromValue(json_value);
  if (!value) {
    return nullptr;
  }

  if (!value->route.transaction_request) {
    return nullptr;
  }

  auto result = mojom::SquidEvmTransaction::New();
  result->data = value->route.transaction_request->data;
  result->target = value->route.transaction_request->target;
  result->value = value->route.transaction_request->value;
  result->gas_limit = value->route.transaction_request->gas_limit;
  result->gas_price = value->route.transaction_request->gas_price;
  result->last_base_fee_per_gas =
      value->route.transaction_request->last_base_fee_per_gas;
  result->max_priority_fee_per_gas =
      value->route.transaction_request->max_priority_fee_per_gas;
  result->max_fee_per_gas = value->route.transaction_request->max_fee_per_gas;

  auto chain_id = ChainIdToHex(value->route.estimate.from_token.chain_id);
  if (!chain_id) {
    return nullptr;
  }
  result->chain_id = chain_id.value();

  return mojom::SquidTransactionUnion::NewEvmTransaction(std::move(result));
}

mojom::SquidErrorPtr ParseErrorResponse(const base::Value& json_value) {
  // {
  //   "message": "onChainQuoting must be a `boolean` type.",
  //   "statusCode": "400",
  //   "type": "SCHEMA_VALIDATION_ERROR"
  // }
  auto value = swap_responses::SquidErrorResponse::FromValue(json_value);
  if (!value) {
    return nullptr;
  }

  auto result = mojom::SquidError::New();
  result->message = value->message;
  result->type = ParseErrorType(value->type);
  result->is_insufficient_liquidity =
      base::Contains(result->message, kNoTokenData);

  return result;
}

}  // namespace squid

}  // namespace brave_wallet
