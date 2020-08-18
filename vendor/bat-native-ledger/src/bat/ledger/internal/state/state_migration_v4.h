/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_BAT_STATE_STATE_MIGRATION_V4_H_
#define BRAVELEDGER_BAT_STATE_STATE_MIGRATION_V4_H_

#include "bat/ledger/ledger.h"

namespace bat_ledger {
class LedgerImpl;
}

namespace braveledger_state {

class StateMigrationV4 {
 public:
  explicit StateMigrationV4(bat_ledger::LedgerImpl* ledger);
  ~StateMigrationV4();

  void Migrate(ledger::ResultCallback callback);

 private:
  bat_ledger::LedgerImpl* ledger_;  // NOT OWNED
};

}  // namespace braveledger_state

#endif  // BRAVELEDGER_BAT_STATE_STATE_MIGRATION_V4_H_
