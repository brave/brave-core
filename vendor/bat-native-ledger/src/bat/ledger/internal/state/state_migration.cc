/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>

#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/state/state_migration.h"
#include "bat/ledger/internal/state/state_util.h"

using std::placeholders::_1;

namespace {

const int kCurrentVersionNumber = 4;

}  // namespace

namespace braveledger_state {

StateMigration::StateMigration(bat_ledger::LedgerImpl* ledger) :
    v1_(std::make_unique<StateMigrationV1>(ledger)),
    v2_(std::make_unique<StateMigrationV2>(ledger)),
    v3_(std::make_unique<StateMigrationV3>(ledger)),
    v4_(std::make_unique<StateMigrationV4>(ledger)),
    ledger_(ledger) {
  DCHECK(v1_ && v2_ && v3_ && v4_);
}

StateMigration::~StateMigration() = default;

void StateMigration::Migrate(ledger::ResultCallback callback) {
  const int current_version = GetVersion(ledger_);
  const int new_version = current_version + 1;

  if (current_version == kCurrentVersionNumber) {
    callback(ledger::Result::LEDGER_OK);
    return;
  }

  auto migrate_callback = std::bind(&StateMigration::OnMigration,
      this,
      _1,
      new_version,
      callback);

  switch (new_version) {
    case 1: {
      v1_->Migrate(migrate_callback);
      return;
    }
    case 2: {
      v2_->Migrate(migrate_callback);
      return;
    }
    case 3: {
      v3_->Migrate(migrate_callback);
      return;
    }
    case 4: {
      v4_->Migrate(migrate_callback);
      return;
    }
  }

  BLOG(0, "Migration version is not handled " << new_version);
  NOTREACHED();
}

void StateMigration::OnMigration(
    ledger::Result result,
    const int version,
    ledger::ResultCallback callback) {
  if (result != ledger::Result::LEDGER_OK) {
    BLOG(0, "State: Error with migration from " << (version - 1) <<
        " to " << version);
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  BLOG(1, "State: Migrated to version " << version);

  SetVersion(ledger_, version);
  Migrate(callback);
}

}  // namespace braveledger_state
