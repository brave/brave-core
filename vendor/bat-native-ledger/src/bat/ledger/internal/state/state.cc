/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/state/state.h"
#include "bat/ledger/internal/state/state_migration.h"

namespace braveledger_state {

State::State(bat_ledger::LedgerImpl* ledger) :
    migration_(std::make_unique<StateMigration>(ledger)) {
}

State::~State() = default;

void State::Initialize() {
  migration_->Migrate();
}

}  // namespace braveledger_state
