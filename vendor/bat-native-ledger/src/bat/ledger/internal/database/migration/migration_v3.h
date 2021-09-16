/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_DATABASE_MIGRATION_MIGRATION_V3_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_DATABASE_MIGRATION_MIGRATION_V3_H_

namespace ledger {
namespace database {
namespace migration {

const char v3[] = R"(
  PRAGMA foreign_keys = off;
    DROP TABLE IF EXISTS pending_contribution;
  PRAGMA foreign_keys = on;

  CREATE TABLE pending_contribution (
    publisher_id LONGVARCHAR NOT NULL,
    amount DOUBLE DEFAULT 0 NOT NULL,
    added_date INTEGER DEFAULT 0 NOT NULL,
    viewing_id LONGVARCHAR NOT NULL,
    category INTEGER NOT NULL,
    CONSTRAINT fk_pending_contribution_publisher_id
      FOREIGN KEY (publisher_id)
      REFERENCES publisher_info (publisher_id)
      ON DELETE CASCADE
  );

  CREATE INDEX pending_contribution_publisher_id_index
    ON pending_contribution (publisher_id);
)";

}  // namespace migration
}  // namespace database
}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_DATABASE_MIGRATION_MIGRATION_V3_H_
