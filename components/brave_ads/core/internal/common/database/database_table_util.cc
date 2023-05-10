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

std::string BuildInsertSql(const std::string& from,
                           const std::string& to,
                           const std::vector<std::string>& from_columns,
                           const std::vector<std::string>& to_columns) {
  CHECK(!from.empty());
  CHECK(!to.empty());
  CHECK_NE(from, to);
  CHECK(!from_columns.empty());
  CHECK(!to_columns.empty());
  CHECK_EQ(from_columns.size(), to_columns.size());

  return base::ReplaceStringPlaceholders(
      "INSERT INTO $1 ($2) SELECT $3 FROM $4;",
      {to, base::JoinString(to_columns, ", "),
       base::JoinString(from_columns, ", "), from},
      nullptr);
}

}  // namespace

void CreateTableIndex(mojom::DBTransactionInfo* transaction,
                      const std::string& table_name,
                      const std::string& key) {
  CHECK(transaction);
  CHECK(!table_name.empty());
  CHECK(!key.empty());

  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::EXECUTE;
  command->sql = base::ReplaceStringPlaceholders(
      "CREATE INDEX IF NOT EXISTS $1_$2_index ON $3 ($4);",
      {table_name, key, table_name, key}, nullptr);
  transaction->commands.push_back(std::move(command));
}

void DropTable(mojom::DBTransactionInfo* transaction,
               const std::string& table_name) {
  CHECK(transaction);
  CHECK(!table_name.empty());

  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::EXECUTE;
  command->sql = base::ReplaceStringPlaceholders("DROP TABLE IF EXISTS $1;",
                                                 {table_name}, nullptr);
  transaction->commands.push_back(std::move(command));
}

void DeleteTable(mojom::DBTransactionInfo* transaction,
                 const std::string& table_name) {
  CHECK(transaction);
  CHECK(!table_name.empty());

  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::EXECUTE;
  command->sql =
      base::ReplaceStringPlaceholders("DELETE FROM $1;", {table_name}, nullptr);
  transaction->commands.push_back(std::move(command));
}

void CopyTableColumns(mojom::DBTransactionInfo* transaction,
                      const std::string& from,
                      const std::string& to,
                      const std::vector<std::string>& from_columns,
                      const std::vector<std::string>& to_columns,
                      const bool should_drop) {
  CHECK(transaction);
  CHECK(!from.empty());
  CHECK(!to.empty());
  CHECK_NE(from, to);
  CHECK(!from_columns.empty());
  CHECK(!to_columns.empty());
  CHECK_EQ(from_columns.size(), to_columns.size());

  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::EXECUTE;
  command->sql = BuildInsertSql(from, to, from_columns, to_columns);
  transaction->commands.push_back(std::move(command));

  if (should_drop) {
    DropTable(transaction, from);
  }
}

void CopyTableColumns(mojom::DBTransactionInfo* transaction,
                      const std::string& from,
                      const std::string& to,
                      const std::vector<std::string>& columns,
                      const bool should_drop) {
  return CopyTableColumns(transaction, from, to, columns, columns, should_drop);
}

void RenameTable(mojom::DBTransactionInfo* transaction,
                 const std::string& from,
                 const std::string& to) {
  CHECK(transaction);
  CHECK(!from.empty());
  CHECK(!to.empty());
  CHECK_NE(from, to);

  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::EXECUTE;
  command->sql = base::ReplaceStringPlaceholders("ALTER TABLE $1 RENAME TO $2;",
                                                 {from, to}, nullptr);
  transaction->commands.push_back(std::move(command));
}

}  // namespace brave_ads::database
