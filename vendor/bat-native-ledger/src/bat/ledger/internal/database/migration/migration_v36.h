/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_DATABASE_MIGRATION_MIGRATION_V36_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_DATABASE_MIGRATION_MIGRATION_V36_H_

namespace ledger::database::migration {

// Migration 36 converts all stored publisher status values of "CONNECTED" to
// "NOT_VERIFIED".
const char v36[] = R"(
  UPDATE server_publisher_info SET status = 0 WHERE status = 1;
)";

}  // namespace ledger::database::migration

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_DATABASE_MIGRATION_MIGRATION_V36_H_
