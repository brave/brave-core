/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/polkadot/polkadot_block_header.h"

#include "brave/components/brave_wallet/browser/internal/polkadot_extrinsic.rs.h"
#include "brave/components/brave_wallet/common/hash_utils.h"

namespace brave_wallet {

PolkadotBlockHeader::PolkadotBlockHeader() = default;
PolkadotBlockHeader::PolkadotBlockHeader(PolkadotBlockHeader&&) = default;

PolkadotBlockHeader::~PolkadotBlockHeader() = default;

PolkadotBlockHeader& PolkadotBlockHeader::operator=(PolkadotBlockHeader&&) =
    default;

std::array<uint8_t, 32> PolkadotBlockHeader::GetHash() const {
  auto enc_block_num = compact_scale_encode_u32(block_number);
  return Blake2bHash<32>(
      {parent_hash, enc_block_num, state_root, extrinsics_root, encoded_logs});
}

}  // namespace brave_wallet
