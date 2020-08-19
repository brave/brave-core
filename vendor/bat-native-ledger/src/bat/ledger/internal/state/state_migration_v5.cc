/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "bat/ledger/internal/state/state_migration_v5.h"

#include <map>
#include <string>
#include <utility>

#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/state/state_keys.h"
#include "bat/ledger/internal/state/state_migration_v4.h"

namespace braveledger_state {

StateMigrationV5::StateMigrationV5(bat_ledger::LedgerImpl* ledger) :
    ledger_(ledger) {
  DCHECK(ledger_);
}

StateMigrationV5::~StateMigrationV5() = default;

void StateMigrationV5::Migrate(ledger::ResultCallback callback) {
  const auto seed = ledger_->ledger_client()->GetStringState(
      ledger::kStateRecoverySeed);
  if (seed.empty()) {
    callback(ledger::Result::LEDGER_OK);
    return;
  }

  std::map<std::string, std::string> events;

  // Auto contribute
  auto enabled = ledger_->ledger_client()->GetBooleanState(
      ledger::kStateAutoContributeEnabled);
  events.insert(std::make_pair(
      ledger::kStateAutoContributeEnabled,
      std::to_string(enabled)));

  // Seed
  if (seed.size() > 1) {
    const std::string event_string = seed.substr(0, 2);
    events.insert(std::make_pair(ledger::kStateRecoverySeed, event_string));
  }

  // Payment id
  events.insert(std::make_pair(
      ledger::kStatePaymentId,
      ledger_->ledger_client()->GetStringState(ledger::kStatePaymentId)));

  // Enabled
  enabled = ledger_->ledger_client()->GetBooleanState(ledger::kStateEnabled);
  events.insert(std::make_pair(ledger::kStateEnabled, std::to_string(enabled)));

  // Next reconcile
  const auto reconcile_stamp = ledger_->ledger_client()->GetUint64State(
      ledger::kStateNextReconcileStamp);
  events.insert(std::make_pair(
      ledger::kStateNextReconcileStamp,
      std::to_string(reconcile_stamp)));

  // Creation stamp
  const auto creation_stamp =
      ledger_->ledger_client()->GetUint64State(ledger::kStateCreationStamp);
  events.insert(std::make_pair(
      ledger::kStateCreationStamp,
      std::to_string(creation_stamp)));

  ledger_->database()->SaveEventLogs(events, callback);
}

}  // namespace braveledger_state
