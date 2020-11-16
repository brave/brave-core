/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "bat/ledger/internal/state/state_migration_v8.h"

#include <string>

#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/state/state_keys.h"

namespace ledger {
namespace state {

StateMigrationV8::StateMigrationV8(LedgerImpl* ledger) :
    ledger_(ledger) {
}

StateMigrationV8::~StateMigrationV8() = default;

void StateMigrationV8::Migrate(ledger::ResultCallback callback) {
  const bool enabled = ledger_->ledger_client()->GetBooleanState("enabled");

  if (!enabled) {
    ledger_->ledger_client()->SetBooleanState(kAutoContributeEnabled, false);
  }

  callback(type::Result::LEDGER_OK);
}

}  // namespace state
}  // namespace ledger
