/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>
#include <utility>

#include "bat/ledger/internal/database/database.h"
#include "bat/ledger/internal/database/database_initialize.h"
#include "bat/ledger/internal/ledger_impl.h"

namespace braveledger_database {

Database::Database(bat_ledger::LedgerImpl* ledger) :
    ledger_(ledger) {
  initialize_ = std::make_unique<DatabaseInitialize>(ledger_);
}

Database::~Database() = default;

void Database::Initialize(
    const bool execute_create_script,
    ledger::ResultCallback callback) {
  initialize_->Start(
      execute_create_script,
      callback);
}


}  // namespace braveledger_database
