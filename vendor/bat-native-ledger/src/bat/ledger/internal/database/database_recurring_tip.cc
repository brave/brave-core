/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <map>
#include <utility>

#include "base/strings/stringprintf.h"
#include "bat/ledger/internal/database/database_recurring_tip.h"
#include "bat/ledger/internal/database/database_util.h"
#include "bat/ledger/internal/ledger_impl.h"

using std::placeholders::_1;

namespace braveledger_database {

namespace {

// TODO(https://github.com/brave/brave-browser/issues/7144):
//  rename to recurring_tip
const char table_name_[] = "recurring_donation";

}  // namespace

DatabaseRecurringTip::DatabaseRecurringTip(
    bat_ledger::LedgerImpl* ledger) :
    DatabaseTable(ledger) {
}

DatabaseRecurringTip::~DatabaseRecurringTip() = default;

bool DatabaseRecurringTip::CreateTableV2(ledger::DBTransaction* transaction) {
  DCHECK(transaction);

  const std::string query = base::StringPrintf(
      "CREATE TABLE %s ("
        "publisher_id LONGVARCHAR NOT NULL PRIMARY KEY UNIQUE,"
        "amount DOUBLE DEFAULT 0 NOT NULL,"
        "added_date INTEGER DEFAULT 0 NOT NULL,"
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

bool DatabaseRecurringTip::CreateTableV15(ledger::DBTransaction* transaction) {
  DCHECK(transaction);

  const std::string query = base::StringPrintf(
      "CREATE TABLE %s ("
        "publisher_id LONGVARCHAR NOT NULL PRIMARY KEY UNIQUE,"
        "amount DOUBLE DEFAULT 0 NOT NULL,"
        "added_date INTEGER DEFAULT 0 NOT NULL"
      ")",
      table_name_);

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::EXECUTE;
  command->command = query;
  transaction->commands.push_back(std::move(command));

  return true;
}

bool DatabaseRecurringTip::CreateIndexV2(ledger::DBTransaction* transaction) {
  DCHECK(transaction);

  return this->InsertIndex(transaction, table_name_, "publisher_id");
}

bool DatabaseRecurringTip::CreateIndexV15(ledger::DBTransaction* transaction) {
  DCHECK(transaction);

  return this->InsertIndex(transaction, table_name_, "publisher_id");
}

bool DatabaseRecurringTip::Migrate(
    ledger::DBTransaction* transaction,
    const int target) {
  DCHECK(transaction);

  switch (target) {
    case 2: {
      return MigrateToV2(transaction);
    }
    case 15: {
      return MigrateToV15(transaction);
    }
    default: {
      return true;
    }
  }
}

bool DatabaseRecurringTip::MigrateToV2(ledger::DBTransaction* transaction) {
  DCHECK(transaction);

  if (!DropTable(transaction, table_name_)) {
    return false;
  }

  if (!CreateTableV2(transaction)) {
    return false;
  }

  if (!CreateIndexV2(transaction)) {
    return false;
  }

  return true;
}

bool DatabaseRecurringTip::MigrateToV15(ledger::DBTransaction* transaction) {
  DCHECK(transaction);

  const std::string temp_table_name = base::StringPrintf(
      "%s_temp",
      table_name_);

  if (!RenameDBTable(transaction, table_name_, temp_table_name)) {
    return false;
  }

  const std::string query =
      "DROP INDEX IF EXISTS recurring_donation_publisher_id_index;";
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
    { "publisher_id", "publisher_id" },
    { "amount", "amount" },
    { "added_date", "added_date" }
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

void DatabaseRecurringTip::InsertOrUpdate(
    ledger::RecurringTipPtr info,
    ledger::ResultCallback callback) {
  if (!info || info->publisher_key.empty()) {
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  auto transaction = ledger::DBTransaction::New();

  const std::string query = base::StringPrintf(
      "INSERT OR REPLACE INTO %s "
      "(publisher_id, amount, added_date) "
      "VALUES (?, ?, ?)",
      table_name_);

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::RUN;
  command->command = query;

  BindString(command.get(), 0, info->publisher_key);
  BindDouble(command.get(), 1, info->amount);
  BindInt64(command.get(), 2, info->created_at);

  transaction->commands.push_back(std::move(command));

  auto transaction_callback = std::bind(&OnResultCallback,
      _1,
      callback);

  ledger_->RunDBTransaction(std::move(transaction), transaction_callback);
}

void DatabaseRecurringTip::GetAllRecords(
    ledger::PublisherInfoListCallback callback) {
  auto transaction = ledger::DBTransaction::New();

  const std::string query = base::StringPrintf(
    "SELECT pi.publisher_id, pi.name, pi.url, pi.favIcon, "
    "rd.amount, rd.added_date, spi.status, pi.provider "
    "FROM %s as rd "
    "INNER JOIN publisher_info AS pi ON rd.publisher_id = pi.publisher_id "
    "LEFT JOIN server_publisher_info AS spi "
    "ON spi.publisher_key = pi.publisher_id ",
    table_name_);

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::READ;
  command->command = query;

  command->record_bindings = {
      ledger::DBCommand::RecordBindingType::STRING_TYPE,
      ledger::DBCommand::RecordBindingType::STRING_TYPE,
      ledger::DBCommand::RecordBindingType::STRING_TYPE,
      ledger::DBCommand::RecordBindingType::STRING_TYPE,
      ledger::DBCommand::RecordBindingType::DOUBLE_TYPE,
      ledger::DBCommand::RecordBindingType::INT64_TYPE,
      ledger::DBCommand::RecordBindingType::INT64_TYPE,
      ledger::DBCommand::RecordBindingType::STRING_TYPE
  };

  transaction->commands.push_back(std::move(command));

  auto transaction_callback = std::bind(&DatabaseRecurringTip::OnGetAllRecords,
      this,
      _1,
      callback);

  ledger_->RunDBTransaction(std::move(transaction), transaction_callback);
}

void DatabaseRecurringTip::OnGetAllRecords(
    ledger::DBCommandResponsePtr response,
    ledger::PublisherInfoListCallback callback) {
  if (!response
      || response->status != ledger::DBCommandResponse::Status::RESPONSE_OK) {
    callback({});
    return;
  }

  ledger::PublisherInfoList list;
  for (auto const& record : response->result->get_records()) {
    auto info = ledger::PublisherInfo::New();
    auto* record_pointer = record.get();

    info->id = GetStringColumn(record_pointer, 0);
    info->name = GetStringColumn(record_pointer, 1);
    info->url = GetStringColumn(record_pointer, 2);
    info->favicon_url = GetStringColumn(record_pointer, 3);
    info->weight = GetDoubleColumn(record_pointer, 4);
    info->reconcile_stamp = GetInt64Column(record_pointer, 5);
    info->status = static_cast<ledger::mojom::PublisherStatus>(
        GetInt64Column(record_pointer, 6));
    info->provider = GetStringColumn(record_pointer, 7);

    list.push_back(std::move(info));
  }

  callback(std::move(list));
}

void DatabaseRecurringTip::DeleteRecord(
    const std::string& publisher_key,
    ledger::ResultCallback callback) {
  if (publisher_key.empty()) {
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  auto transaction = ledger::DBTransaction::New();

  const std::string query = base::StringPrintf(
    "DELETE FROM %s WHERE publisher_id = ?",
    table_name_);

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::RUN;
  command->command = query;

  BindString(command.get(), 0, publisher_key);

  transaction->commands.push_back(std::move(command));

  auto transaction_callback = std::bind(&OnResultCallback,
      _1,
      callback);

  ledger_->RunDBTransaction(std::move(transaction), transaction_callback);
}

}  // namespace braveledger_database
