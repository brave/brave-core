/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_BAT_STATE_STATE_MIGRATION_V2_H_
#define BRAVELEDGER_BAT_STATE_STATE_MIGRATION_V2_H_

#include <memory>
#include <string>

#include "bat/ledger/internal/legacy/bat_state.h"
#include "bat/ledger/ledger.h"

namespace ledger {
class LedgerImpl;

namespace state {

class StateMigrationV2 {
 public:
  explicit StateMigrationV2(LedgerImpl* ledger);
  ~StateMigrationV2();

  void Migrate(ledger::ResultCallback callback);

 private:
  void OnLoadState(
      const type::Result result,
      ledger::ResultCallback callback);

  std::unique_ptr<braveledger_bat_state::LegacyBatState> legacy_state_;
  LedgerImpl* ledger_;  // NOT OWNED
};

}  // namespace state
}  // namespace ledger

#endif  // BRAVELEDGER_BAT_STATE_STATE_MIGRATION_V2_H_
