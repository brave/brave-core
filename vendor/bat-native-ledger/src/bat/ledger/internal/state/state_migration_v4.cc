/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "bat/ledger/internal/state/state_migration_v4.h"

#include "bat/ledger/internal/ledger_impl.h"

namespace ledger {
namespace state {

StateMigrationV4::StateMigrationV4(LedgerImpl* ledger) :
    ledger_(ledger) {
  DCHECK(ledger_);
}

StateMigrationV4::~StateMigrationV4() = default;

void StateMigrationV4::Migrate(ledger::LegacyResultCallback callback) {
  ledger_->ledger_client()->DeleteLog(callback);
}

}  // namespace state
}  // namespace ledger
