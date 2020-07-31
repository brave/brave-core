/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/recovery/recovery.h"
#include "bat/ledger/internal/recovery/recovery_empty_balance.h"

namespace braveledger_recovery {

void Check(bat_ledger::LedgerImpl* ledger) {
  if (!ledger->state()->GetEmptyBalanceChecked()) {
    BLOG(1, "Running empty balance check...")
    EmptyBalance::Check(ledger);
  }
}

}  // namespace braveledger_recovery
