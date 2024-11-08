// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_RUST_ORCHARD_DECODED_BLOCKS_BUNDLE_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_RUST_ORCHARD_DECODED_BLOCKS_BUNDLE_H_

#include <memory>
#include <vector>

#include "brave/components/brave_wallet/browser/internal/orchard_storage/orchard_shard_tree_types.h"

namespace brave_wallet::orchard {

// Contains result of the batch block decoding.
// This includes Orchard leafs to be inserterted to the shard tree and
// a set of dicovered Orchard spendable notes.
class OrchardDecodedBlocksBundle {
 public:
  virtual ~OrchardDecodedBlocksBundle() = default;
  virtual std::optional<std::vector<::brave_wallet::OrchardNote>>
  GetDiscoveredNotes() = 0;
};

}  // namespace brave_wallet::orchard

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_RUST_ORCHARD_DECODED_BLOCKS_BUNDLE_H_
