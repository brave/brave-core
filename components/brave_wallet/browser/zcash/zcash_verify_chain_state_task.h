/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_VERIFY_CHAIN_STATE_TASK_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_VERIFY_CHAIN_STATE_TASK_H_

#include <string>

#include "brave/components/brave_wallet/browser/zcash/zcash_shield_sync_service.h"

namespace brave_wallet {

// Ensures that the hash of the latest scanned block remains unchanged for the
// reason of a possible chain reorganization event. If the hash has changed, the
// latest scanned block is rolled back, and outdated data is removed from the
// Orchard database.
class ZCashVerifyChainStateTask {
 public:
  using ZCashVerifyChainStateTaskCallback = base::OnceCallback<void(
      base::expected<bool, ZCashShieldSyncService::Error>)>;
  ZCashVerifyChainStateTask(ZCashActionContext& context,
                            ZCashVerifyChainStateTaskCallback callback);
  ~ZCashVerifyChainStateTask();

  void Start();

 private:
  enum class VerificationState { kNoReorg, kReorg };

  void WorkOnTask();
  void ScheduleWorkOnTask();

  void GetAccountMeta();
  void OnGetAccountMeta(
      base::expected<std::optional<OrchardStorage::AccountMeta>,
                     OrchardStorage::Error> result);

  void GetChainTipBlock();
  void OnGetChainTipBlock(
      base::expected<zcash::mojom::BlockIDPtr, std::string> result);

  // Verifies that last known scanned block hash is unchanged
  void GetTreeStateForLatestScannedBlock();
  void OnGetTreeStateForChainVerification(
      base::expected<zcash::mojom::TreeStatePtr, std::string> tree_state);

  void VerifyChainState();

  void GetMinCheckpointId();
  void OnGetMinCheckpointId(
      base::expected<std::optional<uint32_t>, OrchardStorage::Error> result);

  // Resolves block hash for the block we are going to fallback
  void GetRewindBlockTreeState();
  void OnGetRewindBlockTreeState(
      base::expected<zcash::mojom::TreeStatePtr, std::string> tree_state);

  void Rewind();
  void OnRewindResult(
      base::expected<OrchardStorage::Result, OrchardStorage::Error> result);

  raw_ref<ZCashActionContext> context_;
  ZCashVerifyChainStateTaskCallback callback_;

  std::optional<ZCashShieldSyncService::Error> error_;
  std::optional<OrchardStorage::AccountMeta> account_meta_;

  // Latest block in the blockchain.
  std::optional<uint32_t> chain_tip_block_;
  // Information whether reorg has been happened.
  std::optional<VerificationState> verification_state_;

  // Information needed to update sync_state.
  // We use min checkpoint id which represents minimal checkpointed block height
  // as the rewind block height.
  std::optional<uint32_t> rewind_block_heght_;
  // Tree state contains block hash to update latest scanned block hash.
  std::optional<zcash::mojom::TreeStatePtr> rewind_block_tree_state_;
  // Result of the sync_state update.
  std::optional<bool> rewind_result_;

  base::WeakPtrFactory<ZCashVerifyChainStateTask> weak_ptr_factory_{this};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_VERIFY_CHAIN_STATE_TASK_H_
