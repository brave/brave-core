/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_DATABASE_MIGRATION_MIGRATION_V8_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_DATABASE_MIGRATION_MIGRATION_V8_H_

namespace ledger {
namespace database {
namespace migration {

const char v8[] = R"(
  ALTER TABLE contribution_info RENAME TO contribution_info_temp;

  DROP INDEX IF EXISTS contribution_info_publisher_id_index;

  CREATE TABLE contribution_info (
    publisher_id LONGVARCHAR,
    probi TEXT '0'  NOT NULL,
    date INTEGER NOT NULL,
    type INTEGER NOT NULL,
    month INTEGER NOT NULL,
    year INTEGER NOT NULL,
    CONSTRAINT fk_contribution_info_publisher_id
      FOREIGN KEY (publisher_id)
      REFERENCES publisher_info (publisher_id)
      ON DELETE CASCADE
  );

  CREATE INDEX contribution_info_publisher_id_index
    ON contribution_info (publisher_id);

  PRAGMA foreign_keys = off;
    INSERT INTO contribution_info (type, date, month, probi, publisher_id, year)
    SELECT category, date, month, probi, publisher_id, year
    FROM contribution_info_temp;

    DROP TABLE contribution_info_temp;
  PRAGMA foreign_keys = on;

  ALTER TABLE pending_contribution RENAME TO pending_contribution_temp;

  DROP INDEX IF EXISTS pending_contribution_publisher_id_index;

  CREATE TABLE pending_contribution (
    publisher_id LONGVARCHAR NOT NULL,
    amount DOUBLE DEFAULT 0 NOT NULL,
    added_date INTEGER DEFAULT 0 NOT NULL,
    viewing_id LONGVARCHAR NOT NULL,
    type INTEGER NOT NULL,
    CONSTRAINT fk_pending_contribution_publisher_id
      FOREIGN KEY (publisher_id)
       REFERENCES publisher_info (publisher_id)
      ON DELETE CASCADE
  );

  CREATE INDEX pending_contribution_publisher_id_index
    ON pending_contribution (publisher_id);

  PRAGMA foreign_keys = off;
    INSERT INTO pending_contribution (added_date, amount, type, publisher_id,
    viewing_id) SELECT added_date, amount, category, publisher_id,
    viewing_id FROM pending_contribution_temp;

    DROP TABLE pending_contribution_temp;
  PRAGMA foreign_keys = on;
)";

}  // namespace migration
}  // namespace database
}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_DATABASE_MIGRATION_MIGRATION_V8_H_
