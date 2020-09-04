/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/recovery/recovery.h"

namespace ledger {
namespace recovery {

Recovery::Recovery(LedgerImpl* ledger):
    ledger_(ledger),
    empty_balance_(std::make_unique<EmptyBalance>(ledger)) {
  DCHECK(ledger_);
}

Recovery::~Recovery() = default;

void Recovery::Check() {
  if (!ledger_->state()->GetEmptyBalanceChecked()) {
    BLOG(1, "Running empty balance check...")
    empty_balance_->Check();
  }
}

}  // namespace recovery
}  // namespace ledger
