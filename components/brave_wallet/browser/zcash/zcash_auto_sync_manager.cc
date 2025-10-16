/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/zcash/zcash_auto_sync_manager.h"

#include <utility>

#include "base/check.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_wallet_service.h"

namespace brave_wallet {

ZCashAutoSyncManager::ZCashAutoSyncManager(
    ZCashWalletService& zcash_wallet_service,
    ZCashActionContext action_context)
    : zcash_wallet_service_(zcash_wallet_service),
      zcash_action_context_(std::move(action_context)) {}

ZCashAutoSyncManager::~ZCashAutoSyncManager() {}

bool ZCashAutoSyncManager::IsStarted() {
  return started_;
}

void ZCashAutoSyncManager::Start() {
  CHECK(!started_);
  started_ = true;
  zcash_wallet_service_->GetChainTipStatus(
      zcash_action_context_.account_id.Clone(),
      base::BindOnce(&ZCashAutoSyncManager::OnGetChainTipStatus,
                     weak_ptr_factory_.GetWeakPtr()));
  timer_.Start(FROM_HERE, kZCashAutoSyncRefreshInterval,
               base::BindRepeating(&ZCashAutoSyncManager::OnTimerHit,
                                   weak_ptr_factory_.GetWeakPtr()));
}

void ZCashAutoSyncManager::OnTimerHit() {
  zcash_wallet_service_->GetChainTipStatus(
      zcash_action_context_.account_id.Clone(),
      base::BindOnce(&ZCashAutoSyncManager::OnGetChainTipStatus,
                     weak_ptr_factory_.GetWeakPtr()));
}

void ZCashAutoSyncManager::OnGetChainTipStatus(
    mojom::ZCashChainTipStatusPtr status,
    const std::optional<std::string>& error) {
  if (!status) {
    return;
  }

  if (status->chain_tip < status->latest_scanned_block) {
    return;
  }

  if (status->chain_tip - status->latest_scanned_block <
      kZCashAutoSyncMaxBlocksDelta) {
    zcash_wallet_service_->StartShieldSync(
        zcash_action_context_.account_id.Clone(), 0, base::DoNothing());
  }
}

}  // namespace brave_wallet
