/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/strings/stringprintf.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_rewards/browser/database/database_util.h"

namespace brave_rewards {

std::string GenerateDBInsertQuery(
    sql::Database* db,
    const std::string& from,
    const std::string& to,
    const std::map<std::string, std::string>& columns) {
  DCHECK_GT(columns.size(), 0UL);

  std::vector<std::string> from_columns;
  std::vector<std::string> to_columns;

  for (const auto& column : columns) {
    from_columns.push_back(column.first);
    to_columns.push_back(column.second);
  }

  const auto comma_separated_from_columns = base::JoinString(from_columns, ",");
  const auto comma_separated_to_columns = base::JoinString(to_columns, ",");

  return base::StringPrintf("INSERT INTO %s (%s) SELECT %s FROM %s;",
      to.c_str(), comma_separated_to_columns.c_str(),
          comma_separated_from_columns.c_str(), from.c_str());
}

bool MigrateDBTable(
    sql::Database* db,
    const std::string& from,
    const std::string& to,
    const std::map<std::string, std::string>& columns,
    const bool should_drop) {
  DCHECK_NE(from, to);
  DCHECK(!from.empty());
  DCHECK(!to.empty());

  std::string sql = "PRAGMA foreign_keys = off;";

  if (!columns.empty()) {
    const auto insert = GenerateDBInsertQuery(db, from, to, columns);
    sql.append(insert);
  }

  if (should_drop) {
    sql.append(base::StringPrintf("DROP TABLE %s;", from.c_str()));
  }

  sql.append("PRAGMA foreign_keys = on;");

  return db->Execute(sql.c_str());
}

bool MigrateDBTable(
    sql::Database* db,
    const std::string& from,
    const std::string& to,
    const std::vector<std::string>& columns,
    const bool should_drop) {
  std::map<std::string, std::string> new_columns;
  for (const auto& column : columns) {
    new_columns[column] = column;
  }

  return MigrateDBTable(db, from, to, new_columns, should_drop);
}

bool RenameDBTable(
    sql::Database* db,
    const std::string& from,
    const std::string& to) {
  DCHECK_NE(from, to);

  const auto sql = base::StringPrintf("ALTER TABLE %s RENAME TO %s;",
      from.c_str(), to.c_str());

  return db->Execute(sql.c_str());
}

}  // namespace brave_rewards
