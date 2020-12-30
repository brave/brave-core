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

namespace ledger {
namespace state {

StateMigrationV5::StateMigrationV5(LedgerImpl* ledger) :
    ledger_(ledger) {
  DCHECK(ledger_);
}

StateMigrationV5::~StateMigrationV5() = default;

void StateMigrationV5::Migrate(ledger::ResultCallback callback) {
  const auto seed = ledger_->ledger_client()->GetStringState(
      kRecoverySeed);
  if (seed.empty()) {
    callback(type::Result::LEDGER_OK);
    return;
  }

  std::map<std::string, std::string> events;

  // Auto contribute
  auto enabled = ledger_->ledger_client()->GetBooleanState(
      kAutoContributeEnabled);
  events.insert(std::make_pair(
      kAutoContributeEnabled,
      std::to_string(enabled)));

  // Seed
  if (seed.size() > 1) {
    const std::string event_string = seed.substr(0, 2);
    events.insert(std::make_pair(kRecoverySeed, event_string));
  }

  // Payment id
  events.insert(std::make_pair(
      kPaymentId,
      ledger_->ledger_client()->GetStringState(kPaymentId)));

  // Enabled
  enabled = ledger_->ledger_client()->GetBooleanState("enabled");
  events.insert(
      std::make_pair("enabled", std::to_string(enabled)));

  // Next reconcile
  const auto reconcile_stamp = ledger_->ledger_client()->GetUint64State(
      kNextReconcileStamp);
  events.insert(std::make_pair(
      kNextReconcileStamp,
      std::to_string(reconcile_stamp)));

  // Creation stamp
  const auto creation_stamp =
      ledger_->ledger_client()->GetUint64State(kCreationStamp);
  events.insert(std::make_pair(
      kCreationStamp,
      std::to_string(creation_stamp)));

  ledger_->database()->SaveEventLogs(events, callback);
}

}  // namespace state
}  // namespace ledger
