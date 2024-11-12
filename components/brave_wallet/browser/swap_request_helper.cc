/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/swap_request_helper.h"

#include <limits>
#include <optional>
#include <utility>
#include <vector>

#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "base/values.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/json_rpc_requests_helper.h"
#include "brave/components/brave_wallet/browser/solana_keyring.h"
#include "brave/components/brave_wallet/common/brave_wallet_constants.h"
#include "brave/components/brave_wallet/common/encoding_utils.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "brave/components/json/json_helper.h"

namespace brave_wallet {

namespace jupiter {
namespace {

// Docs: https://station.jup.ag/docs/apis/adding-fees
std::optional<std::string> GetFeeAccount(const std::string& output_mint) {
  std::vector<std::vector<uint8_t>> seeds;
  std::vector<uint8_t> referral_account_pubkey_bytes;
  std::vector<uint8_t> output_mint_bytes;

  if (!Base58Decode(kJupiterReferralKey, &referral_account_pubkey_bytes,
                    kSolanaPubkeySize) ||
      !Base58Decode(output_mint, &output_mint_bytes, kSolanaPubkeySize)) {
    return std::nullopt;
  }

  const std::string& referral_fee_header = kJupiterReferralProgramHeader;
  std::vector<uint8_t> referral_ata_bytes(referral_fee_header.begin(),
                                          referral_fee_header.end());

  seeds.push_back(std::move(referral_ata_bytes));
  seeds.push_back(std::move(referral_account_pubkey_bytes));
  seeds.push_back(std::move(output_mint_bytes));

  return SolanaKeyring::FindProgramDerivedAddress(seeds,
                                                  kJupiterReferralProgram);
}

}  // namespace

std::optional<std::string> EncodeTransactionParams(
    const mojom::JupiterTransactionParams& params) {
  base::Value::Dict tx_params;

  // The code below does the following two things:
  //   - compute the ATA address that should be used to receive fees
  //   - verify if output_mint is a valid address
  std::optional<std::string> fee_account =
      GetFeeAccount(params.quote->output_mint);
  if (!fee_account) {
    return std::nullopt;
  }

  // If the if-condition below is false, fee_account is unused,
  // but the originating call to GetFeeAccount() is still done to ensure
  // output_mint is always valid.
  if (params.quote->platform_fee) {
    tx_params.Set("feeAccount", *fee_account);
  }

  tx_params.Set("userPublicKey", params.user_public_key);
  tx_params.Set("dynamicComputeUnitLimit", true);
  tx_params.Set("prioritizationFeeLamports", "auto");

  base::Value::Dict quote;
  quote.Set("inputMint", params.quote->input_mint);
  quote.Set("inAmount", params.quote->in_amount);
  quote.Set("outputMint", params.quote->output_mint);
  quote.Set("outAmount", params.quote->out_amount);
  quote.Set("otherAmountThreshold", params.quote->other_amount_threshold);
  quote.Set("swapMode", params.quote->swap_mode);
  quote.Set("slippageBps", params.quote->slippage_bps);
  quote.Set("priceImpactPct", params.quote->price_impact_pct);

  if (params.quote->platform_fee) {
    base::Value::Dict platform_fee;
    platform_fee.Set("amount", params.quote->platform_fee->amount);
    platform_fee.Set("feeBps", params.quote->platform_fee->fee_bps);
    quote.Set("platformFee", std::move(platform_fee));
  } else {
    quote.Set("platformFee", base::Value());
  }

  base::Value::List route_plan_value;
  for (const auto& step : params.quote->route_plan) {
    base::Value::Dict step_value;
    step_value.Set("percent", step->percent);

    base::Value::Dict swap_info_value;
    swap_info_value.Set("ammKey", step->swap_info->amm_key);
    swap_info_value.Set("label", step->swap_info->label);
    swap_info_value.Set("inputMint", step->swap_info->input_mint);
    swap_info_value.Set("outputMint", step->swap_info->output_mint);
    swap_info_value.Set("inAmount", step->swap_info->in_amount);
    swap_info_value.Set("outAmount", step->swap_info->out_amount);
    swap_info_value.Set("feeAmount", step->swap_info->fee_amount);
    swap_info_value.Set("feeMint", step->swap_info->fee_mint);

    step_value.Set("swapInfo", std::move(swap_info_value));
    route_plan_value.Append(std::move(step_value));
  }

  quote.Set("routePlan", std::move(route_plan_value));

  tx_params.Set("quoteResponse", std::move(quote));

  // FIXME - GetJSON should be refactored to accept a base::Value::Dict
  std::string result = GetJSON(base::Value(std::move(tx_params)));

  result = json::convert_string_value_to_uint64("/quoteResponse/slippageBps",
                                                result, false);

  if (params.quote->platform_fee) {
    result = json::convert_string_value_to_uint64(
        "/quoteResponse/platformFee/feeBps", result, false);
  }

  for (int i = 0; i < static_cast<int>(params.quote->route_plan.size()); i++) {
    result = json::convert_string_value_to_uint64(
        base::StringPrintf("/quoteResponse/routePlan/%d/percent", i), result,
        false);
  }

  return result;
}

}  // namespace jupiter

namespace lifi {

namespace {

std::optional<std::string> EncodeChainId(const std::string& value) {
  if (value == mojom::kSolanaMainnet) {
    return kLiFiSolanaMainnetChainID;
  }

  uint256_t val;
  if (!HexValueToUint256(value, &val)) {
    return std::nullopt;
  }

  if (val > std::numeric_limits<uint64_t>::max()) {
    return std::nullopt;
  }

  return base::NumberToString(static_cast<uint64_t>(val));
}

base::Value::Dict EncodeToolDetails(
    const mojom::LiFiToolDetailsPtr& tool_details) {
  base::Value::Dict result;
  result.Set("key", tool_details->key);
  result.Set("name", tool_details->name);
  result.Set("logoURI", tool_details->logo);
  return result;
}

std::optional<base::Value::Dict> EncodeToken(
    const mojom::BlockchainTokenPtr& token) {
  base::Value::Dict result;

  if (token->contract_address.empty()) {
    result.Set("address", token->coin == mojom::CoinType::SOL
                              ? kLiFiNativeSVMAssetContractAddress
                              : kLiFiNativeEVMAssetContractAddress);
  } else {
    result.Set("address", token->contract_address);
  }

  result.Set("decimals", token->decimals);
  result.Set("symbol", token->symbol);

  if (auto chain_id = EncodeChainId(token->chain_id)) {
    result.Set("chainId", *chain_id);
  } else {
    return std::nullopt;
  }

  result.Set("name", token->name);

  // fake the usd value since it is not used by LiFi
  result.Set("priceUSD", "0");
  return result;
}

std::string EncodeStepType(const mojom::LiFiStepType type) {
  switch (type) {
    case mojom::LiFiStepType::kSwap:
      return "swap";
    case mojom::LiFiStepType::kCross:
      return "cross";
    case mojom::LiFiStepType::kLiFi:
      return "lifi";
  }
  NOTREACHED();
}

std::optional<base::Value::Dict> EncodeStepAction(mojom::LiFiActionPtr action) {
  base::Value::Dict result;

  if (auto chain_id = EncodeChainId(action->from_token->chain_id)) {
    result.Set("fromChainId", *chain_id);
  } else {
    return std::nullopt;
  }

  result.Set("fromAmount", action->from_amount);

  if (auto token = EncodeToken(action->from_token)) {
    result.Set("fromToken", std::move(*token));
  } else {
    return std::nullopt;
  }

  if (action->from_address) {
    result.Set("fromAddress", *action->from_address);
  }

  if (auto chain_id = EncodeChainId(action->to_token->chain_id)) {
    result.Set("toChainId", *chain_id);
  } else {
    return std::nullopt;
  }

  if (auto token = EncodeToken(action->to_token)) {
    result.Set("toToken", std::move(*token));
  } else {
    return std::nullopt;
  }

  if (action->to_address) {
    result.Set("toAddress", *action->to_address);
  }

  double slippage = 0.0;
  if (base::StringToDouble(action->slippage, &slippage)) {
    result.Set("slippage", slippage);
  } else {
    return std::nullopt;
  }

  if (action->destination_call_data) {
    result.Set("destinationCallData", *action->destination_call_data);
  }

  return result;
}

std::optional<base::Value::Dict> EncodeStepEstimate(
    mojom::LiFiStepEstimatePtr estimate) {
  base::Value::Dict result;

  result.Set("tool", estimate->tool);
  result.Set("fromAmount", estimate->from_amount);
  result.Set("toAmount", estimate->to_amount);
  result.Set("toAmountMin", estimate->to_amount_min);
  result.Set("approvalAddress", estimate->approval_address);

  double execution_duration = 0.0;
  if (base::StringToDouble(estimate->execution_duration, &execution_duration)) {
    result.Set("executionDuration", execution_duration);
  } else {
    return std::nullopt;
  }

  if (estimate->fee_costs) {
    base::Value::List fee_costs_value;
    for (const auto& fee_cost : *estimate->fee_costs) {
      base::Value::Dict fee_cost_value;
      fee_cost_value.Set("name", fee_cost->name);
      fee_cost_value.Set("description", fee_cost->description);
      fee_cost_value.Set("amount", fee_cost->amount);
      fee_cost_value.Set("percentage", fee_cost->percentage);
      fee_cost_value.Set("included", fee_cost->included);

      // fake the USD amount value since it is not used by LiFi
      fee_cost_value.Set("amountUSD", "0");

      if (auto token = EncodeToken(fee_cost->token)) {
        fee_cost_value.Set("token", std::move(*token));
      } else {
        return std::nullopt;
      }

      fee_costs_value.Append(std::move(fee_cost_value));
    }
    result.Set("feeCosts", std::move(fee_costs_value));
  }

  base::Value::List gas_costs_value;
  for (const auto& gas_cost : estimate->gas_costs) {
    base::Value::Dict gas_cost_value;
    gas_cost_value.Set("type", gas_cost->type);
    gas_cost_value.Set("estimate", gas_cost->estimate);
    gas_cost_value.Set("limit", gas_cost->limit);
    gas_cost_value.Set("amount", gas_cost->amount);

    // fake the price and USD amount values since they are not used by LiFi
    gas_cost_value.Set("price", "0");
    gas_cost_value.Set("amountUSD", "0");

    if (auto token = EncodeToken(gas_cost->token)) {
      gas_cost_value.Set("token", std::move(*token));
    } else {
      return std::nullopt;
    }

    gas_costs_value.Append(std::move(gas_cost_value));
  }
  result.Set("gasCosts", std::move(gas_costs_value));

  return result;
}

std::optional<base::Value::Dict> EncodeStep(mojom::LiFiStepPtr step) {
  base::Value::Dict result;
  result.Set("id", step->id);
  result.Set("type", EncodeStepType(step->type));
  result.Set("tool", step->tool);

  if (auto action = EncodeStepAction(std::move(step->action))) {
    result.Set("action", std::move(*action));
  } else {
    return std::nullopt;
  }

  if (auto estimate = EncodeStepEstimate(step->estimate->Clone())) {
    result.Set("estimate", std::move(*estimate));
  } else {
    return std::nullopt;
  }

  if (step->integrator) {
    result.Set("integrator", *step->integrator);
  }

  result.Set("toolDetails", EncodeToolDetails(std::move(step->tool_details)));

  if (!step->included_steps) {
    return result;
  }

  base::Value::List included_steps_value;
  for (auto& included_step : *step->included_steps) {
    if (auto included_step_value = EncodeStep(std::move(included_step))) {
      included_steps_value.Append(std::move(*included_step_value));
    } else {
      return std::nullopt;
    }
  }

  result.Set("includedSteps", std::move(included_steps_value));
  return result;
}

}  // namespace

std::optional<std::string> EncodeQuoteParams(
    mojom::SwapQuoteParamsPtr params,
    const std::optional<std::string>& fee_param) {
  base::Value::Dict result;

  if (auto chain_id = EncodeChainId(params->from_chain_id)) {
    result.Set("fromChainId", *chain_id);
  } else {
    return std::nullopt;
  }

  result.Set("fromAmount", params->from_amount);
  result.Set("fromTokenAddress",
             params->from_token.empty()
                 ? params->from_chain_id == mojom::kSolanaMainnet
                       ? kLiFiNativeSVMAssetContractAddress
                       : kLiFiNativeEVMAssetContractAddress
                 : params->from_token);
  result.Set("fromAddress", params->from_account_id->address);

  if (auto chain_id = EncodeChainId(params->to_chain_id)) {
    result.Set("toChainId", *chain_id);
  } else {
    return std::nullopt;
  }

  result.Set("toTokenAddress",
             params->to_token.empty()
                 ? params->to_chain_id == mojom::kSolanaMainnet
                       ? kLiFiNativeSVMAssetContractAddress
                       : kLiFiNativeEVMAssetContractAddress
                 : params->to_token);
  result.Set("toAddress", params->to_account_id->address);
  result.Set("allowDestinationCall", true);

  base::Value::Dict options;
  options.Set("insurance", true);
  options.Set("integrator", kLiFiIntegratorID);
  options.Set("allowSwitchChain", false);

  if (fee_param.has_value() && !fee_param->empty()) {
    double fee = 0.0;
    if (base::StringToDouble(fee_param.value(), &fee)) {
      options.Set("fee", fee);
    }
  }

  double slippage_percentage = 0.0;
  if (base::StringToDouble(params->slippage_percentage, &slippage_percentage)) {
    options.Set("slippage", slippage_percentage / 100);
  }

  result.Set("options", std::move(options));

  return GetJSON(base::Value(std::move(result)));
}

std::optional<std::string> EncodeTransactionParams(mojom::LiFiStepPtr step) {
  auto result = EncodeStep(std::move(step));
  if (!result) {
    return std::nullopt;
  }

  std::string result_str = GetJSON(base::Value(std::move(*result)));
  if (result_str.empty()) {
    return std::nullopt;
  }

  return result_str;
}

}  // namespace lifi

namespace squid {

namespace {

std::optional<std::string> EncodeChainId(const std::string& value) {
  uint256_t val;
  if (!HexValueToUint256(value, &val)) {
    return std::nullopt;
  }

  if (val > std::numeric_limits<uint64_t>::max()) {
    return std::nullopt;
  }

  return base::NumberToString(static_cast<uint64_t>(val));
}

std::optional<std::string> EncodeParams(mojom::SwapQuoteParamsPtr params) {
  base::Value::Dict result;
  if (auto chain_id = EncodeChainId(params->from_chain_id)) {
    result.Set("fromChain", *chain_id);
  } else {
    return std::nullopt;
  }

  result.Set("fromAddress", params->from_account_id->address);
  result.Set("fromToken", params->from_token.empty()
                              ? kNativeEVMAssetContractAddress
                              : params->from_token);
  result.Set("fromAmount", params->from_amount);

  if (auto chain_id = EncodeChainId(params->to_chain_id)) {
    result.Set("toChain", *chain_id);
  } else {
    return std::nullopt;
  }

  result.Set("toAddress", params->to_account_id->address);
  result.Set("toToken", params->to_token.empty()
                            ? kNativeEVMAssetContractAddress
                            : params->to_token);

  double slippage_percentage = 0.0;
  if (base::StringToDouble(params->slippage_percentage, &slippage_percentage)) {
    result.Set("slippage", slippage_percentage);
  }

  base::Value::Dict slippage_config;
  slippage_config.Set("autoMode", 1);
  result.Set("slippageConfig", std::move(slippage_config));

  result.Set("enableBoost", true);
  result.Set("quoteOnly", false);

  return GetJSON(base::Value(std::move(result)));
}

}  // namespace

std::optional<std::string> EncodeQuoteParams(mojom::SwapQuoteParamsPtr params) {
  return EncodeParams(std::move(params));
}

std::optional<std::string> EncodeTransactionParams(
    mojom::SwapQuoteParamsPtr params) {
  return EncodeParams(std::move(params));
}

}  // namespace squid

}  // namespace brave_wallet
