/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_BAT_STATE_STATE_MIGRATION_V5_H_
#define BRAVELEDGER_BAT_STATE_STATE_MIGRATION_V5_H_

#include "bat/ledger/ledger.h"

namespace ledger {
class LedgerImpl;

namespace state {

class StateMigrationV5 {
 public:
  explicit StateMigrationV5(LedgerImpl* ledger);
  ~StateMigrationV5();

  void Migrate(ledger::ResultCallback callback);

 private:
  LedgerImpl* ledger_;  // NOT OWNED
};

}  // namespace state
}  // namespace ledger

#endif  // BRAVELEDGER_BAT_STATE_STATE_MIGRATION_V5_H_
