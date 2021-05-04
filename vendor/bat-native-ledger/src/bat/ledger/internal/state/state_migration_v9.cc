/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/state/state_migration_v9.h"

#include <string>

#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/state/state_keys.h"
#include "bat/ledger/option_keys.h"

namespace ledger {
namespace state {

StateMigrationV9::StateMigrationV9(LedgerImpl* ledger) : ledger_(ledger) {}

StateMigrationV9::~StateMigrationV9() = default;

void StateMigrationV9::Migrate(ledger::ResultCallback callback) {
#if !defined(OS_ANDROID)
  // Set the AC pref to false for all users located in a bitFlyer region.
  if (ledger_->ledger_client()->GetBooleanOption(option::kIsBitflyerRegion))
    ledger_->ledger_client()->SetBooleanState(kAutoContributeEnabled, false);
#endif

  callback(type::Result::LEDGER_OK);
}

}  // namespace state
}  // namespace ledger
