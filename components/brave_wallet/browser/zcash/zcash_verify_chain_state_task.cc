/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/zcash/zcash_verify_chain_state_task.h"

#include <string>
#include <utility>
#include <vector>

#include "brave/components/brave_wallet/browser/zcash/zcash_wallet_service.h"

namespace brave_wallet {

ZCashVerifyChainStateTask::ZCashVerifyChainStateTask(
    ZCashActionContext& context,
    ZCashVerifyChainStateTaskCallback callback)
    : context_(context), callback_(std::move(callback)) {}
ZCashVerifyChainStateTask::~ZCashVerifyChainStateTask() = default;

void ZCashVerifyChainStateTask::Start() {
  ScheduleWorkOnTask();
}

void ZCashVerifyChainStateTask::WorkOnTask() {
  if (error_) {
    std::move(callback_).Run(base::unexpected(*error_));
    return;
  }

  if (!account_meta_) {
    GetAccountMeta();
    return;
  }

  // In case no blocks were scanned return early.
  if (!account_meta_->latest_scanned_block_id) {
    std::move(callback_).Run(true);
    return;
  }

  if (!chain_tip_block_) {
    GetChainTipBlock();
    return;
  }

  if (!verification_state_) {
    VerifyChainState();
    return;
  }

  // Reorg not needed
  if (verification_state_.value() == VerificationState::kNoReorg) {
    std::move(callback_).Run(true);
    return;
  }

  // Reorg flow
  if (!rewind_block_heght_) {
    GetMinCheckpointId();
    return;
  }

  if (!rewind_block_tree_state_) {
    GetRewindBlockTreeState();
    return;
  }

  if (!rewind_result_) {
    Rewind();
    return;
  }

  std::move(callback_).Run(rewind_result_.value());
}

void ZCashVerifyChainStateTask::ScheduleWorkOnTask() {
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, base::BindOnce(&ZCashVerifyChainStateTask::WorkOnTask,
                                weak_ptr_factory_.GetWeakPtr()));
}

void ZCashVerifyChainStateTask::GetAccountMeta() {
  context_->sync_state->AsyncCall(&OrchardSyncState::GetAccountMeta)
      .WithArgs(context_->account_id.Clone())
      .Then(base::BindOnce(&ZCashVerifyChainStateTask::OnGetAccountMeta,
                           weak_ptr_factory_.GetWeakPtr()));
}

void ZCashVerifyChainStateTask::OnGetAccountMeta(
    base::expected<std::optional<OrchardStorage::AccountMeta>,
                   OrchardStorage::Error> result) {
  if (!result.has_value()) {
    error_ = ZCashShieldSyncService::Error{
        ZCashShieldSyncService::ErrorCode::kFailedToRetrieveAccount,
        result.error().message};
    ScheduleWorkOnTask();
    return;
  }

  if (!result.value()) {
    error_ = ZCashShieldSyncService::Error{
        ZCashShieldSyncService::ErrorCode::kFailedToRetrieveAccount,
        "Account doesn't exist"};
    ScheduleWorkOnTask();
    return;
  }

  if (result.value()->account_birthday < kNu5BlockUpdate) {
    error_ = ZCashShieldSyncService::Error{
        ZCashShieldSyncService::ErrorCode::kFailedToRetrieveAccount,
        "Wrong birthday block height"};
    ScheduleWorkOnTask();
    return;
  }

  account_meta_ = **result;
  ScheduleWorkOnTask();
}

void ZCashVerifyChainStateTask::GetChainTipBlock() {
  context_->zcash_rpc->GetLatestBlock(
      context_->chain_id,
      base::BindOnce(&ZCashVerifyChainStateTask::OnGetChainTipBlock,
                     weak_ptr_factory_.GetWeakPtr()));
}

void ZCashVerifyChainStateTask::OnGetChainTipBlock(
    base::expected<zcash::mojom::BlockIDPtr, std::string> result) {
  if (!result.has_value()) {
    error_ = ZCashShieldSyncService::Error{
        ZCashShieldSyncService::ErrorCode::kFailedToUpdateChainTip,
        result.error()};
    ScheduleWorkOnTask();
    return;
  }

  chain_tip_block_ = (*result)->height;

  ScheduleWorkOnTask();
}

void ZCashVerifyChainStateTask::GetMinCheckpointId() {
  context_->sync_state->AsyncCall(&OrchardSyncState::GetMinCheckpointId)
      .WithArgs(context_->account_id.Clone())
      .Then(base::BindOnce(&ZCashVerifyChainStateTask::OnGetMinCheckpointId,
                           weak_ptr_factory_.GetWeakPtr()));
}

void ZCashVerifyChainStateTask::OnGetMinCheckpointId(
    base::expected<std::optional<uint32_t>, OrchardStorage::Error> result) {
  if (!result.has_value()) {
    error_ = ZCashShieldSyncService::Error{
        ZCashShieldSyncService::ErrorCode::kDatabaseError,
        result.error().message};
    ScheduleWorkOnTask();
    return;
  }

  if (!result.value()) {
    error_ = ZCashShieldSyncService::Error{
        ZCashShieldSyncService::ErrorCode::kFailedToVerifyChainState,
        "CheckpointId doesn't exist"};
    ScheduleWorkOnTask();
    return;
  }

  rewind_block_heght_ = *(result.value());

  ScheduleWorkOnTask();
}

void ZCashVerifyChainStateTask::VerifyChainState() {
  // Skip chain state verification if no blocks were scanned yet
  if (!account_meta_->latest_scanned_block_id) {
    verification_state_ = VerificationState::kNoReorg;
    ScheduleWorkOnTask();
    return;
  }
  // If block chain has removed blocks we already scanned then we need to handle
  // chain reorg.
  if (*chain_tip_block_ < account_meta_->latest_scanned_block_id.value()) {
    // Assume that chain reorg can't affect more than kChainReorgBlockDelta
    // blocks So we can just fallback on this number from the chain tip block.
    verification_state_ = VerificationState::kReorg;
    ScheduleWorkOnTask();
    return;
  }
  // Retrieve block info for last scanned block id to check whether block hash
  // is the same
  auto block_id = zcash::mojom::BlockID::New(
      account_meta_->latest_scanned_block_id.value(), std::vector<uint8_t>());

  context_->zcash_rpc->GetTreeState(
      context_->chain_id, std::move(block_id),
      base::BindOnce(
          &ZCashVerifyChainStateTask::OnGetTreeStateForChainVerification,
          weak_ptr_factory_.GetWeakPtr()));
}

void ZCashVerifyChainStateTask::OnGetTreeStateForChainVerification(
    base::expected<zcash::mojom::TreeStatePtr, std::string> tree_state) {
  CHECK(account_meta_.has_value());
  CHECK(account_meta_->latest_scanned_block_hash.has_value());
  CHECK(account_meta_->latest_scanned_block_id.has_value());
  if (!tree_state.has_value() || !tree_state.value()) {
    error_ = ZCashShieldSyncService::Error{
        ZCashShieldSyncService::ErrorCode::kFailedToReceiveTreeState,
        base::StrCat({"Verification tree state failed, ", tree_state.error()})};
    ScheduleWorkOnTask();
    return;
  }
  auto backend_block_hash = RevertHex(tree_state.value()->hash);
  if (!backend_block_hash) {
    error_ = ZCashShieldSyncService::Error{
        ZCashShieldSyncService::ErrorCode::kFailedToReceiveTreeState,
        "Wrong block hash format"};
    ScheduleWorkOnTask();
    return;
  }
  if (backend_block_hash.value() !=
      account_meta_->latest_scanned_block_hash.value()) {
    verification_state_ = VerificationState::kReorg;
    ScheduleWorkOnTask();
    return;
  }

  verification_state_ = VerificationState::kNoReorg;
  ScheduleWorkOnTask();
}

void ZCashVerifyChainStateTask::GetRewindBlockTreeState() {
  auto block_id = zcash::mojom::BlockID::New(rewind_block_heght_.value(),
                                             std::vector<uint8_t>());
  context_->zcash_rpc->GetTreeState(
      context_->chain_id, std::move(block_id),
      base::BindOnce(&ZCashVerifyChainStateTask::OnGetRewindBlockTreeState,
                     weak_ptr_factory_.GetWeakPtr()));
}

void ZCashVerifyChainStateTask::OnGetRewindBlockTreeState(
    base::expected<zcash::mojom::TreeStatePtr, std::string> tree_state) {
  if (!tree_state.has_value() || !tree_state.value() ||
      rewind_block_heght_.value() != (*tree_state)->height) {
    error_ = ZCashShieldSyncService::Error{
        ZCashShieldSyncService::ErrorCode::kFailedToReceiveTreeState,
        base::StrCat({"Reorg tree state failed, ", tree_state.error()})};
    ScheduleWorkOnTask();
    return;
  }

  rewind_block_tree_state_ = std::move(tree_state.value());
  ScheduleWorkOnTask();
}

void ZCashVerifyChainStateTask::Rewind() {
  auto reverted_hex = RevertHex((*rewind_block_tree_state_)->hash);
  if (!reverted_hex) {
    error_ = ZCashShieldSyncService::Error{
        ZCashShieldSyncService::ErrorCode::kFailedToReceiveTreeState,
        "Wrong block hash format"};
    ScheduleWorkOnTask();
    return;
  }
  // Reorg database so records related to removed blocks are wiped out
  context_->sync_state->AsyncCall(&OrchardSyncState::Rewind)
      .WithArgs(context_->account_id.Clone(), rewind_block_heght_.value(),
                *reverted_hex)
      .Then(base::BindOnce(&ZCashVerifyChainStateTask::OnRewindResult,
                           weak_ptr_factory_.GetWeakPtr()));
}

void ZCashVerifyChainStateTask::OnRewindResult(
    base::expected<OrchardStorage::Result, OrchardStorage::Error> result) {
  if (!result.has_value()) {
    error_ = ZCashShieldSyncService::Error{
        ZCashShieldSyncService::ErrorCode::kFailedToUpdateDatabase,
        result.error().message};
    ScheduleWorkOnTask();
    return;
  }

  rewind_result_ = true;
  ScheduleWorkOnTask();
}

}  // namespace brave_wallet
