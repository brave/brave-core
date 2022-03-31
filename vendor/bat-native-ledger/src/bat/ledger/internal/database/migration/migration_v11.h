/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_DATABASE_MIGRATION_MIGRATION_V11_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_DATABASE_MIGRATION_MIGRATION_V11_H_

namespace ledger {
namespace database {
namespace migration {

const char v11[] = R"(
  ALTER TABLE contribution_info RENAME TO contribution_info_temp;

  DROP INDEX IF EXISTS contribution_info_publisher_id_index;

  CREATE TABLE contribution_info (
    contribution_id TEXT NOT NULL,
    amount DOUBLE NOT NULL,
    type INTEGER NOT NULL,
    step INTEGER NOT NULL DEFAULT -1,
    retry_count INTEGER NOT NULL DEFAULT -1,
    created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (contribution_id)
  );

  PRAGMA foreign_keys = off;
    DROP TABLE IF EXISTS contribution_info_publishers;
  PRAGMA foreign_keys = on;

  CREATE TABLE contribution_info_publishers (
    contribution_id TEXT NOT NULL,
    publisher_key TEXT NOT NULL,
    total_amount DOUBLE NOT NULL,
    contributed_amount DOUBLE,
    CONSTRAINT fk_contribution_info_publishers_contribution_id
      FOREIGN KEY (contribution_id)
      REFERENCES contribution_info (contribution_id)
      ON DELETE CASCADE,
    CONSTRAINT fk_contribution_info_publishers_publisher_id
      FOREIGN KEY (publisher_key)
      REFERENCES publisher_info (publisher_id)
  );

  CREATE INDEX contribution_info_publishers_contribution_id_index
    ON contribution_info_publishers (contribution_id);

  CREATE INDEX contribution_info_publishers_publisher_key_index
    ON contribution_info_publishers (publisher_key);

  ALTER TABLE contribution_info_temp ADD contribution_id TEXT;

  ALTER TABLE contribution_info_temp ADD amount DOUBLE;

  UPDATE contribution_info_temp SET
  contribution_id = PRINTF('id_%s_%s', date, ABS(RANDOM())),
  amount = CAST(
    PRINTF('%s.%s', SUBSTR(probi, 0, LENGTH(probi)-17),
    SUBSTR(SUBSTR(probi, LENGTH(probi)-17, LENGTH(probi)), 0, 2)) as decimal
  );

  INSERT INTO contribution_info (contribution_id, amount, type, step,
  retry_count, created_at) SELECT contribution_id, amount, type, -1, -1, date
  FROM contribution_info_temp;

  INSERT INTO contribution_info_publishers (contribution_id, publisher_key,
  total_amount, contributed_amount) SELECT contribution_id, publisher_id,
  amount, amount FROM contribution_info_temp WHERE publisher_id IS NOT NULL AND
  publisher_id != '';

  PRAGMA foreign_keys = off;
    DROP TABLE IF EXISTS contribution_info_temp;
  PRAGMA foreign_keys = on;
)";

}  // namespace migration
}  // namespace database
}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_DATABASE_MIGRATION_MIGRATION_V11_H_
