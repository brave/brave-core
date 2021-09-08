/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/database/database_table_util.h"

#include <utility>

#include "base/check_op.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "bat/ads/internal/logging.h"

namespace ads {
namespace database {
namespace table {
namespace util {

namespace {

std::string BuildInsertQuery(const std::string& from,
                             const std::string& to,
                             const std::map<std::string, std::string>& columns,
                             const std::string& group_by) {
  DCHECK(!from.empty());
  DCHECK(!to.empty());
  DCHECK_NE(from, to);
  DCHECK(!columns.empty());

  std::vector<std::string> from_columns;
  std::vector<std::string> to_columns;

  for (const auto& column : columns) {
    from_columns.push_back(column.first);
    to_columns.push_back(column.second);
  }

  const std::string comma_separated_from_columns =
      base::JoinString(from_columns, ", ");

  const std::string comma_separated_to_columns =
      base::JoinString(to_columns, ", ");

  return base::StringPrintf("INSERT INTO %s (%s) SELECT %s FROM %s %s;",
                            to.c_str(), comma_separated_to_columns.c_str(),
                            comma_separated_from_columns.c_str(), from.c_str(),
                            group_by.c_str());
}

}  // namespace

void CreateIndex(mojom::DBTransaction* transaction,
                 const std::string& table_name,
                 const std::string& key) {
  DCHECK(transaction);
  DCHECK(!table_name.empty());
  DCHECK(!key.empty());

  const std::string query = base::StringPrintf(
      "CREATE INDEX %s_%s_index ON %s (%s)", table_name.c_str(), key.c_str(),
      table_name.c_str(), key.c_str());

  mojom::DBCommandPtr command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::EXECUTE;
  command->command = query;

  transaction->commands.push_back(std::move(command));
}

void Drop(mojom::DBTransaction* transaction, const std::string& table_name) {
  DCHECK(transaction);
  DCHECK(!table_name.empty());

  const std::string query = base::StringPrintf(
      "PRAGMA foreign_keys = off;"
      "DROP TABLE IF EXISTS %s;"
      "PRAGMA foreign_keys = on;",
      table_name.c_str());

  mojom::DBCommandPtr command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::EXECUTE;
  command->command = query;

  transaction->commands.push_back(std::move(command));
}

void Delete(mojom::DBTransaction* transaction, const std::string& table_name) {
  DCHECK(transaction);
  DCHECK(!table_name.empty());

  const std::string query =
      base::StringPrintf("DELETE FROM %s", table_name.c_str());

  mojom::DBCommandPtr command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::EXECUTE;
  command->command = query;

  transaction->commands.push_back(std::move(command));
}

void CopyColumns(mojom::DBTransaction* transaction,
                 const std::string& from,
                 const std::string& to,
                 const std::map<std::string, std::string>& columns,
                 const bool should_drop,
                 const std::string& group_by) {
  DCHECK(transaction);
  DCHECK(!from.empty());
  DCHECK(!to.empty());
  DCHECK_NE(from, to);
  DCHECK(!columns.empty());

  std::string query = "PRAGMA foreign_keys = off;";

  const std::string insert_query =
      BuildInsertQuery(from, to, columns, group_by);
  query.append(insert_query);

  if (should_drop) {
    query.append(base::StringPrintf("DROP TABLE %s;", from.c_str()));
  }

  query.append("PRAGMA foreign_keys = on;");

  mojom::DBCommandPtr command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::EXECUTE;
  command->command = query;

  transaction->commands.push_back(std::move(command));
}

void CopyColumns(mojom::DBTransaction* transaction,
                 const std::string& from,
                 const std::string& to,
                 const std::vector<std::string>& columns,
                 const bool should_drop,
                 const std::string& group_by) {
  DCHECK(transaction);
  DCHECK(!from.empty());
  DCHECK(!to.empty());
  DCHECK_NE(from, to);
  DCHECK(!columns.empty());

  std::map<std::string, std::string> new_columns;
  for (const auto& column : columns) {
    new_columns[column] = column;
  }

  return CopyColumns(transaction, from, to, new_columns, should_drop, group_by);
}

void Rename(mojom::DBTransaction* transaction,
            const std::string& from,
            const std::string& to) {
  DCHECK(transaction);
  DCHECK(!from.empty());
  DCHECK(!to.empty());
  DCHECK_NE(from, to);

  const std::string query = base::StringPrintf("ALTER TABLE %s RENAME TO %s",
                                               from.c_str(), to.c_str());

  mojom::DBCommandPtr command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::EXECUTE;
  command->command = query;

  transaction->commands.push_back(std::move(command));
}

}  // namespace util
}  // namespace table
}  // namespace database
}  // namespace ads
