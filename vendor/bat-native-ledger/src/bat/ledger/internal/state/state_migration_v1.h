/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_STATE_STATE_MIGRATION_V1_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_STATE_STATE_MIGRATION_V1_H_

#include <memory>
#include <string>
#include <vector>

#include "bat/ledger/internal/legacy/publisher_state.h"
#include "bat/ledger/ledger.h"

namespace ledger {
class LedgerImpl;

namespace state {

class StateMigrationV1 {
 public:
  explicit StateMigrationV1(LedgerImpl* ledger);
  ~StateMigrationV1();

  void Migrate(ledger::LegacyResultCallback callback);

  bool legacy_data_migrated() const { return legacy_data_migrated_; }

 private:
  void OnLoadState(mojom::Result result, ledger::LegacyResultCallback callback);

  void BalanceReportsSaved(mojom::Result result,
                           ledger::LegacyResultCallback callback);

  void SaveProcessedPublishers(ledger::LegacyResultCallback callback);

  void ProcessedPublisherSaved(mojom::Result result,
                               ledger::LegacyResultCallback callback);

  std::unique_ptr<publisher::LegacyPublisherState> legacy_publisher_;
  LedgerImpl* ledger_;  // NOT OWNED
  bool legacy_data_migrated_ = false;
};

}  // namespace state
}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_STATE_STATE_MIGRATION_V1_H_
