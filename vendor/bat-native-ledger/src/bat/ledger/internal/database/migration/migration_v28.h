/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_DATABASE_MIGRATION_MIGRATION_V28_H_
#define BRAVELEDGER_DATABASE_MIGRATION_MIGRATION_V28_H_

namespace ledger {
namespace database {
namespace migration {

const char v28[] = R"(
  PRAGMA foreign_keys = off;
    DROP TABLE IF EXISTS server_publisher_info;
  PRAGMA foreign_keys = on;

  CREATE TABLE server_publisher_info (
    publisher_key LONGVARCHAR PRIMARY KEY NOT NULL,
    status INTEGER DEFAULT 0 NOT NULL,
    address TEXT NOT NULL,
    updated_at TIMESTAMP NOT NULL
  );

  DELETE FROM server_publisher_banner;

  DELETE FROM server_publisher_links;

  DELETE FROM server_publisher_amounts;

  PRAGMA foreign_keys = off;
    DROP TABLE IF EXISTS publisher_prefix_list;
  PRAGMA foreign_keys = on;

  CREATE TABLE publisher_prefix_list (hash_prefix BLOB PRIMARY KEY NOT NULL);
)";

}  // namespace migration
}  // namespace database
}  // namespace ledger

#endif  // BRAVELEDGER_DATABASE_MIGRATION_MIGRATION_V28_H_
