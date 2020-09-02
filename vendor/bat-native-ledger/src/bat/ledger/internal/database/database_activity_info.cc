/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <map>
#include <memory>
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
    ledger::type::ActivityInfoFilterPtr filter) {
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

  if (filter->excluded != ledger::type::ExcludeFilter::FILTER_ALL &&
      filter->excluded !=
        ledger::type::ExcludeFilter::FILTER_ALL_EXCEPT_EXCLUDED) {
    query += " AND pi.excluded = ?";
  }

  if (filter->excluded ==
    ledger::type::ExcludeFilter::FILTER_ALL_EXCEPT_EXCLUDED) {
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
        ledger::type::PublisherStatus::NOT_VERIFIED);
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
    ledger::type::DBCommand* command,
    ledger::type::ActivityInfoFilterPtr filter) {
  if (!command || !filter) {
    return;
  }

  int column = 0;
  if (!filter->id.empty()) {
    ledger::database::BindString(command, column++, filter->id);
  }

  if (filter->reconcile_stamp > 0) {
    ledger::database::BindInt64(command, column++, filter->reconcile_stamp);
  }

  if (filter->min_duration > 0) {
    ledger::database::BindInt(command, column++, filter->min_duration);
  }

  if (filter->excluded != ledger::type::ExcludeFilter::FILTER_ALL &&
      filter->excluded !=
      ledger::type::ExcludeFilter::FILTER_ALL_EXCEPT_EXCLUDED) {
    ledger::database::BindInt(
        command,
        column++,
        static_cast<int32_t>(filter->excluded));
  }

  if (filter->excluded ==
      ledger::type::ExcludeFilter::FILTER_ALL_EXCEPT_EXCLUDED) {
    ledger::database::BindInt(
        command,
        column++,
        static_cast<int>(ledger::type::PublisherExclude::EXCLUDED));
  }

  if (filter->percent > 0) {
    ledger::database::BindInt(command, column++, filter->percent);
  }

  if (filter->min_visits > 0) {
    ledger::database::BindInt(command, column++, filter->min_visits);
  }
}

}  // namespace

namespace ledger {
namespace database {

DatabaseActivityInfo::DatabaseActivityInfo(
    LedgerImpl* ledger) :
    DatabaseTable(ledger) {
}

DatabaseActivityInfo::~DatabaseActivityInfo() = default;

void DatabaseActivityInfo::NormalizeList(
    type::PublisherInfoList list,
    ledger::ResultCallback callback) {
  if (list.empty()) {
    callback(type::Result::LEDGER_OK);
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
    callback(type::Result::LEDGER_ERROR);
    return;
  }

  auto transaction = type::DBTransaction::New();
  auto command = type::DBCommand::New();
  command->type = type::DBCommand::Type::EXECUTE;
  command->command = main_query;

  transaction->commands.push_back(std::move(command));

  auto shared_list = std::make_shared<type::PublisherInfoList>(
      std::move(list));

  ledger_->ledger_client()->RunDBTransaction(
      std::move(transaction),
      [this, shared_list, callback](type::DBCommandResponsePtr response) {
        if (!response || response->status !=
              type::DBCommandResponse::Status::RESPONSE_OK) {
          callback(type::Result::LEDGER_ERROR);
          return;
        }

        ledger_->ledger_client()->PublisherListNormalized(
            std::move(*shared_list));

        callback(type::Result::LEDGER_OK);
      });
}

void DatabaseActivityInfo::InsertOrUpdate(
    type::PublisherInfoPtr info,
    ledger::ResultCallback callback) {
  if (!info) {
    callback(type::Result::LEDGER_ERROR);
    return;
  }

  auto transaction = type::DBTransaction::New();
  const std::string query = base::StringPrintf(
      "INSERT OR REPLACE INTO %s "
      "(publisher_id, duration, score, percent, "
      "weight, reconcile_stamp, visits) "
      "VALUES (?, ?, ?, ?, ?, ?, ?)",
      kTableName);

  auto command = type::DBCommand::New();
  command->type = type::DBCommand::Type::RUN;
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

  ledger_->ledger_client()->RunDBTransaction(
      std::move(transaction),
      transaction_callback);
}

void DatabaseActivityInfo::GetRecord(
    const std::string& publisher_key,
    GetActivityInfoCallback callback) {
  if (publisher_key.empty()) {
    BLOG(1, "Publisher key is empty");
    callback({});
    return;
  }

  auto transaction = type::DBTransaction::New();

  const std::string query = base::StringPrintf(
      "SELECT publisher_id, duration, visits, score, percent, "
      "weight, reconcile_stamp FROM %s WHERE publisher_id = ?",
      kTableName);

  auto command = type::DBCommand::New();
  command->type = type::DBCommand::Type::READ;
  command->command = query;

  BindString(command.get(), 0, publisher_key);

  command->record_bindings = {
      type::DBCommand::RecordBindingType::STRING_TYPE,
      type::DBCommand::RecordBindingType::INT64_TYPE,
      type::DBCommand::RecordBindingType::INT_TYPE,
      type::DBCommand::RecordBindingType::DOUBLE_TYPE,
      type::DBCommand::RecordBindingType::INT64_TYPE,
      type::DBCommand::RecordBindingType::DOUBLE_TYPE,
      type::DBCommand::RecordBindingType::INT64_TYPE
  };

  transaction->commands.push_back(std::move(command));

  auto transaction_callback = std::bind(&DatabaseActivityInfo::OnGetRecord,
      this,
      _1,
      callback);

  ledger_->ledger_client()->RunDBTransaction(
      std::move(transaction),
      transaction_callback);
}

void DatabaseActivityInfo::OnGetRecord(
    type::DBCommandResponsePtr response,
    GetActivityInfoCallback callback) {
  if (!response ||
      response->status != type::DBCommandResponse::Status::RESPONSE_OK) {
    BLOG(0, "Response is wrong");
    callback({});
    return;
  }

  if (response->result->get_records().size() != 1) {
    BLOG(1, "Record size is not correct: " <<
        response->result->get_records().size());
    callback({});
    return;
  }

  auto* record = response->result->get_records()[0].get();
  auto info = type::ActivityInfo::New();
  info->id = GetStringColumn(record, 0);
  info->duration = GetInt64Column(record, 1);
  info->visits = GetIntColumn(record, 2);
  info->score = GetDoubleColumn(record, 3);
  info->percent = GetInt64Column(record, 4);
  info->weight = GetDoubleColumn(record, 5);
  info->reconcile_stamp = GetInt64Column(record, 6);

  callback(std::move(info));
}

void DatabaseActivityInfo::GetRecordsList(
    const int start,
    const int limit,
    type::ActivityInfoFilterPtr filter,
    ledger::PublisherInfoListCallback callback) {
  if (!filter) {
    callback({});
    return;
  }

  auto transaction = type::DBTransaction::New();

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

  auto command = type::DBCommand::New();
  command->type = type::DBCommand::Type::READ;
  command->command = query;

  GenerateActivityFilterBind(command.get(), filter->Clone());

  command->record_bindings = {
      type::DBCommand::RecordBindingType::STRING_TYPE,
      type::DBCommand::RecordBindingType::INT64_TYPE,
      type::DBCommand::RecordBindingType::DOUBLE_TYPE,
      type::DBCommand::RecordBindingType::INT64_TYPE,
      type::DBCommand::RecordBindingType::DOUBLE_TYPE,
      type::DBCommand::RecordBindingType::INT_TYPE,
      type::DBCommand::RecordBindingType::INT64_TYPE,
      type::DBCommand::RecordBindingType::INT_TYPE,
      type::DBCommand::RecordBindingType::STRING_TYPE,
      type::DBCommand::RecordBindingType::STRING_TYPE,
      type::DBCommand::RecordBindingType::STRING_TYPE,
      type::DBCommand::RecordBindingType::STRING_TYPE,
      type::DBCommand::RecordBindingType::INT64_TYPE,
      type::DBCommand::RecordBindingType::INT_TYPE
  };

  transaction->commands.push_back(std::move(command));

  auto transaction_callback = std::bind(&DatabaseActivityInfo::OnGetRecordsList,
      this,
      _1,
      callback);

  ledger_->ledger_client()->RunDBTransaction(
      std::move(transaction),
      transaction_callback);
}

void DatabaseActivityInfo::OnGetRecordsList(
    type::DBCommandResponsePtr response,
    ledger::PublisherInfoListCallback callback) {
  if (!response ||
      response->status != type::DBCommandResponse::Status::RESPONSE_OK) {
    callback({});
    return;
  }

  type::PublisherInfoList list;
  for (auto const& record : response->result->get_records()) {
    auto info = type::PublisherInfo::New();
    auto* record_pointer = record.get();

    info->id = GetStringColumn(record_pointer, 0);
    info->duration = GetInt64Column(record_pointer, 1);
    info->score = GetDoubleColumn(record_pointer, 2);
    info->percent = GetInt64Column(record_pointer, 3);
    info->weight = GetDoubleColumn(record_pointer, 4);
    info->status = static_cast<type::PublisherStatus>(
        GetIntColumn(record_pointer, 5));
    info->status_updated_at = GetInt64Column(record_pointer, 6);
    info->excluded = static_cast<type::PublisherExclude>(
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
    callback(type::Result::LEDGER_ERROR);
    return;
  }

  auto transaction = type::DBTransaction::New();

  const std::string query = base::StringPrintf(
      "DELETE FROM %s WHERE publisher_id = ? AND reconcile_stamp = ?",
      kTableName);

  auto command = type::DBCommand::New();
  command->type = type::DBCommand::Type::RUN;
  command->command = query;

  BindString(command.get(), 0, publisher_key);
  BindInt64(command.get(), 1, ledger_->state()->GetReconcileStamp());

  transaction->commands.push_back(std::move(command));

  auto transaction_callback = std::bind(&OnResultCallback,
      _1,
      callback);

  ledger_->ledger_client()->RunDBTransaction(
      std::move(transaction),
      transaction_callback);
}

}  // namespace database
}  // namespace ledger
