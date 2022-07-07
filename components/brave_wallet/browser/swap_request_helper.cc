/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/swap_request_helper.h"

#include <utility>

#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "base/values.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/json_rpc_requests_helper.h"
#include "brave/components/brave_wallet/browser/solana_keyring.h"
#include "brave/components/json/rs/src/lib.rs.h"

namespace brave_wallet {

absl::optional<std::string> EncodeJupiterTransactionParams(
    mojom::JupiterSwapParamsPtr params) {
  DCHECK(params);
  base::Value::Dict tx_params;

  // feeAccount is the ATA account for the output mint where the fee will be
  // sent to.
  absl::optional<std::string> associated_token_account =
      SolanaKeyring::GetAssociatedTokenAccount(
          params->output_mint, brave_wallet::kSolanaFeeRecipient);
  if (!associated_token_account)
    return absl::nullopt;

  tx_params.Set("feeAccount", *associated_token_account);
  tx_params.Set("userPublicKey", params->user_public_key);

  base::Value::Dict route;
  route.Set("inAmount", base::NumberToString(params->route->in_amount));
  route.Set("outAmount", base::NumberToString(params->route->out_amount));
  route.Set("amount", base::NumberToString(params->route->amount));
  route.Set("otherAmountThreshold",
            base::NumberToString(params->route->other_amount_threshold));
  route.Set("swapMode", params->route->swap_mode);
  route.Set("priceImpactPct", params->route->price_impact_pct);

  base::Value::List market_infos_value;
  for (const auto& market_info : params->route->market_infos) {
    base::Value::Dict market_info_value;
    market_info_value.Set("id", market_info->id);
    market_info_value.Set("label", market_info->label);
    market_info_value.Set("inputMint", market_info->input_mint);
    market_info_value.Set("outputMint", market_info->output_mint);
    market_info_value.Set("notEnoughLiquidity",
                          market_info->not_enough_liquidity);
    market_info_value.Set("inAmount",
                          base::NumberToString(market_info->in_amount));
    market_info_value.Set("outAmount",
                          base::NumberToString(market_info->out_amount));
    market_info_value.Set("priceImpactPct", market_info->price_impact_pct);

    base::Value::Dict lp_fee_value;
    lp_fee_value.Set("amount",
                     base::NumberToString(market_info->lp_fee->amount));
    lp_fee_value.Set("mint", market_info->lp_fee->mint);
    lp_fee_value.Set("pct", market_info->lp_fee->pct);

    market_info_value.Set("lpFee", std::move(lp_fee_value));

    base::Value::Dict platform_fee_value;
    platform_fee_value.Set(
        "amount", base::NumberToString(market_info->platform_fee->amount));
    platform_fee_value.Set("mint", market_info->platform_fee->mint);
    platform_fee_value.Set("pct", market_info->platform_fee->pct);
    market_info_value.Set("platformFee", std::move(platform_fee_value));

    market_infos_value.Append(std::move(market_info_value));
  }

  route.Set("marketInfos", std::move(market_infos_value));
  tx_params.Set("route", std::move(route));

  // FIXME - GetJSON should be refactored to accept a base::Value::Dict
  std::string result = GetJSON(base::Value(std::move(tx_params)));
  result = std::string(
      json::convert_string_value_to_uint64("/route/inAmount", result, false));
  result = std::string(
      json::convert_string_value_to_uint64("/route/outAmount", result, false));
  result = std::string(
      json::convert_string_value_to_uint64("/route/amount", result, false));
  result = std::string(json::convert_string_value_to_uint64(
      "/route/otherAmountThreshold", result, false));

  for (int i = 0; i < static_cast<int>(params->route->market_infos.size());
       i++) {
    result = std::string(json::convert_string_value_to_uint64(
        base::StringPrintf("/route/marketInfos/%d/inAmount", i), result,
        false));
    result = std::string(json::convert_string_value_to_uint64(
        base::StringPrintf("/route/marketInfos/%d/outAmount", i), result,
        false));
    result = std::string(json::convert_string_value_to_uint64(
        base::StringPrintf("/route/marketInfos/%d/lpFee/amount", i), result,
        false));
    result = std::string(json::convert_string_value_to_uint64(
        base::StringPrintf("/route/marketInfos/%d/platformFee/amount", i),
        result, false));
  }

  return result;
}

}  // namespace brave_wallet
