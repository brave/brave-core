// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_RUST_CXX_SRC_SHARD_STORE_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_RUST_CXX_SRC_SHARD_STORE_H_

#include "brave/components/brave_wallet/common/zcash_utils.h"
#include "third_party/rust/cxx/v1/cxx.h"

namespace brave_wallet::orchard {

enum class ShardStoreStatusCode : uint32_t;
struct ShardTreeShard;
struct ShardTreeAddress;
struct ShardTreeCheckpoint;
struct ShardTreeCap;
struct ShardTreeCheckpointBundle;

using ShardTreeDelegate = ::brave_wallet::OrchardShardTreeDelegate;

ShardStoreStatusCode shard_store_last_shard(const ShardTreeDelegate& delegate,
                                            ShardTreeShard& into,
                                            uint8_t shard_level);
ShardStoreStatusCode shard_store_put_shard(ShardTreeDelegate& delegate,
                                           const ShardTreeShard& tree);
ShardStoreStatusCode shard_store_get_shard(const ShardTreeDelegate& delegate,
                                           const ShardTreeAddress& addr,
                                           ShardTreeShard& tree);
ShardStoreStatusCode shard_store_get_shard_roots(
    const ShardTreeDelegate& delegate,
    ::rust::Vec<ShardTreeAddress>& into,
    uint8_t shard_level);
ShardStoreStatusCode shard_store_truncate(ShardTreeDelegate& delegate,
                                          const ShardTreeAddress& address);
ShardStoreStatusCode shard_store_get_cap(const ShardTreeDelegate& delegate,
                                         ShardTreeCap& into);
ShardStoreStatusCode shard_store_put_cap(ShardTreeDelegate& delegate,
                                         const ShardTreeCap& tree);
ShardStoreStatusCode shard_store_min_checkpoint_id(
    const ShardTreeDelegate& delegate,
    uint32_t& into);
ShardStoreStatusCode shard_store_max_checkpoint_id(
    const ShardTreeDelegate& delegate,
    uint32_t& into);
ShardStoreStatusCode shard_store_add_checkpoint(
    ShardTreeDelegate& delegate,
    uint32_t checkpoint_id,
    const ShardTreeCheckpoint& checkpoint);
ShardStoreStatusCode shard_store_checkpoint_count(
    const ShardTreeDelegate& delegate,
    size_t& into);
ShardStoreStatusCode shard_store_get_checkpoint_at_depth(
    const ShardTreeDelegate& delegate,
    size_t depth,
    uint32_t& into_checkpoint_id,
    ShardTreeCheckpoint& into_checpoint);
ShardStoreStatusCode shard_store_get_checkpoint(
    const ShardTreeDelegate& delegate,
    uint32_t checkpoint_id,
    ShardTreeCheckpoint& into);
ShardStoreStatusCode shard_store_update_checkpoint(
    ShardTreeDelegate& delegate,
    uint32_t checkpoint_id,
    const ShardTreeCheckpoint& checkpoint);
ShardStoreStatusCode shard_store_remove_checkpoint(ShardTreeDelegate& delegate,
                                                   uint32_t checkpoint_id);
ShardStoreStatusCode shard_store_truncate_checkpoint(
    ShardTreeDelegate& delegate,
    uint32_t checkpoint_id);
ShardStoreStatusCode shard_store_get_checkpoints(
    const ShardTreeDelegate& delegate,
    size_t limit,
    ::rust::Vec<ShardTreeCheckpointBundle>& into);

}  // namespace brave_wallet::orchard

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_RUST_CXX_SRC_SHARD_STORE_H_
