// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_RUST_ORCHARD_SHARD_TREE_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_RUST_ORCHARD_SHARD_TREE_H_

#include <memory>
#include <string>
#include <vector>

#include "brave/components/brave_wallet/browser/zcash/rust/orchard_decoded_blocks_bunde.h"
#include "brave/components/brave_wallet/common/zcash_utils.h"

namespace brave_wallet::orchard {

class OrchardShardTree {
 public:
  virtual ~OrchardShardTree() {}

  virtual bool TruncateToCheckpoint(uint32_t checkpoint_id) = 0;

  virtual bool ApplyScanResults(
      std::unique_ptr<OrchardDecodedBlocksBundle> commitments) = 0;

  virtual base::expected<OrchardNoteWitness, std::string> CalculateWitness(
      uint32_t note_commitment_tree_position,
      uint32_t checkpoint) = 0;

  static std::unique_ptr<OrchardShardTree> Create(
      std::unique_ptr<::brave_wallet::OrchardShardTreeDelegate> delegate);

  static std::unique_ptr<OrchardShardTree> CreateForTesting(
      std::unique_ptr<::brave_wallet::OrchardShardTreeDelegate> delegate);
};

}  // namespace brave_wallet::orchard

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_RUST_ORCHARD_SHARD_TREE_H_
