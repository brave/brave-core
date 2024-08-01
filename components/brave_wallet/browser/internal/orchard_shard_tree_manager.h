/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_INTERNAL_ORCHARD_SHARD_TREE_MANAGER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_INTERNAL_ORCHARD_SHARD_TREE_MANAGER_H_

#include "base/types/expected.h"
#include "brave/components/brave_wallet/browser/zcash/rust/orchard_block_decoder.h"
#include "brave/components/brave_wallet/browser/zcash/rust/orchard_shard_tree.h"
#include "brave/components/brave_wallet/common/zcash_utils.h"

namespace brave_wallet {

class OrchardShardTreeManager {
 public:
  OrchardShardTreeManager(std::unique_ptr<::brave_wallet::orchard::OrchardShardTree> shard_tree);
  ~OrchardShardTreeManager();
  bool InsertCommitments(std::vector<OrchardCommitment> commitments);
  bool InsertSubtreeRoots(std::vector<OrchardShard> roots);

  static std::unique_ptr<OrchardShardTreeManager> Create(std::unique_ptr<orchard::OrchardShardTreeDelegate> delegate);

 private:
  std::unique_ptr<::brave_wallet::orchard::OrchardShardTree> orchard_shard_tree_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_INTERNAL_ORCHARD_SHARD_TREE_MANAGER_H_
