/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/files/file_path.h"
#include "bat/ledger/internal/database/database.h"
#include "bat/ledger/internal/ledger_impl.h"

namespace braveledger_database {

Database::Database(bat_ledger::LedgerImpl* ledger) :
    ledger_(ledger) {
}

Database::~Database() {
}

void Database::Initialize() {
  const std::string db_path = ledger_->GetDatabasePath();
  const auto file = base::FilePath(db_path);
}

}  // namespace braveledger_database
