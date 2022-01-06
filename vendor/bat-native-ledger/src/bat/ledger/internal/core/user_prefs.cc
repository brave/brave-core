/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/core/user_prefs.h"

#include <string>

#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/state/state_keys.h"

namespace ledger {

bool UserPrefs::ac_enabled() {
  return context().GetLedgerClient()->GetBooleanState(
      state::kAutoContributeEnabled);
}

int UserPrefs::ac_minimum_visits() {
  return context().GetLedgerClient()->GetIntegerState(state::kMinVisits);
}

base::TimeDelta UserPrefs::ac_minimum_duration() {
  return base::Seconds(
      context().GetLedgerClient()->GetIntegerState(state::kMinVisitTime));
}

double UserPrefs::ac_amount() {
  return context().GetLedgerClient()->GetDoubleState(
      state::kAutoContributeAmount);
}

}  // namespace ledger
