// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_RUST_ORCHARD_SHARD_TREE_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_RUST_ORCHARD_SHARD_TREE_H_

#include <memory>
#include <string>

#include "brave/components/brave_wallet/browser/zcash/rust/orchard_decoded_blocks_bundle.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"

namespace brave_wallet {

class OrchardStorage;

namespace orchard {

// Facade for the ShardTree from the shardtree crate:
// https://github.com/zcash/incrementalmerkletree/blob/db4ad58965f1870d2dac1d8e0d594cfaa0541e98/shardtree/src/lib.rs#L68
// backed by the provided OrchardStorage which represents ShardStore.
class OrchardShardTree {
 public:
  virtual ~OrchardShardTree() = default;

  // Truncates commitment tree to the provided checkpoint position;
  virtual bool TruncateToCheckpoint(uint32_t checkpoint_id) = 0;

  // Applies previously decoded blocks to the commitment tree.
  virtual bool ApplyScanResults(
      std::unique_ptr<OrchardDecodedBlocksBundle> commitments) = 0;

  virtual base::expected<OrchardNoteWitness, std::string> CalculateWitness(
      uint32_t note_commitment_tree_position,
      uint32_t checkpoint) = 0;

  // Creates original tree height of 32.
  static std::unique_ptr<OrchardShardTree> Create(
      ::brave_wallet::OrchardStorage& storage,
      const mojom::AccountIdPtr& account_id);

  // Creates a small tree height of 8 for testing purposes.
  static std::unique_ptr<OrchardShardTree> CreateForTesting(
      ::brave_wallet::OrchardStorage& storage,
      const mojom::AccountIdPtr& account_id);
};

}  // namespace orchard

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_RUST_ORCHARD_SHARD_TREE_H_
