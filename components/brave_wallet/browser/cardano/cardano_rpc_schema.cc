/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/cardano/cardano_rpc_schema.h"

#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_rpc_blockfrost_api.h"

namespace brave_wallet::cardano_rpc {

namespace {

constexpr char kNativeLovelaceToken[] = "lovelace";
constexpr size_t kCardanoScriptHashSize = 28u;

}  // namespace

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
  if (!base::StringToUint64(api_epoch_parameters->coins_per_utxo_size,
                            &result.coins_per_utxo_size)) {
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

UnspentOutput::UnspentOutput() = default;
UnspentOutput::UnspentOutput(const UnspentOutput&) = default;
UnspentOutput::UnspentOutput(UnspentOutput&&) = default;
UnspentOutput& UnspentOutput::operator=(const UnspentOutput&) = default;
UnspentOutput& UnspentOutput::operator=(UnspentOutput&&) = default;
UnspentOutput::~UnspentOutput() = default;

// static
std::optional<UnspentOutput> UnspentOutput::FromBlockfrostApiValue(
    CardanoAddress address_to,
    std::optional<blockfrost_api::UnspentOutput> api_unspent_output) {
  if (!api_unspent_output) {
    return std::nullopt;
  }

  UnspentOutput result;
  result.address_to = std::move(address_to);
  if (!base::HexStringToSpan(api_unspent_output->tx_hash, result.tx_hash)) {
    return std::nullopt;
  }
  if (!base::StringToUint(api_unspent_output->output_index,
                          &result.output_index)) {
    return std::nullopt;
  }
  bool found_lovelace = false;
  for (const auto& asset : api_unspent_output->amount) {
    uint64_t amount = 0;
    if (!base::StringToUint64(asset.quantity, &amount)) {
      return std::nullopt;
    }

    if (asset.unit == kNativeLovelaceToken) {
      if (found_lovelace) {
        return std::nullopt;
      }
      result.lovelace_amount = amount;
      found_lovelace = true;
    } else {
      TokenId token_id;
      if (!base::HexStringToBytes(asset.unit, &token_id)) {
        return std::nullopt;
      }
      // Fixed-size policy_id and non-empty name.
      if (token_id.size() < kCardanoScriptHashSize + 1u) {
        return std::nullopt;
      }

      if (!result.tokens.emplace(std::move(token_id), amount).second) {
        return std::nullopt;
      }
    }
  }

  if (!found_lovelace) {
    return std::nullopt;
  }

  return result;
}

// static
std::optional<Transaction> Transaction::FromBlockfrostApiValue(
    std::optional<blockfrost_api::Transaction> api_transaction) {
  if (!api_transaction) {
    return std::nullopt;
  }

  Transaction result;
  if (!base::HexStringToSpan(api_transaction->hash, result.tx_hash)) {
    return std::nullopt;
  }

  return result;
}

AssetInfo::AssetInfo() = default;
AssetInfo::AssetInfo(const AssetInfo&) = default;
AssetInfo::AssetInfo(AssetInfo&&) = default;
AssetInfo& AssetInfo::operator=(const AssetInfo&) = default;
AssetInfo& AssetInfo::operator=(AssetInfo&&) = default;
AssetInfo::~AssetInfo() = default;

// static
std::optional<AssetInfo> AssetInfo::FromBlockfrostApiValue(
    std::optional<blockfrost_api::Asset> api_asset) {
  if (!api_asset) {
    return std::nullopt;
  }

  AssetInfo result;
  result.asset = api_asset->asset;
  result.name = api_asset->metadata.name;
  result.ticker = api_asset->metadata.ticker;
  result.decimals = api_asset->metadata.decimals;

  return result;
}
}  // namespace brave_wallet::cardano_rpc
