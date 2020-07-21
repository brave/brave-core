/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_DATABASE_MIGRATION_MIGRATION_V17_H_
#define BRAVELEDGER_DATABASE_MIGRATION_MIGRATION_V17_H_

namespace ledger {
namespace database {
namespace migration {

const char v17[] = R"(
  ALTER TABLE contribution_info ADD processor INTEGER NOT NULL DEFAULT 1;
)";

}  // namespace migration
}  // namespace database
}  // namespace ledger

#endif  // BRAVELEDGER_DATABASE_MIGRATION_MIGRATION_V17_H_
