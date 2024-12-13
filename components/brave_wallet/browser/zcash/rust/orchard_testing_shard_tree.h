// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_RUST_ORCHARD_TESTING_SHARD_TREE_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_RUST_ORCHARD_TESTING_SHARD_TREE_H_

#include <memory>

#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"

namespace brave_wallet {

class OrchardStorage;

namespace orchard {

class OrchardShardTree;

// Creates a small tree height of 8 for testing purposes.
std::unique_ptr<OrchardShardTree> CreateShardTreeForTesting(  // IN-TEST
    ::brave_wallet::OrchardStorage& storage,
    const mojom::AccountIdPtr& account_id);

}  // namespace orchard
}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_RUST_ORCHARD_TESTING_SHARD_TREE_H_
