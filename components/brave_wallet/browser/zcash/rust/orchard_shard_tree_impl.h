// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_RUST_ORCHARD_SHARD_TREE_IMPL_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_RUST_ORCHARD_SHARD_TREE_IMPL_H_

#include <memory>
#include <string>

#include "brave/components/brave_wallet/browser/internal/orchard_storage/orchard_types.h"
#include "brave/components/brave_wallet/browser/zcash/rust/orchard_shard_tree.h"
#include "third_party/rust/cxx/v1/cxx.h"

namespace brave_wallet::orchard {

struct CxxOrchardShardTreeBundle;

class OrchardShardTreeImpl : public OrchardShardTree {
 public:
  ~OrchardShardTreeImpl() override;

  bool TruncateToCheckpoint(uint32_t checkpoint_id) override;

  bool ApplyScanResults(
      std::unique_ptr<OrchardDecodedBlocksBundle> commitments) override;

  base::expected<OrchardNoteWitness, std::string> CalculateWitness(
      uint32_t note_commitment_tree_position,
      uint32_t checkpoint) override;

 private:
  friend class OrchardShardTree;
  explicit OrchardShardTreeImpl(
      rust::Box<CxxOrchardShardTreeBundle> orcard_shard_tree);
  ::rust::Box<CxxOrchardShardTreeBundle> orhcard_shard_tree_;
};

}  // namespace brave_wallet::orchard

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_RUST_ORCHARD_SHARD_TREE_IMPL_H_
