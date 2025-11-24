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

UnspentOutput::Token::Token() = default;
UnspentOutput::Token::~Token() = default;
UnspentOutput::Token::Token(const UnspentOutput::Token&) = default;
UnspentOutput::Token::Token(UnspentOutput::Token&&) = default;
UnspentOutput::Token& UnspentOutput::Token::operator=(const Token&) = default;
UnspentOutput::Token& UnspentOutput::Token::operator=(Token&&) = default;

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
      result.lovelace_amount = amount;
      found_lovelace = true;
    } else {
      std::vector<uint8_t> policy_and_name;
      if (!base::HexStringToBytes(asset.unit, &policy_and_name)) {
        return std::nullopt;
      }
      if (policy_and_name.size() < 28) {
        return std::nullopt;
      }
      auto [policy, name] = base::span(policy_and_name).split_at<28>();

      UnspentOutput::Token token;
      base::span(token.policy_id).copy_from(policy);
      base::span(token.name).copy_from(name);
      result.tokens.emplace(std::move(token), amount);
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

}  // namespace brave_wallet::cardano_rpc
