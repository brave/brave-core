// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_wallet/browser/zcash/zcash_discover_next_unused_zcash_address_task.h"

#include <string>
#include <utility>

#include "brave/components/brave_wallet/common/common_utils.h"
#include "brave/components/brave_wallet/common/zcash_utils.h"
#include "components/grit/brave_components_strings.h"
#include "ui/base/l10n/l10n_util.h"

namespace brave_wallet {

ZCashDiscoverNextUnusedZCashAddressTask::
    ZCashDiscoverNextUnusedZCashAddressTask(
        base::PassKey<ZCashWalletService> pass_key,
        base::WeakPtr<ZCashWalletService> zcash_wallet_service,
        const mojom::AccountIdPtr& account_id,
        const mojom::ZCashAddressPtr& start_address,
        ZCashWalletService::DiscoverNextUnusedAddressCallback callback)
    : zcash_wallet_service_(std::move(zcash_wallet_service)),
      account_id_(account_id.Clone()),
      start_address_(start_address.Clone()),
      callback_(std::move(callback)) {}

ZCashDiscoverNextUnusedZCashAddressTask::
    ~ZCashDiscoverNextUnusedZCashAddressTask() = default;

void ZCashDiscoverNextUnusedZCashAddressTask::Start() {
  DCHECK(!started_);
  started_ = true;
  ScheduleWorkOnTask();
}

void ZCashDiscoverNextUnusedZCashAddressTask::ScheduleWorkOnTask() {
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE,
      base::BindOnce(&ZCashDiscoverNextUnusedZCashAddressTask::WorkOnTask,
                     this));
}

mojom::ZCashAddressPtr ZCashDiscoverNextUnusedZCashAddressTask::GetNextAddress(
    const mojom::ZCashAddressPtr& address) {
  auto next_key_id = current_address_->key_id.Clone();
  next_key_id->index++;
  return zcash_wallet_service_->keyring_service().GetZCashAddress(*account_id_,
                                                                  *next_key_id);
}

void ZCashDiscoverNextUnusedZCashAddressTask::WorkOnTask() {
  if (!callback_) {
    return;
  }

  if (!zcash_wallet_service_) {
    std::move(callback_).Run(
        base::unexpected(l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR)));
    return;
  }

  if (error_) {
    std::move(callback_).Run(base::unexpected(std::move(*error_)));
    return;
  }

  if (result_) {
    std::move(callback_).Run(base::ok(std::move(result_)));
    return;
  }

  if (!block_end_) {
    zcash_wallet_service_->zcash_rpc().GetLatestBlock(
        GetNetworkForZCashKeyring(account_id_->keyring_id),
        base::BindOnce(&ZCashDiscoverNextUnusedZCashAddressTask::OnGetLastBlock,
                       this));
    return;
  }

  if (start_address_) {
    current_address_ = std::move(start_address_);
  } else {
    current_address_ = GetNextAddress(current_address_);
  }

  if (!current_address_) {
    error_ = l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR);
    ScheduleWorkOnTask();
    return;
  }

  zcash_wallet_service_->zcash_rpc().IsKnownAddress(
      GetNetworkForZCashKeyring(account_id_->keyring_id),
      current_address_->address_string, 1, block_end_.value(),
      base::BindOnce(
          &ZCashDiscoverNextUnusedZCashAddressTask::OnGetIsKnownAddress, this));
}

void ZCashDiscoverNextUnusedZCashAddressTask::OnGetLastBlock(
    base::expected<zcash::mojom::BlockIDPtr, std::string> result) {
  if (!result.has_value() || !result.value()) {
    error_ = result.error();
    WorkOnTask();
    return;
  }

  block_end_ = (*result)->height;
  WorkOnTask();
}

void ZCashDiscoverNextUnusedZCashAddressTask::OnGetIsKnownAddress(
    base::expected<bool, std::string> result) {
  if (!result.has_value()) {
    error_ = result.error();
    WorkOnTask();
    return;
  }

  if (!result.value()) {
    result_ = current_address_->Clone();
  }

  WorkOnTask();
}

}  // namespace brave_wallet
