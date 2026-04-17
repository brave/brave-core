/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/swap_request_helper.h"

#include <limits>
#include <optional>
#include <utility>
#include <vector>

#include "base/notreached.h"
#include "base/strings/string_number_conversions.h"
#include "base/values.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/json_rpc_requests_helper.h"
#include "brave/components/brave_wallet/browser/solana_keyring.h"
#include "brave/components/brave_wallet/common/brave_wallet_constants.h"
#include "brave/components/brave_wallet/common/encoding_utils.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "brave/components/json/json_helper.h"
#include "third_party/abseil-cpp/absl/strings/str_format.h"

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
  base::DictValue tx_params;

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

  // Ref:
  // https://station.jup.ag/docs/apis/landing-transactions#an-example-of-how-jupiter-estimates-priority-fees
  base::DictValue priority_level_with_max_lamports;
  priority_level_with_max_lamports.Set("maxLamports", 4000000);
  priority_level_with_max_lamports.Set("global", false);
  priority_level_with_max_lamports.Set("priorityLevel", "high");

  base::DictValue prioritization_fee_lamports;
  prioritization_fee_lamports.Set("priorityLevelWithMaxLamports",
                                  std::move(priority_level_with_max_lamports));

  tx_params.Set("prioritizationFeeLamports",
                std::move(prioritization_fee_lamports));

  base::DictValue quote;
  quote.Set("inputMint", params.quote->input_mint);
  quote.Set("inAmount", params.quote->in_amount);
  quote.Set("outputMint", params.quote->output_mint);
  quote.Set("outAmount", params.quote->out_amount);
  quote.Set("otherAmountThreshold", params.quote->other_amount_threshold);
  quote.Set("swapMode", params.quote->swap_mode);
  quote.Set("slippageBps", params.quote->slippage_bps);
  quote.Set("priceImpactPct", params.quote->price_impact_pct);

  if (params.quote->platform_fee) {
    base::DictValue platform_fee;
    platform_fee.Set("amount", params.quote->platform_fee->amount);
    platform_fee.Set("feeBps", params.quote->platform_fee->fee_bps);
    quote.Set("platformFee", std::move(platform_fee));
  } else {
    quote.Set("platformFee", base::Value());
  }

  base::ListValue route_plan_value;
  for (const auto& step : params.quote->route_plan) {
    base::DictValue step_value;
    step_value.Set("percent", step->percent);

    base::DictValue swap_info_value;
    swap_info_value.Set("ammKey", step->swap_info->amm_key);
    swap_info_value.Set("label", step->swap_info->label);
    swap_info_value.Set("inputMint", step->swap_info->input_mint);
    swap_info_value.Set("outputMint", step->swap_info->output_mint);
    swap_info_value.Set("inAmount", step->swap_info->in_amount);
    swap_info_value.Set("outAmount", step->swap_info->out_amount);
    if (step->swap_info->fee_amount.has_value()) {
      swap_info_value.Set("feeAmount", step->swap_info->fee_amount.value());
    }
    if (step->swap_info->fee_mint.has_value()) {
      swap_info_value.Set("feeMint", step->swap_info->fee_mint.value());
    }

    step_value.Set("swapInfo", std::move(swap_info_value));
    route_plan_value.Append(std::move(step_value));
  }

  quote.Set("routePlan", std::move(route_plan_value));

  tx_params.Set("quoteResponse", std::move(quote));

  // FIXME - GetJSON should be refactored to accept a base::DictValue
  std::string result = GetJSON(base::Value(std::move(tx_params)));

  result = json::convert_string_value_to_uint64("/quoteResponse/slippageBps",
                                                result, false);

  if (params.quote->platform_fee) {
    result = json::convert_string_value_to_uint64(
        "/quoteResponse/platformFee/feeBps", result, false);
  }

  for (int i = 0; i < static_cast<int>(params.quote->route_plan.size()); i++) {
    result = json::convert_string_value_to_uint64(
        absl::StrFormat("/quoteResponse/routePlan/%d/percent", i), result,
        false);
  }

  return result;
}

}  // namespace jupiter


// namespace gate3 currently only supports Near Intents provider
//
// Docs: https://gate3.bsg.brave.com/docs (requires internal Brave VPN)
namespace gate3 {

namespace {

std::optional<std::string> EncodeCoinType(mojom::CoinType coin) {
  switch (coin) {
    case mojom::CoinType::ETH:
      return "ETH";
    case mojom::CoinType::SOL:
      return "SOL";
    case mojom::CoinType::BTC:
      return "BTC";
    case mojom::CoinType::FIL:
      return "FIL";
    case mojom::CoinType::ZEC:
      return "ZEC";
    case mojom::CoinType::ADA:
      return "ADA";
    case mojom::CoinType::DOT:
      return "DOT";
  }
  return std::nullopt;
}

std::optional<std::string> EncodeProvider(mojom::SwapProvider provider) {
  switch (provider) {
    case mojom::SwapProvider::kJupiter:
      return "JUPITER";
    case mojom::SwapProvider::kSquid:
      return "SQUID";
    case mojom::SwapProvider::kNearIntents:
      return "NEAR_INTENTS";
    case mojom::SwapProvider::kLiFi:
      return "LIFI";

    default:
      return std::nullopt;
  }
}

std::optional<std::string> EncodeRoutePriority(
    mojom::RoutePriority route_priority) {
  switch (route_priority) {
    case mojom::RoutePriority::kFastest:
      return "FASTEST";
    case mojom::RoutePriority::kCheapest:
      return "CHEAPEST";
    default:
      return std::nullopt;
  }
}

}  // namespace

std::optional<std::string> EncodeQuoteParams(mojom::SwapQuoteParamsPtr params) {
  base::DictValue result;

  // Source coin type from account
  auto source_coin = EncodeCoinType(params->from_account_id->coin);
  if (!source_coin) {
    return std::nullopt;
  }

  // Destination coin type from account
  auto destination_coin = EncodeCoinType(params->to_account_id->coin);
  if (!destination_coin) {
    return std::nullopt;
  }

  // Source
  result.Set("sourceCoin", *source_coin);
  result.Set("sourceChainId", params->from_chain_id);
  result.Set("sourceTokenAddress", params->from_token);
  result.Set("refundTo", params->from_account_id->address);

  // Destination
  result.Set("destinationCoin", *destination_coin);
  result.Set("destinationChainId", params->to_chain_id);
  result.Set("destinationTokenAddress", params->to_token);
  result.Set("recipient", params->to_account_id->address);

  if (!params->from_amount.empty() && params->to_amount.empty()) {
    result.Set("swapType", "EXACT_INPUT");
    result.Set("amount", params->from_amount);
  } else if (params->from_amount.empty() && !params->to_amount.empty()) {
    result.Set("swapType", "EXACT_OUTPUT");
    result.Set("amount", params->to_amount);
  } else {
    return std::nullopt;
  }

  result.Set("slippagePercentage", params->slippage_percentage);

  auto provider = EncodeProvider(params->provider);
  if (!provider) {
    return std::nullopt;
  }
  result.Set("provider", *provider);

  auto route_priority = EncodeRoutePriority(params->route_priority);
  if (!route_priority) {
    return std::nullopt;
  }
  result.Set("routePriority", *route_priority);

  return GetJSON(result);
}

std::optional<std::string> EncodeStatusParams(
    mojom::Gate3SwapStatusParamsPtr params) {
  base::DictValue result;

  result.Set("routeId", params->route_id);
  result.Set("txHash", params->tx_hash);

  auto source_coin = EncodeCoinType(params->source_coin);
  if (!source_coin) {
    return std::nullopt;
  }
  result.Set("sourceCoin", *source_coin);
  result.Set("sourceChainId", params->source_chain_id);

  auto destination_coin = EncodeCoinType(params->destination_coin);
  if (!destination_coin) {
    return std::nullopt;
  }
  result.Set("destinationCoin", *destination_coin);
  result.Set("destinationChainId", params->destination_chain_id);

  result.Set("depositAddress", params->deposit_address);
  if (params->deposit_memo) {
    result.Set("depositMemo", std::string(params->deposit_memo->begin(),
                                          params->deposit_memo->end()));
  } else {
    result.Set("depositMemo", "");
  }

  auto provider = EncodeProvider(params->provider);
  if (!provider) {
    return std::nullopt;
  }
  result.Set("provider", *provider);

  return GetJSON(result);
}
}  // namespace gate3

}  // namespace brave_wallet
