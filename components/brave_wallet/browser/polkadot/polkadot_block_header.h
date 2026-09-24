/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_POLKADOT_POLKADOT_BLOCK_HEADER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_POLKADOT_POLKADOT_BLOCK_HEADER_H_

#include <stdint.h>

#include <array>
#include <vector>

#include "brave/components/brave_wallet/browser/polkadot/polkadot_utils.h"

namespace brave_wallet {

struct PolkadotBlockHeader {
  PolkadotBlockHeader();
  PolkadotBlockHeader(const PolkadotBlockHeader&) = delete;
  PolkadotBlockHeader(PolkadotBlockHeader&&);

  ~PolkadotBlockHeader();

  PolkadotBlockHeader& operator=(const PolkadotBlockHeader&) = delete;
  PolkadotBlockHeader& operator=(PolkadotBlockHeader&&);

  std::array<uint8_t, 32> GetHash() const;

  std::array<uint8_t, kPolkadotBlockHashSize> parent_hash = {};
  uint32_t block_number = 0;
  std::array<uint8_t, kPolkadotBlockHashSize> state_root = {};
  std::array<uint8_t, kPolkadotBlockHashSize> extrinsics_root = {};

  // Store the logs from the block header as a single flat vector, prefixed by a
  // Compact<u32> representing the number of logs present in the header.
  std::vector<uint8_t> encoded_logs;
};

struct PolkadotBlock {
  PolkadotBlock();
  PolkadotBlock(const PolkadotBlock&) = delete;
  PolkadotBlock(PolkadotBlock&&);

  ~PolkadotBlock();

  PolkadotBlock& operator=(const PolkadotBlock&) = delete;
  PolkadotBlock& operator=(PolkadotBlock&&);

  PolkadotBlockHeader header;
  std::vector<std::string> extrinsics;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_POLKADOT_POLKADOT_BLOCK_HEADER_H_
