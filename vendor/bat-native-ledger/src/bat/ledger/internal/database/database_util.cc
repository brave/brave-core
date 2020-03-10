/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>

#include "base/strings/stringprintf.h"
#include "base/strings/string_util.h"
#include "bat/ledger/internal/database/database_util.h"

namespace {

const int kCurrentVersionNumber = 19;
const int kCompatibleVersionNumber = 1;

}  // namespace

namespace braveledger_database {

bool DropTable(
    ledger::DBTransaction* transaction,
    const std::string& table_name) {
  DCHECK(!table_name.empty());
  DCHECK(transaction);

  const std::string query = base::StringPrintf(
      "PRAGMA foreign_keys = off;"
      "DROP TABLE IF EXISTS %s;"
      "PRAGMA foreign_keys = on;",
      table_name.c_str());

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::EXECUTE;
  command->command = query;
  transaction->commands.push_back(std::move(command));

  return true;
}

std::string GenerateDBInsertQuery(
    const std::string& from,
    const std::string& to,
    const std::map<std::string, std::string>& columns,
    const std::string group_by) {
  DCHECK_GT(columns.size(), 0ul);

  std::vector<std::string> from_columns;
  std::vector<std::string> to_columns;

  for (const auto& column : columns) {
    from_columns.push_back(column.first);
    to_columns.push_back(column.second);
  }

  const auto comma_separated_from_columns = base::JoinString(from_columns, ",");
  const auto comma_separated_to_columns = base::JoinString(to_columns, ",");

  return base::StringPrintf(
      "INSERT INTO %s (%s) SELECT %s FROM %s %s;",
      to.c_str(),
      comma_separated_to_columns.c_str(),
      comma_separated_from_columns.c_str(),
      from.c_str(),
      group_by.c_str());
}

bool MigrateDBTable(
    ledger::DBTransaction* transaction,
    const std::string& from,
    const std::string& to,
    const std::map<std::string, std::string>& columns,
    const bool should_drop,
    const std::string group_by) {
  DCHECK_NE(from, to);
  DCHECK(!from.empty());
  DCHECK(!to.empty());
  DCHECK(transaction);

  std::string query = "PRAGMA foreign_keys = off;";

  if (!columns.empty()) {
    const auto insert = GenerateDBInsertQuery(from, to, columns, group_by);
    query.append(insert);
  }

  if (should_drop) {
    query.append(base::StringPrintf("DROP TABLE %s;", from.c_str()));
  }

  query.append("PRAGMA foreign_keys = on;");

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::EXECUTE;
  command->command = query;
  transaction->commands.push_back(std::move(command));

  return true;
}

bool MigrateDBTable(
    ledger::DBTransaction* transaction,
    const std::string& from,
    const std::string& to,
    const std::vector<std::string>& columns,
    const bool should_drop,
    const std::string group_by) {
  std::map<std::string, std::string> new_columns;
  for (const auto& column : columns) {
    new_columns[column] = column;
  }

  return MigrateDBTable(
      transaction,
      from,
      to,
      new_columns,
      should_drop,
      group_by);
}

bool RenameDBTable(
    ledger::DBTransaction* transaction,
    const std::string& from,
    const std::string& to) {
  DCHECK_NE(from, to);
  DCHECK(transaction);

  const auto query = base::StringPrintf(
      "ALTER TABLE %s RENAME TO %s;",
      from.c_str(),
      to.c_str());

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::EXECUTE;
  command->command = query;
  transaction->commands.push_back(std::move(command));

  return true;
}

void BindNull(
    ledger::DBCommand* command,
    const int index) {
  if (!command) {
    return;
  }

  auto binding = ledger::DBCommandBinding::New();
  binding->index = index;
  binding->value = ledger::DBValue::New();
  binding->value->set_null_value(0);
  command->bindings.push_back(std::move(binding));
}

void BindInt(
    ledger::DBCommand* command,
    const int index,
    const int32_t value) {
  if (!command) {
    return;
  }

  auto binding = ledger::DBCommandBinding::New();
  binding->index = index;
  binding->value = ledger::DBValue::New();
  binding->value->set_int_value(value);
  command->bindings.push_back(std::move(binding));
}

void BindInt64(
    ledger::DBCommand* command,
    const int index,
    const int64_t value) {
  if (!command) {
    return;
  }

  auto binding = ledger::DBCommandBinding::New();
  binding->index = index;
  binding->value = ledger::DBValue::New();
  binding->value->set_int64_value(value);
  command->bindings.push_back(std::move(binding));
}

void BindDouble(
    ledger::DBCommand* command,
    const int index,
    const double value) {
  if (!command) {
    return;
  }

  auto binding = ledger::DBCommandBinding::New();
  binding->index = index;
  binding->value = ledger::DBValue::New();
  binding->value->set_double_value(value);
  command->bindings.push_back(std::move(binding));
}

void BindBool(
    ledger::DBCommand* command,
    const int index,
    const bool value) {
  if (!command) {
    return;
  }

  auto binding = ledger::DBCommandBinding::New();
  binding->index = index;
  binding->value = ledger::DBValue::New();
  binding->value->set_bool_value(value);
  command->bindings.push_back(std::move(binding));
}

void BindString(
    ledger::DBCommand* command,
    const int index,
    const std::string& value) {
  if (!command) {
    return;
  }

  auto binding = ledger::DBCommandBinding::New();
  binding->index = index;
  binding->value = ledger::DBValue::New();
  binding->value->set_string_value(value);
  command->bindings.push_back(std::move(binding));
}

int32_t GetCurrentVersion() {
  return kCurrentVersionNumber;
}

int32_t GetCompatibleVersion() {
  return kCompatibleVersionNumber;
}

void OnResultCallback(
    ledger::DBCommandResponsePtr response,
    ledger::ResultCallback callback) {
  if (!response ||
      response->status != ledger::DBCommandResponse::Status::RESPONSE_OK) {
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  callback(ledger::Result::LEDGER_OK);
}

int GetIntColumn(ledger::DBRecord* record, const int index) {
  if (!record || static_cast<int>(record->fields.size()) < index) {
    return 0;
  }

  if (record->fields.at(index)->which() != ledger::DBValue::Tag::INT_VALUE) {
    DCHECK(false);
    return 0;
  }

  return record->fields.at(index)->get_int_value();
}

int64_t GetInt64Column(ledger::DBRecord* record, const int index) {
  if (!record || static_cast<int>(record->fields.size()) < index) {
    return 0;
  }

  if (record->fields.at(index)->which() != ledger::DBValue::Tag::INT64_VALUE) {
    DCHECK(false);
    return 0;
  }

  return record->fields.at(index)->get_int64_value();
}

double GetDoubleColumn(ledger::DBRecord* record, const int index) {
  if (!record || static_cast<int>(record->fields.size()) < index) {
    return 0.0;
  }

  if (record->fields.at(index)->which() != ledger::DBValue::Tag::DOUBLE_VALUE) {
    DCHECK(false);
    return 0.0;
  }

  return record->fields.at(index)->get_double_value();
}

bool GetBoolColumn(ledger::DBRecord* record, const int index) {
  if (!record || static_cast<int>(record->fields.size()) < index) {
    return false;
  }

  if (record->fields.at(index)->which() != ledger::DBValue::Tag::BOOL_VALUE) {
    DCHECK(false);
    return false;
  }

  return record->fields.at(index)->get_bool_value();
}

std::string GetStringColumn(ledger::DBRecord* record, const int index) {
  if (!record || static_cast<int>(record->fields.size()) < index) {
    return "";
  }

  if (record->fields.at(index)->which() != ledger::DBValue::Tag::STRING_VALUE) {
    DCHECK(false);
    return "";
  }

  return record->fields.at(index)->get_string_value();
}

std::string GenerateStringInCase(const std::vector<std::string>& items) {
  if (items.empty()) {
    return "";
  }

  std::string items_join = base::JoinString(items, ", ");
  base::ReplaceSubstringsAfterOffset(
      &items_join,
      0,
      ", ",
      "\", \"");

  return base::StringPrintf("\"%s\"", items_join.c_str());
}

}  // namespace braveledger_database
