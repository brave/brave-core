/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>

#include "bat/ledger/global_constants.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/state/state_keys.h"
#include "bat/ledger/internal/state/state_migration_v3.h"
#include "bat/ledger/internal/uphold/uphold_util.h"

namespace braveledger_state {

StateMigrationV3::StateMigrationV3(bat_ledger::LedgerImpl* ledger) :
    ledger_(ledger) {
}

StateMigrationV3::~StateMigrationV3() = default;

void StateMigrationV3::Migrate(ledger::ResultCallback callback) {
    const std::string anon_address = ledger_->ledger_client()->GetStringState(
        ledger::kStateUpholdAnonAddress);

  if (!anon_address.empty()) {
    auto wallets = ledger_->ledger_client()->GetExternalWallets();
    auto wallet = braveledger_uphold::GetWallet(std::move(wallets));
    if (!wallet) {
      BLOG(0, "Wallet is null, but we can't recover");
      callback(ledger::Result::LEDGER_OK);
      return;
    }

    wallet->anon_address = anon_address;
    ledger_->ledger_client()->SaveExternalWallet(
        ledger::kWalletUphold,
        std::move(wallet));
    ledger_->ledger_client()->ClearState(ledger::kStateUpholdAnonAddress);
  }

  callback(ledger::Result::LEDGER_OK);
}

}  // namespace braveledger_state
