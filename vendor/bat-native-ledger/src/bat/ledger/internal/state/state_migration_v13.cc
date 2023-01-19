/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/state/state_migration_v13.h"

#include <vector>

#include "base/ranges/algorithm.h"
#include "bat/ledger/global_constants.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/wallet/wallet_util.h"

namespace ledger::state {

StateMigrationV13::StateMigrationV13(LedgerImpl* ledger) : ledger_(ledger) {
  DCHECK(ledger_);
}

StateMigrationV13::~StateMigrationV13() = default;

bool StateMigrationV13::MigrateExternalWallet(const std::string& wallet_type) {
  DCHECK(ledger_);
  if (!ledger_) {
    BLOG(0, "ledger_ is null!");
    return false;
  }

  if (!wallet::GetWalletIf(ledger_, wallet_type,
                           {mojom::WalletStatus::kConnected})) {
    BLOG(1, "User doesn't have a connected " << wallet_type << " wallet.");
  } else {
    ledger_->ledger_client()->ExternalWalletConnected();
  }

  return true;
}

void StateMigrationV13::Migrate(ledger::LegacyResultCallback callback) {
  callback(base::ranges::all_of(
               std::vector{constant::kWalletBitflyer, constant::kWalletGemini,
                           constant::kWalletUphold},
               [this](const std::string& wallet_type) {
                 return MigrateExternalWallet(wallet_type);
               })
               ? mojom::Result::LEDGER_OK
               : mojom::Result::LEDGER_ERROR);
}

}  // namespace ledger::state
