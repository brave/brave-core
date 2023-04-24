/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/database/database_table_util.h"

#include <utility>

#include "base/check_op.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_ads/common/interfaces/brave_ads.mojom.h"

namespace brave_ads::database {

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

  return base::ReplaceStringPlaceholders(
      "INSERT INTO $1 ($2) SELECT $3 FROM $4 $5;",
      {to, base::JoinString(to_columns, ", "),
       base::JoinString(from_columns, ", "), from, group_by},
      nullptr);
}

}  // namespace

void CreateTableIndex(mojom::DBTransactionInfo* transaction,
                      const std::string& table_name,
                      const std::string& key) {
  DCHECK(transaction);
  DCHECK(!table_name.empty());
  DCHECK(!key.empty());

  const std::string query = base::ReplaceStringPlaceholders(
      "CREATE INDEX IF NOT EXISTS $1_$2_index ON $3 ($4)",
      {table_name, key, table_name, key}, nullptr);

  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::EXECUTE;
  command->command = query;

  transaction->commands.push_back(std::move(command));
}

void DropTable(mojom::DBTransactionInfo* transaction,
               const std::string& table_name) {
  DCHECK(transaction);
  DCHECK(!table_name.empty());

  const std::string query = base::ReplaceStringPlaceholders(
      "PRAGMA foreign_keys = off; DROP TABLE IF EXISTS $1; PRAGMA foreign_keys "
      "= on;",
      {table_name}, nullptr);

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
      base::ReplaceStringPlaceholders("DELETE FROM $1", {table_name}, nullptr);

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

  query += BuildInsertQuery(from, to, from_columns, to_columns, group_by);

  if (should_drop) {
    query += base::ReplaceStringPlaceholders("DROP TABLE $1;", {from}, nullptr);
  }

  query += "PRAGMA foreign_keys = on;";

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

  const std::string query = base::ReplaceStringPlaceholders(
      "ALTER TABLE $1 RENAME TO $2", {from, to}, nullptr);

  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::EXECUTE;
  command->command = query;

  transaction->commands.push_back(std::move(command));
}

}  // namespace brave_ads::database
