/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/common/database/database_table_util.h"

#include <utility>

#include "base/check_op.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "bat/ads/public/interfaces/ads.mojom.h"

namespace ads::database {

namespace {

std::string BuildInsertQuery(const std::string& from,
                             const std::string& to,
                             const std::vector<std::string>& from_columns,
                             const std::vector<std::string>& to_columns,
                             const std::string& group_by) {
  DCHECK(!from.empty());
  DCHECK(!to.empty());
  DCHECK_NE(from, to);
  DCHECK(!from_columns.empty());
  DCHECK(!to_columns.empty());
  DCHECK_EQ(from_columns.size(), to_columns.size());

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

void CreateTableIndex(mojom::DBTransactionInfo* transaction,
                      const std::string& table_name,
                      const std::string& key) {
  DCHECK(transaction);
  DCHECK(!table_name.empty());
  DCHECK(!key.empty());

  const std::string query = base::StringPrintf(
      "CREATE INDEX IF NOT EXISTS %s_%s_index ON %s (%s)", table_name.c_str(),
      key.c_str(), table_name.c_str(), key.c_str());

  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::EXECUTE;
  command->command = query;

  transaction->commands.push_back(std::move(command));
}

void DropTable(mojom::DBTransactionInfo* transaction,
               const std::string& table_name) {
  DCHECK(transaction);
  DCHECK(!table_name.empty());

  const std::string query = base::StringPrintf(
      "PRAGMA foreign_keys = off;"
      "DROP TABLE IF EXISTS %s;"
      "PRAGMA foreign_keys = on;",
      table_name.c_str());

  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::EXECUTE;
  command->command = query;

  transaction->commands.push_back(std::move(command));
}

void DeleteTable(mojom::DBTransactionInfo* transaction,
                 const std::string& table_name) {
  DCHECK(transaction);
  DCHECK(!table_name.empty());

  const std::string query =
      base::StringPrintf("DELETE FROM %s", table_name.c_str());

  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::EXECUTE;
  command->command = query;

  transaction->commands.push_back(std::move(command));
}

void CopyTableColumns(mojom::DBTransactionInfo* transaction,
                      const std::string& from,
                      const std::string& to,
                      const std::vector<std::string>& from_columns,
                      const std::vector<std::string>& to_columns,
                      const bool should_drop,
                      const std::string& group_by) {
  DCHECK(transaction);
  DCHECK(!from.empty());
  DCHECK(!to.empty());
  DCHECK_NE(from, to);
  DCHECK(!from_columns.empty());
  DCHECK(!to_columns.empty());
  DCHECK_EQ(from_columns.size(), to_columns.size());

  std::string query = "PRAGMA foreign_keys = off;";

  const std::string insert_query =
      BuildInsertQuery(from, to, from_columns, to_columns, group_by);
  query.append(insert_query);

  if (should_drop) {
    query.append(base::StringPrintf("DROP TABLE %s;", from.c_str()));
  }

  query.append("PRAGMA foreign_keys = on;");

  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::EXECUTE;
  command->command = query;

  transaction->commands.push_back(std::move(command));
}

void CopyTableColumns(mojom::DBTransactionInfo* transaction,
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

  return CopyTableColumns(transaction, from, to, columns, columns, should_drop,
                          group_by);
}

void RenameTable(mojom::DBTransactionInfo* transaction,
                 const std::string& from,
                 const std::string& to) {
  DCHECK(transaction);
  DCHECK(!from.empty());
  DCHECK(!to.empty());
  DCHECK_NE(from, to);

  const std::string query = base::StringPrintf("ALTER TABLE %s RENAME TO %s",
                                               from.c_str(), to.c_str());

  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::EXECUTE;
  command->command = query;

  transaction->commands.push_back(std::move(command));
}

}  // namespace ads::database
