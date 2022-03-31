/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_DATABASE_MIGRATION_MIGRATION_V6_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_DATABASE_MIGRATION_MIGRATION_V6_H_

namespace ledger {
namespace database {
namespace migration {

const char v6[] = R"(
  ALTER TABLE activity_info RENAME TO activity_info_temp;

  DROP INDEX IF EXISTS activity_info_publisher_id_index;

  CREATE TABLE activity_info (
    publisher_id LONGVARCHAR NOT NULL,
    duration INTEGER DEFAULT 0 NOT NULL,
    visits INTEGER DEFAULT 0 NOT NULL,
    score DOUBLE DEFAULT 0 NOT NULL,
    percent INTEGER DEFAULT 0 NOT NULL,
    weight DOUBLE DEFAULT 0 NOT NULL,
    reconcile_stamp INTEGER DEFAULT 0 NOT NULL,
    CONSTRAINT activity_unique
      UNIQUE (publisher_id, reconcile_stamp)
    CONSTRAINT fk_activity_info_publisher_id
      FOREIGN KEY (publisher_id)
      REFERENCES publisher_info (publisher_id)
      ON DELETE CASCADE
  );

  CREATE INDEX activity_info_publisher_id_index
    ON activity_info (publisher_id);

  PRAGMA foreign_keys = off;
    INSERT INTO activity_info (publisher_id, reconcile_stamp, duration, percent,
    score, visits, weight) SELECT publisher_id, reconcile_stamp,
    sum(duration) as duration, sum(percent) as percent, sum(score) as score,
    sum(visits) as visits, sum(weight) as weight FROM activity_info_temp
    GROUP BY publisher_id, reconcile_stamp;

    DROP TABLE activity_info_temp;
  PRAGMA foreign_keys = on;
)";

}  // namespace migration
}  // namespace database
}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_DATABASE_MIGRATION_MIGRATION_V6_H_
