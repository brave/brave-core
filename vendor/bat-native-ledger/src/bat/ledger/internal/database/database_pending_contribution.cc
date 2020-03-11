/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <map>
#include <utility>

#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "bat/ledger/internal/database/database_pending_contribution.h"
#include "bat/ledger/internal/database/database_util.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/static_values.h"

using std::placeholders::_1;

namespace braveledger_database {

namespace {

const char table_name_[] = "pending_contribution";

}  // namespace

DatabasePendingContribution::DatabasePendingContribution(
    bat_ledger::LedgerImpl* ledger) :
    DatabaseTable(ledger) {
}

DatabasePendingContribution::~DatabasePendingContribution() = default;

bool DatabasePendingContribution::CreateTableV3(
    ledger::DBTransaction* transaction) {
  DCHECK(transaction);

  const std::string query = base::StringPrintf(
      "CREATE TABLE %s ("
        "publisher_id LONGVARCHAR NOT NULL,"
        "amount DOUBLE DEFAULT 0 NOT NULL,"
        "added_date INTEGER DEFAULT 0 NOT NULL,"
        "viewing_id LONGVARCHAR NOT NULL,"
        "category INTEGER NOT NULL,"
        "CONSTRAINT fk_%s_publisher_id"
        "    FOREIGN KEY (publisher_id)"
        "    REFERENCES publisher_info (publisher_id)"
        "    ON DELETE CASCADE"
      ")",
      table_name_,
      table_name_);

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::EXECUTE;
  command->command = query;
  transaction->commands.push_back(std::move(command));

  return true;
}

bool DatabasePendingContribution::CreateTableV8(
    ledger::DBTransaction* transaction) {
  DCHECK(transaction);

  const std::string query = base::StringPrintf(
      "CREATE TABLE %s ("
        "publisher_id LONGVARCHAR NOT NULL,"
        "amount DOUBLE DEFAULT 0 NOT NULL,"
        "added_date INTEGER DEFAULT 0 NOT NULL,"
        "viewing_id LONGVARCHAR NOT NULL,"
        "type INTEGER NOT NULL,"
        "CONSTRAINT fk_%s_publisher_id"
        "    FOREIGN KEY (publisher_id)"
        "    REFERENCES publisher_info (publisher_id)"
        "    ON DELETE CASCADE"
      ")",
      table_name_,
      table_name_);

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::EXECUTE;
  command->command = query;
  transaction->commands.push_back(std::move(command));

  return true;
}

bool DatabasePendingContribution::CreateTableV12(
    ledger::DBTransaction* transaction) {
  DCHECK(transaction);

  const std::string query = base::StringPrintf(
      "CREATE TABLE %s ("
        "pending_contribution_id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,"
        "publisher_id LONGVARCHAR NOT NULL,"
        "amount DOUBLE DEFAULT 0 NOT NULL,"
        "added_date INTEGER DEFAULT 0 NOT NULL,"
        "viewing_id LONGVARCHAR NOT NULL,"
        "type INTEGER NOT NULL,"
        "CONSTRAINT fk_%s_publisher_id"
        "    FOREIGN KEY (publisher_id)"
        "    REFERENCES publisher_info (publisher_id)"
        "    ON DELETE CASCADE"
      ")",
      table_name_,
      table_name_);

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::EXECUTE;
  command->command = query;
  transaction->commands.push_back(std::move(command));

  return true;
}

bool DatabasePendingContribution::CreateTableV15(
    ledger::DBTransaction* transaction) {
  DCHECK(transaction);

  const std::string query = base::StringPrintf(
      "CREATE TABLE %s ("
        "pending_contribution_id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,"
        "publisher_id LONGVARCHAR NOT NULL,"
        "amount DOUBLE DEFAULT 0 NOT NULL,"
        "added_date INTEGER DEFAULT 0 NOT NULL,"
        "viewing_id LONGVARCHAR NOT NULL,"
        "type INTEGER NOT NULL"
      ")",
      table_name_);

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::EXECUTE;
  command->command = query;
  transaction->commands.push_back(std::move(command));

  return true;
}

bool DatabasePendingContribution::CreateIndexV3(
    ledger::DBTransaction* transaction) {
  DCHECK(transaction);

  return this->InsertIndex(transaction, table_name_, "publisher_id");
}

bool DatabasePendingContribution::CreateIndexV8(
    ledger::DBTransaction* transaction) {
  DCHECK(transaction);

  return this->InsertIndex(transaction, table_name_, "publisher_id");
}

bool DatabasePendingContribution::CreateIndexV12(
    ledger::DBTransaction* transaction) {
  DCHECK(transaction);

  return this->InsertIndex(transaction, table_name_, "publisher_id");
}

bool DatabasePendingContribution::CreateIndexV15(
    ledger::DBTransaction* transaction) {
  DCHECK(transaction);

  return this->InsertIndex(transaction, table_name_, "publisher_id");
}

bool DatabasePendingContribution::Migrate(
    ledger::DBTransaction* transaction,
    const int target) {
  DCHECK(transaction);

  switch (target) {
    case 3: {
      return MigrateToV3(transaction);
    }
    case 8: {
      return MigrateToV8(transaction);
    }
    case 12: {
      return MigrateToV12(transaction);
    }
    case 15: {
      return MigrateToV15(transaction);
    }
    default: {
      return true;
    }
  }
}

bool DatabasePendingContribution::MigrateToV3(
    ledger::DBTransaction* transaction) {
  DCHECK(transaction);

  if (!DropTable(transaction, table_name_)) {
    return false;
  }

  if (!CreateTableV3(transaction)) {
    return false;
  }

  if (!CreateIndexV3(transaction)) {
    return false;
  }

  return true;
}

bool DatabasePendingContribution::MigrateToV8(
    ledger::DBTransaction* transaction) {
  DCHECK(transaction);

  const std::string temp_table_name = base::StringPrintf(
      "%s_temp",
      table_name_);

  if (!RenameDBTable(transaction, table_name_, temp_table_name)) {
    return false;
  }

  const std::string query =
      "DROP INDEX IF EXISTS pending_contribution_publisher_id_index;";
  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::EXECUTE;
  command->command = query;
  transaction->commands.push_back(std::move(command));

  if (!CreateTableV8(transaction)) {
    return false;
  }

  if (!CreateIndexV8(transaction)) {
    return false;
  }

  const std::map<std::string, std::string> columns = {
    { "publisher_id", "publisher_id" },
    { "amount", "amount" },
    { "added_date", "added_date" },
    { "viewing_id", "viewing_id" },
    { "category", "type" }
  };

  if (!MigrateDBTable(
      transaction,
      temp_table_name,
      table_name_,
      columns,
      true)) {
    return false;
  }

  return true;
}

bool DatabasePendingContribution::MigrateToV12(
    ledger::DBTransaction* transaction) {
  DCHECK(transaction);

  const std::string temp_table_name = base::StringPrintf(
      "%s_temp",
      table_name_);

  if (!RenameDBTable(transaction, table_name_, temp_table_name)) {
    return false;
  }

  const std::string query =
      "DROP INDEX IF EXISTS pending_contribution_publisher_id_index;";
  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::EXECUTE;
  command->command = query;
  transaction->commands.push_back(std::move(command));

  if (!CreateTableV12(transaction)) {
    return false;
  }

  if (!CreateIndexV12(transaction)) {
    return false;
  }

  const std::map<std::string, std::string> columns = {
    { "publisher_id", "publisher_id" },
    { "amount", "amount" },
    { "added_date", "added_date" },
    { "viewing_id", "viewing_id" },
    { "type", "type" }
  };

  if (!MigrateDBTable(
      transaction,
      temp_table_name,
      table_name_,
      columns,
      true)) {
    return false;
  }
  return true;
}

bool DatabasePendingContribution::MigrateToV15(
    ledger::DBTransaction* transaction) {
  DCHECK(transaction);

  const std::string temp_table_name = base::StringPrintf(
      "%s_temp",
      table_name_);

  if (!RenameDBTable(transaction, table_name_, temp_table_name)) {
    return false;
  }

  const std::string query =
      "DROP INDEX IF EXISTS pending_contribution_publisher_id_index;";
  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::EXECUTE;
  command->command = query;
  transaction->commands.push_back(std::move(command));

  if (!CreateTableV15(transaction)) {
    return false;
  }

  if (!CreateIndexV15(transaction)) {
    return false;
  }

  const std::map<std::string, std::string> columns = {
    { "pending_contribution_id", "pending_contribution_id" },
    { "publisher_id", "publisher_id" },
    { "amount", "amount" },
    { "added_date", "added_date" },
    { "viewing_id", "viewing_id" },
    { "type", "type" }
  };

  if (!MigrateDBTable(
      transaction,
      temp_table_name,
      table_name_,
      columns,
      true)) {
    return false;
  }
  return true;
}

void DatabasePendingContribution::InsertOrUpdateList(
    ledger::PendingContributionList list,
    ledger::ResultCallback callback) {
  if (list.empty()) {
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  auto transaction = ledger::DBTransaction::New();
  const uint64_t now = static_cast<uint64_t>(base::Time::Now().ToDoubleT());

  const std::string query = base::StringPrintf(
    "INSERT INTO %s (pending_contribution_id, publisher_id, amount, "
    "added_date, viewing_id, type) VALUES (?, ?, ?, ?, ?, ?)",
    table_name_);

  for (const auto& item : list) {
    auto command = ledger::DBCommand::New();
    command->type = ledger::DBCommand::Type::RUN;
    command->command = query;

    BindNull(command.get(), 0);
    BindString(command.get(), 1, item->publisher_key);
    BindDouble(command.get(), 2, item->amount);
    BindInt64(command.get(), 3, now);
    BindString(command.get(), 4, item->viewing_id);
    BindInt(command.get(), 5, static_cast<int>(item->type));

    transaction->commands.push_back(std::move(command));
  }

  auto transaction_callback = std::bind(&OnResultCallback,
      _1,
      callback);

  ledger_->RunDBTransaction(std::move(transaction), transaction_callback);
}

void DatabasePendingContribution::GetReservedAmount(
    ledger::PendingContributionsTotalCallback callback) {
  auto transaction = ledger::DBTransaction::New();
  const std::string query = base::StringPrintf(
    "SELECT SUM(amount) FROM %s",
    table_name_);

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::READ;
  command->command = query;

  command->record_bindings = {
      ledger::DBCommand::RecordBindingType::DOUBLE_TYPE
  };

  transaction->commands.push_back(std::move(command));

  auto transaction_callback =
      std::bind(&DatabasePendingContribution::OnGetReservedAmount,
          this,
          _1,
          callback);

  ledger_->RunDBTransaction(std::move(transaction), transaction_callback);
}

void DatabasePendingContribution::OnGetReservedAmount(
    ledger::DBCommandResponsePtr response,
    ledger::PendingContributionsTotalCallback callback) {
  if (!response
      || response->status != ledger::DBCommandResponse::Status::RESPONSE_OK) {
    callback(0.0);
    return;
  }

  if (response->result->get_records().size() != 1) {
    callback(0.0);
    return;
  }

  auto* record = response->result->get_records()[0].get();

  callback(GetDoubleColumn(record, 0));
}

void DatabasePendingContribution::GetAllRecords(
    ledger::PendingContributionInfoListCallback callback) {
  auto transaction = ledger::DBTransaction::New();
  const std::string query = base::StringPrintf(
    "SELECT pc.pending_contribution_id, pi.publisher_id, pi.name, "
    "pi.url, pi.favIcon, spi.status, pi.provider, pc.amount, pc.added_date, "
    "pc.viewing_id, pc.type "
    "FROM %s as pc "
    "INNER JOIN publisher_info AS pi ON pc.publisher_id = pi.publisher_id "
    "LEFT JOIN server_publisher_info AS spi "
    "ON spi.publisher_key = pi.publisher_id",
    table_name_);

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::READ;
  command->command = query;

  command->record_bindings = {
      ledger::DBCommand::RecordBindingType::INT64_TYPE,
      ledger::DBCommand::RecordBindingType::STRING_TYPE,
      ledger::DBCommand::RecordBindingType::STRING_TYPE,
      ledger::DBCommand::RecordBindingType::STRING_TYPE,
      ledger::DBCommand::RecordBindingType::STRING_TYPE,
      ledger::DBCommand::RecordBindingType::INT64_TYPE,
      ledger::DBCommand::RecordBindingType::STRING_TYPE,
      ledger::DBCommand::RecordBindingType::DOUBLE_TYPE,
      ledger::DBCommand::RecordBindingType::INT64_TYPE,
      ledger::DBCommand::RecordBindingType::STRING_TYPE,
      ledger::DBCommand::RecordBindingType::INT_TYPE
  };

  transaction->commands.push_back(std::move(command));

  auto transaction_callback =
      std::bind(&DatabasePendingContribution::OnGetAllRecords,
          this,
          _1,
          callback);

  ledger_->RunDBTransaction(std::move(transaction), transaction_callback);
}

void DatabasePendingContribution::OnGetAllRecords(
    ledger::DBCommandResponsePtr response,
    ledger::PendingContributionInfoListCallback callback) {
  if (!response
      || response->status != ledger::DBCommandResponse::Status::RESPONSE_OK) {
    callback({});
    return;
  }

  ledger::PendingContributionInfoList list;
  for (auto const& record : response->result->get_records()) {
    auto info = ledger::PendingContributionInfo::New();
    auto* record_pointer = record.get();

    info->id = GetInt64Column(record_pointer, 0);
    info->publisher_key = GetStringColumn(record_pointer, 1);
    info->name = GetStringColumn(record_pointer, 2);
    info->url = GetStringColumn(record_pointer, 3);
    info->favicon_url = GetStringColumn(record_pointer, 4);
    info->status = static_cast<ledger::mojom::PublisherStatus>(
        GetInt64Column(record_pointer, 5));
    info->provider = GetStringColumn(record_pointer, 6);
    info->amount = GetDoubleColumn(record_pointer, 7);
    info->added_date = GetInt64Column(record_pointer, 8);
    info->viewing_id = GetStringColumn(record_pointer, 9);
    info->type = static_cast<ledger::RewardsType>(
        GetIntColumn(record_pointer, 10));
    info->expiration_date =
        info->added_date +
        braveledger_ledger::_pending_contribution_expiration;

    list.push_back(std::move(info));
  }

  callback(std::move(list));
}

void DatabasePendingContribution::DeleteRecord(
    const uint64_t id,
    ledger::ResultCallback callback) {
  if (id == 0) {
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  auto transaction = ledger::DBTransaction::New();
  const std::string query = base::StringPrintf(
    "DELETE FROM %s WHERE pending_contribution_id = ?",
    table_name_);

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::RUN;
  command->command = query;

  BindInt64(command.get(), 0, id);

  transaction->commands.push_back(std::move(command));

  auto transaction_callback = std::bind(&OnResultCallback,
      _1,
      callback);

  ledger_->RunDBTransaction(std::move(transaction), transaction_callback);
}

void DatabasePendingContribution::DeleteAllRecords(
    ledger::ResultCallback callback) {
  auto transaction = ledger::DBTransaction::New();
  const std::string query = base::StringPrintf("DELETE FROM %s", table_name_);

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::RUN;
  command->command = query;

  transaction->commands.push_back(std::move(command));

  auto transaction_callback = std::bind(&OnResultCallback,
      _1,
      callback);

  ledger_->RunDBTransaction(std::move(transaction), transaction_callback);
}

}  // namespace braveledger_database
