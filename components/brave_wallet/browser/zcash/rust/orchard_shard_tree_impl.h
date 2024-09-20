// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/components/brave_wallet/browser/zcash/rust/lib.rs.h"
#include "brave/components/brave_wallet/browser/zcash/rust/orchard_shard_tree.h"

namespace brave_wallet::orchard {

class OrchardShardTreeImpl : public OrchardShardTree {
 public:
  OrchardShardTreeImpl(rust::Box<OrchardShardTreeBundle> orcard_shard_tree);
  ~OrchardShardTreeImpl() override;

  bool ApplyScanResults(
      std::unique_ptr<OrchardDecodedBlocksBundle> commitments) override;

  base::expected<OrchardNoteWitness, std::string> CalculateWitness(
      const OrchardNote& note,
      uint32_t checkpoint) override;

 private:
  static std::unique_ptr<OrchardShardTreeImpl> Create(
      std::unique_ptr<::brave_wallet::orchard::ShardStoreContext> context);

  ::rust::Box<OrchardShardTreeBundle> orcard_shard_tree_;
};

}  // namespace brave_wallet::orchard
