/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <map>
#include <utility>

#include "base/strings/stringprintf.h"
#include "bat/ledger/internal/database/database_activity_info.h"
#include "bat/ledger/internal/database/database_util.h"
#include "bat/ledger/internal/ledger_impl.h"

using std::placeholders::_1;

namespace {

const char kTableName[] = "activity_info";

std::string GenerateActivityFilterQuery(
    const int start,
    const int limit,
    ledger::ActivityInfoFilterPtr filter) {
  std::string query = "";
  if (!filter) {
    return query;
  }

  if (!filter->id.empty()) {
    query += " AND ai.publisher_id = ?";
  }

  if (filter->reconcile_stamp > 0) {
    query += " AND ai.reconcile_stamp = ?";
  }

  if (filter->min_duration > 0) {
    query += " AND ai.duration >= ?";
  }

  if (filter->excluded != ledger::ExcludeFilter::FILTER_ALL &&
      filter->excluded !=
        ledger::ExcludeFilter::FILTER_ALL_EXCEPT_EXCLUDED) {
    query += " AND pi.excluded = ?";
  }

  if (filter->excluded ==
    ledger::ExcludeFilter::FILTER_ALL_EXCEPT_EXCLUDED) {
    query += " AND pi.excluded != ?";
  }

  if (filter->percent > 0) {
    query += " AND ai.percent >= ?";
  }

  if (filter->min_visits > 0) {
    query += " AND ai.visits >= ?";
  }

  if (!filter->non_verified) {
    const std::string status = base::StringPrintf(
        " AND spi.status != %1d",
        ledger::mojom::PublisherStatus::NOT_VERIFIED);
    query += status;
  }

  for (const auto& it : filter->order_by) {
    query += " ORDER BY " + it->property_name;
    query += (it->ascending ? " ASC" : " DESC");
  }

  if (limit > 0) {
    query += " LIMIT " + std::to_string(limit);

    if (start > 1) {
      query += " OFFSET " + std::to_string(start);
    }
  }

  return query;
}

void GenerateActivityFilterBind(
    ledger::DBCommand* command,
    ledger::ActivityInfoFilterPtr filter) {
  if (!command || !filter) {
    return;
  }

  int column = 0;
  if (!filter->id.empty()) {
    braveledger_database::BindString(command, column++, filter->id);
  }

  if (filter->reconcile_stamp > 0) {
    braveledger_database::BindInt64(command, column++, filter->reconcile_stamp);
  }

  if (filter->min_duration > 0) {
    braveledger_database::BindInt(command, column++, filter->min_duration);
  }

  if (filter->excluded != ledger::ExcludeFilter::FILTER_ALL &&
      filter->excluded !=
      ledger::ExcludeFilter::FILTER_ALL_EXCEPT_EXCLUDED) {
    braveledger_database::BindInt(
        command,
        column++,
        static_cast<int32_t>(filter->excluded));
  }

  if (filter->excluded ==
      ledger::ExcludeFilter::FILTER_ALL_EXCEPT_EXCLUDED) {
    braveledger_database::BindInt(
        command,
        column++,
        static_cast<int>(ledger::PublisherExclude::EXCLUDED));
  }

  if (filter->percent > 0) {
    braveledger_database::BindInt(command, column++, filter->percent);
  }

  if (filter->min_visits > 0) {
    braveledger_database::BindInt(command, column++, filter->min_visits);
  }
}

}  // namespace

namespace braveledger_database {

DatabaseActivityInfo::DatabaseActivityInfo(
    bat_ledger::LedgerImpl* ledger) :
    DatabaseTable(ledger) {
}

DatabaseActivityInfo::~DatabaseActivityInfo() = default;

bool DatabaseActivityInfo::CreateTableV1(ledger::DBTransaction* transaction) {
  DCHECK(transaction);

  const std::string query = base::StringPrintf(
      "CREATE TABLE %s ("
        "publisher_id LONGVARCHAR NOT NULL,"
        "duration INTEGER DEFAULT 0 NOT NULL,"
        "score DOUBLE DEFAULT 0 NOT NULL,"
        "percent INTEGER DEFAULT 0 NOT NULL,"
        "weight DOUBLE DEFAULT 0 NOT NULL,"
        "category INTEGER NOT NULL,"
        "month INTEGER NOT NULL,"
        "year INTEGER NOT NULL,"
        "CONSTRAINT fk_%s_publisher_id"
        "    FOREIGN KEY (publisher_id)"
        "    REFERENCES publisher_info (publisher_id)"
        "    ON DELETE CASCADE"
      ")",
      kTableName,
      kTableName);

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::EXECUTE;
  command->command = query;
  transaction->commands.push_back(std::move(command));

  return true;
}

bool DatabaseActivityInfo::CreateTableV2(ledger::DBTransaction* transaction) {
  DCHECK(transaction);

  const std::string query = base::StringPrintf(
      "CREATE TABLE %s ("
        "publisher_id LONGVARCHAR NOT NULL,"
        "duration INTEGER DEFAULT 0 NOT NULL,"
        "score DOUBLE DEFAULT 0 NOT NULL,"
        "percent INTEGER DEFAULT 0 NOT NULL,"
        "weight DOUBLE DEFAULT 0 NOT NULL,"
        "category INTEGER NOT NULL,"
        "month INTEGER NOT NULL,"
        "year INTEGER NOT NULL,"
        "reconcile_stamp INTEGER DEFAULT 0 NOT NULL,"
        "CONSTRAINT fk_%s_publisher_id"
        "    FOREIGN KEY (publisher_id)"
        "    REFERENCES publisher_info (publisher_id)"
        "    ON DELETE CASCADE"
      ")",
      kTableName,
      kTableName);

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::EXECUTE;
  command->command = query;
  transaction->commands.push_back(std::move(command));

  return true;
}

bool DatabaseActivityInfo::CreateTableV4(ledger::DBTransaction* transaction) {
  DCHECK(transaction);

  const std::string query = base::StringPrintf(
      "CREATE TABLE %s ("
        "publisher_id LONGVARCHAR NOT NULL,"
        "duration INTEGER DEFAULT 0 NOT NULL,"
        "visits INTEGER DEFAULT 0 NOT NULL,"
        "score DOUBLE DEFAULT 0 NOT NULL,"
        "percent INTEGER DEFAULT 0 NOT NULL,"
        "weight DOUBLE DEFAULT 0 NOT NULL,"
        "month INTEGER NOT NULL,"
        "year INTEGER NOT NULL,"
        "reconcile_stamp INTEGER DEFAULT 0 NOT NULL,"
        "CONSTRAINT activity_unique "
        "UNIQUE (publisher_id, month, year, reconcile_stamp) "
        "CONSTRAINT fk_%s_publisher_id"
        "    FOREIGN KEY (publisher_id)"
        "    REFERENCES publisher_info (publisher_id)"
        "    ON DELETE CASCADE"
      ")",
      kTableName,
      kTableName);

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::EXECUTE;
  command->command = query;
  transaction->commands.push_back(std::move(command));

  return true;
}

bool DatabaseActivityInfo::CreateTableV6(ledger::DBTransaction* transaction) {
  DCHECK(transaction);

  const std::string query = base::StringPrintf(
      "CREATE TABLE %s ("
        "publisher_id LONGVARCHAR NOT NULL,"
        "duration INTEGER DEFAULT 0 NOT NULL,"
        "visits INTEGER DEFAULT 0 NOT NULL,"
        "score DOUBLE DEFAULT 0 NOT NULL,"
        "percent INTEGER DEFAULT 0 NOT NULL,"
        "weight DOUBLE DEFAULT 0 NOT NULL,"
        "reconcile_stamp INTEGER DEFAULT 0 NOT NULL,"
        "CONSTRAINT activity_unique "
        "UNIQUE (publisher_id, reconcile_stamp) "
        "CONSTRAINT fk_%s_publisher_id"
        "    FOREIGN KEY (publisher_id)"
        "    REFERENCES publisher_info (publisher_id)"
        "    ON DELETE CASCADE"
      ")",
      kTableName,
      kTableName);

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::EXECUTE;
  command->command = query;
  transaction->commands.push_back(std::move(command));

  return true;
}

bool DatabaseActivityInfo::CreateTableV15(ledger::DBTransaction* transaction) {
  DCHECK(transaction);

  const std::string query = base::StringPrintf(
      "CREATE TABLE %s ("
        "publisher_id LONGVARCHAR NOT NULL,"
        "duration INTEGER DEFAULT 0 NOT NULL,"
        "visits INTEGER DEFAULT 0 NOT NULL,"
        "score DOUBLE DEFAULT 0 NOT NULL,"
        "percent INTEGER DEFAULT 0 NOT NULL,"
        "weight DOUBLE DEFAULT 0 NOT NULL,"
        "reconcile_stamp INTEGER DEFAULT 0 NOT NULL,"
        "CONSTRAINT activity_unique "
        "UNIQUE (publisher_id, reconcile_stamp)"
      ")",
      kTableName);

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::EXECUTE;
  command->command = query;
  transaction->commands.push_back(std::move(command));

  return true;
}

bool DatabaseActivityInfo::CreateIndexV2(ledger::DBTransaction* transaction) {
  DCHECK(transaction);

  return this->InsertIndex(transaction, kTableName, "publisher_id");
}

bool DatabaseActivityInfo::CreateIndexV4(ledger::DBTransaction* transaction) {
  DCHECK(transaction);

  return this->InsertIndex(transaction, kTableName, "publisher_id");
}

bool DatabaseActivityInfo::CreateIndexV6(ledger::DBTransaction* transaction) {
  DCHECK(transaction);

  return this->InsertIndex(transaction, kTableName, "publisher_id");
}

bool DatabaseActivityInfo::CreateIndexV15(ledger::DBTransaction* transaction) {
  DCHECK(transaction);

  return this->InsertIndex(transaction, kTableName, "publisher_id");
}

bool DatabaseActivityInfo::Migrate(
    ledger::DBTransaction* transaction,
    const int target) {
  DCHECK(transaction);

  switch (target) {
    case 1: {
      return MigrateToV1(transaction);
    }
    case 2: {
      return MigrateToV2(transaction);
    }
    case 4: {
      return MigrateToV4(transaction);
    }
    case 5: {
      return MigrateToV5(transaction);
    }
    case 6: {
      return MigrateToV6(transaction);
    }
    case 15: {
      return MigrateToV15(transaction);
    }
    default: {
      return true;
    }
  }
}

bool DatabaseActivityInfo::MigrateToV1(ledger::DBTransaction* transaction) {
  DCHECK(transaction);

  if (!DropTable(transaction, kTableName)) {
    BLOG(0, "Table couldn't be dropped");
    return false;
  }

  if (!CreateTableV1(transaction)) {
    BLOG(0, "Table couldn't be created");
    return false;
  }

  return true;
}

bool DatabaseActivityInfo::MigrateToV2(ledger::DBTransaction* transaction) {
  DCHECK(transaction);

  const std::string query = base::StringPrintf(
      "ALTER TABLE %s ADD reconcile_stamp INTEGER DEFAULT 0 NOT NULL;",
      kTableName);

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::EXECUTE;
  command->command = query;
  transaction->commands.push_back(std::move(command));

  return true;
}

bool DatabaseActivityInfo::MigrateToV4(ledger::DBTransaction* transaction) {
  DCHECK(transaction);

  const std::string temp_table_name = base::StringPrintf(
      "%s_temp",
      kTableName);

  if (!RenameDBTable(transaction, kTableName, temp_table_name)) {
    return false;
  }

  std::string query = "DROP INDEX IF EXISTS activity_info_publisher_id_index;";
  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::EXECUTE;
  command->command = query;
  transaction->commands.push_back(std::move(command));

  if (!CreateTableV4(transaction)) {
    return false;
  }

  if (!CreateIndexV4(transaction)) {
    return false;
  }

  const std::map<std::string, std::string> columns = {
    { "publisher_id", "publisher_id" },
    { "duration", "duration" },
    { "score", "score" },
    { "percent", "percent" },
    { "weight", "weight" },
    { "month", "month" },
    { "year", "year" },
    { "reconcile_stamp", "reconcile_stamp" }
  };

  if (!MigrateDBTable(
      transaction,
      temp_table_name,
      kTableName,
      columns,
      true)) {
    return false;
  }

  query = base::StringPrintf("UPDATE %s SET visits=5;", kTableName);
  command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::EXECUTE;
  command->command = query;
  transaction->commands.push_back(std::move(command));

  return true;
}

bool DatabaseActivityInfo::MigrateToV5(ledger::DBTransaction* transaction) {
  DCHECK(transaction);

  const std::string query = base::StringPrintf(
      "UPDATE %s SET visits = 1 WHERE visits = 0",
      kTableName);

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::EXECUTE;
  command->command = query;
  transaction->commands.push_back(std::move(command));

  return true;
}

bool DatabaseActivityInfo::MigrateToV6(ledger::DBTransaction* transaction) {
  DCHECK(transaction);

  const std::string temp_table_name = base::StringPrintf(
      "%s_temp",
      kTableName);

  if (!RenameDBTable(transaction, kTableName, temp_table_name)) {
    return false;
  }

  const std::string query =
      "DROP INDEX IF EXISTS activity_info_publisher_id_index;";
  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::EXECUTE;
  command->command = query;
  transaction->commands.push_back(std::move(command));

  if (!CreateTableV6(transaction)) {
    return false;
  }

  if (!CreateIndexV6(transaction)) {
    return false;
  }

  const std::map<std::string, std::string> columns = {
    { "publisher_id", "publisher_id" },
    { "sum(duration) as duration", "duration" },
    { "sum(visits) as visits", "visits" },
    { "sum(score) as score", "score" },
    { "sum(percent) as percent", "percent" },
    { "sum(weight) as weight", "weight" },
    { "reconcile_stamp", "reconcile_stamp" }
  };

  const std::string group_by = "GROUP BY publisher_id, reconcile_stamp";

  if (!MigrateDBTable(
      transaction,
      temp_table_name,
      kTableName,
      columns,
      true,
      group_by)) {
    return false;
  }

  return true;
}

bool DatabaseActivityInfo::MigrateToV15(ledger::DBTransaction* transaction) {
  DCHECK(transaction);

  const std::string temp_table_name = base::StringPrintf(
      "%s_temp",
      kTableName);

  if (!RenameDBTable(transaction, kTableName, temp_table_name)) {
    return false;
  }

  const std::string query =
      "DROP INDEX IF EXISTS activity_info_publisher_id_index;";
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
    { "duration", "duration" },
    { "visits", "visits" },
    { "score", "score" },
    { "percent", "percent" },
    { "weight", "weight" },
    { "reconcile_stamp", "reconcile_stamp" }
  };

  if (!MigrateDBTable(
      transaction,
      temp_table_name,
      kTableName,
      columns,
      true)) {
    return false;
  }

  return true;
}

void DatabaseActivityInfo::NormalizeList(
    ledger::PublisherInfoList list,
    ledger::ResultCallback callback) {
  if (list.empty()) {
    callback(ledger::Result::LEDGER_OK);
    return;
  }
  std::string main_query;
  for (const auto& info : list) {
    main_query += base::StringPrintf(
      "UPDATE %s SET percent = %d, weight = %f WHERE publisher_id = \"%s\";",
      kTableName,
      info->percent,
      info->weight,
      info->id.c_str());
  }

  if (main_query.empty()) {
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  auto transaction = ledger::DBTransaction::New();
  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::EXECUTE;
  command->command = main_query;

  transaction->commands.push_back(std::move(command));

  auto transaction_callback = std::bind(&OnResultCallback,
      _1,
      callback);

  ledger_->RunDBTransaction(std::move(transaction), transaction_callback);
}

void DatabaseActivityInfo::InsertOrUpdate(
    ledger::PublisherInfoPtr info,
    ledger::ResultCallback callback) {
  if (!info) {
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  auto transaction = ledger::DBTransaction::New();
  const std::string query = base::StringPrintf(
      "INSERT OR REPLACE INTO %s "
      "(publisher_id, duration, score, percent, "
      "weight, reconcile_stamp, visits) "
      "VALUES (?, ?, ?, ?, ?, ?, ?)",
      kTableName);

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::RUN;
  command->command = query;

  BindString(command.get(), 0, info->id);
  BindInt64(command.get(), 1, static_cast<int>(info->duration));
  BindDouble(command.get(), 2, info->score);
  BindInt64(command.get(), 3, static_cast<int>(info->percent));
  BindDouble(command.get(), 4, info->weight);
  BindInt64(command.get(), 5, info->reconcile_stamp);
  BindInt(command.get(), 6, info->visits);

  transaction->commands.push_back(std::move(command));

  auto transaction_callback = std::bind(&OnResultCallback,
      _1,
      callback);

  ledger_->RunDBTransaction(std::move(transaction), transaction_callback);
}

void DatabaseActivityInfo::GetRecordsList(
    const int start,
    const int limit,
    ledger::ActivityInfoFilterPtr filter,
    ledger::PublisherInfoListCallback callback) {
  if (!filter) {
    callback({});
    return;
  }

  auto transaction = ledger::DBTransaction::New();

  std::string query = base::StringPrintf(
    "SELECT ai.publisher_id, ai.duration, ai.score, "
    "ai.percent, ai.weight, spi.status, spi.updated_at, pi.excluded, "
    "pi.name, pi.url, pi.provider, "
    "pi.favIcon, ai.reconcile_stamp, ai.visits "
    "FROM %s AS ai "
    "INNER JOIN publisher_info AS pi "
    "ON ai.publisher_id = pi.publisher_id "
    "LEFT JOIN server_publisher_info AS spi "
    "ON spi.publisher_key = pi.publisher_id "
    "WHERE 1 = 1",
    kTableName);

  query += GenerateActivityFilterQuery(start, limit, filter->Clone());

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::READ;
  command->command = query;

  GenerateActivityFilterBind(command.get(), filter->Clone());

  command->record_bindings = {
      ledger::DBCommand::RecordBindingType::STRING_TYPE,
      ledger::DBCommand::RecordBindingType::INT64_TYPE,
      ledger::DBCommand::RecordBindingType::DOUBLE_TYPE,
      ledger::DBCommand::RecordBindingType::INT64_TYPE,
      ledger::DBCommand::RecordBindingType::DOUBLE_TYPE,
      ledger::DBCommand::RecordBindingType::INT_TYPE,
      ledger::DBCommand::RecordBindingType::INT64_TYPE,
      ledger::DBCommand::RecordBindingType::INT_TYPE,
      ledger::DBCommand::RecordBindingType::STRING_TYPE,
      ledger::DBCommand::RecordBindingType::STRING_TYPE,
      ledger::DBCommand::RecordBindingType::STRING_TYPE,
      ledger::DBCommand::RecordBindingType::STRING_TYPE,
      ledger::DBCommand::RecordBindingType::INT64_TYPE,
      ledger::DBCommand::RecordBindingType::INT_TYPE
  };

  transaction->commands.push_back(std::move(command));

  auto transaction_callback = std::bind(&DatabaseActivityInfo::OnGetRecordsList,
      this,
      _1,
      callback);

  ledger_->RunDBTransaction(std::move(transaction), transaction_callback);
}

void DatabaseActivityInfo::OnGetRecordsList(
    ledger::DBCommandResponsePtr response,
    ledger::PublisherInfoListCallback callback) {
  if (!response ||
      response->status != ledger::DBCommandResponse::Status::RESPONSE_OK) {
    callback({});
    return;
  }

  ledger::PublisherInfoList list;
  for (auto const& record : response->result->get_records()) {
    auto info = ledger::PublisherInfo::New();
    auto* record_pointer = record.get();

    info->id = GetStringColumn(record_pointer, 0);
    info->duration = GetInt64Column(record_pointer, 1);
    info->score = GetDoubleColumn(record_pointer, 2);
    info->percent = GetInt64Column(record_pointer, 3);
    info->weight = GetDoubleColumn(record_pointer, 4);
    info->status = static_cast<ledger::mojom::PublisherStatus>(
        GetIntColumn(record_pointer, 5));
    info->status_updated_at = GetInt64Column(record_pointer, 6);
    info->excluded = static_cast<ledger::PublisherExclude>(
        GetIntColumn(record_pointer, 7));
    info->name = GetStringColumn(record_pointer, 8);
    info->url = GetStringColumn(record_pointer, 9);
    info->provider = GetStringColumn(record_pointer, 10);
    info->favicon_url = GetStringColumn(record_pointer, 11);
    info->reconcile_stamp = GetInt64Column(record_pointer, 12);
    info->visits = GetIntColumn(record_pointer, 13);

    list.push_back(std::move(info));
  }

  callback(std::move(list));
}

void DatabaseActivityInfo::DeleteRecord(
    const std::string& publisher_key,
    ledger::ResultCallback callback) {
  if (publisher_key.empty()) {
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  auto transaction = ledger::DBTransaction::New();

  const std::string query = base::StringPrintf(
      "DELETE FROM %s WHERE publisher_id = ? AND reconcile_stamp = ?",
      kTableName);

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::RUN;
  command->command = query;

  BindString(command.get(), 0, publisher_key);
  BindInt64(command.get(), 1, ledger_->GetReconcileStamp());

  transaction->commands.push_back(std::move(command));

  auto transaction_callback = std::bind(&OnResultCallback,
      _1,
      callback);

  ledger_->RunDBTransaction(std::move(transaction), transaction_callback);
}

}  // namespace braveledger_database
