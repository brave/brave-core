/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/wallet/wallet_create.h"

#include <utility>
#include <vector>

#include "bat/ledger/internal/common/security_util.h"
#include "bat/ledger/internal/common/time_util.h"
#include "bat/ledger/internal/constants.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/logging/event_log_keys.h"

using std::placeholders::_1;
using std::placeholders::_2;

namespace ledger {
namespace wallet {

WalletCreate::WalletCreate(LedgerImpl* ledger) :
    ledger_(ledger),
    promotion_server_(std::make_unique<endpoint::PromotionServer>(ledger)) {
  DCHECK(ledger_);
}

WalletCreate::~WalletCreate() = default;

void WalletCreate::Start(ledger::ResultCallback callback) {
  bool corrupted = false;
  auto wallet = ledger_->wallet()->GetWallet(&corrupted);

  if (corrupted) {
    BLOG(0, "Rewards wallet data is corrupted - generating a new wallet");
    ledger_->database()->SaveEventLog(log::kWalletCorrupted, "");
  }

  if (!wallet) {
    wallet = type::BraveWallet::New();
    wallet->recovery_seed = util::Security::GenerateSeed();
    if (!ledger_->wallet()->SetWallet(wallet->Clone())) {
      std::move(callback).Run(type::Result::LEDGER_ERROR);
      return;
    }
  }

  if (!wallet->payment_id.empty()) {
    BLOG(1, "Wallet already exists");
    std::move(callback).Run(type::Result::WALLET_CREATED);
    return;
  }

  auto url_callback = base::BindOnce(
      &WalletCreate::OnCreate, base::Unretained(this), std::move(callback));

  promotion_server_->post_wallet_brave()->Request(std::move(url_callback));
}

void WalletCreate::OnCreate(ledger::ResultCallback callback,
                            type::Result result,
                            const std::string& payment_id) {
  if (result != type::Result::LEDGER_OK) {
    std::move(callback).Run(result);
    return;
  }

  auto wallet = ledger_->wallet()->GetWallet();
  wallet->payment_id = payment_id;
  const bool success = ledger_->wallet()->SetWallet(std::move(wallet));

  if (!success) {
    std::move(callback).Run(type::Result::LEDGER_ERROR);
    return;
  }

  ledger_->state()->ResetReconcileStamp();
  if (!ledger::is_testing) {
    ledger_->state()->SetEmptyBalanceChecked(true);
    ledger_->state()->SetPromotionCorruptedMigrated(true);
  }
  ledger_->state()->SetCreationStamp(util::GetCurrentTimeStamp());
  std::move(callback).Run(type::Result::WALLET_CREATED);
}

}  // namespace wallet
}  // namespace ledger
