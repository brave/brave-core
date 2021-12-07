/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_UPGRADES_UPGRADE_30_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_UPGRADES_UPGRADE_30_H_

#include "bat/ledger/internal/core/bat_ledger_job.h"
#include "bat/ledger/internal/upgrades/migration_job.h"
#include "bat/ledger/option_keys.h"

namespace ledger {

// Archives and clears the user's unblinded tokens table. It is intended only
// for users transitioning from "BAP" (a Japan-specific representation of BAT)
// to BAT with bitFlyer support.
class Upgrade30 : public BATLedgerJob<bool> {
 public:
  static constexpr int kVersion = 30;

  static inline const char kSQL[] = R"sql(
    CREATE TABLE unblinded_tokens_bap AS SELECT * FROM unblinded_tokens;
    DELETE FROM unblinded_tokens;
  )sql";

  void Start() {
    bool is_bitflyer_region = context().GetLedgerClient()->GetBooleanOption(
        option::kIsBitflyerRegion);

    const char* sql = is_bitflyer_region ? kSQL : "";
    CompleteWithFuture(context().StartJob<MigrationJob>(kVersion, sql));
  }
};

}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_UPGRADES_UPGRADE_30_H_
