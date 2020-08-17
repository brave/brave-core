/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_BAT_STATE_STATE_MIGRATION_V3_H_
#define BRAVELEDGER_BAT_STATE_STATE_MIGRATION_V3_H_

#include "bat/ledger/ledger.h"

namespace braveledger_state {

class StateMigrationV3 {
 public:
  StateMigrationV3();
  ~StateMigrationV3();

  void Migrate(ledger::ResultCallback callback);
};

}  // namespace braveledger_state

#endif  // BRAVELEDGER_BAT_STATE_STATE_MIGRATION_V3_H_
