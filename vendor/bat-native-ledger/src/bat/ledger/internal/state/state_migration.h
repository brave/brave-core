/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_BAT_STATE_STATE_MIGRATION_H_
#define BRAVELEDGER_BAT_STATE_STATE_MIGRATION_H_

#include <map>
#include <memory>
#include <string>

#include "bat/ledger/ledger.h"

namespace bat_ledger {
class LedgerImpl;
}

namespace braveledger_state {

class StateMigration {
 public:
  explicit StateMigration(bat_ledger::LedgerImpl* ledger);
  ~StateMigration();

  bool Migrate();

 private:
  bool Migrate(int version);

  bool MigrateToVersion1();

  bat_ledger::LedgerImpl* ledger_;  // NOT OWNED
};

}  // namespace braveledger_state

#endif  // BRAVELEDGER_BAT_STATE_STATE_MIGRATION_H_
