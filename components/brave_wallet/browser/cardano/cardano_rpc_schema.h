/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_CARDANO_CARDANO_RPC_SCHEMA_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_CARDANO_CARDANO_RPC_SCHEMA_H_

#include <stdint.h>

#include <array>
#include <optional>
#include <vector>

#include "base/containers/flat_map.h"
#include "brave/components/brave_wallet/common/cardano_address.h"

namespace brave_wallet::cardano_rpc {

namespace blockfrost_api {
struct EpochParameters;
struct Block;
struct UnspentOutput;
struct Transaction;
struct Asset;
}  // namespace blockfrost_api

using TokenId = std::vector<uint8_t>;  // 28-bytes policy_id and non-empty name.
using Tokens = base::flat_map<TokenId, uint64_t>;

// Adapter of Blockfrost's EpochParameters struct for wallet's use.
struct EpochParameters {
  bool operator==(const EpochParameters& other) const = default;

  uint64_t min_fee_coefficient = 0;
  uint64_t min_fee_constant = 0;
  uint64_t coins_per_utxo_size = 0;

  static std::optional<EpochParameters> FromBlockfrostApiValue(
      std::optional<blockfrost_api::EpochParameters> api_epoch_parameters);
};

// Adapter of Blockfrost's Block struct for wallet's use.
struct Block {
  bool operator==(const Block& other) const = default;

  uint32_t height = 0;
  uint64_t slot = 0;
  uint32_t epoch = 0;

  static std::optional<Block> FromBlockfrostApiValue(
      std::optional<blockfrost_api::Block> api_block);
};

// Adapter of Blockfrost's UnspentOutput struct for wallet's use.
struct UnspentOutput {
  UnspentOutput();
  UnspentOutput(const UnspentOutput&);
  UnspentOutput(UnspentOutput&&);
  UnspentOutput& operator=(const UnspentOutput&);
  UnspentOutput& operator=(UnspentOutput&&);
  ~UnspentOutput();

  bool operator<=>(const UnspentOutput& other) const = default;

  std::array<uint8_t, 32> tx_hash = {};
  uint32_t output_index = 0;
  uint64_t lovelace_amount = 0;
  Tokens tokens;
  CardanoAddress address_to;

  static std::optional<UnspentOutput> FromBlockfrostApiValue(
      CardanoAddress address_to,
      std::optional<blockfrost_api::UnspentOutput> api_unspent_output);
};

using UnspentOutputs = std::vector<UnspentOutput>;

// Adapter of Blockfrost's Transaction struct for wallet's use.
struct Transaction {
  bool operator==(const Transaction& other) const = default;

  std::array<uint8_t, 32> tx_hash = {};

  static std::optional<Transaction> FromBlockfrostApiValue(
      std::optional<blockfrost_api::Transaction> api_epoch_parameters);
};

// Adapter of Blockfrost's Asset struct for wallet's use.
struct AssetInfo {
  AssetInfo();
  AssetInfo(const AssetInfo&);
  AssetInfo(AssetInfo&&);
  AssetInfo& operator=(const AssetInfo&);
  AssetInfo& operator=(AssetInfo&&);
  ~AssetInfo();

  bool operator==(const AssetInfo& other) const = default;

  std::string asset;
  std::string name;
  std::string ticker;
  uint32_t decimals = 0;

  static std::optional<AssetInfo> FromBlockfrostApiValue(
      std::optional<blockfrost_api::Asset> api_epoch_parameters);
};

}  // namespace brave_wallet::cardano_rpc

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_CARDANO_CARDANO_RPC_SCHEMA_H_
