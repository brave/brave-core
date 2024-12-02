// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_RUST_ORCHARD_TESTING_SHARD_TREE_IMPL_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_RUST_ORCHARD_TESTING_SHARD_TREE_IMPL_H_

#include <memory>
#include <string>
#include <utility>

#include "brave/components/brave_wallet/browser/zcash/rust/lib.rs.h"
#include "brave/components/brave_wallet/browser/zcash/rust/orchard_shard_tree.h"

namespace brave_wallet::orchard {

class OrchardTestingShardTreeImpl : public OrchardShardTree {
 public:
  ~OrchardTestingShardTreeImpl() override;

  bool TruncateToCheckpoint(uint32_t checkpoint_id) override;

  bool ApplyScanResults(
      std::unique_ptr<OrchardDecodedBlocksBundle> commitments) override;

  base::expected<OrchardNoteWitness, std::string> CalculateWitness(
      uint32_t note_commitment_tree_position,
      uint32_t checkpoint) override;

 private:
  friend class OrchardShardTree;

  explicit OrchardTestingShardTreeImpl(
      rust::Box<CxxOrchardTestingShardTreeBundle> orcard_shard_tree);
  ::rust::Box<CxxOrchardTestingShardTreeBundle> orchard_shard_tree_;
};
}  // namespace brave_wallet::orchard

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_RUST_ORCHARD_TESTING_SHARD_TREE_IMPL_H_
