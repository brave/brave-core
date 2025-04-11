/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/cardano/cardano_rpc_schema.h"

#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_rpc_blockfrost_api.h"
#include "brave/components/brave_wallet/common/hex_utils.h"

namespace brave_wallet::cardano_rpc {

namespace {

constexpr char kNativeLovelaceToken[] = "lovelace";

}

// static
std::optional<EpochParameters> EpochParameters::FromBlockfrostApiValue(
    std::optional<blockfrost_api::EpochParameters> api_epoch_parameters) {
  if (!api_epoch_parameters) {
    return std::nullopt;
  }

  EpochParameters result;
  if (!base::StringToUint64(api_epoch_parameters->min_fee_a,
                            &result.min_fee_coefficient)) {
    return std::nullopt;
  }
  if (!base::StringToUint64(api_epoch_parameters->min_fee_b,
                            &result.min_fee_constant)) {
    return std::nullopt;
  }

  return result;
}

// static
std::optional<Block> Block::FromBlockfrostApiValue(
    std::optional<blockfrost_api::Block> api_block) {
  if (!api_block) {
    return std::nullopt;
  }

  Block result;
  if (!base::StringToUint(api_block->height, &result.height)) {
    return std::nullopt;
  }
  if (!base::StringToUint64(api_block->slot, &result.slot)) {
    return std::nullopt;
  }
  if (!base::StringToUint(api_block->epoch, &result.epoch)) {
    return std::nullopt;
  }

  return result;
}

// static
std::optional<UnspentOutput> UnspentOutput::FromBlockfrostApiValue(
    std::optional<blockfrost_api::UnspentOutput> api_unspent_output) {
  if (!api_unspent_output) {
    return std::nullopt;
  }

  UnspentOutput result;
  if (!base::HexStringToSpan(api_unspent_output->tx_hash, result.tx_hash)) {
    return std::nullopt;
  }
  if (!base::StringToUint(api_unspent_output->output_index,
                          &result.output_index)) {
    return std::nullopt;
  }
  bool found_lovelace = false;
  for (const auto& asset : api_unspent_output->amount) {
    if (asset.unit == kNativeLovelaceToken) {
      if (!base::StringToUint64(asset.quantity, &result.lovelace_amount)) {
        return std::nullopt;
      }
      found_lovelace = true;
      break;
    }
  }

  if (!found_lovelace) {
    return std::nullopt;
  }

  return result;
}

}  // namespace brave_wallet::cardano_rpc
