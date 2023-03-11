/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_DATABASE_MIGRATION_MIGRATION_V38_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_DATABASE_MIGRATION_MIGRATION_V38_H_

namespace ledger::database::migration {

// Migration 38 adds a date/time column to the monthly contribution table in
// order to trigger contributions based on the date/time that the monthly
// contribution was set, rather than a "master" next contribution time.
constexpr char v38[] = R"sql(
  ALTER TABLE recurring_donation ADD COLUMN next_contribution_at TIMESTAMP;
)sql";

}  // namespace ledger::database::migration

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_DATABASE_MIGRATION_MIGRATION_V38_H_
