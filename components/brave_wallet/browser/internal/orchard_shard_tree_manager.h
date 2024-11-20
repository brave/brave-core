/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_INTERNAL_ORCHARD_SHARD_TREE_MANAGER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_INTERNAL_ORCHARD_SHARD_TREE_MANAGER_H_

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/types/expected.h"
#include "brave/components/brave_wallet/browser/internal/orchard_block_scanner.h"

namespace brave_wallet {

class OrchardShardTreeDelegate;
namespace orchard {
class OrchardShardTree;
}  // namespace orchard

// Presents Orchard commitment tree.
// Provides methods for inserting leafs to the tree and
// calculating witness information for specified leaf positions.
class OrchardShardTreeManager {
 public:
  explicit OrchardShardTreeManager(
      std::unique_ptr<::brave_wallet::orchard::OrchardShardTree> shard_tree);
  ~OrchardShardTreeManager();
  // Inserts leafs extracted from the provided scan result.
  bool InsertCommitments(OrchardBlockScanner::Result&& commitments);
  // Calculates witness(merkle path to the tree root) for the provided
  // set of notes.
  // Checkpoint is also provided as an achor(selected right-most border of the
  // tree).
  base::expected<std::vector<OrchardInput>, std::string> CalculateWitness(
      const std::vector<OrchardInput>& notes,
      uint32_t checkpoint_position);
  // Truncates tree including specified checkpoint.
  // Needed for chain reorg cases.
  bool Truncate(uint32_t checkpoint);

  // Creates shard tree size of 32.
  static std::unique_ptr<OrchardShardTreeManager> Create(
      std::unique_ptr<OrchardShardTreeDelegate> delegate);

  // Creates shard tree size of 8 for testing.
  static std::unique_ptr<OrchardShardTreeManager> CreateForTesting(
      std::unique_ptr<OrchardShardTreeDelegate> delegate);

 private:
  std::unique_ptr<::brave_wallet::orchard::OrchardShardTree>
      orchard_shard_tree_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_INTERNAL_ORCHARD_SHARD_TREE_MANAGER_H_
