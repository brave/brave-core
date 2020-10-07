/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>

#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/state/state_migration.h"

using std::placeholders::_1;

namespace {

const int kCurrentVersionNumber = 6;

}  // namespace

namespace ledger {
namespace state {

StateMigration::StateMigration(LedgerImpl* ledger) :
    v1_(std::make_unique<StateMigrationV1>(ledger)),
    v2_(std::make_unique<StateMigrationV2>(ledger)),
    v3_(std::make_unique<StateMigrationV3>()),
    v4_(std::make_unique<StateMigrationV4>(ledger)),
    v5_(std::make_unique<StateMigrationV5>(ledger)),
    v6_(std::make_unique<StateMigrationV6>(ledger)),
    ledger_(ledger) {
  DCHECK(v1_ && v2_ && v3_ && v4_ && v5_ && v6_);
}

StateMigration::~StateMigration() = default;

void StateMigration::Start(ledger::ResultCallback callback) {
  const int current_version = ledger_->state()->GetVersion();
  const bool fresh_install = current_version == 0;

  if (fresh_install) {
    BLOG(1, "Fresh install, state version set to " << kCurrentVersionNumber);
    ledger_->state()->SetVersion(kCurrentVersionNumber);
    callback(type::Result::LEDGER_OK);
    return;
  }

  Migrate(callback);
}

void StateMigration::Migrate(ledger::ResultCallback callback) {
  const int current_version = ledger_->state()->GetVersion();
  const int new_version = current_version + 1;

  if (current_version == kCurrentVersionNumber) {
    callback(type::Result::LEDGER_OK);
    return;
  }

  auto migrate_callback = std::bind(&StateMigration::OnMigration,
      this,
      _1,
      new_version,
      callback);

  switch (new_version) {
    case 0: {
      ledger_->state()->SetVersion(new_version);
      Migrate(callback);
      return;
    }
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
    case 5: {
      v5_->Migrate(migrate_callback);
      return;
    }
    case 6: {
      v6_->Migrate(migrate_callback);
      return;
    }
  }

  BLOG(0, "Migration version is not handled " << new_version);
  NOTREACHED();
}

void StateMigration::OnMigration(
    type::Result result,
    const int version,
    ledger::ResultCallback callback) {
  if (result != type::Result::LEDGER_OK) {
    BLOG(0, "State: Error with migration from " << (version - 1) <<
        " to " << version);
    callback(type::Result::LEDGER_ERROR);
    return;
  }

  BLOG(1, "State: Migrated to version " << version);

  ledger_->state()->SetVersion(version);
  Migrate(callback);
}

}  // namespace state
}  // namespace ledger
