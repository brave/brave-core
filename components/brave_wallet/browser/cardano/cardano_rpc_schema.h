/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_CARDANO_CARDANO_RPC_SCHEMA_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_CARDANO_CARDANO_RPC_SCHEMA_H_

#include <optional>
#include <vector>

#include "base/values.h"

namespace brave_wallet::cardano_rpc {

struct EpochParameters {
  bool operator==(const EpochParameters& other) const = default;

  uint64_t min_fee_coefficient = 0;
  uint64_t min_fee_constant = 0;

  static std::optional<EpochParameters> FromBlockfrostApiValue(
      const base::Value& value);
};

struct Block {
  bool operator==(const Block& other) const = default;

  uint32_t height = 0;
  uint64_t slot = 0;
  uint32_t epoch = 0;

  static std::optional<Block> FromBlockfrostApiValue(const base::Value& value);
};

struct UnspentOutput {
  bool operator==(const UnspentOutput& other) const = default;

  std::array<uint8_t, 32> tx_hash = {};
  uint32_t output_index = 0;
  uint64_t lovelace_amount = 0;

  static std::optional<UnspentOutput> FromBlockfrostApiValue(
      const base::Value& value);
};

using UnspentOutputs = std::vector<UnspentOutput>;

}  // namespace brave_wallet::cardano_rpc

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_CARDANO_CARDANO_RPC_SCHEMA_H_
