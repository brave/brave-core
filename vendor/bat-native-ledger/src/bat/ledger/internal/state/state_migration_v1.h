/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_BAT_STATE_STATE_MIGRATION_V1_H_
#define BRAVELEDGER_BAT_STATE_STATE_MIGRATION_V1_H_

#include <memory>
#include <string>

#include "bat/ledger/internal/legacy/publisher_state.h"
#include "bat/ledger/ledger.h"

namespace bat_ledger {
class LedgerImpl;
}

namespace braveledger_state {

class StateMigrationV1 {
 public:
  explicit StateMigrationV1(bat_ledger::LedgerImpl* ledger);
  ~StateMigrationV1();

  void Migrate(ledger::ResultCallback callback);

 private:
  void OnLoadState(
      const ledger::Result result,
      ledger::ResultCallback callback);

  void BalanceReportsSaved(
      const ledger::Result result,
      ledger::ResultCallback callback);

  void SaveProcessedPublishers(ledger::ResultCallback callback);

  void ProcessedPublisherSaved(
      const ledger::Result result,
      ledger::ResultCallback callback);

  std::unique_ptr<braveledger_publisher::LegacyPublisherState>
  legacy_publisher_;
  bat_ledger::LedgerImpl* ledger_;  // NOT OWNED
};

}  // namespace braveledger_state

#endif  // BRAVELEDGER_BAT_STATE_STATE_MIGRATION_V1_H_
