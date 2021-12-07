/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_UPGRADES_UPGRADE_32_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_UPGRADES_UPGRADE_32_H_

#include "bat/ledger/internal/core/bat_ledger_job.h"
#include "bat/ledger/internal/upgrades/migration_job.h"
#include "bat/ledger/option_keys.h"

namespace ledger {

// Archives and clears additional data associated with BAP in order to prevent
// display of BAP historical information in monthly reports.
class Upgrade32 : public BATLedgerJob<bool> {
 public:
  static constexpr int kVersion = 32;

  static inline const char kSQL[] = R"sql(
    CREATE TABLE balance_report_info_bap AS SELECT * FROM balance_report_info;
    DELETE FROM balance_report_info;
  )sql";

  void Start() {
    bool is_bitflyer_region = context().GetLedgerClient()->GetBooleanOption(
        option::kIsBitflyerRegion);

    const char* sql = is_bitflyer_region ? kSQL : "";
    CompleteWithFuture(context().StartJob<MigrationJob>(kVersion, sql));
  }
};

}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_UPGRADES_UPGRADE_32_H_
