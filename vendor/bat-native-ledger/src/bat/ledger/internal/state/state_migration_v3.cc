/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/state/state_migration_v3.h"

namespace braveledger_state {

StateMigrationV3::StateMigrationV3() = default;

StateMigrationV3::~StateMigrationV3() = default;

void StateMigrationV3::Migrate(ledger::ResultCallback callback) {
  // In this migration we migrated anon address to uphold wallet in preferences
  // because anon address was removed we can also remove this step
  // Ref: https://github.com/brave/brave-browser/issues/11150
  callback(ledger::Result::LEDGER_OK);
}

}  // namespace braveledger_state
