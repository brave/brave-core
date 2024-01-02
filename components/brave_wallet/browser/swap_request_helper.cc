/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/swap_request_helper.h"

#include <optional>
#include <utility>

#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "base/values.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/json_rpc_requests_helper.h"
#include "brave/components/brave_wallet/browser/solana_keyring.h"
#include "brave/components/json/rs/src/lib.rs.h"

namespace brave_wallet {

std::optional<std::string> EncodeJupiterTransactionParams(
    const mojom::JupiterTransactionParams& params,
    bool has_fee) {
  base::Value::Dict tx_params;

  // The code below does the following two things:
  //   - compute the ATA address that should be used to receive fees
  //   - verify if output_mint is a valid address
  std::optional<std::string> associated_token_account =
      SolanaKeyring::GetAssociatedTokenAccount(
          params.quote->output_mint, brave_wallet::kSolanaFeeRecipient);
  if (!associated_token_account) {
    return std::nullopt;
  }

  // If the if-condition below is false, associated_token_account is unused,
  // but the originating call to SolanaKeyring::GetAssociatedTokenAccount()
  // is still done to ensure output_mint is always valid.
  if (has_fee) {
    // feeAccount is the ATA account for the output mint where the fee will be
    // sent to.
    tx_params.Set("feeAccount", *associated_token_account);
  }

  tx_params.Set("userPublicKey", params.user_public_key);

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

  result = std::string(json::convert_string_value_to_uint64(
      "/quoteResponse/slippageBps", result, false));

  if (params.quote->platform_fee) {
    result = std::string(json::convert_string_value_to_uint64(
        "/quoteResponse/platformFee/feeBps", result, false));
  }

  for (int i = 0; i < static_cast<int>(params.quote->route_plan.size()); i++) {
    result = std::string(json::convert_string_value_to_uint64(
        base::StringPrintf("/quoteResponse/routePlan/%d/percent", i), result,
        false));
  }

  return result;
}

}  // namespace brave_wallet
