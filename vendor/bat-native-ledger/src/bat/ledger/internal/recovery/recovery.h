/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_RECOVERY_RECOVERY_H_
#define BRAVELEDGER_RECOVERY_RECOVERY_H_

#include "bat/ledger/internal/ledger_impl.h"

namespace braveledger_recovery {

void Check(bat_ledger::LedgerImpl* ledger);

}  // namespace braveledger_recovery

#endif  // BRAVELEDGER_RECOVERY_RECOVERY_H_
