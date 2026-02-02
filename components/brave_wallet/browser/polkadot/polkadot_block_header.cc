/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/polkadot/polkadot_block_header.h"

#include "base/containers/extend.h"
#include "brave/components/brave_wallet/browser/internal/polkadot_extrinsic.rs.h"
#include "brave/components/brave_wallet/common/hash_utils.h"

namespace brave_wallet {

PolkadotBlockHeader::PolkadotBlockHeader() = default;
PolkadotBlockHeader::PolkadotBlockHeader(PolkadotBlockHeader&&) = default;

PolkadotBlockHeader::~PolkadotBlockHeader() = default;

PolkadotBlockHeader& PolkadotBlockHeader::operator=(PolkadotBlockHeader&&) =
    default;

std::array<uint8_t, 32> PolkadotBlockHeader::hash() const {
  auto enc_block_num = compact_scale_encode_u32(block_number);

  // We can die here because it means our parsing layer failed and we have a
  // precondition violation we didn't catch during development or testing.
  auto enc_num_logs = compact_scale_encode_u32(
      base::CheckedNumeric<uint32_t>(logs.size()).ValueOrDie());

  std::vector<uint8_t> blob;
  base::Extend(blob, parent_hash);
  base::Extend(blob, enc_block_num);
  base::Extend(blob, state_root);
  base::Extend(blob, extrinsics_root);
  base::Extend(blob, enc_num_logs);
  for (const auto& log : logs) {
    base::Extend(blob, log);
  }

  return Blake2bHash<32>({blob});
}

}  // namespace brave_wallet
