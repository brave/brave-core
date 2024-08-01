// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_RUST_CXX_SRC_SHARD_STORE_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_RUST_CXX_SRC_SHARD_STORE_H_

#include "brave/components/brave_wallet/common/zcash_utils.h"
#include "third_party/rust/cxx/v1/cxx.h"

namespace brave_wallet::orchard {

enum class ShardStoreStatusCode : uint32_t;
struct FfiShardTree;
struct FfiShardAddress;
struct FfiCheckpoint;
struct FfiCap;

using ShardStoreContext = ::brave_wallet::OrchardShardTreeDelegate;

ShardStoreStatusCode orchard_last_shard(const ShardStoreContext& ctx,
                                        FfiShardTree& into);
ShardStoreStatusCode orchard_put_shard(ShardStoreContext& ctx,
                                       const FfiShardTree& tree);
ShardStoreStatusCode orchard_get_shard(const ShardStoreContext& ctx,
                                       const FfiShardAddress& addr,
                                       FfiShardTree& tree);
ShardStoreStatusCode orchard_get_shard_roots(
    const ShardStoreContext& ctx,
    ::rust::Vec<FfiShardAddress>& into);
ShardStoreStatusCode orchard_truncate(ShardStoreContext& ctx,
                                      const FfiShardAddress& address);
ShardStoreStatusCode orchard_get_cap(const ShardStoreContext& ctx,
                                     FfiCap& into);
ShardStoreStatusCode orchard_put_cap(ShardStoreContext& ctx,
                                     const FfiCap& tree);
ShardStoreStatusCode orchard_min_checkpoint_id(const ShardStoreContext& ctx,
                                               uint32_t& into);
ShardStoreStatusCode orchard_max_checkpoint_id(const ShardStoreContext& ctx,
                                               uint32_t& into);
ShardStoreStatusCode orchard_add_checkpoint(ShardStoreContext& ctx,
                                            uint32_t checkpoint_id,
                                            const FfiCheckpoint& checkpoint);
ShardStoreStatusCode orchard_checkpoint_count(const ShardStoreContext& ctx,
                                              size_t& into);
ShardStoreStatusCode orchard_get_checkpoint_at_depth(
    const ShardStoreContext& ctx,
    size_t depth,
    uint32_t& into_checkpoint_id,
    FfiCheckpoint& into_checpoint);
ShardStoreStatusCode orchard_get_checkpoint(const ShardStoreContext& ctx,
                                            uint32_t checkpoint_id,
                                            FfiCheckpoint& into);
ShardStoreStatusCode orchard_update_checkpoint(ShardStoreContext& ctx,
                                               uint32_t checkpoint_id,
                                               const FfiCheckpoint& checkpoint);
ShardStoreStatusCode orchard_remove_checkpoint(ShardStoreContext& ctx,
                                               uint32_t checkpoint_id);
ShardStoreStatusCode orchard_truncate_checkpoint(ShardStoreContext& ctx,
                                                 uint32_t checkpoint_id);
ShardStoreStatusCode orchard_with_checkpoints(
    const ShardStoreContext& ctx,
    size_t limit,
    rust::cxxbridge1::Fn<ShardStoreStatusCode(uint32_t checkpoint_id,
                                              const FfiCheckpoint& checkpoint)>
        fn);
ShardStoreStatusCode orchard_get_checkpoints(const ShardStoreContext& ctx,
                                             size_t limit,
                                             ::rust::Vec<FfiCheckpoint>& into);

}  // namespace brave_wallet::orchard

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_RUST_CXX_SRC_SHARD_STORE_H_
