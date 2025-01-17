// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_wallet/browser/zcash/zcash_get_zcash_chain_tip_status_task.h"

#include <utility>

namespace brave_wallet {

ZCashGetZCashChainTipStatusTask::ZCashGetZCashChainTipStatusTask(
    base::PassKey<ZCashWalletService> pass_key,
    ZCashWalletService& zcash_wallet_service,
    ZCashActionContext context,
    ZCashGetZCashChainTipStatusTaskCallback callback)
    : zcash_wallet_service_(zcash_wallet_service),
      context_(std::move(context)),
      callback_(std::move(callback)) {}

ZCashGetZCashChainTipStatusTask::~ZCashGetZCashChainTipStatusTask() = default;

void ZCashGetZCashChainTipStatusTask::Start() {
  CHECK(!started_);
  started_ = true;
  ScheduleWorkOnTask();
}

void ZCashGetZCashChainTipStatusTask::WorkOnTask() {
  if (error_) {
    std::move(callback_).Run(base::unexpected(error_.value()));
    zcash_wallet_service_->GetZCashChainTipStatusTaskDone(this);
    return;
  }

  if (!account_meta_) {
    GetAccountMeta();
    return;
  }

  if (!chain_tip_height_) {
    GetChainTipHeight();
    return;
  }

  uint32_t latest_scanned_block =
      account_meta_->latest_scanned_block_id
          ? account_meta_->latest_scanned_block_id.value()
          : account_meta_->account_birthday;

  std::move(callback_).Run(base::ok(mojom::ZCashChainTipStatus::New(
      latest_scanned_block, chain_tip_height_.value())));

  zcash_wallet_service_->GetZCashChainTipStatusTaskDone(this);
}

void ZCashGetZCashChainTipStatusTask::GetAccountMeta() {
  context_.sync_state->AsyncCall(&OrchardSyncState::GetAccountMeta)
      .WithArgs(context_.account_id.Clone())
      .Then(base::BindOnce(&ZCashGetZCashChainTipStatusTask::OnGetAccountMeta,
                           weak_ptr_factory_.GetWeakPtr()));
}

void ZCashGetZCashChainTipStatusTask::GetChainTipHeight() {
  context_.zcash_rpc->GetLatestBlock(
      context_.chain_id,
      base::BindOnce(
          &ZCashGetZCashChainTipStatusTask::OnGetChainTipHeightResult,
          weak_ptr_factory_.GetWeakPtr()));
}

void ZCashGetZCashChainTipStatusTask::OnGetChainTipHeightResult(
    base::expected<zcash::mojom::BlockIDPtr, std::string> result) {
  if (!result.has_value()) {
    error_ = "Failed to resolve chain tip";
    ScheduleWorkOnTask();
    return;
  }

  chain_tip_height_ = (*result)->height;
  ScheduleWorkOnTask();
}

void ZCashGetZCashChainTipStatusTask::OnGetAccountMeta(
    base::expected<std::optional<OrchardStorage::AccountMeta>,
                   OrchardStorage::Error> result) {
  if (!result.has_value() || !result.value()) {
    error_ = "Failed to resolve account's meta";
    ScheduleWorkOnTask();
    return;
  }

  account_meta_ = **result;
  ScheduleWorkOnTask();
}

void ZCashGetZCashChainTipStatusTask::ScheduleWorkOnTask() {
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, base::BindOnce(&ZCashGetZCashChainTipStatusTask::WorkOnTask,
                                weak_ptr_factory_.GetWeakPtr()));
}

}  // namespace brave_wallet
