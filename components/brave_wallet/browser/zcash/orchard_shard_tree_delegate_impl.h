/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ORCHARD_SHARD_TREE_DELEGATE_IMPL_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ORCHARD_SHARD_TREE_DELEGATE_IMPL_H_

#include "brave/components/brave_wallet/browser/zcash/zcash_orchard_storage.h"
#include "brave/components/brave_wallet/common/zcash_utils.h"

namespace brave_wallet {

class OrchardShardTreeDelegateImpl : public OrchardShardTreeDelegate {
 public:
  OrchardShardTreeDelegateImpl(mojom::AccountIdPtr account_id,
                               scoped_refptr<ZCashOrchardStorage> storage);
  ~OrchardShardTreeDelegateImpl() override;

  base::expected<std::optional<OrchardCap>, Error> GetCap() const override;
  base::expected<bool, Error> PutCap(OrchardCap cap) override;
  base::expected<bool, Error> Truncate(uint32_t block_height) override;
  base::expected<std::optional<uint32_t>, Error> GetLatestShardIndex()
      const override;
  base::expected<bool, Error> PutShard(OrchardShard shard) override;
  base::expected<std::optional<OrchardShard>, Error> GetShard(
      OrchardShardAddress address) const override;
  base::expected<std::optional<OrchardShard>, Error> LastShard(
      uint8_t shard_height) const override;
  base::expected<size_t, Error> CheckpointCount() const override;
  base::expected<std::optional<uint32_t>, Error> MinCheckpointId()
      const override;
  base::expected<std::optional<uint32_t>, Error> MaxCheckpointId()
      const override;
  base::expected<std::optional<uint32_t>, Error> GetCheckpointAtDepth(
      uint32_t depth) const override;
  base::expected<std::optional<OrchardCheckpointBundle>, Error> GetCheckpoint(
      uint32_t checkpoint_id) const override;
  base::expected<std::vector<OrchardCheckpointBundle>, Error> GetCheckpoints(
      size_t limit) const override;
  base::expected<bool, Error> AddCheckpoint(
      uint32_t id,
      OrchardCheckpoint checkpoint) override;
  base::expected<bool, Error> TruncateCheckpoints(
      uint32_t checkpoint_id) override;
  base::expected<bool, Error> RemoveCheckpoint(uint32_t checkpoint_id) override;
  base::expected<bool, Error> RemoveCheckpointAt(uint32_t depth) override;
  base::expected<std::vector<OrchardShardAddress>, Error> GetShardRoots(
      uint8_t shard_level) const override;
  base::expected<bool, Error> UpdateCheckpoint(
      uint32_t id,
      OrchardCheckpoint checkpoint) override;

 private:
  mojom::AccountIdPtr account_id_;
  scoped_refptr<ZCashOrchardStorage> storage_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ORCHARD_SHARD_TREE_DELEGATE_IMPL_H_
