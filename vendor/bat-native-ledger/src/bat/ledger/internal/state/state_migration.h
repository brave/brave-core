/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_BAT_STATE_STATE_MIGRATION_H_
#define BRAVELEDGER_BAT_STATE_STATE_MIGRATION_H_

#include <memory>
#include <string>

#include "bat/ledger/internal/state/state_migration_v1.h"
#include "bat/ledger/internal/state/state_migration_v2.h"
#include "bat/ledger/internal/state/state_migration_v3.h"
#include "bat/ledger/internal/state/state_migration_v4.h"
#include "bat/ledger/internal/state/state_migration_v5.h"
#include "bat/ledger/internal/state/state_migration_v6.h"
#include "bat/ledger/ledger.h"

namespace ledger {
class LedgerImpl;

namespace state {

class StateMigration {
 public:
  explicit StateMigration(LedgerImpl* ledger);
  ~StateMigration();

  void Start(ledger::ResultCallback callback);

  void Migrate(ledger::ResultCallback callback);

 private:
  void OnMigration(
      type::Result result,
      const int version,
      ledger::ResultCallback callback);

  std::unique_ptr<StateMigrationV1> v1_;
  std::unique_ptr<StateMigrationV2> v2_;
  std::unique_ptr<StateMigrationV3> v3_;
  std::unique_ptr<StateMigrationV4> v4_;
  std::unique_ptr<StateMigrationV5> v5_;
  std::unique_ptr<StateMigrationV6> v6_;
  LedgerImpl* ledger_;  // NOT OWNED
};

}  // namespace state
}  // namespace ledger

#endif  // BRAVELEDGER_BAT_STATE_STATE_MIGRATION_H_
