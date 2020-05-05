/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/state/state_migration.h"
#include "bat/ledger/internal/state/state_util.h"

namespace {

const int kCurrentVersionNumber = 1;

}  // namespace

namespace braveledger_state {

StateMigration::StateMigration(bat_ledger::LedgerImpl* ledger) :
    ledger_(ledger) {
}

StateMigration::~StateMigration() = default;


bool StateMigration::Migrate() {
  const int current_version = GetVersion(ledger_);

  for (auto i = current_version + 1; i <= kCurrentVersionNumber; i++) {
    if (!Migrate(i)) {
      BLOG(ledger_, ledger::LogLevel::LOG_ERROR) <<
        "State: Error with migration from " << (i - 1) << " to " << i;
      return false;
    }

    BLOG(ledger_, ledger::LogLevel::LOG_INFO) <<
      "State: Migrated to version " << i;

    SetVersion(ledger_, i);
  }

  return true;
}

bool StateMigration::Migrate(int version) {
  switch (version) {
    case 1: {
      return MigrateToVersion1();
    }
  }

  NOTREACHED();
  return false;
}

bool StateMigration::MigrateToVersion1() {
  return true;
}

}  // namespace braveledger_state
