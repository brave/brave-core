/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_DATABASE_MIGRATION_MIGRATION_V16_H_
#define BRAVELEDGER_DATABASE_MIGRATION_MIGRATION_V16_H_

namespace ledger {
namespace database {
namespace migration {

const char v16[] = R"(
  UPDATE contribution_info SET
  created_at = (
    CASE WHEN datetime(created_at, 'unixepoch') IS NULL
    THEN strftime('%s', datetime(created_at))
    ELSE created_at END
  );
)";

}  // namespace migration
}  // namespace database
}  // namespace ledger

#endif  // BRAVELEDGER_DATABASE_MIGRATION_MIGRATION_V16_H_
