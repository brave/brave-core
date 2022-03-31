/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "bat/ledger/internal/state/state_migration_v7.h"

#include <string>

#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/state/state_keys.h"

namespace ledger {
namespace state {

StateMigrationV7::StateMigrationV7(LedgerImpl* ledger) :
    ledger_(ledger) {
}

StateMigrationV7::~StateMigrationV7() = default;

void StateMigrationV7::Migrate(ledger::ResultCallback callback) {
  const std::string brave =
      ledger_->ledger_client()->GetStringState(kWalletBrave);

  if (!ledger_->state()->SetEncryptedString(kWalletBrave, brave)) {
    callback(type::Result::LEDGER_ERROR);
    return;
  }

  const std::string uphold =
      ledger_->ledger_client()->GetStringState(kWalletUphold);

  if (!ledger_->state()->SetEncryptedString(kWalletUphold, uphold)) {
    callback(type::Result::LEDGER_ERROR);
    return;
  }

  callback(type::Result::LEDGER_OK);
}

}  // namespace state
}  // namespace ledger
