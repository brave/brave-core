// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_wallet/browser/zcash/zcash_resolve_transaction_status_task.h"

#include <optional>
#include <utility>
#include <variant>

#include "base/check.h"
#include "base/time/time.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_tx_meta.h"

namespace brave_wallet {

namespace {
constexpr size_t kTransactionCompleteConfirmations = 3u;
constexpr size_t kTransactionExpiryHours = 2u;

using ResolveTransactionStatusResult =
    ZCashWalletService::ResolveTransactionStatusResult;

std::optional<ResolveTransactionStatusResult> ResolveExpiredStatus(
    uint32_t chain_tip,
    const ZCashTxMeta& tx_meta) {
  uint32_t expiry_height = tx_meta.tx()->expiry_height();
  if (expiry_height != 0 && chain_tip > expiry_height) {
    return ResolveTransactionStatusResult::kExpired;
  }

  base::Time current_time = base::Time::Now();
  if (expiry_height == 0 && !tx_meta.submitted_time().is_null() &&
      current_time >= tx_meta.submitted_time() &&
      current_time - tx_meta.submitted_time() >
          base::Hours(kTransactionExpiryHours)) {
    return ResolveTransactionStatusResult::kExpired;
  }

  return std::nullopt;
}

}  // namespace

ZCashResolveTransactionStatusTask::ZCashResolveTransactionStatusTask(
    std::variant<base::PassKey<ZCashWalletService>,
                 base::PassKey<class ZCashResolveTransactionStatusTaskTest>>
        pass_key,
    ZCashActionContext context,
    ZCashWalletService& zcash_wallet_service,
    std::unique_ptr<ZCashTxMeta> tx_meta)
    : context_(std::move(context)),
      zcash_wallet_service_(zcash_wallet_service),
      tx_meta_(std::move(tx_meta)) {}

ZCashResolveTransactionStatusTask::~ZCashResolveTransactionStatusTask() =
    default;

void ZCashResolveTransactionStatusTask::Start(
    ZCashResolveTransactionStatusTaskCallback callback) {
  DCHECK(!callback_);
  callback_ = std::move(callback);
  ScheduleWorkOnTask();
}

void ZCashResolveTransactionStatusTask::ScheduleWorkOnTask() {
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, base::BindOnce(&ZCashResolveTransactionStatusTask::WorkOnTask,
                                weak_ptr_factory_.GetWeakPtr()));
}

void ZCashResolveTransactionStatusTask::WorkOnTask() {
  if (error_) {
    std::move(callback_).Run(base::unexpected(error_.value()));
    return;
  }

  if (!tx_meta_ || !tx_meta_->tx()) {
    std::move(callback_).Run(ResolveTransactionStatusResult::kExpired);
    return;
  }

  if (!chain_tip_) {
    GetChainTip();
    return;
  }

  if (!transaction_) {
    GetTransaction();
    return;
  }

  CHECK(tx_meta_);
  CHECK(tx_meta_->tx());
  CHECK(chain_tip_.value());
  CHECK(transaction_.value());

  uint32_t chain_tip = chain_tip_.value()->height;
  uint32_t tx_height = transaction_.value()->height;

  // https://github.com/zcash/lightwalletd/blob/339b6d37e839d27bbd167ed02627ab7ab4d7051f/walletrpc/service.proto#L53
  if (tx_height == 0 || tx_height == 0xFFFFFFFF) {
    auto result = ResolveExpiredStatus(chain_tip, *tx_meta_);
    std::move(callback_).Run(
        base::ok(result.value_or(ResolveTransactionStatusResult::kInProgress)));
    return;
  }

  // Could be the case of chain reorg event.
  if (chain_tip < tx_height) {
    std::move(callback_).Run(
        base::ok(ResolveTransactionStatusResult::kExpired));
    return;
  }

  bool completed = chain_tip - tx_height > kTransactionCompleteConfirmations;
  const auto result = completed ? ResolveTransactionStatusResult::kCompleted
                                : ResolveTransactionStatusResult::kInProgress;
  std::move(callback_).Run(base::ok(result));
}

void ZCashResolveTransactionStatusTask::GetChainTip() {
  context_.zcash_rpc->GetLatestBlock(
      context_.chain_id,
      base::BindOnce(&ZCashResolveTransactionStatusTask::OnGetChainTipResult,
                     weak_ptr_factory_.GetWeakPtr()));
}

void ZCashResolveTransactionStatusTask::OnGetChainTipResult(
    base::expected<zcash::mojom::BlockIDPtr, std::string> result) {
  if (!result.has_value()) {
    error_ = result.error();
    ScheduleWorkOnTask();
    return;
  }

  if (!result.value()) {
    error_ = "Failed to resolve chain tip";
    ScheduleWorkOnTask();
    return;
  }

  chain_tip_ = std::move(result.value());
  ScheduleWorkOnTask();
}

void ZCashResolveTransactionStatusTask::GetTransaction() {
  CHECK(tx_meta_);

  context_.zcash_rpc->GetTransaction(
      context_.chain_id, tx_meta_->tx_hash(),
      base::BindOnce(&ZCashResolveTransactionStatusTask::OnGetTransactionResult,
                     weak_ptr_factory_.GetWeakPtr()));
}

void ZCashResolveTransactionStatusTask::OnGetTransactionResult(
    base::expected<zcash::mojom::RawTransactionPtr, std::string> result) {
  if (!result.has_value() || !result.value()) {
    CHECK(chain_tip_);
    auto expired_status =
        ResolveExpiredStatus(chain_tip_.value()->height, *tx_meta_);
    if (expired_status) {
      std::move(callback_).Run(base::ok(expired_status.value()));
      return;
    }

    error_ =
        result.has_value() ? "Failed to resolve transaction" : result.error();
    ScheduleWorkOnTask();
    return;
  }

  transaction_ = std::move(result.value());
  ScheduleWorkOnTask();
}

}  // namespace brave_wallet
